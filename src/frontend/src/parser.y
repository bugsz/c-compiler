%{
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

void yyerror(char* s);

extern int yylex();
extern char* yytext;
extern int yylineno;
%}

%union { double num; }
%start line
%token quit
%token <num> number
%type  <num> line exp term
%left '+' '-'
%left '*' '/'
%right '^'
%left '(' ')'

%% 

line  :  exp '\n'         { printf("%g\n", $1); }
      |  quit '\n'        { exit(0); }
      |  '\n'             {}
      |  line exp '\n'    { printf("%g\n", $2); }
      |  line quit '\n'   { exit(0); }
      |  line '\n'        {}
      ;
    
exp   :  term             { $$ = $1; }
      |  exp '+' exp      { $$ = $1 + $3; }
      |  exp '-' exp      { $$ = $1 - $3; }
      |  exp '*' exp      { $$ = $1 * $3; }
      |  exp '/' exp      { $$ = $1 / $3; }
      |  exp '^' exp      { $$ = pow($1, $3); }
      | '(' exp ')'       { $$ = $2; }
      ;
    
term  :  number           { $$ = $1; }
      |  '-' term         { $$ = -$2; }

%%

void yyerror(char *s){
    printf("calc: %s: '%s' in line %d",s, yytext, yylineno);
}