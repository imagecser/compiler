%{
    #include <stdio.h>
    #include <stdlib.h>
    #include <stdarg.h>
    #include <string.h>
    #include <assert.h>
    int err_count = 0;
    int yylex();
    void yyerror(const char *s);
    typedef enum {
        TINT,
        TFLOAT,
        TID,
		TTYPE, // int or float
        TSYM, // symbol
        TREL, // relop
        TDEF, // definition
        TNUL
    } ttype;
    typedef struct object_s {
        ttype type;
        int fl;
        int ll;
        struct object_s *child;
        struct object_s *next;
        union {
            int vint;
            float vfloat;
            char vchar;
            char *vstr;
        };
    }object;

    object *head = NULL;

    void cus_error(char _type, int _line, const char *s) {
        char err[100];
        sprintf(err, "Error type %c at Line %d: %s", _type, _line, s);
        yyerror(err);
    }

    void tree_output(object *head, int tab) {
        int i;
        if (head == NULL)
            return;
        if (head->type != TNUL)
            for (i = 0; i < tab * 2; i++)
                printf(" ");
        switch(head->type) {
            case TINT:
                printf("INT: %d\n", head->vint);
                break;
            case TFLOAT:
                printf("FLOAT: %f\n", head->vfloat);
                break;
            case TID:
                printf("ID: %s\n", head->vstr);
                break;
            case TTYPE:
                printf("TYPE: %s\n", head->vstr);
                break;
            case TSYM:
                printf("%s\n", head->vstr);
                break;
            case TREL:
                printf("%s\n", head->vstr);
                break;
            case TDEF:
                printf("%s (%d)\n", head->vstr, head->fl);
                tree_output(head->child, tab+1);
                break;
            case TNUL:
                tree_output(head->child, tab+1);
                break;
        }
        tree_output(head->next, tab);
    }

    object *merge_node(int num, ...) {
        va_list valist;
        int i;
        object *node = (object *)malloc(sizeof(object));
        va_start(valist, num);
        if (num > 0) {
            object *cur = va_arg(valist, object *);
            node->child = cur;
            for (i = 1; i < num; ++i) {
                cur->next = va_arg(valist, object *);
                cur = cur->next;
            }
            node->fl = node->child->fl;
        }
        node->next = NULL;
        return node;
    }
    object *init_tnul() {
        object *node = malloc(sizeof(object));
        node->type = TNUL;
        node->child = node->next = NULL;
        node->fl = node->vint = 0;
        return node;
    }
   
    #define _ac(n)\
        int i;\
        object *node = (object *)malloc(sizeof(object));\
        node->next = NULL;\
        if (n > 0) {\
            node->child = yyvsp[-n+1].o;\
            for (i = -n+1; i < 0; i++) {\
                yyvsp[i].o->next = yyvsp[i+1].o;\
            }\
            node->fl = node->child->fl;\
            node->ll = yyvsp[0].o->ll;\
            node->type = TDEF;\
            node->vstr = malloc(sizeof(char) * strlen(yytname[yyr1[yyn]]));\
            memcpy(node->vstr, yytname[yyr1[yyn]], strlen(yytname[yyr1[yyn]]));\
        } else {\
            node->child = NULL;\
            node->type = TNUL;\
        }\
        yyval.o = node;
    
    #define _gerror(n)\
        _ac(n);\
        cus_error('B', yylsp[-n+1].first_line, yytname[yyr1[yyn]]);
    
    #define _miss(n, _name)\
        _ac(n);\
        char buf[128];\
        sprintf(buf, "Missing character: %s", #_name);\
        cus_error('B', yylsp[0].first_line, buf);
    
    // #define _mn(1) (yyval.o) = merge_node(1, (yyvsp[0].o))
    // #define _mn(2) (yyval.o) = merge_node(2, (yyvsp[-1].o), (yyvsp[0].o))
    // #define _mn(3) (yyval.o) = merge_node(3, (yyvsp[-2].o), (yyvsp[-1].o), (yyvsp[0].o))
    // #define _mn(4) (yyval.o) = merge_node(4, (yyvsp[-3].o), (yyvsp[-2].o), (yyvsp[-1].o), (yyvsp[0].o))
    // #define _mn(5) (yyval.o) = merge_node(5, (yyvsp[-4].o), (yyvsp[-3].o), (yyvsp[-2].o), (yyvsp[-1].o), (yyvsp[0].o))
    // #define _mn(7) (yyval.o) = merge_node(7, (yyvsp[-6].o), (yyvsp[-5].o), (yyvsp[-4].o), (yyvsp[-3].o), (yyvsp[-2].o), (yyvsp[-1].o), (yyvsp[0].o))
     
%}
%locations
%debug
%error-verbose
%union {
    object *o;
};
%type<o> error END

%type<o> SEMI COMMA ASSIGNOP RELOP PLUS MINUS STAR DIV
%type<o> AND OR DOT NOT LP RP LB RB LC RC
%type<o> STRUCT RETURN IF ELSE WHILE
%type<o> Args Exp Dec DecList Def DefList Stmt StmtList CompSt
%type<o> ParamDec VarList FunDec VarDec Tag OptTag StructSpecifier Specifier
%type<o> ExtDecList ExtDef ExtDefList Program

%token TYPE INT ID FLOAT
%token SEMI COMMA ASSIGNOP RELOP 
%token PLUS MINUS STAR DIV AND OR NOT 
%token DOT LP RP LB RB LC RC 
%token STRUCT RETURN IF ELSE WHILE

%start Program

%right ASSIGNOP
%left OR
%left AND
%left RELOP
%left PLUS MINUS
%left STAR DIV
%right NOT
%left LP RP LB RB DOT
%right COMMENT

%precedence ELSE

%%

Program : ExtDefList {
            _ac(1);
            head = $$;
        }
;
ExtDefList : ExtDef ExtDefList {_ac(2);}
           | {_ac(0);}
;
ExtDef : Specifier ExtDecList SEMI {_ac(3);}
       | Specifier SEMI {_ac(2);}
       | Specifier FunDec CompSt {_ac(3);}
       | error END {_gerror(2);}
;
ExtDecList : VarDec {_ac(1);}
           | VarDec COMMA ExtDecList {_ac(3);}
;

Specifier : TYPE {_ac(1);}
          | StructSpecifier {_ac(1);}
;
StructSpecifier : STRUCT OptTag LC DefList RC {_ac(5);}
                | STRUCT Tag {_ac(2);}
;
OptTag : ID {_ac(1);}
       | {_ac(0);}
;
Tag : ID {_ac(1);}
;

VarDec : ID {_ac(1);}
       | VarDec LB INT RB {_ac(4);}
;
FunDec : ID LP VarList RP {_ac(4);}
       | ID LP RP {_ac(3);}
;
VarList : ParamDec COMMA VarList {_ac(3);}
        | ParamDec {_ac(1);}
;
ParamDec : Specifier VarDec {_ac(2);}
;

CompSt : LC DefList StmtList RC {_ac(4);}
;
StmtList : Stmt StmtList {_ac(2);}
         | {_ac(0);}
;
Stmt : Exp SEMI {_ac(2);}
     | CompSt {_ac(1);}
	 | RETURN Exp SEMI {_ac(3);}
	 | IF LP Exp RP Stmt {_ac(5);}
	 | IF LP Exp RP Stmt ELSE Stmt {_ac(7);}
	 | WHILE LP Exp RP Stmt {_ac(5);}
     | error END {_gerror(2);}
;
DefList : Def DefList {_ac(2);}
        | {_ac(0);}
; 
Def : Specifier DecList SEMI {_ac(3);}
    | error END {_gerror(2);}
;
DecList : Dec {_ac(1);}
        | Dec COMMA DecList {_ac(3);}
;
Dec : VarDec {_ac(1);}
    | VarDec ASSIGNOP Exp {_ac(3);}
;

Exp : Exp ASSIGNOP Exp {_ac(3);}
    | Exp AND Exp {_ac(3);}
    | Exp OR Exp {_ac(3);}
    | Exp RELOP Exp {_ac(3);}
    | Exp PLUS Exp {_ac(3);}
    | Exp MINUS Exp {_ac(3);}
    | Exp STAR Exp {_ac(3);}
    | Exp DIV Exp {_ac(3);}
    | LP Exp RP {_ac(3);}
    | MINUS Exp {_ac(2);}
    | NOT Exp {_ac(2);}
    | ID LP Args RP {_ac(4);}
    | ID LP RP {_ac(3);}
    | Exp LB Exp RB {_ac(4);}
    | Exp DOT ID {_ac(3);}
    | ID {_ac(1);}
    | INT {_ac(1);}
    | FLOAT {_ac(1);}
;
Args : Exp COMMA Args {_ac(3);}
     | Exp {_ac(1);}
;
END : SEMI
    | RC
;
%%
#include "lex.yy.c"
void yyerror(const char *s) {
    if (s[0] == 's') {
        return;
    }
        
    err_count += 1;
    fprintf(stderr, "%s\n", s);
}
