%{
    #include "syntax.tab.h"
    int yycolumn = 1;
    #define YY_USER_ACTION \
        yylloc.first_line = yylloc.last_line = yylineno; \
        yylloc.first_column = yycolumn; \
        yylloc.last_column = yycolumn + yyleng - 1; \
        yycolumn += yyleng;
    
    #define alloc_leng malloc(sizeof(char) * yyleng)
    #define init_yylval(_type, _union, _val)\
        yylval.o = (object *)malloc(sizeof(object));\
        yylval.o->fl = yylineno;\
        yylval.o->ll = yylineno;\
        yylval.o->child = NULL;\
        yylval.o->next = NULL;\
        yylval.o->type = _type;\
        yylval.o->_union = _val;
    #define sym_ret(_symbol)\
        init_yylval(TSYM, vstr, #_symbol);\
        return _symbol;
    #define copy_init(_type)\
        init_yylval(_type, vstr, alloc_leng);\
        memcpy(yylval.o->vstr, yytext, sizeof(char)*yyleng);

    #define lex_err(_s)\
        char buf[128];\
        sprintf(buf, "%s \"%s\"", _s, yytext);\
        cus_error('A', yylineno, buf);
%}
%option yylineno

octal_digit [0-7]
hex_digit [0-9a-zA-Z]
nz_digit [1-9]
digit [0-9]
digits {digit}+

hex 0[xX]{hex_digit}*
decimal {nz_digit}|{digits}
octal 0{octal_digit}*

letter_ [a-zA-Z_]
letter_digit [a-zA-Z_0-9]
com_line .*\n?
sign [+-]
exponent_part [eE]{sign}?{digits}
fractional_constant {digits}?\.{digits}|{digits}\.
floating_literal {fractional_constant}{exponent_part}?|{digits}\.{exponent_part}
%%
\n {}
[ \t\r] {}
";" {sym_ret(SEMI)}
"," {sym_ret(COMMA)}
"=" {sym_ret(ASSIGNOP)}
">"|"<"|">="|"<="|"=="|"!=" {copy_init(TREL); return RELOP;}
"+" {sym_ret(PLUS)}
"-" {sym_ret(MINUS)}
"*" {sym_ret(STAR)}
"/" {sym_ret(DIV)}
"&&" {sym_ret(AND)}
"||" {sym_ret(OR)}
"." {sym_ret(DOT)}
"!" {sym_ret(NOT)}
"(" {sym_ret(LP)}
")" {sym_ret(RP)}
"[" {sym_ret(LB)}
"]" {sym_ret(RB)}
"{" {sym_ret(LC)}
"}" {sym_ret(RC)}
"int"|"float" {
    copy_init(TTYPE);
    return TYPE;
}

"struct" {sym_ret(STRUCT)}
"return" {sym_ret(RETURN)}
"if" {sym_ret(IF)}
"else" {sym_ret(ELSE)}
"while" {sym_ret(WHILE)} 

"//".* {}
"/*"(([^\*]*(\*[^\/])?)*)"*/" {}
{letter_}{letter_digit}* {
    copy_init(TID);
    return ID;
}
{floating_literal} {
    char *end;
    init_yylval(TFLOAT, vfloat, strtof(yytext, &end));
    if (*end != 0) {
        lex_err("Illegal floating point number");
    }
    return FLOAT;
}
{decimal}|{octal}|{hex} {
    char *end;
    init_yylval(TINT, vint, strtol(yytext, &end, 0));
    if (*end != 0) {
        lex_err("Illegal number");
    }
    return INT;
}

. {
    lex_err("Mysterious character");
}
%%
int main(int argc, char **argv) {
    if (argc <= 1)
        return 1;
    FILE *f = fopen(argv[1], "r");
    if (!f) {
        perror(argv[1]);
        return 1;
    }
    yylineno = 1;
    yyrestart(f);
    yyparse();
    if (err_count==0) {
        tree_output(head, 0);
        
    }
    return 0;
}
