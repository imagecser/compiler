%{
    #include "node.h"
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
