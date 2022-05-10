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
#include <math.h>

#include "ast_impl.h"
#include "semantic.h"
#include "builtin.h"
#include "config.h"
#include "tinyexpr.h"

void yyerror(int*, struct ast_node_impl*, char* s, char* tmp_file);
void transfer_type(struct ast_node_impl* node, int type_id);
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
%token LE GE EQ NE LAND LOR SHL SHR INC DEC
%token ADD_ASSIGN SUB_ASSIGN MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN AND_ASSIGN XOR_ASSIGN OR_ASSIGN SHL_ASSIGN SHR_ASSIGN
%token IF ELSE
%token DO FOR WHILE 
%token RETURN BREAK CONTINUE
%token TYPEDEF SIZEOF BUILTIN_ITOA BUILTIN_STRCAT BUILTIN_STRLEN BUILTIN_STRGET BUILTIN_EVAL

%type <node> PROG FN_DECL FN_DEF PARAM_LIST PARAM_LIST_RIGHT PARAM_DECL
%type <node> GLOBAL_DECL DECL_LIST DECL_LIST_RIGHT DECL DECL_DECLARATOR ARRAY_DECL INIT_LIST INIT_LIST_RIGHT
%type <node> STMT COMPOUND_STMT SELECT_STMT EXPR_STMT ITERATE_STMT JMP_STMT MIX_LIST
%type <node> EXPR ARG_LIST ARG_LIST_RIGHT FOR_EXPR 
%type <typeid> TYPE_SPEC PRIMITIVE_SPEC PTR_SPEC

%nonassoc OUTERELSE
%nonassoc ELSE

%left ','
%right '=' ADD_ASSIGN SUB_ASSIGN MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN AND_ASSIGN XOR_ASSIGN OR_ASSIGN SHL_ASSIGN SHR_ASSIGN
%left LOR
%left LAND
%left '|'
%left '^'
%left '&'
%left EQ NE
%left '>' '<' LE GE
%left SHL SHR
%left '+' '-'
%left '*' '/' '%'
%right '!' '~' SIZEOF INC DEC
%left '(' ')' '[' ']'

%% 
TRANSLATION_UNIT : PROG { 
    postproc_after_parse(root);
}
;

PROG : 
    GLOBAL_DECL { 
        append_child(root, $1); 
    }
    | GLOBAL_DECL PROG { 
        append_child(root, $1); 
    }
    ;

GLOBAL_DECL : 
    FN_DECL
    | FN_DEF
    | DECL
    | TYPE_ALIAS {$$ = NULL;}
    ;

TYPE_ALIAS :
    TYPEDEF TYPE_SPEC IDENTIFIER ';' {
        add_type_alias($3, $2);
    }
    ;

FN_DECL :
    TYPE_SPEC IDENTIFIER '(' PARAM_LIST ')' ';' {
        $$ = mknode("ProtoDecl", $4);
        $$->type_id = $1;
        strcpy($$->val, $2);
        $$->pos = @2;
    }
    | TYPE_SPEC IDENTIFIER '(' ')' ';' {
        $$ = mknode("ProtoDecl");
        $$->type_id = $1;
        strcpy($$->val, $2);
        $$->pos = @2;
    }
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
    | TYPE_SPEC IDENTIFIER '(' PARAM_LIST ')' COMPOUND_STMT ';' {
        $$ = mknode("FunctionDecl", $4, $6);
        $$->type_id = $1;
        strcpy($$->val, $2);
        $$->pos = @2;
    }
    | TYPE_SPEC IDENTIFIER '(' ')' COMPOUND_STMT ';' {
        $$ = mknode("FunctionDecl", $5);
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

DECL_LIST :
    DECL_DECLARATOR DECL_LIST_RIGHT {
        $$ = mknode("TO_BE_MERGED", $1, $2);
    }
    | DECL_DECLARATOR
    ;

DECL_LIST_RIGHT :
    ',' DECL_DECLARATOR DECL_LIST_RIGHT {
        $$ = mknode("TO_BE_MERGED", $2, $3);
    }
    | ',' DECL_DECLARATOR { $$ = $2; }
    ;

DECL_DECLARATOR :
    IDENTIFIER {
        $$ = mknode("VarDecl");
        strcpy($$->val, $1);
        $$->pos = @1;
    }
    | IDENTIFIER '=' EXPR {
        $$ = mknode("VarDecl", $3);
        strcpy($$->val, $1);
        $$->pos = @1;
    }
    | '*' IDENTIFIER {
        $$ = mknode("VarDecl");
        strcpy($$->val, $2);
        $$->pos = @1;
        $$->type_id = -1;
    }
    | '*' IDENTIFIER '=' EXPR {
        $$ = mknode("VarDecl", $4);
        strcpy($$->val, $2);
        $$->pos = @1;
        $$->type_id = -1;
    }

    | IDENTIFIER ARRAY_DECL {
        ast_node_ptr arr_decl = $2, temp;
        $2->pos = @2;
        temp = mknode("VarDecl");
        temp->pos = @1;
        strcpy(temp->val, $1);
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

DECL :
    TYPE_SPEC DECL_LIST ';' {
        if ($1 >= TYPEID_VOID_PTR) {
            $2->child[0]->type_id = -1; 
            $1 = $1 - TYPEID_VOID_PTR + TYPEID_VOID;
        }
        transfer_type($2, $1);
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
        $$->pos = @1;
        strcpy($$->val, $2);
    }
    | '[' ']' '=' '{' INIT_LIST '}' {
        ast_node_ptr temp = mknode("InitializerList", $5);
        $$ = mknode("ArrayDecl", temp);
        $$->pos = @1;
        strcpy($$->val, "length_tbd");
    }
    | '[' ']' '=' CONSTANT {
        if(get_literal_type($4) != TYPEID_STR) {
            yyerror(n_errs, root, tmp_file, "array initializer must be an initializer list or wide string literal");
        }
        $$->type_id = TYPEID_CHAR;
        ast_node_ptr temp = mknode("InitializerList");
        temp->type_id = TYPEID_CHAR;
        for(int i = 1; i < strlen($4)-1; i++){
            ast_node_ptr ch = mknode("Literal");
            ch->val[0] = $4[i];
            ch->val[1] = 0;
            ch->type_id = TYPEID_CHAR;
            append_child(temp, ch);
        }
        ast_node_ptr ch = mknode("Literal");
        ch->val[0] = 0;
        ch->type_id = TYPEID_CHAR;
        append_child(temp, ch);
        $$ = mknode("ArrayDecl", temp);
        $$->pos = @1;
        strcpy($$->val, "length_tbd");
    }
    | '[' CONSTANT ']' '=' CONSTANT {
        if(get_literal_type($5) != TYPEID_STR) {
            yyerror(n_errs, root, tmp_file, "array initializer must be an initializer list or wide string literal");
        }
        $$->type_id = TYPEID_CHAR;
        ast_node_ptr temp = mknode("InitializerList");
        temp->type_id = TYPEID_CHAR;
        for(int i = 1; i < strlen($5)-1; i++){
            ast_node_ptr ch = mknode("Literal");
            ch->val[0] = $5[i];
            ch->val[1] = 0;
            ch->type_id = TYPEID_CHAR;
            append_child(temp, ch);
        }
        ast_node_ptr ch = mknode("Literal");
        ch->val[0] = 0;
        ch->type_id = TYPEID_CHAR;
        append_child(temp, ch);
        $$ = mknode("ArrayDecl", temp);
        $$->pos = @1;
        strcpy($$->val, $2);
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

STMT :
    COMPOUND_STMT { $$ = $1; }
    | EXPR_STMT   { $$ = $1; }
    | SELECT_STMT { $$ = $1; }
    | ITERATE_STMT { $$ = $1; }
    | JMP_STMT { $$ = $1; }
    ;

TYPE_SPEC :
    PRIMITIVE_SPEC
    | PTR_SPEC
    ;

PRIMITIVE_SPEC:
    CHAR {}
    | SHORT {}
    | INT {}
    | LONG {}
    | FLOAT {}
    | DOUBLE {}
    | VOID {}
    | STRING {}
    ;

PTR_SPEC: 
    VOID_PTR {}
    | CHAR_PTR {}
    | SHORT_PTR {}
    | INT_PTR {}
    | LONG_PTR {}
    | FLOAT_PTR {}
    | DOUBLE_PTR {}
    ;



COMPOUND_STMT :
    '{' '}' { $$ = mknode("CompoundStmt"); }
    | '{' MIX_LIST '}' { $$ = mknode("CompoundStmt", $2); }
    ;

MIX_LIST :
    DECL MIX_LIST {
        $$ = mknode("TO_BE_MERGED", $1, $2);
    }
    | STMT MIX_LIST {$$ = mknode("TO_BE_MERGED", $1, $2);}
    | STMT {$$ = $1;}
    | DECL {$$ = $1;}
    ;

EXPR_STMT :
    ';' { $$ = mknode("NullStmt"); }
    | EXPR ';' { $$ = $1; }
    | EXPR ',' EXPR_STMT { $$ = mknode("TO_BE_MERGED", $1, $3); }
    ;

FOR_EXPR :
    ';' { $$ = mknode("NullStmt"); }
    | EXPR ';' { $$ = $1; }

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
    | EXPR ADD_ASSIGN EXPR { 
        $$ = mknode("BinaryOperator", $1, $3);
        strcpy($$->val, "+=");
        $$->pos = @2;
    }
    | EXPR SUB_ASSIGN EXPR { 
        $$ = mknode("BinaryOperator", $1, $3);
        strcpy($$->val, "-=");
        $$->pos = @2;
    }
    | EXPR MUL_ASSIGN EXPR { 
        $$ = mknode("BinaryOperator", $1, $3);
        strcpy($$->val, "*=");
        $$->pos = @2;
    }
    | EXPR DIV_ASSIGN EXPR { 
        $$ = mknode("BinaryOperator", $1, $3);
        strcpy($$->val, "/=");
        $$->pos = @2;
    }
    | EXPR MOD_ASSIGN EXPR { 
        $$ = mknode("BinaryOperator", $1, $3);
        strcpy($$->val, "%=");
        $$->pos = @2;
    }
    | EXPR AND_ASSIGN EXPR { 
        $$ = mknode("BinaryOperator", $1, $3);
        strcpy($$->val, "&=");
        $$->pos = @2;
    }
    | EXPR OR_ASSIGN EXPR { 
        $$ = mknode("BinaryOperator", $1, $3);
        strcpy($$->val, "|=");
        $$->pos = @2;
    }
    | EXPR XOR_ASSIGN EXPR { 
        $$ = mknode("BinaryOperator", $1, $3);
        strcpy($$->val, "^=");
        $$->pos = @2;
    }
    | EXPR SHL_ASSIGN EXPR { 
        $$ = mknode("BinaryOperator", $1, $3);
        strcpy($$->val, "<<=");
        $$->pos = @2;
    }
    | EXPR SHR_ASSIGN EXPR { 
        $$ = mknode("BinaryOperator", $1, $3);
        strcpy($$->val, ">>=");
        $$->pos = @2;
    }
    | EXPR SHL EXPR {
        $$ = mknode("BinaryOperator", $1, $3);
        strcpy($$->val, "<<");
        $$->pos = @2;
    }
    | EXPR SHR EXPR {
        $$ = mknode("BinaryOperator", $1, $3);
        strcpy($$->val, ">>");
        $$->pos = @2;
    }
    | EXPR '&' EXPR { 
        $$ = mknode("BinaryOperator", $1, $3);
        strcpy($$->val, "&");
        $$->pos = @2;
    }
    | EXPR '|' EXPR { 
        $$ = mknode("BinaryOperator", $1, $3);
        strcpy($$->val, "|");
        $$->pos = @2;
    }
    | EXPR '^' EXPR { 
        $$ = mknode("BinaryOperator", $1, $3);
        strcpy($$->val, "^");
        $$->pos = @2;
    }
    | EXPR LOR EXPR { 
        $$ = mknode("BinaryOperator", $1, $3);
        strcpy($$->val, "||");
        $$->pos = @2;
    }
    | EXPR LAND EXPR { 
        $$ = mknode("BinaryOperator", $1, $3);
        strcpy($$->val, "&&");
        $$->pos = @2;
    }
    | EXPR INC {
        ast_node_ptr l1 = mknode("Literal");
        l1->type_id = TYPEID_INT;
        strcpy(l1->val, "1");
        l1->pos = @2;
        $$ = mknode("BinaryOperator", $1, l1);
        strcpy($$->val, "++");
        $$->pos = @2;
    }
    | EXPR DEC {
        ast_node_ptr l1 = mknode("Literal");
        l1->type_id = TYPEID_INT;
        strcpy(l1->val, "1");
        l1->pos = @2;
        $$ = mknode("BinaryOperator", $1, l1);
        strcpy($$->val, "--");
        $$->pos = @2;
    }
    | INC EXPR {
        ast_node_ptr l1 = mknode("Literal");
        l1->type_id = TYPEID_INT;
        strcpy(l1->val, "1");
        l1->pos = @1;
        $$ = mknode("BinaryOperator", $2, l1);
        strcpy($$->val, "+=");
        $$->pos = @1;
    }
    | DEC EXPR {
        ast_node_ptr l1 = mknode("Literal");
        l1->type_id = TYPEID_INT;
        strcpy(l1->val, "1");
        l1->pos = @1;
        $$ = mknode("BinaryOperator", $2, l1);
        strcpy($$->val, "-=");
        $$->pos = @1;
    }
    | '-' EXPR %prec '!'    { 
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
    | IDENTIFIER '[' EXPR ']' {
        ast_node_ptr temp = mknode("DeclRefExpr");
        strcpy(temp->val, $1);
        temp->pos = @1;
        $$ = mknode("ArraySubscriptExpr", temp, $3);
        $$->pos = @1;
    }
    | IDENTIFIER '(' ARG_LIST ')' { 
        ast_node_ptr id = mknode("DeclRefExpr");
        strcpy(id->val, $1);
        id->pos = @1;
        $$ = mknode("CallExpr", id, $3); 
        $$->pos = @1;
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
    | SIZEOF '(' EXPR ')' {
        $$ = mknode("__SIZEOF", $3);
        $$->pos = @1;
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
    | BUILTIN_EVAL '(' CONSTANT ')' {
        if(get_literal_type($3) != TYPEID_STR) {
            yyerror(n_errs, root, tmp_file, "invalid type to builtin function 'eval'");
        }
        $$ = mknode("Literal");
        $$->type_id = TYPEID_DOUBLE;
        $$->pos = @1;
        int eval_err = 0;
        char *math_str = strdup($3+1);
        math_str[strlen(math_str)-1] = 0;
        double res = te_interp(math_str, &eval_err);
        if(eval_err) {
            fprintf(stderr, COLOR_BOLD"%s:%d:%d: "COLOR_RED"%s"COLOR_NORMAL"\n%s\n", \
                global_filename, yylineno, @3.last_column + eval_err, "invalid syntax in '__builtin_eval'", yyline);
            for(int i = 0; i < @3.last_column + eval_err - 1; i++)
                fprintf(stderr, " ");
            fprintf(stderr, COLOR_GREEN"^\n"COLOR_NORMAL);
            (*n_errs)++;
            sprintf($$->val, "%f", 0.0);
        } else {
            if(isnan(fabs(res)) || isinf(fabs(res))){
                fprintf(stderr, COLOR_BOLD"%s:%d:%d: "COLOR_RED"%s"COLOR_NORMAL"\n%s\n", \
                    global_filename, yylineno, @3.last_column + eval_err, "math error in '__builtin_eval'", yyline);
                for(int i = 0; i < @3.last_column + eval_err - 1; i++)
                    fprintf(stderr, " ");
                fprintf(stderr, COLOR_GREEN"^\n"COLOR_NORMAL);
                (*n_errs)++;
            }
            sprintf($$->val, "%.16lf", res);
        }
    }
    | CONSTANT     { 
        $$ = mknode("Literal");
        $$->type_id = get_literal_type($1);
        if($$->type_id < 0) yyerror(n_errs, root, tmp_file, "integer constant is too large for its type");
        strcpy($$->val, $1);
        $$->pos = @1;
    }
    | '(' EXPR ')'    { $$ = $2; }
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
    FOR '(' FOR_EXPR FOR_EXPR EXPR ')' STMT {
        ast_node_ptr delim = mknode("ForDelimiter");
        ast_node_ptr t = mknode("TO_BE_MERGED", $3, $4);
        ast_node_ptr t2 = mknode("TO_BE_MERGED", $5, delim, $7);
        ast_node_ptr t3 = 
        $$ = mknode("ForStmt", t, t2);
    }
    | FOR '(' TYPE_SPEC IDENTIFIER '=' EXPR ';' FOR_EXPR EXPR ')' STMT {
        ast_node_ptr delim = mknode("ForDelimiter");
        ast_node_ptr vardecl = mknode("VarDecl", $6);
        vardecl->type_id = $3;
        strcpy(vardecl->val, $4);
        ast_node_ptr t = mknode("TO_BE_MERGED", vardecl, $8);
        ast_node_ptr t2 = mknode("TO_BE_MERGED", $9, delim, $11);
        ast_node_ptr t3 = mknode("ForStmt", t, t2);
        $$ = mknode("CompoundStmt", t3);
    }
    | FOR '(' FOR_EXPR FOR_EXPR ')' STMT {
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

void yyerror(int* n_errs, struct ast_node_impl* node, char* tmp_file, char *s) {
    (*n_errs)++;
    fprintf(stderr, COLOR_BOLD"%s:%d:%d: "COLOR_RED"%s"COLOR_NORMAL"\n%s\n", \
        global_filename, yylineno, yycolno, s, yyline);
    for(int i = 0; i < yycolno - 1; i++)
        fprintf(stderr, " ");
    fprintf(stderr, COLOR_GREEN"^\n"COLOR_NORMAL);
    unlink(tmp_file);
}

void transfer_type(struct ast_node_impl* node, int type_id) {
    if(node == NULL) return;
    int tmp_type_id = node->type_id;
    node->type_id = type_id;
    if(strcmp(node->token, "VarDecl") == 0) {
        if(tmp_type_id == -1) {
            // if it is a pointer, transfer to ptr type
            // in this dirty fix, we don't consider the type of the first variable
            if(type_id >= TYPEID_VOID_PTR) return ;
            node->type_id = type_id + TYPEID_VOID_PTR - TYPEID_VOID;
        }
        else {
            if (type_id >= TYPEID_VOID_PTR) 
                node->type_id = type_id - TYPEID_VOID_PTR + TYPEID_VOID;
        }

        return; 
    } else if(strcmp(node->token, "ArrayDecl") == 0) {
        node->child[0]->type_id = type_id + TYPEID_VOID_PTR - TYPEID_VOID;
    } else {
        for(int i = 0; i < node->n_child; i++) {
            transfer_type(node->child[i], type_id);
        }
    }
}