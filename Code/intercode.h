#ifndef COMPILER_INTERCODE_H_
#define COMPILER_INTERCODE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
  oVariable,
  oTempVariable,
  oConstant,
  oAddress,
  oTempAddress,
  oLabel,
  oFunction,
  oDebug
} _OperandKind;

typedef struct Operand_ *Operand;
typedef struct Operand_ {
  _OperandKind kind;
  union {
    int tempVarIndex;
    int labelIndex;
    char value[32];
    Operand name;
  } un;
  struct Operand_ *nextArg;
  struct Operand_ *prevArg;
} Operand_;

typedef enum {
  iLabel,
  iFunction,
  iAssign,
  iPlus,
  iMinus,
  iStar,
  iDiv,
  iGetAddress,
  iGetValue,
  iToMemory,
  iGoto,
  iIfGoto,
  iReturn,
  iDec,
  iArg,
  iCall,
  iParam,
  iRead,
  iWrite,
  iDebug,
  iRightAt
} _InterCodeKind;

typedef struct InterCode_ *InterCode;
typedef struct InterCode_ {
  _InterCodeKind kind;
  InterCode next;
  union {
    struct {
      Operand op;
    } unary;
    struct {
      Operand left;
      Operand right;
    } binary;
    struct {
      Operand res;
      Operand left;
      Operand right;
    } ternary;
    struct {
      Operand label;
      Operand left;
      Operand right;
      char rel[20];
    } ifGoto;
    struct {
      Operand op;
      int size;
    } dec;
  };
} InterCode_;

void initInterCodeList();

void insertCode(InterCode interCode);

void writeInterCode(const char *_filename);

void writeOperand(Operand operand);

#endif