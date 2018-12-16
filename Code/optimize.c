//
// Created by suun on 12/16/18.
//

#include "optimize.h"
#include "intercode.h"

VisitRecord variableHashMap[HASH_SIZE];

VisitRecord genVisitRecord(char *name) {
  VisitRecord visitRecord = malloc(sizeof(VisitRecord_));
  strcpy(visitRecord->name, name);
  strcpy(visitRecord->equivalent, name);
  visitRecord->read = visitRecord->write = 0;
  return visitRecord;
}

void initVariableHashMap() {
  for (int i = 0; i < HASH_SIZE; i++)
    variableHashMap[i] = NULL;
}

VisitRecord getVariableHashMap(char *name) {
  unsigned int key = hashFunc(name);
  while (true) {
    VisitRecord value = variableHashMap[key];
    if (value == NULL) {
      value = genVisitRecord(name);
      variableHashMap[key] = value;
    } else if (strcmp(name, value->name) != 0) {
      key++;
      key = key % HASH_SIZE;
      continue;
    }
    return value;
  }
}

void getVariableName(Operand operand, char *name) {
  if (operand->kind == oTempVariable)
    sprintf(name, "t%d", operand->un.tempVarIndex);
  else
    strcpy(name, operand->un.value);
}

void increaseRecord(Operand operand, bool isRead) {
  if (operand->kind != oVariable && operand->kind != oTempVariable)
    return;
  char name[32];
  getVariableName(operand, name);
  VisitRecord record = getVariableHashMap(name);
  if (isRead)
    record->read++;
  else
    record->write++;
}

void recordVariable() {
  InterCode head = getHeadInterCode();
  for (InterCode code = head->next; code != head; code = code->next) {
    switch (code->kind) {
      case iArg:
      case iWrite:
      case iReturn:increaseRecord(code->unary.op, true);
        break;
      case iRead:increaseRecord(code->unary.op, false);
        break;
      case iCall:increaseRecord(code->binary.left, false);
        break;
      case iAssign:increaseRecord(code->binary.left, false);
        increaseRecord(code->binary.right, true);
        break;
      case iPlus:
      case iMinus:
      case iStar:
      case iDiv:increaseRecord(code->ternary.res, false);
        increaseRecord(code->ternary.left, true);
        increaseRecord(code->ternary.right, true);
        break;
      case iGetAddress:increaseRecord(code->ternary.res, false);
        increaseRecord(code->ternary.left, true);
        break;
      case iIfGoto:increaseRecord(code->ifGoto.left, false);
        increaseRecord(code->ifGoto.right, true);
        break;
      default:break;
    }
  }
}

void findVariableEquivalent() {
  InterCode head = getHeadInterCode();
  for (InterCode code = head->next; code != head; code = code->next) {
    if (code->kind == iAssign) {
      if ((code->binary.left->kind != oVariable && code->binary.left->kind != oTempVariable) ||
          (code->binary.right->kind != oVariable && code->binary.right->kind != oTempVariable))
        break;
      char leftName[32], rightName[32], lastName[32] = {0};
      getVariableName(code->binary.left, leftName);
      getVariableName(code->binary.right, rightName);
      InterCode prev = code->prev;
      if (prev->kind == iCall || prev->kind == iRead)
        getVariableName(prev->unary.op, lastName);
      else if (prev->kind == iAssign)
        getVariableName(prev->binary.left, lastName);
      if (!strcmp(rightName, lastName)) {
        strcpy(getVariableHashMap(leftName)->equivalent, rightName);
        deleteNode(code);
      }
    }
  }
}

void replaceVariableName(Operand operand) {
  if (operand->kind != oTempVariable && operand->kind != oVariable)
    return;
  char name[32];
  getVariableName(operand, name);
  operand->kind = oVariable;
  VisitRecord record = getVariableHashMap(name);
  strcpy(operand->un.value, record->equivalent);
}

void traverseReplaceVariable() {
  InterCode head = getHeadInterCode();
  for (InterCode code = head->next; code != head; code = code->next) {
    switch (code->kind) {
      case iGoto:
      case iReturn:
      case iArg:
      case iParam:
      case iRead:
      case iWrite:replaceVariableName(code->unary.op);
        break;
      case iAssign:
      case iCall:replaceVariableName(code->binary.left);
        replaceVariableName(code->binary.right);
        break;
      case iPlus:
      case iMinus:
      case iStar:
      case iDiv:
      case iGetAddress:replaceVariableName(code->ternary.res);
        replaceVariableName(code->ternary.left);
        replaceVariableName(code->ternary.right);
        break;
      case iIfGoto:replaceVariableName(code->ternary.left);
        replaceVariableName(code->ternary.right);
        break;
      default:break;
    }
  }
}

void optimize() {
  initVariableHashMap();
  recordVariable();
  findVariableEquivalent();
  traverseReplaceVariable();
#ifdef COMPILER_DEBUG
  for (unsigned i = 0; i < HASH_SIZE; i++) {
    VisitRecord record = variableHashMap[i];
    if (record != NULL) {
      printf("%s read: %d write: %d\n", record->name, record->read, record->write);
    }
  }
  printf("\n");
#endif
}
