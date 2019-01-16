//
// Created by suun on 1/13/19.
//

#ifndef COMPILER_OBJECTCODE_H
#define COMPILER_OBJECTCODE_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "optimize.h"


typedef struct VarObject {
  int regIndex;
  Operand op;
  struct VarObject *next;
} VarObject;

typedef struct RegObject {
  char name[8];
  int age;
  VarObject *var;
} RegObject;

#define _STACK_VAR_NUM 1 << 10

typedef struct StackObject {
  int length;
  int age[_STACK_VAR_NUM];
  VarObject *varStack[_STACK_VAR_NUM];
} StackObject;

typedef struct ArrayObject {
  char name[32];
  int length;
  struct ArrayObject *next;
}ArrayObject;

void writeObjectFile(char *name);

#endif //COMPILER_OBJECTCODE_H
