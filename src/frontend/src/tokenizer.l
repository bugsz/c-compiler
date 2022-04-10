%{
#include <stdio.h>
#include "parser.h"
%}

%option yylineno
%option noyywrap

%%
"char"		{ return CHAR; }
"short"		{ return SHORT; }
"int"		{ return INT; }
"long"		{ return LONG; }
"float"		{ return FLOAT; }
"double"	{ return DOUBLE; }
"void"		{ return VOID; }

[a-zA-Z_][a-zA-Z0-9_]*	{ return IDENTIFIER; }
[0-9]+			{ return CONSTANT; }

"<=" { return LE; }
">=" { return GE; }
"==" { return EQ; }
"!=" { return NE; }

[;\{\},:=\(\)\.&!~\-\+\*/%<>\^\|\?\[\]]	{ return yytext[0];}
[ \t\r\n]+	{}
.           {}

%%
