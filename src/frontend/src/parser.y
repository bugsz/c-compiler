%{
#include <stdio.h>
#include <stdlib.h>

#include "ast_impl.h"

#define YYSTYPE ast_node_ptr
#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#define COLOR_RED ""
#define COLOR_GREEN ""
#define COLOR_NORMAL ""
#endif

void yyerror(const char*, int*, ast_node_ptr, char* s);
extern int yylex();

extern char* yytext;
extern char yyline[1024];
extern int yylineno;
extern int yycolno;

%}

%parse-param {const char* filename}
%parse-param {int* n_errs}
%parse-param {ast_node_ptr root}

%start TRANSLATION_UNIT
%token IDENTIFIER
%token CHAR SHORT INT LONG FLOAT DOUBLE
%token VOID CONSTANT
%token LE GE EQ NE
%token IF ELSE
%token DO FOR WHILE BREAK CONTINUE
%token RETURN

%nonassoc OUTERTHEN
%nonassoc ELSE

%right '='
%left EQ NE
%left '>' '<' LE GE
%left '+' '-'
%left '*' '/' '%'
%left '(' ')'

%% 
TRANSLATION_UNIT : PROG { print_ast(root); }

PROG : 
    GLOBAL_DECL { 
        append_child(root, $1); 
    }
    | PROG GLOBAL_DECL { 
        append_child(root, $2); 
    }
    ;

GLOBAL_DECL : 
    FN_DEF {}
    | DECL { $$ = $1; }
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
    TYPE_SPEC DECLARATOR ';' { $$ = mknode("VarDecl", $2); }
    ;

DECL_LIST :
    DECL_LIST DECL
    | DECL
    ;

DECLARATOR :
    IDENTIFIER  {}
    | IDENTIFIER '=' EXPR { 
        $$ = $3;
    }
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
    | JMP_STMT
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
    EXPR '<' EXPR    { $$ = mknode("BinaryOperator", $1, $3); }
    | EXPR '>' EXPR  { $$ = mknode("GT", $1, $3); }
    | EXPR LE EXPR  { $$ = mknode("LE", $1, $3); }
    | EXPR GE EXPR  { $$ = mknode("GE", $1, $3); }
    | EXPR EQ EXPR  { $$ = mknode("EQ", $1, $3); }
    | EXPR NE EXPR  { $$ = mknode("NE", $1, $3); }
    | EXPR '+' EXPR { $$ = mknode("ADD", $1, $3); }
    | EXPR '-' EXPR { $$ = mknode("SUB", $1, $3); }
    | EXPR '*' EXPR { $$ = mknode("MUL", $1, $3); }
    | EXPR '/' EXPR { $$ = mknode("DIV", $1, $3); }
    | EXPR '%' EXPR { $$ = mknode("MOD", $1, $3); }
    | EXPR '=' EXPR { $$ = mknode("ASSIGN", $1, $3); }
    | '-' EXPR %prec '*'    { $$ = mknode("NEG", $2); }
    | EXPR '(' ARG_LIST ')' { $$ = mknode("CALL", $1, $3); }
    | IDENTIFIER     { $$ = mknode("IDENTIFIER"); }
    | CONSTANT     { $$ = mknode("Literal"); }
    | '(' EXPR ')' %prec '('    { $$ = $2; }
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

JMP_STMT :
    BREAK ';'
    | CONTINUE ';'
    | RETURN ';'
    | RETURN EXPR ';'
    ;
%%

void yyerror(const char* filename, int* n_errs, ast_node_ptr node, char *s){
    (*n_errs)++;
    fprintf(stderr, COLOR_BOLD"%s:%d:%d: "COLOR_RED"%s:"COLOR_NORMAL"\n%s\n", \
        filename, yylineno, yycolno, s, yyline);
    for(int i = 0; i < yycolno - 1; i++)
        fprintf(stderr, COLOR_GREEN"~");
    fprintf(stderr, COLOR_GREEN"^\n"COLOR_NORMAL);
}