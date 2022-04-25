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
#include <assert.h>

#include "parser.h"
#include "config.h"
#include "builtin.h"

int yycolno = 1;
char yyline[1024] = {0};

extern char global_filename[256];

void set_col();
void set_include();

char* to_int(char* s, int base);
char* char_to_int(char* s);

extern YYLTYPE yylloc;
#define YY_USER_ACTION         \
  yylloc.last_line = yylineno; \
  yylloc.last_column = yycolno + 1; \
  strcpy(yylloc.filename, global_filename);
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

"sizeof"    { set_col(); return SIZEOF; }
"__builtin_itoa" { set_col(); return BUILTIN_ITOA; }
"__builtin_strcat" { set_col(); return BUILTIN_STRCAT; }
"__builtin_strlen" { set_col(); return BUILTIN_STRLEN; }
"__builtin_strget" { set_col(); return BUILTIN_STRGET; }

"#line "[0-9]+" ".*\n { set_include();}

[a-zA-Z_][a-zA-Z0-9_]*	{ set_col(); assert(yyleng < 50); yylval.str = strdup(yytext); return IDENTIFIER; }
("0"|[1-9]+[0-9]*)			{ set_col(); assert(yyleng < 50); yylval.str = strdup(yytext); return CONSTANT; }
("0"|[1-9]+[0-9]*)\.[0-9]+		{ set_col(); assert(yyleng < 50); yylval.str = strdup(yytext); return CONSTANT; }
"'"."'"         { set_col(); yylval.str = char_to_int(yytext); return CONSTANT;}
"0x"[0-9a-fA-F]+	{ set_col(); assert(yyleng < 50); yylval.str = to_int(yytext, 16); return CONSTANT; }
"0"[0-7]*       { set_col(); assert(yyleng < 50); yylval.str = to_int(yytext, 8); return CONSTANT; }
"0b"[01]+		{ set_col(); assert(yyleng < 50); yylval.str = to_int(yytext, 2); return CONSTANT; }
[a-zA-Z_]?\"(\\.|[^\\"])*\"          { set_col(); assert(yyleng < 50); yylval.str = strdup(yytext); return CONSTANT; }

"<=" { set_col(); return LE; }
">=" { set_col(); return GE; }
"==" { set_col(); return EQ; }
"!=" { set_col(); return NE; }

[;\{\},:=\(\)\.&!~\-\+\*/%<>\^\|\?\[\]]	{ set_col(); return yytext[0];}
[ \t]+	        { set_col(); }
[\r\n]          { set_col(); }
"//".*\n        { set_col(); }
\/\*([^\*]|(\*)*[^\*/])*(\*)*\*\/ 	{ set_col(); }
.           { set_col(); }

%%

void set_col() {
    if(yytext[0] == '\n') {
        memset(yyline, 0, sizeof(yyline));
        yycolno = 1;
    } else {
        strcat(yyline, yytext);
        yycolno = strlen(yyline);
    }
}

void set_include() {
    yycolno = 1;
    const char* delim = " \n";
    char* s = strdup(yytext);
    char* token = strtok(s, delim);
    int cnt = 0;
    while(token){
        if(cnt == 1) {
            yylineno = atoi(token);
        }
        if(cnt == 2) {
            token[strlen(token) - 1] = '\0';
            strcpy(global_filename, token + 1);
        }
        cnt++;
        token = strtok(NULL, delim);
    }
}

char* to_int(char* s, int base){
    int res = (base == 8) ? builtin_atoi(s + 1, base) : builtin_atoi(s + 2, base);
    char* ret = builtin_itoa(res, 10);
    ret[strlen(ret) - 1] = '\0';
    return ret + 1;
}

char* char_to_int(char* s) {
    assert(strlen(s) == 3);
    char ch = s[1];
    char* ret = builtin_itoa(ch, 10);
    ret[strlen(ret) - 1] = '\0';
    return ret + 1;
}