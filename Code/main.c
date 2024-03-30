#include "asmGen.h"
#include "ir.h"
#include "symbolTable.h"
#include "syntax.tab.h"
#include "syntaxParsingTree.h"
#include <stdio.h>

void yyrestart(FILE *input_file);

FILE *writeFile;

int main(int argc, char **argv) {
  if (argc <= 1)
    return 1;
  FILE *f = fopen(argv[1], "r");
  if (!f) {
    perror(argv[1]);
    return 1;
  }
  yyrestart(f);
  // yydebug = 1;
  yyparse();
  // draw_node(root, 0);
  if (errorFlag)
    return 1;
  initHeader();
  addNewStack();
  initFunction(); // for experiment3
  commonParser(root, NULL);
  fclose(f);
  if (argc < 3) {
    return 1;
  }
  writeFile = fopen(argv[2], "w");
  if (!writeFile) {
    perror(argv[2]);
    return 1;
  }
  if (errorFlag)
    return 1;
  genIC();
  if (errorFlag)
    return 1;
  genAsm();
  fclose(writeFile);
  return 0;
}

/*
{int}/{white}*{numb} {
  printf("INT %s\n", yytext);
}
{int} {
  printf("Error type B at LIne %d: illegal int\n", yylineno);
  return 1;
}
0(?:[1-7][0-7]*|0) {
  printf("8INT %s\n", yytext);
}
0[xX](?:[1-9a-fA-F][0-9a-fA-F]*|0) {
  printf("16INT %s\n", yytext);
}
bison -d -t -v syntax1.y
flex lexical.l
gcc main.c syntax1.tab.c ast.c  -lfl -ly -o main
*/
