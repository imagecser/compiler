#ifndef COMPILER_INTERCODE_H_
#define COMPILER_INTERCODE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
  oVariable,
  oTempVariable,
  oConstant,
  oTempAddress,
  oLabel,
  oFunction,
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
  iGoto,
  iIfGoto,
  iReturn,
  iDec,
  iArg,
  iCall,
  iParam,
  iRead,
  iWrite
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

void writeLastInterCode();

void writeFile(const char *_filename);

void writeOperand(Operand operand);

#endif