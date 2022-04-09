%{
#include <stdio.h>
#include "parser.h"
%}

%option yylineno
%option noyywrap

%%
"exit"           { return quit; }
"quit"           { return quit; }
[0-9]+(\.[0-9]+)?           { yylval.num = atof(yytext); return number; }
[ \t]            ;
[+\-\*/\^()\n]   { return yytext[0]; }
.                { printf("calc: unexpected character: '%s' in line %d\n", yytext, yylineno); }
%%
