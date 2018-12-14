#include "intercode.h"

InterCode *icList;
InterCode headInterCode, tailInterCode;
int icSize;
int icCapacity;
FILE *file;
#define fp(param) fputs(param, file)
#define INTER_CODE_LIST_SIZE 16

void initInterCodeList() {
//  icList = (InterCode *) malloc(sizeof(InterCode) * INTER_CODE_LIST_SIZE);
//  if (icList == NULL) {
//    printf("init inter code list error\n");
//    return;
//  }
//  icSize = 0;
//  icCapacity = INTER_CODE_LIST_SIZE;
  headInterCode = tailInterCode = malloc(sizeof(InterCode_));
}

void insertCode(InterCode interCode) {
//  if (icSize >= icCapacity) {
//    icCapacity *= 2;
//    icList = (InterCode *) realloc(icList, icCapacity * sizeof(InterCode));
//  }
//  icList[icSize++] = interCode;
  if (interCode == NULL)
    return;
  tailInterCode->next = interCode;
  tailInterCode = interCode;
}

void writeInterCode(const char *_filename) {
  if (!strcmp(_filename, "stdout"))
    file = stdout;
  else
    file = fopen(_filename, "w");
  for (InterCode code = headInterCode->next; code != NULL; code = code->next) {
    switch (code->kind) {
#define _uf(f) fp(f); writeOperand(code->unary.op); break;
#define _ufb(f, b) fp(f); writeOperand(code->unary.op); fp(b); break;
#define _bm(m) writeOperand(code->binary.left); fp(m);\
               writeOperand(code->binary.right); break;
#define _tfb(f, b) writeOperand(code->ternary.res); fp(f);\
                   writeOperand(code->ternary.left); fp(b);\
                   writeOperand(code->ternary.right); break;
      case iLabel:_ufb("LABEL", " : ");
      case iFunction:
      _ufb("FUNCTION", " : ");
      case iAssign:_bm(" := ");
      case iPlus:_tfb(" := ", " + ");
      case iMinus: _tfb(" := ", " - ");
      case iStar: _tfb(" := ", " * ");
      case iDiv: _tfb(" := ", " / ");
      case iGetAddress: _tfb(" := &", " + ");
      case iGoto: _uf("GOTO ");
      case iIfGoto: {
        fp("IF ");
        writeOperand(code->ifGoto.left);
        fprintf(file, " %s ", code->ifGoto.rel);
        writeOperand(code->ifGoto.right);
        fp(" GOTO ");
        writeOperand(code->ifGoto.label);
        break;
      }
      case iReturn: _uf("RETURN ");
      case iDec: {
        fp("DEC ");
        writeOperand(code->dec.op);
        fprintf(file, " %d ", code->dec.size);
        break;
      }
      case iArg: _uf("ARG ");
      case iCall: _bm(" := CALL ");
      case iParam: _uf("PARAM ");
      case iRead: _uf("READ ");
      case iWrite: _uf("WRITE ");
      default:
        fp("error");
        break;
    }
    fp("\n");
  }
  fclose(file);
}

void writeOperand(Operand operand) {
  if (operand == NULL) {
    fp("t0  ");
    return;
  }
  char s[128];
  memset(s, 0, sizeof(s));
  switch (operand->kind) {
    case oVariable: strcpy(s, operand->un.value);
      break;
    case oTempVariable:sprintf(s, "t%d", operand->un.tempVarIndex);
      break;
    case oConstant:sprintf(s, "#%s", operand->un.value);
      break;
    case oTempAddress:sprintf(s, "*t%d", operand->un.tempVarIndex);
      break;
    case oLabel:sprintf(s, "label%d", operand->un.labelIndex);
      break;
    case oFunction:strcpy(s, operand->un.value);
      break;
    default:break;
  }
  fp(s);
}
