/*
 * @Author: Pan Zhiyuan
 * @Date: 2022-04-09 23:17:45
 * @LastEditors: Pan Zhiyuan
 * @FilePath: /frontend/src/parser.y
 * @Description: 
 */

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "ast_impl.h"
#include "semantic.h"
#include "builtin.h"
#include "config.h"

void yyerror(int*, struct ast_node_impl*, char* s, char* tmp_file);
extern int yylex();

extern char* yytext;
extern char yyline[1024];

extern char global_filename[256];

extern int yylineno;
extern int yycolno;

%}

%define api.location.type {struct ast_yyltype}

%parse-param {int* n_errs}
%parse-param {struct ast_node_impl* root}
%parse-param {char* tmp_file}

%start TRANSLATION_UNIT

%union {
    int typeid;
    char* str;
    struct ast_node_impl* node;
};

%token <str> IDENTIFIER
%token <typeid> CHAR SHORT INT LONG FLOAT DOUBLE VOID STRING
%token <typeid> VOID_PTR CHAR_PTR SHORT_PTR INT_PTR LONG_PTR FLOAT_PTR DOUBLE_PTR
%token <str> CONSTANT
%token LE GE EQ NE
%token IF ELSE
%token DO FOR WHILE 
%token RETURN BREAK CONTINUE
%token SIZEOF BUILTIN_ITOA BUILTIN_STRCAT BUILTIN_STRLEN BUILTIN_STRGET

%type <node> PROG FN_DEF PARAM_LIST PARAM_LIST_RIGHT PARAM_DECL
%type <node> GLOBAL_DECL DECL DECL_LIST DECLARATOR ARRAY_DECL INIT_LIST INIT_LIST_RIGHT
%type <node> STMT STMT_LIST COMPOUND_STMT SELECT_STMT EXPR_STMT ITERATE_STMT JMP_STMT
%type <node> EXPR FUNC_NAME ARG_LIST ARG_LIST_RIGHT
%type <typeid> TYPE_SPEC

%nonassoc OUTERELSE
%nonassoc ELSE

%right '='
%left EQ NE
%left '>' '<' LE GE
%left '+' '-'
%left '*' '/' '%'
%right '!' '~' SIZEOF '&'
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
        $$->type_id = $1;
        strcpy($$->val, $2);
        $$->pos = @2;
    }
    | TYPE_SPEC IDENTIFIER '(' ')' COMPOUND_STMT {
        $$ = mknode("FunctionDecl", $5);
        $$->type_id = $1;
        strcpy($$->val, $2);
        $$->pos = @2;
    }
    | TYPE_SPEC IDENTIFIER '(' PARAM_LIST ')' ';' {
        $$ = mknode("FunctionDecl", $4);
        $$->type_id = $1;
        strcpy($$->val, $2);
        $$->pos = @2;
    }
    | TYPE_SPEC IDENTIFIER '(' ')' ';' {
        $$ = mknode("FunctionDecl");
        $$->type_id = $1;
        strcpy($$->val, $2);
        $$->pos = @2;
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
    | ',' '.' '.' '.' {$$ = mknode("VariadicParms");}
    ;

PARAM_DECL :
    TYPE_SPEC IDENTIFIER { 
        $$ = mknode("ParmVarDecl");
        $$->type_id = $1;
        strcpy($$->val, $2);
        $$->pos = @2;
    }
    | TYPE_SPEC IDENTIFIER '[' ']' {
        if($1 >= TYPEID_VOID_PTR)
            yyerror(n_errs, root, tmp_file, "multi-dimensional pointer is not supported");
        $$ = mknode("ParmVarDecl");
        $$->type_id = $1 + TYPEID_VOID_PTR - TYPEID_VOID;
        strcpy($$->val, $2);
        $$->pos = @2;
    }
    ;

DECL :
    TYPE_SPEC IDENTIFIER DECLARATOR ';' { 
        $$ = mknode("VarDecl", $3);
        $$->type_id = $1;
        $$->pos = @2;
        strcpy($$->val, $2);
    }
    | TYPE_SPEC IDENTIFIER ARRAY_DECL ';' { 
        if($1 >= TYPEID_VOID_PTR)
            yyerror(n_errs, root, tmp_file, "multi-dimensional pointer is not supported");
        ast_node_ptr arr_decl = $3, temp;
        $3->type_id = $1;
        $3->pos = @2;
        temp = mknode("VarDecl");
        temp->type_id = $1  + TYPEID_VOID_PTR - TYPEID_VOID;
        temp->pos = @2;
        strcpy(temp->val, $2);
        append_child(arr_decl, temp);
        assert(arr_decl->n_child == 2 || arr_decl->n_child == 1);
        if(arr_decl->n_child == 2) {
            ast_node_ptr t = arr_decl->child[0];
            arr_decl->child[0] = arr_decl->child[1];
            arr_decl->child[1] = t;
        }
        $$ = arr_decl;
    }
    ;

DECL_LIST :
    DECL DECL_LIST { 
        ast_node_ptr t1 = mknode("DeclStmt", $1);
        $$ = mknode("TO_BE_MERGED", t1, $2); 
    }
    | DECL  { $$ = mknode("DeclStmt", $1); }
    ;

DECLARATOR :
        { $$ = NULL; }
    | '=' EXPR { 
        $$ = $2;
    }
    ;

ARRAY_DECL :
    '[' CONSTANT ']' {
        $$ = mknode("ArrayDecl");
        $$->pos = @1;
        strcpy($$->val, $2);
    }
    | '[' CONSTANT ']' '=' '{' INIT_LIST '}' { 
        ast_node_ptr temp = mknode("InitializerList", $6);
        $$ = mknode("ArrayDecl", temp);
        $$->pos = @4;
        strcpy($$->val, $2);
    }
    | '[' ']' '=' '{' INIT_LIST '}' {
        ast_node_ptr temp = mknode("InitializerList", $5);
        $$ = mknode("ArrayDecl", temp);
        $$->pos = @3;
        strcpy($$->val, "length_tbd");
    }
    ;

INIT_LIST :
    EXPR INIT_LIST_RIGHT {
        $$ = mknode("TO_BE_MERGED", $1, $2);
    }
    | EXPR {
        $$ = $1;
    }
    | EXPR ',' {
        $$ = $1;
    }
    ;

INIT_LIST_RIGHT :
    ',' EXPR INIT_LIST_RIGHT {
        $$ = mknode("TO_BE_MERGED", $2, $3);
    }
    | ',' EXPR {
        $$ = $2;
    }
    | ',' EXPR ',' {
        $$ = $2;
    }
    ;

STMT_LIST :
    STMT STMT_LIST { $$ = mknode("TO_BE_MERGED", $1, $2); }
    | STMT { $$ = $1;}
    ;

STMT :
    COMPOUND_STMT { $$ = $1; }
    | EXPR_STMT   { $$ = $1; }
    | SELECT_STMT { $$ = $1; }
    | ITERATE_STMT { $$ = $1; }
    | JMP_STMT { $$ = $1; }
    ;

TYPE_SPEC :
    CHAR {}
    | SHORT {}
    | INT {}
    | LONG {}
    | FLOAT {}
    | DOUBLE {}
    | VOID {}
    | STRING {}
    | VOID_PTR {}
    | CHAR_PTR {}
    | SHORT_PTR {}
    | INT_PTR {}
    | LONG_PTR {}
    | FLOAT_PTR {}
    | DOUBLE_PTR {}
    ;

COMPOUND_STMT :
    '{' '}' { $$ = mknode("CompoundStmt"); }
    | '{' STMT_LIST '}' { $$ = mknode("CompoundStmt", $2); }
    | '{' DECL_LIST STMT_LIST '}' { $$ = mknode("CompoundStmt", $2, $3); }
    | '{' DECL_LIST '}' { $$ = mknode("CompoundStmt", $2); }
    ;

EXPR_STMT :
    ';' { $$ = mknode("NullStmt"); }
    | EXPR ';' { $$ = $1; }
    ;

EXPR :
    EXPR '<' EXPR    { 
        $$ = mknode("BinaryOperator", $1, $3);
        strcpy($$->val, "<");
        $$->pos = @2;

    }
    | EXPR '>' EXPR  { 
        $$ = mknode("BinaryOperator", $1, $3);
        strcpy($$->val, ">");
        $$->pos = @2;
    }
    | EXPR LE EXPR  { 
        $$ = mknode("BinaryOperator", $1, $3);
        strcpy($$->val, "<=");
        $$->pos = @2;
    }
    | EXPR GE EXPR  { 
        $$ = mknode("BinaryOperator", $1, $3);
        strcpy($$->val, ">=");
        $$->pos = @2;
    }
    | EXPR EQ EXPR  { 
        $$ = mknode("BinaryOperator", $1, $3);
        strcpy($$->val, "==");
        $$->pos = @2;
    }
    | EXPR NE EXPR  { 
        $$ = mknode("BinaryOperator", $1, $3); 
        strcpy($$->val, "!=");
        $$->pos = @2;
    }
    | EXPR '+' EXPR { 
        $$ = mknode("BinaryOperator", $1, $3);
        strcpy($$->val, "+");
        $$->pos = @2;
    }
    | EXPR '-' EXPR { 
        $$ = mknode("BinaryOperator", $1, $3); 
        strcpy($$->val, "-");
        $$->pos = @2;
    }
    | EXPR '*' EXPR { 
        $$ = mknode("BinaryOperator", $1, $3);
        strcpy($$->val, "*");   
        $$->pos = @2; 
    }
    | EXPR '/' EXPR { 
        $$ = mknode("BinaryOperator", $1, $3);
        strcpy($$->val, "/");    
        $$->pos = @2;
    }
    | EXPR '%' EXPR { 
        $$ = mknode("BinaryOperator", $1, $3);
        strcpy($$->val, "%");
        $$->pos = @2;
    }
    | EXPR '=' EXPR { 
        $$ = mknode("BinaryOperator", $1, $3);
        strcpy($$->val, "=");
        $$->pos = @2;
    }
    | '-' EXPR %prec '*'    { 
        $$ = mknode("UnaryOperator", $2);
        strcpy($$->val, "-");   
        $$->pos = @1; 
    }
    | '!' EXPR {
        $$ = mknode("UnaryOperator", $2);
        strcpy($$->val, "!");
        $$->pos = @1;
    }
    | '~' EXPR {
        $$ = mknode("UnaryOperator", $2);
        strcpy($$->val, "~");
        $$->pos = @1;
    }
    | '*' EXPR %prec '!' {
        $$ = mknode("UnaryOperator", $2);
        strcpy($$->val, "*");
        $$->pos = @1;
    }
    | '&' EXPR %prec '!' {
        $$ = mknode("UnaryOperator", $2);
        strcpy($$->val, "&");
        $$->pos = @1;
    }
    | '(' TYPE_SPEC ')' EXPR %prec '!' {
        $$ = mknode("ExplicitCastExpr", $4);
        $$->type_id = $2;
        $$->pos = @1;
    }
    | IDENTIFIER '[' EXPR ']' %prec '!' {
        ast_node_ptr temp = mknode("DeclRefExpr");
        strcpy(temp->val, $1);
        temp->pos = @1;
        $$ = mknode("ArraySubscriptExpr", temp, $3);
        $$->pos = @1;
    }
    | FUNC_NAME '(' ARG_LIST ')' { 
        $$ = mknode("CallExpr", $1, $3); 
    }
    | IDENTIFIER     { 
        $$ = mknode("DeclRefExpr");
        strcpy($$->val, $1);
        $$->pos = @1;
    }
    | SIZEOF '(' TYPE_SPEC ')' {
        $$ = mknode("Literal");
        $$->type_id = TYPEID_INT;
        $$->pos = @1;
        int temp = get_type_size($3);
        if(temp < 0) yyerror(n_errs, root, tmp_file, "invalid type name to operator 'sizeof'");
        char s[2]={0};
        s[0] = temp + '0';
        strcpy($$->val, s);
    }
    | BUILTIN_ITOA '(' EXPR ',' CONSTANT ')' { 
        ast_node_ptr parm2 = mknode("c2");
        strcpy(parm2->val, $5);
        parm2->type_id = get_literal_type($5);
        $$ = mknode("BUILTIN_ITOA", $3, parm2);
        $$->type_id = TYPEID_STR;
        $$->pos = @1;
    }
    | BUILTIN_STRCAT '(' EXPR ',' EXPR ')' { 
        $$ = mknode("BUILTIN_STRCAT", $3, $5);
        $$->type_id = TYPEID_STR;
        $$->pos = @1;
    }
    | BUILTIN_STRLEN '(' EXPR ')' { 
        $$ = mknode("BUILTIN_STRLEN", $3);
        $$->type_id = TYPEID_INT;
        $$->pos = @1;
    }
    | BUILTIN_STRGET '(' EXPR ',' CONSTANT ')' {
        ast_node_ptr parm2 = mknode("c2");
        strcpy(parm2->val, $5);
        parm2->type_id = get_literal_type($5);
        $$ = mknode("BUILTIN_STRGET", $3, parm2);
        $$->type_id = TYPEID_CHAR;
        $$->pos = @1;
    }
    | CONSTANT     { 
        $$ = mknode("Literal");
        $$->type_id = get_literal_type($1);
        strcpy($$->val, $1);
        $$->pos = @1;
    }
    | '(' EXPR ')' %prec '('    { $$ = $2; }
    ;

FUNC_NAME :
    IDENTIFIER  { 
        $$ = mknode("DeclRefExpr"); 
        strcpy($$->val, $1);
        $$->pos = @1;
    }
    ;

ARG_LIST :
    EXPR ARG_LIST_RIGHT { $$ = mknode("TO_BE_MERGED", $1, $2); }
    | EXPR
    | { $$ = NULL;}
    ;

ARG_LIST_RIGHT :
    ',' EXPR ARG_LIST_RIGHT { $$ = mknode("TO_BE_MERGED", $2, $3); }
    | ',' EXPR { $$ = $2; }
    ;

SELECT_STMT :
    IF '(' EXPR ')' STMT %prec OUTERELSE {
        $$ = mknode("IfStmt", $3, $5);
    }
    | IF '(' EXPR ')' STMT ELSE STMT {
        ast_node_ptr temp = mknode("TO_BE_MERGED", $5, $7);
        $$ = mknode("IfStmt", $3, temp);
    }
    ;

ITERATE_STMT :
    FOR '(' EXPR_STMT EXPR_STMT EXPR ')' STMT {
        ast_node_ptr delim = mknode("ForDelimiter");
        ast_node_ptr t = mknode("TO_BE_MERGED", $3, $4);
        ast_node_ptr t2 = mknode("TO_BE_MERGED", $5, delim, $7);
        $$ = mknode("ForStmt", t, t2);
    }
    | FOR '(' EXPR_STMT EXPR_STMT ')' STMT {
        ast_node_ptr delim = mknode("ForDelimiter");
        ast_node_ptr t = mknode("TO_BE_MERGED", $3, $4);
        ast_node_ptr nullstmt = mknode("NullStmt");
        ast_node_ptr t2 = mknode("TO_BE_MERGED", nullstmt, delim, $6);
        $$ = mknode("ForStmt", t, t2);
    }
    | WHILE '(' EXPR ')' STMT {
        $$ = mknode("WhileStmt", $3, $5);
    }
    | DO STMT WHILE '(' EXPR ')' ';' {
        $$ = mknode("DoStmt", $2, $5);
    }
    ;

JMP_STMT :
    BREAK ';'   { $$ = mknode("BreakStmt"); $$->pos = @1; }
    | CONTINUE ';'  { $$ = mknode("ContinueStmt"); $$->pos = @1; }
    | RETURN ';' { $$ = mknode("ReturnStmt"); $$->pos = @1; }
    | RETURN EXPR ';'  { $$ = mknode("ReturnStmt", $2); $$->pos = @1; }
    ;
%%

void yyerror(int* n_errs, struct ast_node_impl* node, char* tmp_file, char *s){
    (*n_errs)++;
    fprintf(stderr, COLOR_BOLD"%s:%d:%d: "COLOR_RED"%s"COLOR_NORMAL"\n%s\n", \
        global_filename, yylineno, yycolno, s, yyline);
    for(int i = 0; i < yycolno - 1; i++)
        fprintf(stderr, COLOR_GREEN"~");
    fprintf(stderr, COLOR_GREEN"^\n"COLOR_NORMAL);
    unlink(tmp_file);
}