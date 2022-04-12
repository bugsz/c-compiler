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
TRANSLATION_UNIT : PROG { 
    postproc_after_parse(root);
}

PROG : 
    GLOBAL_DECL { 
        append_child(root, $1); 
    }
    | GLOBAL_DECL PROG { 
        append_child(root, $1); 
    }
    ;

GLOBAL_DECL : 
    FN_DEF
    | DECL
    ;

FN_DEF :
    TYPE_SPEC IDENTIFIER '(' PARAM_LIST ')' COMPOUND_STMT {
        $$ = mknode("FunctionDecl", $4, $6);
    }
    | TYPE_SPEC IDENTIFIER '(' ')' COMPOUND_STMT {
        $$ = mknode("FunctionDecl", $5);
    }
    | TYPE_SPEC IDENTIFIER '(' PARAM_LIST ')' ';' {
        $$ = mknode("FunctionDecl", $4);
    }
    | TYPE_SPEC IDENTIFIER '(' ')' ';' {
        $$ = mknode("FunctionDecl", $5);
    }
    ;

PARAM_LIST :
    PARAM_DECL PARAM_LIST_RIGHT {
        $$ = mknode("TO_BE_MERGED", $1, $2);
    }
    | PARAM_DECL
    ;

PARAM_LIST_RIGHT :
    ',' PARAM_DECL PARAM_LIST_RIGHT {
        $$ = mknode("TO_BE_MERGED", $2, $3);
    }
    | ',' PARAM_DECL {
        $$ = $2;
    }
    ;

PARAM_DECL :
    TYPE_SPEC IDENTIFIER { $$ = mknode("ParmVarDecl"); }
    ;

DECL :
    TYPE_SPEC DECLARATOR ';' { $$ = mknode("VarDecl", $2); }
    ;

DECL_LIST :
    DECL DECL_LIST { 
        ast_node_ptr t1 = mknode("DeclStmt", $1);
        $$ = mknode("TO_BE_MERGED", t1, $2); 
    }
    | DECL  { $$ = mknode("DeclStmt", $1); }
    ;

DECLARATOR :
    IDENTIFIER  { }
    | IDENTIFIER '=' EXPR { 
        $$ = $3;
    }
    ;

STMT_LIST :
    STMT STMT_LIST { $$ = mknode("TO_BE_MERGED", $1, $2); }
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
    '{' '}' { $$ = mknode("CompoundStmt"); }
    | '{' STMT_LIST '}' { $$ = mknode("CompoundStmt", $2); }
    | '{' DECL_LIST STMT_LIST '}' { $$ = mknode("CompoundStmt", $2, $3); }
    | '{' DECL_LIST '}' { $$ = mknode("CompoundStmt", $2); }
    ;

EXPR_STMT :
    ';'
    | EXPR ';' { $$ = $1; }
    ;

EXPR :
    EXPR '<' EXPR    { $$ = mknode("BinaryOperator", $1, $3); }
    | EXPR '>' EXPR  { $$ = mknode("BinaryOperator", $1, $3); }
    | EXPR LE EXPR  { $$ = mknode("BinaryOperator", $1, $3); }
    | EXPR GE EXPR  { $$ = mknode("BinaryOperator", $1, $3); }
    | EXPR EQ EXPR  { $$ = mknode("BinaryOperator", $1, $3); }
    | EXPR NE EXPR  { $$ = mknode("BinaryOperator", $1, $3); }
    | EXPR '+' EXPR { $$ = mknode("BinaryOperator", $1, $3); }
    | EXPR '-' EXPR { $$ = mknode("BinaryOperator", $1, $3); }
    | EXPR '*' EXPR { $$ = mknode("BinaryOperator", $1, $3); }
    | EXPR '/' EXPR { $$ = mknode("BinaryOperator", $1, $3); }
    | EXPR '%' EXPR { $$ = mknode("BinaryOperator", $1, $3); }
    | EXPR '=' EXPR { $$ = mknode("BinaryOperator", $1, $3); }
    | '-' EXPR %prec '*'    { $$ = mknode("UnaryOperator", $2); }
    | FUNC_NAME '(' ARG_LIST ')' { $$ = mknode("CallExpr", $1, $3); }
    | IDENTIFIER     { $$ = mknode("DeclRefExpr"); }
    | CONSTANT     { $$ = mknode("Literal"); }
    | '(' EXPR ')' %prec '('    { $$ = $2; }
    ;

FUNC_NAME :
    IDENTIFIER  { $$ = mknode("DeclRefExpr"); }
    ;

ARG_LIST :
    EXPR ARG_LIST_RIGHT { $$ = mknode("TO_BE_MERGED", $1, $2); }
    | EXPR { $$ = $1; }
    ;

ARG_LIST_RIGHT :
    ',' EXPR ARG_LIST_RIGHT { $$ = mknode("TO_BE_MERGED", $2, $3); }
    | ',' EXPR { $$ = $2; }
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