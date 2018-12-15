
#include "semantic.h"
#include "intercode.h"

char msg[100];
FieldList hashTable[HASH_SIZE];

object *getChild(object *root, int i) {
  object *ret = root->child;
  while (i--) ret = ret->next;
  return ret;
}

int countChild(object *root) {
  int count = 0;
  object *foo = root->child;
  for (; foo != NULL; foo = foo->next, count++);
  return count;
}

unsigned int hashFunc(char *name) {
  unsigned int val = 0, i;
  for (; *name; ++name) {
    val = (val << 2u) + *name;
    if ((i = val & ~HASH_SIZE)) val = (val ^ (i >> 12u)) & HASH_SIZE;
  }
  return val;
}

void initHashTable() {
  int i;
  for (i = 0; i < HASH_SIZE; i++);
  hashTable[i] = NULL;

  Type returnReadType = malloc(sizeof(Type_));
  returnReadType->kind = KBASIC;
  returnReadType->basic_ = _INT;
  Type readType = malloc(sizeof(Type_));
  readType->kind = KFUNC;
  readType->func_.funcType = returnReadType;
  readType->func_.paramNum = 0;
  readType->func_.params = NULL;
  FieldList readField = malloc(sizeof(FieldList_));
  readField->name = "read";
  readField->type = readType;
  readField->isArg = false;
  insertSymbol(readField);

  Type returnWriteType = malloc(sizeof(Type_));
  returnWriteType->kind = KBASIC;
  returnWriteType->basic_ = _INT;
  Type writeParamType = malloc(sizeof(Type_));
  writeParamType->kind = KBASIC;
  writeParamType->basic_ = _INT;
  FieldList writeParamField = malloc(sizeof(FieldList_));
  writeParamField->name = "dest";
  writeParamField->type = writeParamType;
  writeParamField->isArg = true;
  writeParamField->tail = NULL;
  Type writeType = malloc(sizeof(Type_));
  writeType->kind = KFUNC;
  writeType->func_.funcType = returnWriteType;
  writeType->func_.paramNum = 1;
  writeType->func_.params = writeParamField;
  FieldList writeField = malloc(sizeof(FieldList_));
  writeField->name = "write";
  writeField->type = writeType;
  writeField->isArg = false;
  insertSymbol(writeField);

}

bool insertSymbol(FieldList field) {
  unsigned int key;
  if (field == NULL) return 0;
  if (field->name == NULL) return 0;
  if (field->type->kind == KFUNC)
    key = hashFunc(1 + field->name);
  else
    key = hashFunc(field->name);
  if (hashTable[key] == NULL) {
    hashTable[key] = field;
    return true;
  }
  while (1) {
    key++;
    key = key % HASH_SIZE;
    if (hashTable[key] == NULL) {
      hashTable[key] = field;
      return true;
    }
  }
  return false;
}

FieldList findSymbol(char *name, bool func) {
  unsigned key;
  if (name == NULL) return NULL;
  if (func)
    key = hashFunc(1 + name);
  else
    key = hashFunc(name);
  FieldList field = hashTable[key];
  while (field != NULL) {
    if (strcmp(name, field->name) == 0) {
      if (func && field->type->kind == KFUNC) return field;
      if (!func && field->type->kind != KFUNC) return field;
    }
    key++;
    key = key % HASH_SIZE;
    field = hashTable[key];
  }
  return NULL;
}

bool isTypeEqual(Type p1, Type p2) {
  if (p1 == NULL || p2 == NULL) return false;
  if (p1->kind != p2->kind) return false;
  switch (p1->kind) {
    case KBASIC: {
      return (p1->basic_ == p2->basic_);
    }
    case KARRAY: {
      return (isTypeEqual(p1->array_.type, p2->array_.type));
    }
    case KSTRUCT: {
      FieldList f1 = p1->struct_, f2 = p2->struct_;
      if (f1 != NULL && f2 != NULL) {
        while (f1 != NULL && f2 != NULL) {
          if (!isTypeEqual(f1->type, f2->type)) return false;
          f1 = f1->tail;
          f2 = f2->tail;
        }
        if (f1 == NULL && f2 == NULL) return true;
      }
      return false;
    }
    case KFUNC: {
      if (p1->func_.paramNum != p2->func_.paramNum) return false;
      FieldList param1 = p1->func_.params, param2 = p2->func_.params;
      for (int i = 0; i < p1->func_.paramNum; i++) {
        if (!isTypeEqual(param1->type, param2->type)) return false;
        param1 = param1->tail;
        param2 = param2->tail;
      }
      return true;
    }
    default: { return false; }
  }
}

/*
VarDec : ID {_ac(1);}
       | VarDec LB INT RB {_ac(4);}
;
*/
FieldList goVarDec(object *root, Type type) {
  object *tmp = root;
  int i = 0;
  while (tmp->child->type != TID) {
    tmp = tmp->child;
    i++;
  }
  char *idName = tmp->child->vstr;
  FieldList field = malloc(sizeof(FieldList_));
  field->isArg = false;
  field->name = idName;

  switch (i) {
    // e.g. a
    case 0: {
      field->type = type;
      return field;
    }
      // e.g. a[1]
    case 1: {
      Type var = malloc(sizeof(Type_));
      var->kind = KARRAY;
      // var->array_.size = root->child->next->next->vint;
      var->array_.size = getChild(root, 2)->vint;
      var->array_.type = type;
      field->type = var;
      return field;
    }
      // e.g. a[1][2]
    case 2: {
      Type inner = malloc(sizeof(Type_)),
          outer = malloc(sizeof(Type_));
      inner->kind = KARRAY;
      // inner->array_.size = root->child->next->next->vint;
      inner->array_.size = getChild(root, 2)->vint;
      inner->array_.type = type;
      outer->kind = KARRAY;
      // outer->array_.size = root->child->child->next->next->vint;
      outer->array_.size = getChild(getChild(root, 0), 2)->vint;
      outer->array_.type = inner;
      field->type = outer;
      return field;
    }
    default: {
      printf("error in goVarDec case %d.\n", i);
      return NULL;
    }
  }
}

/*
Specifier : TYPE {_ac(1);}
          | StructSpecifier {_ac(1);}
;
StructSpecifier : STRUCT OptTag LC DefList RC {_ac(5);}
                | STRUCT Tag {_ac(2);}
;
*/

Type goSpecifier(object *root) {
  Type foo = malloc(sizeof(Type_));
  // Specifier: TYPE
  if (root->child->type == TTYPE) {
    foo->kind = KBASIC;
    if (!strcmp(root->child->vstr, "int"))
      foo->basic_ = _INT;
    else
      foo->basic_ = _FLOAT;
    return foo;
  }
  // Specifier: StructSpecifier
  foo->kind = KSTRUCT;
  object *structSpecifier = getChild(root, 0);
  if (countChild(getChild(root, 0)) == 2) {  // STRUCT Tag
    char *idName = getChild(getChild(structSpecifier, 1), 0)->vstr;
    FieldList field = findSymbol(idName, false);
    if (field == NULL) { ;
      sprintf(msg, "Undefined structure \"%s\".", idName);
      sem_error(17, root->fl, msg);
      foo->struct_ = NULL;
      return foo;
    } else if (field->type != NULL) {
      return field->type;
    } else {
      foo->struct_ = NULL;
      return foo;
    }
  } else {  // StructSpecifier : STRUCT OptTag LC DefList RC
    object *defList = getChild(structSpecifier, 3);
    while (defList->type != TNUL) {        // DefList : Def DefList
      object *def = getChild(defList, 0);  // Def : Specifier DecList SEMI
      Type basicType = goSpecifier(getChild(def, 0));
      object *decList = getChild(def, 1);
      while (decList->type != TNUL) {  // DecList: Dec COMMA DecList
        // Dec : VarDec
        FieldList field =
            goVarDec(getChild(getChild(decList, 0), 0), basicType);
        if (countChild(getChild(decList, 0)) != 1) { ;
          sprintf(msg, "Variable %s in struct is initialized.", field->name);
          sem_error(15, def->fl, msg);
        }
        if (findSymbol(field->name, false) != NULL) { ;
          sprintf(msg, "Redefined variable \"%s\".", field->name);
          sem_error(3, def->fl, msg);
        } else {
          insertSymbol(field);
          field->tail = foo->struct_;
          foo->struct_ = field;
        }
        if (countChild(decList) == 3)
          decList = getChild(decList, 2);
        else
          break;
      }
      defList = getChild(defList, 1);
    }  // StructSpecifier : STRUCT OptTag LC DefList RC
    if (getChild(structSpecifier, 1)->type != TNUL) {
      // OptTag : ID | ;
      FieldList field = malloc(sizeof(FieldList_));
      field->isArg = false;
      field->type = foo;
      field->name = getChild(getChild(structSpecifier, 1), 0)->vstr;
      if (findSymbol(field->name, false) != NULL) { ;
        sprintf(msg, "Duplicated name \"%s\".", field->name);
        sem_error(16, root->fl, msg);
      } else {
        insertSymbol(field);
      }
    }
    return foo;
  }
}

/*
ExtDefList : ExtDef ExtDefList | ;
ExtDef : Specifier ExtDecList SEMI
       | Specifier SEMI
       | Specifier FunDec CompSt
*/
void goExtDefList(object *root) {
  object *extDefList = root;
  while (countChild(extDefList) != 0) {
    object *extDef = getChild(extDefList, 0);
    Type basicType = goSpecifier(getChild(extDef, 0));
    if (!strcmp(getChild(extDef, 1)->vstr, "ExtDecList")) {
      // ExtDecList : VarDec
      //  | VarDec COMMA ExtDecList;
      object *extDecList = getChild(extDefList, 1);
      while (extDecList != NULL) {
        FieldList field = goVarDec(getChild(extDecList, 0), basicType);
        if (findSymbol(field->name, false) != NULL) {
          sprintf(msg, "Redefined variable \"%s\".", field->name);
          sem_error(3, extDef->fl, msg);
        } else {
          insertSymbol(field);
        }
        if (countChild(extDecList) == 3)
          extDecList = getChild(extDecList, 2);
        else
          break;
      }
    } else if (!strcmp(getChild(extDef, 1)->vstr, "FunDec")) {
      // FunDec : ID LP VarList RP
      //  | ID LP RP;
      object *funDec = getChild(extDef, 1);
      FieldList field = malloc(sizeof(FieldList_));
      field->isArg = false;
      field->name = getChild(funDec, 0)->vstr;
      Type type = malloc(sizeof(Type_));
      type->kind = KFUNC;
      type->func_.funcType = basicType;
      type->func_.paramNum = 0;
      type->func_.params = NULL;

      // insert code begin
      Operand funcOperand = getOperandStr(oFunction, field->name);
      InterCode funcInterCode = getInterCodeUnary(iFunction, funcOperand);
      insertCode(funcInterCode);
      // end

      if (countChild(funDec) == 4) {
        object *varList = getChild(funDec, 2);
        // VarList : ParamDec COMMA VarList
        // | ParamDec;
        // ParamDec : Specifier VarDec;
        while (varList != NULL) {
          Type varType = goSpecifier(getChild(getChild(varList, 0), 0));
          FieldList varField =
              goVarDec(getChild(getChild(varList, 0), 1), varType);
          if (findSymbol(varField->name, false) != NULL) {
            sprintf(msg, "Redefined variable \"%s\".", varField->name);
            sem_error(3, funDec->fl, msg);
          } else {
            varField->isArg = true;
            insertSymbol(varField);
          }
          type->func_.paramNum++;
          varField->tail = type->func_.params;
          type->func_.params = varField;

          // insert code begin
//          Operand paramOperand = getOperandStr(oVariable, varType->func_.params->name);
          Operand paramOperand = getOperandStr(oVariable, varField->name);
          InterCode paramInterCode = getInterCodeUnary(iParam, paramOperand);
          insertCode(paramInterCode);
          //end

          if (countChild(varList) == 3)
            varList = getChild(varList, 2);
          else
            break;
        }
      }
      field->type = type;
      if (findSymbol(field->name, true) != NULL) { ;
        sprintf(msg, "Redefined function \"%s\".", field->name);
        sem_error(4, funDec->fl, msg);
      } else {
        field->isArg = false;
        insertSymbol(field);
      }
      goCompSt(getChild(extDef, 2), basicType);

    } else {  // Specifier SEMI
    }
    if (getChild(extDefList, 1)->type == TNUL)
      break;
    else
      extDefList = getChild(extDefList, 1);
  }
}

// CompSt : LC DefList StmtList RC
void goCompSt(object *compSt, Type funcType) {
  goDefList(getChild(compSt, 1));
  object *stmtList = getChild(compSt, 2);
  while (stmtList->type != TNUL) {
    object *stmt = getChild(stmtList, 0);
    goStmt(stmt, funcType);
    stmtList = getChild(stmtList, 1);
  }
}

// DefList : Def DefList | ;
// Def : Specifier DecList SEMI
void goDefList(object *defList) {
  while (defList->type != TNUL) {
    object *def = getChild(defList, 0);
    Type basicType = goSpecifier(getChild(def, 0));
    object *decList = getChild(def, 1);
    // DecList : Dec {_ac(1);}
    //        | Dec COMMA DecList {_ac(3);}
    while (decList->type != TNUL) {
      // Dec : VarDec {_ac(1);}
      //    | VarDec ASSIGNOP Exp {_ac(3);}
      object *dec = getChild(decList, 0);
      FieldList field = goVarDec(getChild(dec, 0), basicType);
      // insert code begin
      if (field->type->kind == KARRAY) {
        Operand operand = getOperandStr(oVariable, field->name);

        InterCode interCode = getInterCodeDec(operand, getSize(field->type, false));
        insertCode(interCode);
      }
      if (countChild(dec) == 3) {
        Operand place = getClearOperand();
        if (getChild(getChild(dec, 2), 0)->type == TINT) {
          goExp(getChild(dec, 2), NULL);
          place->kind = oConstant;
          sprintf(place->un.value, "%d", getChild(getChild(dec, 2), 0)->vint);
        } else {
          place->kind = oTempVariable;
          goExp(getChild(dec, 2), place);
        }
        if (place->kind != oVariable || strcpy(place->un.value, field->name) != 0) {
          Operand leftOperand = getOperandStr(oVariable, field->name);
          InterCode addressInterCode = getInterCodeBinary(iAssign, leftOperand, place);
          insertCode(addressInterCode);
        }
      }
      // end
      if (findSymbol(field->name, false) != NULL) { ;
        sprintf(msg, "Redefined variable \"%s\".", field->name);
        sem_error(3, decList->fl, msg);
      } else {
        insertSymbol(field);
      }
      if (countChild(decList) == 3)
        decList = getChild(decList, 2);
      else
        break;
    }
    defList = getChild(defList, 1);
  }
}

/*
Stmt : Exp SEMI
    | CompSt
    | RETURN Exp SEMI
    | IF LP Exp RP Stmt
    | IF LP Exp RP Stmt ELSE Stmt
    | WHILE LP Exp RP Stmt
;
*/
void goStmt(object *stmt, Type funcType) {
  if (!strcmp(getChild(stmt, 0)->vstr, "Exp")) {
    goExp(getChild(stmt, 0), NULL);
  } else if (!strcmp(getChild(stmt, 0)->vstr, "CompSt")) {
    goCompSt(getChild(stmt, 0), funcType);
  } else if (!strcmp(getChild(stmt, 0)->vstr, "RETURN")) {

//    Type returnType = goExp(getChild(stmt, 1), NULL);
//    if (!isTypeEqual(returnType, funcType))
//      sem_error(8, stmt->fl, "Type mismatched for return.");
//    // insert code begin
//    Operand operand = getClearOperand();
//    if (getChild(getChild(stmt, 1), 0)->type == TINT) {
//      operand->kind = oConstant;
//      sprintf(operand->un.value, "%d", getChild(getChild(stmt, 1), 0)->vint);
//    }
    Type type;
    Operand operand;
    if (getChild(getChild(stmt, 1), 0)->type == TINT) {
      operand = getOperandInt(oConstant, getChild(getChild(stmt, 1), 0)->vint);
      type = goExp(getChild(stmt, 1), NULL);
    } else {
      operand = getClearOperand();
      type = goExp(getChild(stmt, 1), operand);
    }
    if (!isTypeEqual(type, funcType))
      sem_error(8, stmt->fl, "Type mismatched for return.");
    InterCode interCode = getInterCodeUnary(iReturn, operand);
    insertCode(interCode);
    // end
  } else if (!strcmp(getChild(stmt, 0)->vstr, "WHILE")) {  // WHILE LP Exp RP Stmt
    // insert code begin
    /*
     * label1
     * if condition goto label2
     * goto label3
     * label2
     * code
     */
    // label1 condition label2 code goto label3
    Operand firstLabelOperand = getLabelOperand(),
        secondLabelOperand = getLabelOperand(),
        thirdLabelOperand = getLabelOperand();

    InterCode firstLabelInterCode = getLabelInterCode(firstLabelOperand),
        secondLabelInterCode = getLabelInterCode(secondLabelOperand),
        thirdLabelInterCode = getLabelInterCode(thirdLabelOperand);

    // label1
    insertCode(firstLabelInterCode);
    // condition
    Type conditionType = goCondition(getChild(stmt, 2), secondLabelOperand, thirdLabelOperand);
    if (conditionType->kind != KBASIC || conditionType->basic_ != _INT)
      sem_error(5, stmt->fl, "Only type INT could be used for judgement.");
    // label2
    insertCode(secondLabelInterCode);
    // code
    goStmt(getChild(stmt, 4), funcType);
    //goto label1
    InterCode gotoLabelInterCode = getGotoLabelInterCode(firstLabelOperand);
    insertCode(gotoLabelInterCode);
    // label3
    insertCode(thirdLabelInterCode);

  } else {
    Operand firstLabelOperand = getLabelOperand(),
        secondLabelOperand = getLabelOperand();

    // if condition goto label1 goto label2
    Type conditionType = goCondition(getChild(stmt, 2), firstLabelOperand, secondLabelOperand);
    if (conditionType->kind != KBASIC || conditionType->basic_ != _INT)
      sem_error(5, stmt->fl, "Only type INT could be used for judgement.");

    // label1
    InterCode firstLabelInterCode = getLabelInterCode(firstLabelOperand);
    insertCode(firstLabelInterCode);
    // code
    goStmt(getChild(stmt, 4), funcType);
    if (countChild(stmt) == 4) {
      // label2
      InterCode secondLabelInterCode = getLabelInterCode(secondLabelOperand);
      insertCode(secondLabelInterCode);
    } else {
      // goto label3
      Operand thirdLabelOperand = getLabelOperand();
      InterCode gotoThirdLabelInterCode = getGotoLabelInterCode(thirdLabelOperand);
      insertCode(gotoThirdLabelInterCode);
      // label2
      InterCode secondLabelInterCode = getLabelInterCode(secondLabelOperand);
      insertCode(secondLabelInterCode);
      // else code
      goStmt(getChild(stmt, 6), funcType);
      // label3
      InterCode thirdLabelInterCode = getLabelInterCode(thirdLabelOperand);
      insertCode(thirdLabelInterCode);
    }
  }
}

/*
Exp : Exp ASSIGNOP Exp
    | Exp AND Exp
    | Exp OR Exp
    | Exp RELOP Exp
    | Exp PLUS Exp
    | Exp MINUS Exp
    | Exp STAR Exp
    | Exp DIV Exp
    | LP Exp RP
    | MINUS Exp
    | NOT Exp
    | ID LP Args RP
    | ID LP RP
    | Exp LB Exp RB
    | Exp DOT ID
    | ID
    | INT
    | FLOAT
;
*/
Type goExp(object *exp, Operand upshot) {
  if (exp == NULL || exp->type == TNUL) return NULL;
  char *firstStr = getChild(exp, 0)->vstr;
  char *secondStr = (countChild(exp) > 1) ? getChild(exp, 1)->vstr : NULL;
  if (getChild(exp, 0)->type == TID && countChild(exp) == 1) {
    FieldList field = findSymbol(firstStr, false);
    if (field == NULL) {
      sprintf(msg, "Undefined variable \"%s\".", firstStr);
      sem_error(1, exp->fl, msg);
      return NULL;
    }
    if (upshot != NULL) {
      upshot->kind = oVariable;
      strcpy(upshot->un.value, getChild(exp, 0)->vstr);
    }
    return field->type;
  } else if (getChild(exp, 0)->type == TINT) {
    Type foo = malloc(sizeof(Type_));
    foo->kind = KBASIC;
    foo->basic_ = _INT;
    if (upshot != NULL) {
      Operand operand = getOperandInt(oConstant, getChild(exp, 0)->vint);
      setOperandTemp(upshot);
      InterCode interCode = getInterCodeBinary(iAssign, upshot, operand);
      insertCode(interCode);
    }
    return foo;
    // } else if (!strcmp(firstStr, "FLOAT")) {
  } else if (getChild(exp, 0)->type == TFLOAT) {
    Type foo = malloc(sizeof(Type_));
    foo->kind = KBASIC;
    foo->basic_ = _FLOAT;
    return foo;
  } else if (!strcmp(firstStr, "LP")) {
    return goExp(getChild(exp, 1), upshot);
  } else if (!strcmp(firstStr, "MINUS")) {
    object *second = getChild(exp, 1);
    if (getChild(second, 0)->type == TINT && upshot != NULL && getChild(second, 0)->vint >= 0) {
      Type foo = goExp(second, NULL);
      upshot->kind = oConstant;
      sprintf(upshot->un.value, "-%d", getChild(second, 0)->vint);
      return foo;
    }
    Operand rightOperand = getClearOperand();
    Type foo = goExp(second, rightOperand);
    if (foo == NULL) return NULL;
    if (upshot != NULL) {
      Operand zeroOperand = getOperandStr(oConstant, "0");
      setOperandTemp(upshot);
      InterCode interCode = getInterCodeTernary(iMinus, upshot, zeroOperand, rightOperand);
      insertCode(interCode);
    }
    return foo;
  } else if (!strcmp(firstStr, "NOT")) {
    if (upshot != NULL) {
      // upshot = 0;
      Operand zeroOperand = getOperandStr(oConstant, "0");
      InterCode initPosInterCode = getInterCodeBinary(iAssign, upshot, zeroOperand);
      insertCode(initPosInterCode);

      Operand firstLabelOperand = getLabelOperand(), secondLabelOperand = getLabelOperand();
      Type foo = goCondition(exp, firstLabelOperand, secondLabelOperand);

      // first label
      InterCode firstLabelInterCode = getLabelInterCode(firstLabelOperand);
      insertCode(firstLabelInterCode);

      // upshot = 1;
      Operand oneOperand = getOperandStr(oConstant, "1");
      InterCode setPosInterCode = getInterCodeBinary(iAssign, upshot, oneOperand);
      insertCode(setPosInterCode);
      // second label
      InterCode secondLabelInterCode = getLabelInterCode(secondLabelOperand);
      insertCode(secondLabelInterCode);
      return foo;
    } else {
      return goExp(getChild(exp, 1), NULL);
    }
  } /* Exp AND Exp
     | Exp OR Exp
     | Exp RELOP Exp
     | Exp PLUS Exp
     | Exp MINUS Exp
     | Exp STAR Exp
     | Exp DIV Exp*/
  else if (!strcmp(secondStr, "PLUS") || !strcmp(secondStr, "MINUS") ||
      !strcmp(secondStr, "STAR") || !strcmp(secondStr, "DIV")) {
    Type leftType, rightType;
    Operand leftOperand, rightOperand;

    if (getChild(getChild(exp, 0), 0)->type == TINT) {
      leftOperand = getOperandInt(oConstant, getChild(getChild(exp, 0), 0)->vint);
      leftType = goExp(getChild(exp, 0), NULL);
    } else {
      leftOperand = getOperand(oTempVariable);
      leftType = goExp(getChild(exp, 0), leftOperand);
    }
    if (getChild(getChild(exp, 2), 0)->type == TINT) {
      rightOperand = getOperandInt(oConstant, getChild(getChild(exp, 2), 0)->vint);
      rightType = goExp(getChild(exp, 0), NULL);
    } else {
      rightOperand = getOperand(oTempVariable);
      rightType = goExp(getChild(exp, 2), rightOperand);
    }

    if (!isTypeEqual(leftType, rightType)) {
      sem_error(7, exp->fl, "Type mismatched for operands.");
      return leftType;
    }

    InterCode interCode;
    if (!strcmp(getChild(exp, 1)->vstr, "PLUS"))
      interCode = getInterCode(iPlus);
    else if (!strcmp(getChild(exp, 1)->vstr, "MINUS"))
      interCode = getInterCode(iMinus);
    else if (!strcmp(getChild(exp, 1)->vstr, "STAR"))
      interCode = getInterCode(iStar);
    else  // DIV
      interCode = getInterCode(iDiv);

    if (upshot != NULL) {
      setOperandTemp(upshot);
      if (leftOperand->kind == oConstant && rightOperand->kind == oConstant) {
        int result, leftNum = getChild(getChild(exp, 0), 0)->vint,
            rightNum = getChild(getChild(exp, 2), 0)->vint;
        switch (interCode->kind) {
          case iPlus:result = leftNum + rightNum;
            break;
          case iMinus:result = leftNum - rightNum;
            break;
          case iStar:result = leftNum * rightNum;
            break;
          case iDiv:result = leftNum / rightNum;
            break;
          default: result = 0;
            break;
        }
        free(interCode);
        free(leftOperand);
        free(rightOperand);
        rightOperand = getOperandInt(oConstant, result);
        interCode = getInterCodeBinary(iAssign, upshot, rightOperand);
      } else {
        interCode->ternary.left = leftOperand;
        interCode->ternary.right = rightOperand;
        interCode->ternary.res = upshot;
      }
      insertCode(interCode);
    }
    return leftType;
  } else if (!strcmp(secondStr, "AND") || !strcmp(secondStr, "OR") ||
      getChild(exp, 1)->type == TREL) {
    if (upshot == NULL) {
      Operand labelOperand = getLabelOperand();
      Type varType = goCondition(exp, labelOperand, labelOperand);
      insertCode(getLabelInterCode(labelOperand));  // end label
      return varType;
    } else {
      Operand firstLabelOperand = getLabelOperand(), secondLabelOperand = getLabelOperand();
      Operand zeroOperand = getOperandStr(oConstant, "0");
      InterCode setZeroInterCode = getInterCodeBinary(iAssign, upshot, zeroOperand);
      insertCode(setZeroInterCode);  // set 0
      Type varType = goCondition(exp, firstLabelOperand, secondLabelOperand);
      InterCode firstLabelInterCode = getLabelInterCode(firstLabelOperand);
      insertCode(firstLabelInterCode);  // first label
      Operand oneOperand = getOperandStr(oConstant, "1");
      InterCode setOneInterCode = getInterCodeBinary(iAssign, upshot, oneOperand);
      insertCode(setOneInterCode);
      InterCode secondLabelInterCode = getLabelInterCode(secondLabelOperand);
      insertCode(secondLabelInterCode);
      return varType;
    }
    /*
    Type leftType = goExp(getChild(exp, 0), NULL),
        rightType = goExp(getChild(exp, 2), NULL);
    if (!isTypeEqual(leftType, rightType))
      sem_error(7, exp->fl, "Type mismatched for operands.");
    if (leftType->kind != KBASIC || leftType->kind != _INT)
      sem_error(5, exp->fl, "Only type INT could be used for judgement.");
    Type foo = malloc(sizeof(Type_));
    foo->kind = KBASIC;
    foo->basic_ = _INT;
    return foo;
  } else if (getChild(exp, 1)->type == TREL) {
    Type leftType = goExp(getChild(exp, 0), NULL),
        rightType = goExp(getChild(exp, 2), NULL);
    if (!isTypeEqual(leftType, rightType))
      sem_error(7, exp->fl, "Type mismatched for operands.");
    Type foo = malloc(sizeof(Type_));
    foo->kind = KBASIC;
    foo->basic_ = _INT;
    return foo;
    */
  } else if (!strcmp(secondStr, "ASSIGNOP")) {
    object *dest = getChild(exp, 0);
    Operand leftOperand = getOperand(oTempVariable);
    Type foo = goExp(getChild(exp, 0), leftOperand), bar;
    Operand rightOperand;
    if (getChild(getChild(exp, 2), 0)->type == TINT) {
      bar = goExp(getChild(exp, 2), NULL);
      rightOperand = getOperandInt(oConstant, getChild(getChild(exp, 2), 0)->vint);
    } else {
      rightOperand = getOperand(oTempVariable);
      bar = goExp(getChild(exp, 2), rightOperand);
    }

    if ((countChild(dest) == 1 && getChild(dest, 0)->type == TID) ||
        (countChild(dest) == 3 && !strcmp(getChild(dest, 0)->vstr, "Exp") &&
            !strcmp(getChild(dest, 1)->vstr, "DOT") &&
            getChild(dest, 2)->type == TID) ||
        (countChild(dest) == 4 && !strcmp(getChild(dest, 0)->vstr, "Exp") &&
            // !strcmp(getChild(dest, 1)->vstr, "LB") &&
            // !strcmp(getChild(dest, 3)->vstr, "RB") &&
            !strcmp(getChild(dest, 2)->vstr, "Exp"))) {
      if (isTypeEqual(foo, bar)) {
        InterCode assignInterCode = getInterCodeBinary(iAssign, leftOperand, rightOperand);
        insertCode(assignInterCode);
        if (upshot != NULL) {
          InterCode upInterCode = getInterCodeBinary(iAssign, upshot, rightOperand);
          insertCode(upInterCode);
        }
      } else {
        sem_error(5, exp->fl, "Type mismatched for assignment.");
      }
    } else {
      sem_error(6, exp->fl,
                "The left-hand side of an assignment must be a variable.");
    }
    return foo;
  } /*
     | ID LP Args RP
     | ID LP RP
   */
  else if (getChild(exp, 0)->type == TID) {
    FieldList foo = findSymbol(getChild(exp, 0)->vstr, true);
    if (foo == NULL) {
      FieldList bar = findSymbol(getChild(exp, 0)->vstr, false);
      if (bar != NULL) {
        sprintf(msg, "\"%s\" is not a function.", getChild(exp, 0)->vstr);
        sem_error(11, exp->fl, msg);
      } else {
        sprintf(msg, "Undefined function \"%s\".", getChild(exp, 0)->vstr);
        sem_error(2, exp->fl, msg);
      }
      return NULL;
    }
    Type definedType, type = malloc(sizeof(Type_));
//    if (strcmp(getChild(exp, 0)->vstr, "read") != 0 && strcmp(getChild(exp, 0)->vstr, "write") != 0)
    definedType = foo->type;
//    else
//      definedType = type;
    type->kind = KFUNC;
    type->func_.paramNum = 0;
    type->func_.params = NULL;
    if (countChild(exp) == 3) {
      if (!strcmp(getChild(exp, 0)->vstr, "read")) {
        if (upshot != NULL) {
          setOperandTemp(upshot);
          InterCode readInterCode = getInterCodeUnary(iRead, upshot);
          insertCode(readInterCode);
        }
      } else {
        Operand funcOperand = getOperandStr(oFunction, getChild(exp, 0)->vstr);
        if (upshot == NULL)
          upshot = getClearOperand();
        setOperandTemp(upshot);
        InterCode interCode = getInterCodeBinary(iCall, upshot, funcOperand);
        insertCode(interCode);
      }
    } else if (!strcmp(getChild(exp, 0)->vstr, "write")) {
      Type varType;
      Operand argOperand;
      object *argExp = getChild(getChild(exp, 2), 0);
      if (getChild(exp, 0)->type == TINT) {
        varType = goExp(argExp, 0);
        argOperand = getOperandStr(oConstant, getChild(exp, 0)->vstr);
      } else {
        argOperand = getOperand(oTempVariable);
        varType = goExp(argExp, argOperand);
      }
      FieldList tmpField = malloc(sizeof(FieldList_));
      tmpField->isArg = false;
      tmpField->type = varType;
      type->func_.paramNum++;
      tmpField->tail = type->func_.params;
      type->func_.params = tmpField;
      InterCode interCode = getInterCodeUnary(iWrite, argOperand);
      insertCode(interCode);
    } else {
      // Args : Exp COMMA Args | Exp;
      object *args = getChild(exp, 2);
      while (args->type != TNUL) {

        Type varType;
        Operand argOperand;
        if (getChild(getChild(args, 0), 0) == TINT) {
          argOperand = getOperandInt(oConstant, getChild(getChild(args, 0), 0)->vint);
          varType = goExp(getChild(args, 0), NULL);
        } else {
          argOperand = getOperand(oTempVariable);
          varType = goExp(getChild(args, 0), argOperand);
          if (varType->kind == KARRAY && argOperand->kind == oVariable) {
            char tmp[128];
            sprintf(tmp, "&%s", argOperand->un.value);
            strcpy(argOperand->un.value, tmp);
          }
        }
        InterCode argInterCode = getInterCodeUnary(iArg, argOperand);
        insertCode(argInterCode);

        FieldList tmpField = malloc(sizeof(FieldList_));
        tmpField->isArg = false;
        tmpField->type = varType;
        type->func_.paramNum++;
        tmpField->tail = type->func_.params;
        type->func_.params = tmpField;
        if (countChild(args) == 3)
          args = getChild(args, 2);
        else
          break;
      }
      Operand funcOperand = getOperandStr(oFunction, getChild(exp, 0)->vstr);
      if (upshot == NULL)
        upshot = getClearOperand();
      setOperandTemp(upshot);
      InterCode funcInterCode = getInterCodeBinary(iCall, upshot, funcOperand);
      insertCode(funcInterCode);
    }
    if (!isTypeEqual(type, definedType)) {
      sprintf(msg, "Params wrong in function \"%s\".", getChild(exp, 0)->vstr);
      sem_error(9, exp->fl, msg);
    }
//      return NULL;
//    } else {
    return definedType->func_.funcType;
  } else if (!strcmp(secondStr, "DOT")) {  // Exp DOT ID
    Type foo = goExp(getChild(exp, 0), NULL);
    if (foo->kind != KSTRUCT) {  // not a struct
      object *var = getChild(exp, 0);
      char *idName = "1";
      switch (countChild(var)) {
        case 1: {
          if (getChild(var, 0)->type == TID) idName = getChild(var, 0)->vstr;
          break;
        }
        case 3: {
          if (getChild(var, 2)->type == TID) idName = getChild(var, 0)->vstr;
          break;
        }
        case 4: {
          if (!strcmp(getChild(var, 0)->vstr, "Exp") &&
              getChild(getChild(var, 0), 0)->type == TID)
            idName = getChild(getChild(var, 0), 0)->vstr;
          break;
        }
        default:break;
      }
      if (findSymbol(idName, false) != NULL)
        sem_error(13, exp->fl, "Illegal use of \".\".");
      return foo;
    }
    char *idName = getChild(exp, 2)->vstr;
    FieldList field = foo->struct_;
    while (field != NULL) {
      if (!strcmp(field->name, idName)) return field->type;
      field = field->tail;
    }
    sprintf(msg, "Non-existed field \"%s\".", getChild(exp, 2)->vstr);
    sem_error(14, exp->fl, msg);
    return foo;
  } else if (!strcmp(getChild(exp, 1)->vstr, "LB")) {  // Exp LB Exp RB
    Operand baseOperand = getClearOperand(), subOperand, offsetOperand;
    Type foo = goExp(getChild(exp, 0), baseOperand);
    if (foo->kind != KARRAY) {  // not an array
      object *var = getChild(exp, 0);
      char *idName = "1";
      switch (countChild(var)) {
        case 1: {
          if (getChild(var, 0)->type == TID) idName = getChild(var, 0)->vstr;
          break;
        }
        case 3: {
          if (getChild(var, 2)->type == TID) idName = getChild(var, 0)->vstr;
          break;
        }
        case 4: {
          if (!strcmp(getChild(var, 0)->vstr, "Exp") &&
              getChild(getChild(var, 0), 0)->type == TID)
            idName = getChild(getChild(var, 0), 0)->vstr;
          break;
        }
        default:break;
      }
      if (findSymbol(idName, false) != NULL) {
        sprintf(msg, "\"%s\" is not an array.", idName);
        sem_error(10, exp->fl, msg);
      }
      return foo;
    }
    Type bar;
    if (getChild(getChild(exp, 2), 0)->type == TINT) {
      subOperand = getOperandInt(oConstant, getChild(getChild(exp, 2), 0)->vint);
      bar = goExp(getChild(exp, 2), NULL);
    } else {
      subOperand = getClearOperand();
      bar = goExp(getChild(exp, 2), subOperand);
    }
    if (bar->kind != KBASIC || bar->basic_ == _FLOAT)
      sem_error(12, exp->fl, "There is not a integer between \"[\" and \"]\".");
    if (getChild(getChild(exp, 2), 0)->type == TINT && getChild(getChild(exp, 2), 0)->vint == 0) {
      free(subOperand);
      offsetOperand = getOperandStr(oConstant, "0");
    } else {
      Operand widthOperand = getOperandInt(oConstant, getSize(foo, true));
      offsetOperand = getTempOperand();
      InterCode offsetInterCode = getInterCodeTernary(iStar, offsetOperand, subOperand, widthOperand);
      insertCode(offsetInterCode);
    }
    InterCode baseInterCode = getInterCodeTernary(iGetAddress, NULL, baseOperand, offsetOperand);
    if (foo->array_.type->kind == KBASIC) {
      Operand addrOperand = getTempOperand();
      baseInterCode->ternary.res = addrOperand;
      upshot->kind = oTempAddress;
      upshot->un.name = addrOperand;
    } else {
      baseInterCode->ternary.res = upshot;
      setOperandTemp(upshot);
    }
    if (getChild(getChild(exp, 0), 0)->type == TID) {
      FieldList fieldList = findSymbol(baseOperand->un.value, false);
      if (fieldList->isArg)
        baseInterCode->kind = iPlus;
    } else {
      baseInterCode->kind = iPlus;
    }
    insertCode(baseInterCode);
    return foo->array_.type;
  } else {
    sem_error(0, exp->fl, "ERROR");
    return NULL;
  }
}

Type goCondition(object *exp, Operand trueLabelOperand, Operand falseLabelOperand) {
  // if left relop right trueLabel
  // goto falseLabel
  if (strcmp(getChild(exp, 0)->vstr, "Exp") == 0) {
    object *operator = getChild(exp, 1);
    if (operator->type == TREL) {
      Operand fooOperand, barOperand;
      Type leftType, rightType;
      // left
      if (getChild(getChild(exp, 0), 0)->type == TINT) {
        leftType = goExp(getChild(exp, 0), NULL);
        fooOperand = getOperandInt(oConstant, getChild(getChild(exp, 0), 0)->vint);
      } else {
        fooOperand = getOperand(oTempVariable);
        leftType = goExp(getChild(exp, 0), fooOperand);
      }
      // right
      if (getChild(getChild(exp, 2), 0)->type == TINT) {
        rightType = goExp(getChild(exp, 2), NULL);
        barOperand = getOperandInt(oConstant, getChild(getChild(exp, 2), 0)->vint);
      } else {
        barOperand = getOperand(oTempVariable);
        rightType = goExp(getChild(exp, 2), barOperand);
      }
      if (leftType == NULL || rightType == NULL)
        return NULL;
      // goto trueLabel
      InterCode ifGotoInterCode = getInterCodeIfGoto(trueLabelOperand, fooOperand, barOperand, getChild(exp, 1)->vstr);
      insertCode(ifGotoInterCode);
      // goto falseLabel
      InterCode gotoInterCode = getGotoLabelInterCode(falseLabelOperand);
      insertCode(gotoInterCode);
      return rightType;
    } else if (strcmp(getChild(exp, 1)->vstr, "AND") == 0 ||
        strcmp(getChild(exp, 1)->vstr, "OR") == 0) {
      // split into double condition
      Operand operand = getLabelOperand();
      Type leftType;
      // first condition
      if (strcmp(getChild(exp, 1)->vstr, "AND") == 0)
        leftType = goCondition(getChild(exp, 0), operand, falseLabelOperand);
      else
        leftType = goCondition(getChild(exp, 0), trueLabelOperand, operand);
      // transfer label
      InterCode interCode = getLabelInterCode(operand);
      insertCode(interCode);
      // second condition
      goCondition(getChild(exp, 2), trueLabelOperand, falseLabelOperand);
      return leftType;
    }
  } else if (strcmp(getChild(exp, 0)->vstr, "NOT") == 0) {
    // reverse
    return goCondition(getChild(exp, 1), falseLabelOperand, trueLabelOperand);
  } else {
    Operand operand = getClearOperand();
    Type type;
    if (getChild(exp, 0)->type == TINT) {
      type = goExp(exp, NULL);
      operand->kind = oConstant;
      sprintf(operand->un.value, "%d", getChild(exp, 0)->vint);
    } else {
      operand->kind = oTempVariable;
      type = goExp(exp, operand);
    }
    // turn `if condition` to `if condition != 0`, goto trueLabel
    Operand zeroOperand = getOperandStr(oConstant, "0");
    InterCode ifGotoInterCode = getInterCodeIfGoto(trueLabelOperand, operand, zeroOperand, "!=");
    insertCode(ifGotoInterCode);
    // goto falseLabel
    InterCode gotoInterCode = getGotoLabelInterCode(falseLabelOperand);
    insertCode(gotoInterCode);
    return type;
  }
  return NULL;
}

void traverseTree(object *root) {
  if (root == NULL) {
  } else if (root->type == TDEF && !strcmp(root->vstr, "ExtDefList")) {
    goExtDefList(root);
  } else if (countChild(root) != 0) {
    for (object *foo = root->child; foo != NULL; foo = foo->next) {
      traverseTree(foo);
    }
  }
}
