%{
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

void yyerror(char* s);

extern int yylex();
extern char* yytext;
extern int yylineno;
%}

%union { 
    char c;
    short s;
    int i;
    long l;
    float f;
    double d; 
}

%start PROG
%token IDENTIFIER
%token CHAR SHORT INT LONG FLOAT DOUBLE
%token VOID CONSTANT
%token LE GE EQ NE
%token IF ELSE
%token DO FOR WHILE

%nonassoc OUTERTHEN
%nonassoc ELSE

%right '='
%left EQ NE
%left '>' '<' LE GE
%left '+' '-'
%left '*' '/' '%'
%left '(' ')'

%% 
PROG : 
    GLOBAL_DECL {}
    | PROG GLOBAL_DECL {}
    ;

GLOBAL_DECL : 
    FN_DEF {}
    | DECL {}
    ;

FN_DEF :
    TYPE_SPEC IDENTIFIER '(' PARAM_LIST ')' COMPOUND_STMT
    | TYPE_SPEC IDENTIFIER '(' ')' COMPOUND_STMT
    ;

PARAM_LIST :
    PARAM_LIST ',' PARAM_DECL
    | PARAM_DECL
    ;

PARAM_DECL :
    TYPE_SPEC IDENTIFIER
    ;

DECL :
    TYPE_SPEC ID_LIST ';' {}
    ;

DECL_LIST :
    DECL_LIST DECL
    | DECL
    ;

ID_LIST :
    ID_LIST ',' DECLARATOR
    | DECLARATOR
    ;

DECLARATOR :
    IDENTIFIER
    | IDENTIFIER '=' EXPR
    ;

STMT_LIST :
    STMT_LIST STMT
    | STMT
    ;

STMT :
    COMPOUND_STMT
    | EXPR_STMT
    | SELECT_STMT
    | ITERATE_STMT
    ;

TYPE_SPEC :
    CHAR {}
    | SHORT {}
    | INT {}
    | LONG {}
    | FLOAT {}
    | DOUBLE {}
    | VOID {}
    ;

COMPOUND_STMT :
    '{' '}' {}
    | '{' STMT_LIST '}' {}
    | '{' DECL_LIST STMT_LIST '}' {}
    | '{' DECL_LIST '}' {}
    ;

EXPR_STMT :
    ';'
    | EXPR ';'
    ;

EXPR :
    EXPR '<' EXPR
    | EXPR '>' EXPR
    | EXPR LE EXPR
    | EXPR GE EXPR
    | EXPR EQ EXPR
    | EXPR NE EXPR
    | EXPR '+' EXPR
    | EXPR '-' EXPR
    | EXPR '*' EXPR
    | EXPR '/' EXPR
    | EXPR '%' EXPR
    | EXPR '=' EXPR
    | '-' EXPR %prec '*'
    | EXPR '(' ARG_LIST ')'
    | IDENTIFIER
    | CONSTANT
    | '(' EXPR ')' %prec '('
    ;

ARG_LIST :
    ARG_LIST ',' EXPR
    | EXPR
    ;

SELECT_STMT :
    IF '(' EXPR ')' STMT %prec OUTERTHEN
    | IF '(' EXPR ')' STMT ELSE STMT
    ;

ITERATE_STMT :
    FOR '(' EXPR_STMT EXPR_STMT EXPR ')' STMT
    | FOR '(' EXPR_STMT EXPR_STMT ')' STMT
    | WHILE '(' EXPR ')' STMT
    | DO STMT WHILE '(' EXPR ')' ';'
    ;
%%

void yyerror(char *s){
    printf("%s: '%s' in line %d",s, yytext, yylineno);
}