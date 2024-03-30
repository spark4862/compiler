%{
#include <stdio.h>
#include "syntax.tab.h"
#include <stdlib.h>
#include <string.h>
#include "dataParser.h"
#include "syntaxParsingTree.h"

#define YY_USER_ACTION \
  yylloc.first_line = yylloc.last_line = yylineno; \
  yylloc.first_column = yycolumn; \
  yylloc.last_column = yycolumn + yyleng - 1; \
  yycolumn += yyleng;

int yycolumn = 1;
%}

%option yylineno

letter    (?:[a-zA-Z])
digit     (?:[0-9])
space     (?:[\t\n\r\f ])
int       (?:[1-9]{digit}*|0)
ftail     (?:0|{digit}*[1-9])
id        (?:(?:{letter}|_)(?:{letter}|{digit}|_)*)
relop     (?:">"|"<"|">="|"<="|"=="|"!=")
numb      (?:";"|","|"="|{relop}|"+"|"-"|"*"|"/"|"&&"|"||"|"]"|")"|(else))

%%
{int} {
  yylval.type_int = parseINT(yytext, yyleng);//dataIndex
  return INT;
}
{int}\.{ftail} {
  yylval.type_int = parseFLOAT(yytext, yyleng);
  return FLOAT;
}
(?:(?:{digit}*\.{digit}+)|(?:{digit}+\.{digit}*))[eE][+-]?{digit}+ {
  yylval.type_int = parseEX(yytext, yyleng);
  return FLOAT;
}//注意，指数的0可以出现在任意位置，可以出现00.0010
";"                 { return SEMI; }
","                 { return COMMA; }
"="                 { return ASSIGNOP; }
">="                { yylval.type_int = GE; return RELOP; }//type
"<="                { yylval.type_int = LE; return RELOP; }
"=="                { yylval.type_int = EQ; return RELOP; }
"!="                { yylval.type_int = NE; return RELOP; }
"<"                 { yylval.type_int = LT; return RELOP; }
">"                 { yylval.type_int = GT; return RELOP; }
"+"                 { return PLUS; }
"-"                 { return MINUS; }
"*"                 { return STAR; }
"/"                 { return DIV; }
"&&"                { return AND; }
"||"                { return OR; }
"."                 { return DOT; }
"!"                 { return NOT; }
"("                 { return LP; }
")"                 { return RP; }
"["                 { return LB; }
"]"                 { return RB; }
"{"                 { return LC; }
"}"                 { return RC; }
int                 { yylval.type_int = INT; return TYPE; }
float               { yylval.type_int = FLOAT; return TYPE; }
struct              { return STRUCT; }
return              { return RETURN; }
if                  { return IF; }
else                { return ELSE; }
while               { return WHILE; }
\n                  { yycolumn = 1; lineError = 0;}
{space}             { }
{id} { 
  yylval.type_int = parseID(yytext, yyleng);
  return ID;
}
"//" {
  char c = input();
  while (c != '\n') c = input();
}
"/*" {
  char cp = input();
  char c = input();
  while (!((cp == '*') && (c == '/'))){
    if ((cp == '/') && (c == '*')){
      printf("Error type B at LIne %d: enbedded /**/\n", yylineno);
      return 1;
    }
    cp = c;
    c = input();
  }
}
. {
  if (!lineError) {
    printf("Error type A at Line %d: Mysterious characters \'%s'\n", yylineno, yytext);
    lineError = 1;
  }
  errorFlag = 1;
//  lexError = 1;
// no return??
}

%%
/*
\"[^\"]*\" {
  if ((yytext[yyleng-2]) == '\\') {
    yyless(yyleng - 1);
    yymore();
    printf("\"");
  } else {
    printf("STR\n");
  }
}
(?:(?:{int}?\.{ftail})|(?:{int}\.{ftail}?))[eE][+-]?{int}
*/


//  if (!lineErrorFlag){
//    printf("Error type A at Line %d: Mysterious characters \'%s'\n",
//      yylineno, yytext);
//    lineErrorFlag = 1;
//  }