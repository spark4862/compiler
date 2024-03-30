#ifndef IR_H
#define IR_H

#include "config.h"
#include <stdio.h>

typedef struct Operand {
  enum { OP_VARIABLE, OP_CONSTANT, OP_LABEL, OP_TEMP, OP_NULL1 } kind1;
  enum {
    OP_ADDRESS,
    OP_ARR,
    OP_STRUCT,
    OP_COM,
    OP_NULL2,
  } kind2;
  union {
    u32_t varno;
    u32_t labelno;
    int instant;
    // note that no float in program
    u32_t tempno;
  } u; // this is necessary for if you dont assign a name for union, all
       // elements in union will be an element in struct, this will confuse you
       // while programing
  u32_t typeId; // INF if ...
  int para;     // is a function para? useful?
} Operand;

typedef struct Var {
  u32_t nameId;
  Operand op;
} Var;

typedef struct IC {
  enum {
    IC_ADD = 0,
    IC_SUB,
    IC_MUL,
    IC_DIV,
    IC_EQ,
    IC_NE,
    IC_LT,
    IC_LE,
    IC_GT,
    IC_GE,
    IC_ARG,
    IC_PARAM,
    IC_READ,
    IC_WRITE,
    NULL_HEADER,
    IC_LABEL,
    IC_FUNCTION,
    IC_ASSIGN,
    IC_ADDR,
    IC_LOAD,
    IC_STORE,
    IC_JUMP,
    IC_RETURN,
    IC_DEC,
    IC_CALL,
    IC_INIT
  } kind;
  union {
    // for label, jump, param, arg, read, write, return
    Operand op;
    // for function def
    u32_t nameId;
    // for assign store load getAddr
    struct {
      Operand right, left;
    } assign;
    // for binop operation
    struct {
      Operand result, op1, op2;
    } binop;
    // for function call and dec
    struct {
      Operand op;
      u32_t value;
    } call;
    // for dec
  } u;
} IC;

typedef struct ICLinker {
  IC code;
  struct ICLinker *prev, *next;
} ICLinker;

extern FILE *writeFile;
extern struct ICLinker *ICHeader;

void genIC();
void writeIC();
void _writeIC(ICLinker *ic);

#endif