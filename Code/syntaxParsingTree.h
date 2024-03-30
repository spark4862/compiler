#ifndef SYNTAXPARSINGTREE_H
#define SYNTAXPARSINGTREE_H

#include "config.h"

#define GETNAME(type)                                                          \
  ((type < 200) ? typeName[type - 100] : typeName[type - INT + 27])

typedef unsigned int u32_t;

typedef union data {
  u32_t i;
  float f;
  char id[32];
} data;

typedef struct node {
  u32_t type; // nodeType
  u32_t row;
  int dataId; // datalist index for INT FLOAT ID; type num for RELOP and TYPE;
              // -1 for others -2 for errors -3 for empty
  struct node *child;
  struct node *next;
} node;

enum {
  Program = 100,
  ExtDefList = 101,
  ExtDef = 102,
  ExtDecList = 103,
  Specifier = 104,
  StructSpecifier = 105,
  OptTag = 106,
  Tag = 107,
  VarDec = 108,
  FunDec = 109,
  VarList = 110,
  ParamDec = 111,
  CompSt = 112,
  StmtList = 113,
  Stmt = 114,
  DefList = 115,
  Def = 116,
  DecList = 117,
  Dec = 118,
  Exp = 119,
  Args = 120,
  LT = 121,
  LE = 122,
  EQ = 123,
  NE = 124,
  GT = 125,
  GE = 126
}; //其余部分在syntax.tab.h定义

extern const char typeName[60][32];
extern data dataList[DATA_SIZE];
extern u32_t dataIndex;
extern node *root;
extern int errorFlag;
extern int lexError;
extern int lineError;

node *make_leaf(u32_t type, u32_t row, int dataId);
node *make_inode(u32_t type, u32_t row);
node *make_error(u32_t type, u32_t row);
node *make_empty(u32_t type, u32_t row);
void draw_node(node *n, int indent);

#endif