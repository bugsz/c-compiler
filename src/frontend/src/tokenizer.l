/*
 * @Author: Pan Zhiyuan
 * @Date: 2022-04-09 23:17:45
 * @LastEditors: Pan Zhiyuan
 * @FilePath: /frontend/src/tokenizer.l
 * @Description: 
 */

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "config.h"

int yycolno;
char yyline[1024] = {0};

void set_col();
%}

%option yylineno
%option noyywrap

%%
"char"		{ set_col(); yylval.typeid = TYPEID_CHAR; return CHAR; }
"short"		{ set_col(); yylval.typeid = TYPEID_SHORT; return SHORT; }
"int"		{ set_col(); yylval.typeid = TYPEID_INT; return INT; }
"long"		{ set_col(); yylval.typeid = TYPEID_LONG; return LONG; }
"float"		{ set_col(); yylval.typeid = TYPEID_FLOAT; return FLOAT; }
"double"	{ set_col(); yylval.typeid = TYPEID_DOUBLE; return DOUBLE; }
"void"		{ set_col(); yylval.typeid = TYPEID_VOID; return VOID; }
"string"    { set_col(); yylval.typeid = TYPEID_STR; return STRING; }

"if"        { set_col(); return IF; }
"else"      { set_col(); return ELSE; }
"while"     { set_col(); return WHILE; }
"for"       { set_col(); return FOR; }
"do"        { set_col(); return DO; }
"break"     { set_col(); return BREAK; }
"continue"  { set_col(); return CONTINUE; }
"return"    { set_col(); return RETURN; }


[a-zA-Z_][a-zA-Z0-9_]*	{ set_col(); yylval.str = strdup(yytext); return IDENTIFIER; }
[0-9]+			{ set_col(); yylval.str = strdup(yytext); return CONSTANT; }
[0-9]+\.[0-9]+		{ set_col(); yylval.str = strdup(yytext); return CONSTANT; }
"0x"[0-9a-fA-F]+	{ set_col(); yylval.str = strdup(yytext); return CONSTANT; }
"0b"[01]+		{ set_col(); yylval.str = strdup(yytext); return CONSTANT; }
"\"".*"\""          { set_col(); yylval.str = strdup(yytext); return CONSTANT; }

"<=" { set_col(); return LE; }
">=" { set_col(); return GE; }
"==" { set_col(); return EQ; }
"!=" { set_col(); return NE; }

[;\{\},:=\(\)\.&!~\-\+\*/%<>\^\|\?\[\]]	{ set_col(); return yytext[0];}
[ \t\r\n]+	    { set_col(); }
"//".+\n        { set_col(); }
\/\*([^\*]|(\*)*[^\*/])*(\*)*\*\/ 	{ set_col(); }
.           { set_col(); }

%%

void set_col() {
    if(yytext[0] == '\n') {
        memset(yyline, 0, sizeof(yyline));
        yycolno = 0;
    } else {
        strcat(yyline, yytext);
        yycolno += yyleng;
    }
}
