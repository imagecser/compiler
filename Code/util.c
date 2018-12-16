//
// Created by suun on 12/13/18.
//

#include "semantic.h"

int labelIndex = 1;
int tempVariableIndex = 1;

InterCode genGotoLabelInterCode(Operand operand) {
  InterCode gotoInterCode = malloc(sizeof(InterCode_));
  gotoInterCode->kind = iGoto;
  gotoInterCode->unary.op = operand;
  return gotoInterCode;
}

Operand genLabelOperand() {
  Operand label = malloc(sizeof(Operand_));
  memset(label, 0, sizeof(Operand_));
  label->kind = oLabel;
  label->un.labelIndex = labelIndex++;
  return label;
}

InterCode genLabelInterCode(Operand operand) {
  InterCode label = malloc(sizeof(InterCode_));
  label->kind = iLabel;
  if (operand == NULL)
    operand = genLabelOperand();
  label->unary.op = operand;
  return label;
}

int getSize(Type type, bool isArray) {
  if (type == NULL) {
    return 0;
  } else if (!isArray) {
    if (type->kind == KBASIC)
      return 4;
    int size = 4;
    while (type->array_.type->kind != KBASIC) {
      size = type->array_.size * size;
      type = type->array_.type;
    }
    size = type->array_.size * size;
    if (type->array_.type->kind == KBASIC)
      return size;
  } else {
    if (type->array_.type->kind == KBASIC)
      return 4;
    int size = 4;
    while (type->array_.type->kind != KBASIC) {
      size = type->array_.size * size;
      if (type->array_.type->array_.type->kind == KBASIC)
        return size;
      type = type->array_.type;
    }
    return type->array_.size * size;
  }
  return 1;
}

InterCode genInterCode(_InterCodeKind kind) {
  InterCode interCode = malloc(sizeof(InterCode_));
  interCode->kind = kind;
  interCode->next = interCode->prev = NULL;
  return interCode;
}

InterCode genInterCodeUnary(_InterCodeKind kind, Operand operand) {
  InterCode interCode = genInterCode(kind);
  interCode->unary.op = operand;
  return interCode;
}
InterCode genInterCodeBinary(_InterCodeKind kind, Operand left, Operand right) {
  InterCode interCode = genInterCode(kind);
  interCode->binary.right = right;
  interCode->binary.left = left;
  return interCode;
}
InterCode genInterCodeTernary(_InterCodeKind kind, Operand res, Operand left, Operand right) {
  InterCode interCode = genInterCode(kind);
  interCode->ternary.left = left;
  interCode->ternary.right = right;
  interCode->ternary.res = res;
  return interCode;
}
InterCode genInterCodeIfGoto(Operand label, Operand left, Operand right, const char *relop) {
  InterCode interCode = genInterCode(iIfGoto);
  interCode->ifGoto.right = right;
  interCode->ifGoto.left = left;
  interCode->ifGoto.label = label;
  strcpy(interCode->ifGoto.rel, relop);
  return interCode;
}
InterCode genInterCodeDec(Operand op, int size) {
  InterCode interCode = genInterCode(iDec);
  interCode->dec.op = op;
  interCode->dec.size = size;
  return interCode;
}

Operand genEmptyOperand() {
  Operand operand = malloc(sizeof(Operand_));
  memset(operand, 0, sizeof(Operand_));
  return operand;
}

Operand genOperand(_OperandKind kind) {
  Operand operand = genEmptyOperand();
  operand->kind = kind;
  return operand;
}

Operand genOperandInt(_OperandKind kind, int value) {
  Operand operand = genOperand(kind);
  sprintf(operand->un.value, "%d", value);
  return operand;
}

void setOperandTemp(Operand operand) {
  operand->kind = oTempVariable;
  operand->un.tempVarIndex = tempVariableIndex++;
}

Operand genTempOperand() {
  Operand operand = genOperand(oTempVariable);
  operand->un.tempVarIndex = tempVariableIndex++;
  return operand;
}

Operand genOperandStr(_OperandKind kind, const char *src) {
  Operand operand = genOperand(kind);
  strcpy(operand->un.value, src);
  // memcpy(operand->un.value, src, strlen(src));
//  operand->un.value[strlen(src)] = 0;
  return operand;
}