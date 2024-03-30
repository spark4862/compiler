#include "syntaxParsingTree.h"
#include "syntax.tab.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAKEINDENT(indent)                                                     \
  short tmp = indent;                                                          \
  while (tmp > 0) {                                                            \
    --tmp;                                                                     \
    putchar(' ');                                                              \
  }

data dataList[DATA_SIZE];
u32_t dataIndex = 0;
node *root;
int errorFlag = 0;
int lexError = 0;
int lineError = 0;
const char typeName[60][32] = {"Program",   "ExtDefList",
                               "ExtDef",    "ExtDecList",
                               "Specifier", "StructSpecifier",
                               "OptTag",    "Tag",
                               "VarDec",    "FunDec",
                               "VarList",   "ParamDec",
                               "CompSt",    "StmtList",
                               "Stmt",      "DefList",
                               "Def",       "DecList",
                               "Dec",       "Exp",
                               "Args",      "LT",
                               "LE",        "EQ",
                               "NE",        "GT",
                               "GE",        "int",
                               "float",     "ID",
                               "TYPE",      "SEMI",
                               "COMMA",     "LC",
                               "RC",        "STRUCT",
                               "RETURN",    "IF",
                               "WHILE",     "LOWER_THAN_ELSE",
                               "ELSE",      "ASSIGNOP",
                               "OR",        "AND",
                               "RELOP",     "PLUS",
                               "MINUS",     "STAR",
                               "DIV",       "NOT",
                               "DOT",       "LP",
                               "RP",        "LB",
                               "RB"};

node *make_node(u32_t type, u32_t row, int dataId, node *next, node *child) {
  node *p = (node *)malloc(sizeof(node));
  p->type = type;
  p->row = row;
  p->dataId = dataId;
  p->next = next;
  p->child = child;
  return p;
}

node *make_leaf(u32_t type, u32_t row, int dataId) {
  return make_node(type, row, dataId, NULL, NULL);
}

node *make_inode(u32_t type, u32_t row) {
  return make_node(type, row, -1, NULL, NULL);
}

node *make_error(u32_t type, u32_t row) {
  return make_node(type, row, -2, NULL, NULL);
}

node *make_empty(u32_t type, u32_t row) {
  return make_node(type, row, -3, NULL, NULL);
}

void draw_node(node *n, int indent) {
  if (!n || (n->dataId == -2) || (n->dataId == -3) || errorFlag)
    return;
  MAKEINDENT(indent)
  if ((n->child) == NULL) {
    switch (n->type) {
    case ID:
      printf("ID: %s\n", dataList[n->dataId].id);
      break;
    case INT:
      printf("INT: %d\n", dataList[n->dataId].i);
      break;
    case FLOAT:
      printf("FLOAT: %.6f\n", dataList[n->dataId].f);
      break;
    case TYPE:
      printf("%s: %s\n", GETNAME(n->type), GETNAME(n->dataId));
      break;
    // case RELOP:
    //   printf("RELOP: %s\n", GETNAME(n->dataId));
    //   break;
    default:
      printf("%s\n", GETNAME(n->type));
    }
    return;
  }
  printf("%s (%d)\n", GETNAME(n->type), n->row);
  node *child = n->child;
  while (child) {
    draw_node(child, indent + 2);
    child = child->next;
  }
}
