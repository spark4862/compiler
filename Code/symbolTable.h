#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H

#include "config.h"
#include "syntaxParsingTree.h"

/*
没有typedef
数组没有名称，采用结构等价判定，且只判定基类型和维数
结构体名等价
结构体赋值为？拷贝
每个结构体均有一个名字，匿名结构体也有且各不相同
结构体与变量不能同名
函数之间不能同名 不能出现多态
函数与变量和结构体可以同名
结构体中的域不与变量重名，并且不同结构体中的域互不重名
不用考虑结构体的作用域
*/

#define INT_ID() 0
#define FLOAT_ID() 1

struct Type;
struct Structure;

typedef struct ArraySize {
  u32_t size;
  struct ArraySize *next;
} ArraySize;

typedef struct Array {
  u32_t typeId; // typeId of baseType
  u32_t dimension;
  ArraySize *size;
} Array;

typedef struct Field {
  u32_t nameId;
  u32_t typeId;
  struct Field *next;
} Field;

typedef struct Structure {
  u32_t nameId;
  u32_t fieldNum;
  Field *field;
} Structure;

typedef struct Type {
  enum { BASIC, ARRAY, STRUCTURE } kind;
  union {
    enum { BASIC_INT, BASIC_FLOAT } basic;
    Array *array;
    Structure *structure;
  };
} Type; // store type

extern Type typeList[];

// typedef struct Varible {
//   u32_t nameId;
//   u32_t typeId;
// } Varible;

typedef struct ParameterTypeIdList {
  u32_t typeId;
  u32_t nameId;
  struct ParameterTypeIdList *next;
} ParameterTypeIdList;

typedef struct Function {
  u32_t nameId;
  u32_t returnTypeId;
  u32_t parameterNum;
  ParameterTypeIdList *p;
} Function;

typedef struct Symbol {
  enum { VARIBLE, FUNCTION, STRUCTURENAME, NULLHEADER, FIELD } kind;
  u32_t nameId;
  union {
    u32_t typeId; // typeId of struct, var
    Function *function;
    void *v; // value can only be NULL
  };
} Symbol;

typedef struct SymbolLinker {
  u32_t stackNum;
  Symbol data;
  struct SymbolLinker *right; // link same hash or link different stack
  struct SymbolLinker *down;  // link same stack
} SymbolLinker;

void initHeader();
SymbolLinker *symbolLinkerMalloc();
void addNewStack();
void freeSymbol(Symbol s);
void deleteCurStack();
void insertSymbol(Symbol data);
SymbolLinker *findSymbol(u32_t nameId);

void commonParser(node *n, Function *funtion);
void initHeader();
void initFunction();

u32_t addNewStructure(u32_t nameId);
void parseVarDecID(u32_t typeId, node *n, Type *structType, Function *function);
u32_t addNewArray(u32_t baseType);
u32_t parseArrayDec(u32_t baseType, node *n, Type *structType,
                    Function *function);
u32_t parseArray(node *n, int dimension);
u32_t parseExp(node *n);
void parseDec(u32_t typeId, node *n, Type *structType);
void parseDef(node *n, Type *structType);
u32_t parseSpecifier(node *n);
void parseExtDef(node *n);
#endif