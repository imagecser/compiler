//
// Created by suun on 1/13/19.
//

#include "objectcode.h"
#include "intercode.h"

#define REG_NUM 1 << 5
#define REG_RANGE 20
RegObject regs[REG_NUM];
VarObject *varList = NULL;
ArrayObject *arrayList = NULL;
StackObject stack;

FILE *file;
int argNum = 0;
extern InterCode headInterCode;

int getReg(Operand operand) {
  char name[4];
  memset(name, 0, sizeof(name));

  VarObject *var = varList;
  for (; var != NULL; var = var->next) {
    if (!strcmp(var->op->un.value, operand->un.value)) {
      sprintf(name, "%s", operand->un.value);
      break;
    }
  }
  // find variable in existed list
  bool isArray = false;
  ArrayObject *array = arrayList;
  for (; array != NULL; array = array->next) {
    if (!strcmp(array->name, operand->un.value)) {
      isArray = true;
    }
  }

  if (var == NULL) {
    var = (VarObject *) (malloc(sizeof(VarObject)));
    memset(var, 0, sizeof(VarObject));
    var->op = operand;
    var->regIndex = -1;
    if (operand->kind == oVariable) {
      var->next = varList;
      varList = var;
    }
  }
  // not found. append variable to list
  if (var->regIndex == -1) {
    for (int i = 0; i < REG_RANGE; i++) {
      regs[i].var != NULL && regs[i].age++;
    }
    // increase every register's age
    for (int i = 0; i < REG_RANGE; i++) {
      if (regs[i].var == NULL) {
        var->regIndex = i;
        break;
      }
    }
    if (var->regIndex == -1) {
      int maxAge = -1;
      for (int i = 0; i < REG_RANGE; i++) {
        if (regs[i].age > maxAge) {
          maxAge = regs[i].age;
          var->regIndex = i;
        }
      }
    }
    regs[var->regIndex].var && (regs[var->regIndex].var->regIndex = -1);
    regs[var->regIndex].var = var;
    regs[var->regIndex].age = 0;
    // find the variable in regs
    // yes: set reg index
    // no: remove oldest register content
    if (operand->kind == oConstant) {
      fp("  li %s, %s\n", regs[var->regIndex].name, operand->un.value);
//    } else if (operand->kind == oTempAddress) {
//      fp("  la %s, %s\n", regs[var->regIndex].name, operand->un.dest->un.value);
    } else if (isArray) {
      fp("  la %s, %s\n", regs[var->regIndex].name, operand->un.value);
    }
  }
  return var->regIndex;
}

void saveVariables() {
  for (VarObject *var = varList; var != NULL; var = var->next) {
    if (var->regIndex != -1) {
      stack.varStack[stack.length++] = var;
      fp("  addi $sp, $sp, -4\n");
      fp("  sw %s, 0($sp)\n", regs[var->regIndex].name);
      regs[var->regIndex].var = NULL;
      stack.age[stack.length] = regs[var->regIndex].age;
      regs[var->regIndex].age = 0;
    }
  }
  varList = NULL;
}

void readVariable() {
  VarObject *var = varList;
  while (var != NULL) {
    varList = var->next;
    free(var);
    var = varList;
  }
  for (int i = stack.length - 1; i >= 0; i--) {
    var = stack.varStack[i];
    if (var == NULL) {
      continue;
    }
    var->next = varList;
    varList = var;
    fp("  lw %s, 0($sp)\n", regs[var->regIndex].name);
    fp("  addi $sp, $sp, 4\n");
    regs[var->regIndex].var = var;
    regs[var->regIndex].age = stack.age[i];
  }
  stack.length = 0;
}

void writeLine(InterCode code) {
  switch (code->kind) {

    case iLabel: {
      fp("label%d:\n", code->unary.op->un.labelIndex);
      break;
    }
    case iFunction: {
      fp("\n%s:\n", code->unary.op->un.value);
      for (VarObject *var = varList; var != NULL;) {
        varList = var->next;
        free(var);
        var = varList;
      }
      for (int i = 0; i < REG_RANGE; i++) {
        regs[i].age = 0;
        regs[i].var = NULL;
      }
      int i = 0;
      for (InterCode paramCode = code->next; paramCode->kind == iParam; paramCode = paramCode->next, i++) {
        Operand operand = paramCode->unary.op;
        int regIndex;
        regIndex = getReg(operand);
        fp("  move %s, $a%d\n", regs[regIndex].name, i);
      }
      break;
    }
    case iAssign: {
      Operand leftOp = code->binary.left, rightOp = code->binary.right;
      int leftRegIndex, rightRegIndex;
      if (leftOp->kind == oTempAddress) {  // *x = y
        leftRegIndex = getReg(leftOp->un.dest);
        rightRegIndex = getReg(rightOp);
        fp("  sw %s, 0(%s)\n", regs[rightRegIndex].name, regs[leftRegIndex].name);
      } else if (rightOp->kind == oTempAddress) {  // x = *y
        leftRegIndex = getReg(leftOp);
        rightRegIndex = getReg(rightOp->un.dest);
        fp("  lw %s, 0(%s)\n", regs[leftRegIndex].name, regs[rightRegIndex].name);
      } else if (rightOp->kind == oConstant) {
        leftRegIndex = getReg(leftOp);
        fp("  li %s, %s\n", regs[leftRegIndex].name, rightOp->un.value);
      } else if (rightOp->kind == oVariable) {
        rightRegIndex = getReg(rightOp);
        leftRegIndex = getReg(leftOp);
        fp("  move %s, %s\n", regs[leftRegIndex].name, regs[rightRegIndex].name);
      }
      break;
    }
    case iPlus: {
      Operand rightOp = code->ternary.right;
      int destRegIndex = getReg(code->ternary.res),
          leftRegIndex = getReg(code->ternary.left),
          rightRegIndex = getReg(code->ternary.right);
      if (code->ternary.right->kind == oConstant) {
        fp("  addi %s, %s, %s\n", regs[destRegIndex].name,
           regs[leftRegIndex].name, rightOp->un.value);
      } else {
        fp("  add %s, %s, %s\n", regs[destRegIndex].name,
           regs[leftRegIndex].name, regs[rightRegIndex].name);
      }
      break;
    }
    case iMinus: {
      Operand rightOp = code->ternary.right;
      int destRegIndex = getReg(code->ternary.res),
          leftRegIndex = getReg(code->ternary.left),
          rightRegIndex;
      if (code->ternary.right->kind == oConstant) {
        if (rightOp->un.value[0] == '-') {
          fp("  addi %s, %s, %s\n", regs[destRegIndex].name,
             regs[leftRegIndex].name, rightOp->un.value + 1);
        } else {
          fp("  addi %s, %s, -%s\n", regs[destRegIndex].name,
             regs[leftRegIndex].name, rightOp->un.value);
        }
      } else {
        rightRegIndex = getReg(rightOp);
        fp("  sub %s, %s, %s\n", regs[destRegIndex].name,
           regs[leftRegIndex].name, regs[rightRegIndex].name);
      }
      break;
    }
    case iStar: {
      int destIndex = getReg(code->ternary.res),
          leftIndex = getReg(code->ternary.left),
          rightIndex = getReg(code->ternary.right);
      fp("  mul %s, %s, %s\n", regs[destIndex].name,
         regs[leftIndex].name, regs[rightIndex].name);
      break;
    }
    case iDiv: {
      int destIndex = getReg(code->ternary.res),
          leftIndex = getReg(code->ternary.left),
          rightIndex = getReg(code->ternary.right);
      fp("  div %s, %s\n", regs[leftIndex].name, regs[rightIndex].name);
      fp("  mflo %s\n", regs[destIndex].name);
      break;
    }
    case iGoto: {
      fp("  j label%d\n", code->unary.op->un.labelIndex);
      break;
    }
    case iIfGoto: {
      int leftIndex = getReg(code->ifGoto.left),
          rightIndex = getReg(code->ifGoto.right);
      char relop[4];
      strcpy(relop, code->ifGoto.rel);
      if (!strcmp(relop, "==")) {
        strcpy(relop, "beq");
      } else if (!strcmp(relop, "!=")) {
        strcpy(relop, "bne");
      } else if (!strcmp(relop, ">")) {
        strcpy(relop, "bgt");
      } else if (!strcmp(relop, "<")) {
        strcpy(relop, "blt");
      } else if (!strcmp(relop, ">=")) {
        strcpy(relop, "bge");
      } else if (!strcmp(relop, "<=")) {
        strcpy(relop, "ble");
      }
      fp("  %s %s, %s, label%d\n",
         relop,
         regs[leftIndex].name,
         regs[rightIndex].name,
         code->ifGoto.label->un.labelIndex);
      break;
    }
    case iReturn: {
      int regIndex = getReg(code->unary.op);
      fp("  move $v0, %s\n", regs[regIndex].name);
      fp("  jr $ra\n");
      break;
    }
    case iDec: {
      ArrayObject *array = malloc(sizeof(ArrayObject));
      array->length = code->dec.size;
      strcpy(array->name, code->dec.op->un.value);
      array->next = arrayList;
      arrayList = array;
      break;
    }
    case iArg: {
      argNum++;
      break;
    }
    case iCall: {
      fp("  addi $sp, $sp, %d\n", -4 * argNum - 4);
      fp("  sw $ra, 0($sp)\n");
      for (int i = 0, j = 4; i < argNum; i++, j += 4) {
        fp("  sw $a%d, %d($sp)\n", i, j);
      }
      InterCode argCode = code->prev;
      for (int i = 0; i < argNum; i++) {
        Operand operand = argCode->unary.op;
        if (operand->kind == oTempAddress) {
          fp("  sw $a%d, 0(%s)\n", i, regs[getReg(operand->un.dest)].name);
        } else {
          fp("  move $a%d, %s\n", i, regs[getReg(argCode->unary.op)].name);
        }
        argCode = argCode->prev;
      }
      saveVariables();
      fp("  jal %s\n", code->binary.right->un.value);
      readVariable();
      for (int i = 0, j = 4; i < argNum; i++, j += 4) {
        fp("  lw $a%d, %d($sp)\n", i, j);
      }
      fp("  lw $ra, 0($sp)\n"
         "  addi $sp, $sp, %d\n", 4 * (argNum + 1));
      fp("  move %s, $v0\n", regs[getReg(code->binary.left)].name);
      argNum = 0;
      break;
    }
    case iParam: {
      break;
    }
    case iRead: {
      Operand operand = code->unary.op;
      fp("  addi $sp, $sp, -4\n"
         "  sw $ra, 0($sp)\n"
         "  jal read\n"
         "  lw $ra, 0($sp)\n"
         "  addi $sp, $sp, 4\n");
      if (operand->kind == oTempAddress) {  // *x = $v0
        fp("  sw $v0, 0(%s)\n", regs[getReg(operand->un.dest)].name);
      } else {
        fp("  move %s, $v0\n", regs[getReg(operand)].name);
      }
      break;
    }
    case iWrite: {
      Operand operand = code->unary.op;
      if (operand->kind == oTempAddress) {  // $a0 = *x
        fp("  lw $a0, 0(%s)\n", regs[getReg(operand->un.dest)].name);
      } else {
        fp("  move $a0, %s\n", regs[getReg(operand)].name);
      }
      fp("  addi $sp, $sp, -4\n"
         "  sw $ra, 0($sp)\n"
         "  jal write\n"
         "  lw $ra, 0($sp)\n"
         "  addi $sp, $sp, 4\n");
      break;
    }
  }
}

void writeObjectFile(char *name) {
  if (!strcmp(name, "stdout")) {
    file = stdout;
  } else {
    file = fopen(name, "w");
  }
  {
    for (int i = 0; i < REG_RANGE; i++) {
      regs[i].var = NULL;
      regs[i].age = 0;
    }
    for (int i = 0; i < 10; i++) {
      sprintf(regs[i].name, "$t%d", i);
    }
    for (int i = 0, j = 10; i < 9; i++, j++) {
      sprintf(regs[j].name, "$s%d", i);
    }
    sprintf(regs[19].name, "$fp");
    for (int i = 0, j = 20; i < 4; i++, j++) {
      sprintf(regs[j].name, "$a%d", i);
    }
    sprintf(regs[24].name, "$v0");
    sprintf(regs[25].name, "$ra");
    sprintf(regs[26].name, "$gp");
    sprintf(regs[27].name, "$sp");
    sprintf(regs[28].name, "$k0");
    sprintf(regs[29].name, "$k1");
    sprintf(regs[30].name, "$at");
    sprintf(regs[31].name, "$zero");
  }
  fp(".globl main\n"
     ".text\n\n"
     "read:\n"
     "  li $v0, 4\n"
     "  la $a0, _prompt\n"
     "  syscall\n"
     "  li $v0, 5\n"
     "  syscall\n"
     "  jr $ra\n\n"
     "write:\n"
     "  li $v0, 1\n"
     "  syscall\n"
     "  li $v0, 4\n"
     "  la $a0, _ret\n"
     "  syscall\n"
     "  move $v0, $0\n"
     "  jr $ra\n\n"
  );
  for (InterCode code = headInterCode->next; code != headInterCode; code = code->next) {
    writeLine(code);
  }
  fp("\n.data\n"
     "_prompt: .asciiz \"Enter an Interger: \"\n"
     "_ret: .asciiz \"\\n\"\n");
  for (ArrayObject *array = arrayList; array != NULL; array = array->next) {
    int arraySize = array->length / 4;
    fp("%s: .word ", array->name);
    fp("0");
    for (int i = 1; i < arraySize; i++) {
      fp(", 0");
    }
    fp("\n");
  }
  fclose(file);
}
