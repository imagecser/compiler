
#include "semantic.h"

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
  for (; foo != NULL; foo = foo->next, count++)
    ;
  return count;
}

unsigned int hashFunc(char *name) {
  unsigned int val = 0, i;
  for (; *name; ++name) {
    val = (val << 2) + *name;
    if (i = val & ~HASH_SIZE) val = (val ^ (i >> 12)) & HASH_SIZE;
  }
  return val;
}

void initialize_hash_table() {
  int i;
  for (i = 0; i < HASH_SIZE; i++)
    ;
  hashTable[i] = NULL;
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
    key = (++key) % HASH_SIZE;
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
    key = (++key) % HASH_SIZE;
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
  FieldList field = (FieldList)malloc(sizeof(FieldList_));
  field->name = idName;

  switch (i) {
    // e.g. a
    case 0: {
      field->type = type;
      return field;
    }
    // e.g. a[1]
    case 1: {
      Type var = (Type)malloc(sizeof(Type_));
      var->kind = KARRAY;
      // var->array_.size = root->child->next->next->vint;
      var->array_.size = getChild(root, 2)->vint;
      var->array_.type = type;
      field->type = var;
      return field;
    }
    // e.g. a[1][2]
    case 2: {
      Type inner = (Type)malloc(sizeof(Type_)),
           outer = (Type)malloc(sizeof(Type_));
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
  Type foo = (Type)malloc(sizeof(Type_));
  // Specifier: TYPE
  if (root->child->type = TTYPE) {
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
    if (field == NULL) {
      ;
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
    while (defList != NULL) {              // DefList : Def DefList
      object *def = getChild(defList, 0);  // Def : Specifier DecList SEMI
      Type basicType = goSpecifier(getChild(def, 0));
      object *decList = getChild(def, 1);
      while (decList->type != TNUL) {  // DecList: Dec COMMA DecList
        // Dec : VarDec
        FieldList field =
            goVarDec(getChild(getChild(decList, 0), 0), basicType);
        if (countChild(getChild(decList, 0)) != 1) {
          ;
          sprintf(msg, "Variable %s in struct is initialized.", field->name);
          sem_error(15, def->fl, msg);
        }
        if (findSymbol(field->name, false) != NULL) {
          ;
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
      FieldList field = (FieldList)malloc(sizeof(FieldList_));
      field->type = foo;
      field->name = getChild(getChild(structSpecifier, 1), 0)->vstr;
      if (findSymbol(field->name, false) != NULL) {
        ;
        sprintf(msg, "Duplicated name \"%s\".", field->name);
        sem_error(16, root->fl, field->name);
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
      object *extDecList = getChild(extDecList, 1);
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
      FieldList field = (FieldList)malloc(sizeof(FieldList_));
      field->name = getChild(funDec, 0)->vstr;
      Type type = (Type)malloc(sizeof(Type_));
      type->kind = KFUNC;
      type->func_.funcType = basicType;
      type->func_.paramNum = 0;
      type->func_.params = NULL;

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
            ;
            sprintf(msg, "Redefined variable \"%s\".", varField->name);
            sem_error(3, funDec->fl, msg);
          } else {
            insertSymbol(field);
          }
          type->func_.paramNum++;
          varField->tail = type->func_.params;
          type->func_.params = varField;
          if (countChild(varList) == 3)
            varList = getChild(varList, 2);
          else
            break;
        }
      }
      field->type = type;
      if (findSymbol(field->name, true) != NULL) {
        ;
        sprintf(msg, "Redefined function \"%s\".", field->name);
        sem_error(4, funDec->fl, msg);
      } else {
        insertSymbol(field);
      }
      goCompSt(getChild(extDef, 2), basicType);

    } else {  // Specifier SEMI
    }
    if (getChild(extDefList, 1)->type = TNUL)
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
    while (decList->type != TNUL) {
      FieldList field = goVarDec(getChild(getChild(decList, 0), 0), basicType);
      if (findSymbol(field->name, false) != NULL) {
        ;
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
    goExp(getChild(stmt, 0));
  } else if (!strcmp(getChild(stmt, 0)->vstr, "CompSt")) {
    goCompSt(getChild(stmt, 0), funcType);
  } else if (!strcmp(getChild(stmt, 0)->vstr, "RETURN")) {
    Type returnType = goExp(getChild(stmt, 1));
    if (!isTypeEqual(returnType, funcType))
      sem_error(8, stmt->fl, "Type mismatched for return.");
  } else if (!strcmp(getChild(stmt, 0)->vstr, "WHILE")) {
    Type conditionType = goExp(getChild(stmt, 2));
    if (conditionType->kind != KBASIC || conditionType->basic_ != _INT)
      sem_error(5, stmt->fl, "Only type INT could be used for judgement.");
    goStmt(getChild(stmt, 4), funcType);
  } else {
    Type conditionType = goExp(getChild(stmt, 2));
    if (conditionType->kind != KBASIC || conditionType->basic_ != _INT)
      sem_error(5, stmt->fl, "Only type INT could be used for judgement.");
    goStmt(getChild(stmt, 4), funcType);
    if (countChild(stmt) == 6) goStmt(getChild(stmt, 6), funcType);
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
Type goExp(object *exp) {
  if (exp == NULL || exp->type == TNUL) return NULL;
  char *firstStr = getChild(exp, 0)->vstr;
  char *secondStr = (countChild(exp) > 1) ? getChild(exp, 1)->vstr : NULL;
  char *thirdStr = (countChild(exp) > 2) ? getChild(exp, 2)->vstr : NULL;
  char *fourthStr = (countChild(exp) > 3) ? getChild(exp, 3)->vstr : NULL;
  if (getChild(exp, 0)->type == TID && countChild(exp) == 1) {
    FieldList field = findSymbol(firstStr, false);
    if (field != NULL) {
      return field->type;
    } else {
      ;
      sprintf(msg, "Undefined variable \"%s\".", firstStr);
      sem_error(1, exp->fl, msg);
    }
  } else if (getChild(exp, 0)->type == TINT) {
    Type foo = (Type)malloc(sizeof(Type_));
    foo->kind = KBASIC;
    foo->basic_ = _INT;
    return foo;
    // } else if (!strcmp(firstStr, "FLOAT")) {
  } else if (getChild(exp, 0)->type == TFLOAT) {
    Type foo = (Type)malloc(sizeof(Type_));
    foo->kind = KBASIC;
    foo->basic_ = _FLOAT;
    return foo;
  } else if (!strcmp(firstStr, "LP") || !strcmp(firstStr, "MINUS") ||
             !strcmp(firstStr, "NOT")) {
    return goExp(getChild(exp, 1));
  } /* Exp AND Exp
     | Exp OR Exp
     | Exp RELOP Exp
     | Exp PLUS Exp
     | Exp MINUS Exp
     | Exp STAR Exp
     | Exp DIV Exp*/
  else if (!strcmp(secondStr, "PLUS") || !strcmp(secondStr, "MINUS") ||
           !strcmp(secondStr, "STAR") || !strcmp(secondStr, "DIV") ||
           !strcmp(secondStr, "AND") || !strcmp(secondStr, "OR") ||
           getChild(exp, 1)->type == TREL) {
    Type leftType = goExp(getChild(exp, 0)),
         rightType = goExp(getChild(exp, 2));
    if (!isTypeEqual(leftType, rightType)) {
      sem_error(7, exp->fl, "Type mismatched for operands.");
    } else if (!strcmp(secondStr, "PLUS") || !strcmp(secondStr, "MINUS") ||
               !strcmp(secondStr, "STAR") || !strcmp(secondStr, "DIV")) {
    } else if (getChild(exp, 1)->type != TREL) {  // AND OR
      if (leftType->kind != KBASIC || leftType->kind != _INT) {
        sem_error(5, exp->fl, "Only type INT could be used for judgement.");
      }
    } else {
      Type foo = (Type)malloc(sizeof(Type_));
      foo->kind = KBASIC;
      foo->basic_ = _INT;
      return foo;
    }
    return leftType;
  } else if (!strcmp(secondStr, "ASSIGNOP")) {
    object *dest = getChild(exp, 0);
    Type foo = goExp(getChild(exp, 0)), bar = goExp(getChild(exp, 2));
    if ((countChild(dest) == 1 && getChild(dest, 0)->type == TID) ||
        (countChild(dest) == 3 && !strcmp(getChild(dest, 0)->vstr, "Exp") &&
         !strcmp(getChild(dest, 1)->vstr, "DOT") &&
         getChild(dest, 2)->type == TID) ||
        countChild(dest) == 4 && !strcmp(getChild(dest, 0)->vstr, "Exp") &&
            // !strcmp(getChild(dest, 1)->vstr, "LB") &&
            // !strcmp(getChild(dest, 3)->vstr, "RB") &&
            !strcmp(getChild(dest, 2)->vstr, "Exp")) {
      if (!isTypeEqual(foo, bar))
        sem_error(5, exp->fl, "Type mismatched for assignment.");
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
    Type definedType = foo->type, type = (Type)malloc(sizeof(Type_));
    type->kind = KFUNC;
    type->func_.paramNum = 0;
    type->func_.params = NULL;
    if (countChild(exp) == 4) {
      // Args : Exp COMMA Args | Exp;
      object *args = getChild(exp, 2);
      while (args->type != TNUL) {
        Type tmpType = goExp(getChild(args, 0));
        FieldList tmpField = (FieldList)malloc(sizeof(FieldList_));
        tmpField->type = tmpType;
        type->func_.paramNum++;
        tmpField->tail = type->func_.params;
        type->func_.params = tmpField;
        if (countChild(args) == 3)
          args = getChild(args, 2);
        else
          break;
      }
    }
    if (!isTypeEqual(type, definedType)) {
      sprintf(msg, "Params wrong in function \"%s\".", getChild(exp, 0)->vstr);
      sem_error(9, exp->fl, msg);
      return NULL;
    } else {
      return definedType->func_.funcType;
    }
  } else if (!strcmp(secondStr, "DOT")) {  // Exp DOT ID
    Type foo = goExp(getChild(exp, 0));
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
      }
      if (findSymbol(idName, false) != NULL)
        sem_error(13, exp->fl, "Illegal use of \".\".");
      return NULL;
    }
    char *idName = getChild(exp, 2)->vstr;
    FieldList field = foo->struct_;
    while (field != NULL) {
      if (!strcmp(field->name, idName)) return field->type;
      field = field->tail;
    }
    sprintf(msg, "Non-existed field \"%s\".", getChild(exp, 2)->vstr);
    sem_error(14, exp->fl, msg);
    return NULL;
  } else if (!strcmp(getChild(exp, 1)->vstr, "LB")) {  // Exp LB Exp RB
    Type foo = goExp(getChild(exp, 0));
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
      }
      if (findSymbol(idName, false) != NULL) {
        sprintf(msg, "\"%s\" is not an array.", idName);
        sem_error(10, exp->fl, msg);
      }
      return NULL;
    }
    Type bar = goExp(getChild(exp, 2));
    if (bar->kind != KBASIC || bar->basic_ == _FLOAT) {
      sem_error(12, exp->fl, "There is not a integer between \"[\" and \"]\".");
      return NULL;
    }
    return foo->array_.type;
  } else {
    sem_error(0, exp->fl, "ERROR");
    return NULL;
  }
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
