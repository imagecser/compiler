//
// Created by suun on 12/13/18.
//

#include "semantic.h"
#include "intercode.h"

int labelIndex = 0;

InterCode getGotoLabelInterCode(Operand operand) {
  InterCode gotoInterCode = malloc(sizeof(InterCode_));
  gotoInterCode->kind = iGoto;
  gotoInterCode->unary.op = operand;
  return gotoInterCode;
}

Operand getLabelOperand() {
  Operand label = malloc(sizeof(Operand_));
  memset(label, 0, sizeof(Operand_));
  label->kind = oLabel;
  label->kind = labelIndex++;
  return label;
}

InterCode getLabelInterCode(Operand operand) {
  InterCode label = malloc(sizeof(InterCode_));
  label->kind = iLabel;
  if (operand == NULL)
    operand = getLabelOperand();
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

InterCode getInterCode(_InterCodeKind kind) {
  InterCode interCode = malloc(sizeof(InterCode_));
  interCode->next = NULL;
  interCode->kind = kind;
  return interCode;
}

InterCode getInterCodeUnary(_InterCodeKind kind, Operand operand) {
  InterCode interCode = getInterCode(kind);
  interCode->unary.op = operand;
  return interCode;
}
InterCode getInterCodeBinary(_InterCodeKind kind, Operand left, Operand right) {
  InterCode interCode = getInterCode(kind);
  interCode->binary.right = right;
  interCode->binary.left = left;
  return interCode;
}
InterCode getInterCodeTernary(_InterCodeKind kind, Operand res, Operand left, Operand right) {
  InterCode interCode = getInterCode(kind);
  interCode->ternary.left = left;
  interCode->ternary.right = right;
  interCode->ternary.res = res;
  return interCode;
}
InterCode getInterCodeIfGoto(Operand label, Operand left, Operand right, const char *relop) {
  InterCode interCode = getInterCode(iIfGoto);
  interCode->ifGoto.right = right;
  interCode->ifGoto.left = left;
  interCode->ifGoto.label = label;
  strcpy(interCode->ifGoto.rel, relop);
  return interCode;
}
InterCode getInterCodeDec(Operand op, int size) {
  InterCode interCode = getInterCode(iDec);
  interCode->dec.op = op;
  interCode->dec.size = size;
  return interCode;
}

Operand getClearOperand() {
  Operand operand = malloc(sizeof(Operand_));
  memset(operand, 0, sizeof(Operand_));
  return operand;
}

Operand getOperand(_OperandKind kind) {
  Operand operand = getClearOperand();
  operand->kind = kind;
  return operand;
}

Operand getOperandInt(_OperandKind kind, int value) {
  Operand operand = getOperand(kind);
  sprintf(operand->un.value, "%d", value);
}

Operand getOperandStr(_OperandKind kind, const char *src) {
  Operand operand = getOperand(kind);
  strcpy(operand->un.value, src);
}