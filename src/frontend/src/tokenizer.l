%{
#include <stdio.h>

#include "parser.h"

int yycolno;
char yyline[1024] = {0};

void set_col();
%}

%option yylineno
%option noyywrap

%%
"char"		{ set_col(); return CHAR; }
"short"		{ set_col(); return SHORT; }
"int"		{ set_col(); return INT; }
"long"		{ set_col(); return LONG; }
"float"		{ set_col(); return FLOAT; }
"double"	{ set_col(); return DOUBLE; }
"void"		{ set_col(); return VOID; }

"if"        { set_col(); return IF; }
"else"      { set_col(); return ELSE; }
"while"     { set_col(); return WHILE; }
"for"       { set_col(); return FOR; }
"do"        { set_col(); return DO; }
"break"     { set_col(); return BREAK; }
"continue"  { set_col(); return CONTINUE; }
"return"    { set_col(); return RETURN; }


[a-zA-Z_][a-zA-Z0-9_]*	{ set_col(); return IDENTIFIER; }
[0-9]+			{ set_col(); return CONSTANT; }

"<=" { set_col(); return LE; }
">=" { set_col(); return GE; }
"==" { set_col(); return EQ; }
"!=" { set_col(); return NE; }

[;\{\},:=\(\)\.&!~\-\+\*/%<>\^\|\?\[\]]	{ set_col(); return yytext[0];}
[ \t\r\n]+	    { set_col(); }
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
