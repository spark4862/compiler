#ifndef DATAPARSER_H
#define DATAPARSER_H

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "syntaxParsingTree.h"

u32_t parseID(const char *yytext, int yyleng) {
  if (yyleng >= 32) {
    // printf("error, ID too long\n");
  }
  strcpy(dataList[dataIndex].id, yytext);
  return dataIndex++;
}

u32_t parseINT(const char *yytext, int yyleng) {
  u32_t ans;
  long tmp = strtol(yytext, NULL, 0);
  if ((yyleng > 10) || ((ans = tmp) != tmp)) {
    // printf("error, INT too long\n");
  }
  dataList[dataIndex].i = ans;
  return dataIndex++;
  // <errno.h> 头文件中的 errno 变量来检查是否有溢出发生
}

u32_t parseFLOAT(const char *yytext, int yyleng) {
  dataList[dataIndex].f = strtof(yytext, NULL);
  return dataIndex++;
}

u32_t parseEX(const char *yytext, int yyleng) {
  float ans;
  int flag = 0;
  int i = 0;
  while ((yytext[i] != 'e') && (yytext[i] != 'E')) {
    ++i;
  }
  char tmp[yyleng + 1];
  strcpy(tmp, yytext);
  tmp[i] = '\0';
  ans = strtof(tmp, NULL);

  ++i; // + -
  if (yytext[i] == '-') {
    flag = 1;
  } else if (yytext[i] != '+') {
    --i;
  }
  unsigned int ex = strtol(&yytext[i + 1], NULL, 0);
  for (ex; ex > 0; --ex) {
    if (flag) {
      ans /= 10;
    } else {
      ans *= 10;
    }
  }
  dataList[dataIndex].f = ans;
  return dataIndex++;
}

#endif