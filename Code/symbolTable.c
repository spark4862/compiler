#include "symbolTable.h"
#include "syntax.tab.h"
#include "syntaxParsingTree.h"
#include <stdlib.h>
// #define NDEBUG

#include <assert.h>
#include <stdio.h>
#include <string.h>

/*
在实验三中，因为是先运行检查在运行代码生成，所以多层作用域需要删除，除非同时运行检查和代码生成
*/

#define GET_CUR_STACK() (symbolStackHeader.right)
#define GOLOBAL_STACK() 1

Type typeList[MAX_TYPE_NUM] = {{.kind = BASIC, .basic = BASIC_INT},
                               {.kind = BASIC, .basic = BASIC_FLOAT}};
int typeNum = 1;
u32_t curStackNum = 0;                 // global stack 1
SymbolLinker symbolTable[MAX_SYM_NUM]; // hashHeaderTable
SymbolLinker symbolStackHeader; // 符号栈链表头的链表头 StackHeaderHeader

u32_t hashSymbol(u32_t nameId);
void initHeader();
SymbolLinker *symbolLinkerMalloc();
void addNewStack();
void freeSymbol(Symbol s);
void deleteCurStack();
void insertSymbol(Symbol data);
SymbolLinker *findSymbol(u32_t nameId);
u8_t typeCompare(u32_t ida, u32_t idb);
void commonParser(node *n, Function *function);
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

u32_t hashSymbol(u32_t nameId) {
  char *name = dataList[nameId].id;
  u32_t val = 0, i;
  for (; *name; ++name) {
    val = (val << 2) + *name;
    if (i = val & ~0xfff)
      val = (val ^ (i >> 12)) & 0xfff;
  }
  return val;
}

void initHeader() {
  symbolStackHeader.right = NULL;
  symbolStackHeader.down = NULL;
  symbolStackHeader.data.kind = NULLHEADER;
  symbolStackHeader.data.nameId = INF;
  symbolStackHeader.data.v = NULL;
  symbolStackHeader.stackNum = 0;
}

void initFunction() {
  // for experiment 3
  // add function read and write
  Symbol tmp1, tmp2;
  tmp1.kind = FUNCTION;
  tmp2.kind = FUNCTION;
  tmp1.nameId = dataIndex++;
  tmp2.nameId = dataIndex++;
  strcpy(dataList[tmp1.nameId].id, "read");
  strcpy(dataList[tmp2.nameId].id, "write");
  Function *func1 = (Function *)malloc(sizeof(Function));
  Function *func2 = (Function *)malloc(sizeof(Function));
  func1->nameId = tmp1.nameId;
  func2->nameId = tmp2.nameId;
  func1->returnTypeId = INT_ID();
  func2->returnTypeId = INT_ID();
  func1->parameterNum = 0;
  func2->parameterNum = 1;
  func1->p = NULL;
  func2->p = NULL;
  ParameterTypeIdList *p =
      (ParameterTypeIdList *)malloc(sizeof(ParameterTypeIdList));
  p->next = func2->p;
  p->typeId = INT_ID();
  func2->p = p;
  tmp1.function = func1;
  tmp2.function = func2;
  insertSymbol(tmp1);
  insertSymbol(tmp2);
}

SymbolLinker *symbolLinkerMalloc() {
  SymbolLinker *ans = (SymbolLinker *)malloc(sizeof(SymbolLinker));
  ans->down = NULL;
  ans->right = NULL;
  ans->data.kind = NULLHEADER;
  ans->data.nameId = INF;
  ans->data.v = NULL;
  ans->stackNum = curStackNum;
  return ans;
}

void addNewStack() {
  ++curStackNum;
  SymbolLinker *curStack = symbolLinkerMalloc();
  curStack->right = symbolStackHeader.right;
  symbolStackHeader.right = curStack;
}

void freeSymbol(Symbol s) {
  switch (s.kind) {
  // case NULLHEADER:
  //   return;
  case VARIBLE: {
    return;
  }
  case FUNCTION: {
    free(s.function);
    return;
  }
  case STRUCTURE: {
    return;
  }
  }
}

void deleteCurStack() {
  --curStackNum;
  SymbolLinker *curStack = GET_CUR_STACK();
  symbolStackHeader.right = curStack->right;
  for (; curStack->down;) {
    SymbolLinker *curSymbol = curStack->down;
    u32_t hashNum = hashSymbol(curSymbol->data.nameId);
    symbolTable[hashNum].right = curSymbol->right; // dont ingore this
    curStack->down = curSymbol->down;
    freeSymbol(curSymbol->data);
    free(curSymbol);
  }               // not only stackHeader should be dealed but also hashHeader
  free(curStack); // need not to free symbol for pointing to NULL
}

void insertSymbol(Symbol data) {
  assert(data.nameId != -1 && data.nameId != -2 && data.nameId != -3);
  u32_t hashNum = hashSymbol(data.nameId);
  SymbolLinker *curSymbol = symbolLinkerMalloc();
  curSymbol->right = curSymbol->down = NULL;
  curSymbol->data = data;
  // curSymbol->stackNum = curStackNum;
  SymbolLinker *curStack = GET_CUR_STACK();
  curSymbol->down = curStack->down;
  curStack->down = curSymbol;
  SymbolLinker *curHashLinker =
      &symbolTable[hashNum]; // note that pointer should be used to make a
                             // valid alter
  curSymbol->right = curHashLinker->right;
  curHashLinker->right = curSymbol;
}

SymbolLinker *findSymbol(u32_t nameId) {
  assert(nameId != -1 && nameId != -2 && nameId != -3);
  u32_t hashNum = hashSymbol(nameId);
  SymbolLinker curHashLinker = symbolTable[hashNum];
  // return curHashLinker.right; //name may not equal but hash is equal
  SymbolLinker *ans = curHashLinker.right;
  while (ans) {
    if (strcmp(dataList[ans->data.nameId].id, dataList[nameId].id) == 0)
      break;
    ans = ans->right;
  }
  return ans;
}

u8_t typeCompare(u32_t ida, u32_t idb) {
  if (ida == idb)
    return 1;
  if (ida == INF || idb == INF) {
    return 0;
  }
  Type *a = typeList + ida, *b = typeList + idb;
  if (a->kind != b->kind)
    return 0;
  switch (a->kind) {
  case BASIC: {
    return 0;
  }
  case STRUCTURE: {
    return 0;
  }
  case ARRAY: {
    return a->array->typeId == b->array->typeId &&
           a->array->dimension == b->array->dimension;
  }
  }
}

// int main() {
//   /*union test funcion*/
//   Symbol s1, s2, s3;
//   Varible *v1 = (Varible *)malloc(sizeof(Varible)),
//           *v2 = (Varible *)malloc(sizeof(Varible)),
//           *v3 = (Varible *)malloc(sizeof(Varible));
//   strcpy(dataList[0].id, "s1");
//   strcpy(dataList[1].id, "s2");
//   strcpy(dataList[2].id, "s1");
//   s1.kind = VARIBLE;
//   s1.nameId = 0;
//   s2.kind = VARIBLE;
//   s2.nameId = 1;
//   s3.kind = VARIBLE;
//   s3.nameId = 2;
//   v1->typeId = INT_ID();
//   v2->typeId = INT_ID();
//   v3->typeId = INT_ID();
//   s1.varible = v1;
//   s2.varible = v2;
//   s3.varible = v3;
//   initHeader();
//   addNewStack();
//   SymbolLinker *f1 = findSymbol(s1.nameId);
//   assert(f1 == NULL);
//   insertSymbol(s1);
//   SymbolLinker *f2 = findSymbol(s2.nameId);
//   assert(f2 == NULL);
//   insertSymbol(s2);
//   addNewStack();
//   SymbolLinker *f3 = findSymbol(s3.nameId);
//   printf("%d\n", f3->stackNum);
//   assert(f3 != NULL);
//   insertSymbol(s3);
//   f3 = findSymbol(s3.nameId);
//   printf("%d\n", f3->stackNum);
//   deleteCurStack();
//   f3 = findSymbol(s3.nameId);
//   assert(f3 != NULL);
//   printf("%d\n", f3->stackNum);
// }

void commonParser(node *n, Function *function) {
  assert(n->dataId != -2); // error
  if (n->dataId == -3) {
    return;
  } //
  switch (n->type) {
  case ExtDef: {
    parseExtDef(n);
    return;
  }
  case Def: {
    parseDef(n, NULL); // not a field
    return;
  }
  case Exp: {
    parseExp(n);
    return;
  }
  }
  if (n->type == Stmt && n->child->type == RETURN) {
    assert(function != NULL);
    assert(n->child->next->type == Exp);
    u32_t returnType = parseExp(n->child->next);
    if (!typeCompare(returnType, function->returnTypeId)) {
      printf("Error type 8 at Line %d: Return type mismatched.\n", n->row);
    }
    return;
  }
  node *child = n->child;
  if (n->type == CompSt) {
    // addNewStack();
    while (child) {
      commonParser(child, function);
      child = child->next;
    }
    // deleteCurStack();
    return;
  }
  while (child) {
    commonParser(child, function);
    child = child->next;
  }
} // parse List
/*
extdef是全局变量，结构体，函数的定义
def是局部变量
如何查找struct对应的typeid
  找struct同名symbol
  typeList[symbol.typeid].struct
无名struct不占用符号表
进入functionde时应该addstack,离开compst时delete
*/

u32_t addNewStructure(u32_t nameId) {
  if (nameId != INF) { // not anomymous
    assert(nameId != -1 && nameId != -2 && nameId != -3);
    SymbolLinker *symbolLinker = findSymbol(nameId);
    if (symbolLinker != NULL &&
        symbolLinker->stackNum == GET_CUR_STACK()->stackNum) { //同名且同作用域
      return INF;
    }
    Symbol data;
    data.kind = STRUCTURENAME;
    data.nameId = nameId;
    data.typeId = typeNum + 1;
    insertSymbol(data);
  }
  ++typeNum;
  typeList[typeNum].kind = STRUCTURE;
  Structure *s = (Structure *)malloc(sizeof(Structure));
  s->fieldNum = 0;
  s->field = NULL;
  s->nameId = nameId;
  typeList[typeNum].structure = s;
  return typeNum;
} // return INF if err16 structname redefined

void parseVarDecID(u32_t typeId, node *n, Type *structType,
                   Function *function) {
  assert(n->type == ID);
  assert(n->dataId != -1 && n->dataId != -2 && n->dataId != -3);
  SymbolLinker *symbolLinker = findSymbol(n->dataId);
  if (symbolLinker != NULL &&
      symbolLinker->stackNum == GET_CUR_STACK()->stackNum) { //存在同名
    if (structType == NULL) {
      printf("Error type 3 at Line %d: Redefined variable\n", n->row);
    } else { // 假设7 若域名重复只能是在同意结构体中
      printf("Error type 15 at Line %d: Redefined field\n", n->row);
    }
  }
  Symbol data;
  data.kind = (structType == NULL) ? VARIBLE : FIELD;
  data.nameId = n->dataId;
  data.typeId = typeId;
  insertSymbol(data);
  if (structType != NULL) {
    ++(structType->structure->fieldNum);
    Field *field = (Field *)malloc(sizeof(Field));
    field->nameId = n->dataId;
    field->typeId = typeId;
    if (structType->structure->field == NULL) {
      field->next = structType->structure->field;
      structType->structure->field = field; // header insert
    } else {
      Field *tmpf = structType->structure->field;
      while (tmpf->next) {
        tmpf = tmpf->next;
      }
      tmpf->next = field;
    }
  }
  if (function != NULL) {
    ++(function->parameterNum);
    ParameterTypeIdList *p =
        (ParameterTypeIdList *)malloc(sizeof(ParameterTypeIdList));
    p->nameId = n->dataId;
    p->typeId = typeId;
    p->next = function->p;
    function->p = p;
  }
}

u32_t addNewArray(u32_t baseType) {
  ++typeNum;
  typeList[typeNum].kind = ARRAY;
  Array *a = (Array *)malloc(sizeof(Array));
  a->typeId = baseType;
  a->dimension = 0;
  a->size = NULL;
  typeList[typeNum].array = a;
  return typeNum;
} // no name

u32_t parseArrayDec(u32_t baseType, node *n, Type *structType,
                    Function *function) {
  //传入的是初始Vardec
  assert(n->type == VarDec);
  u32_t typeId = addNewArray(baseType);
  Array *a = typeList[typeId].array;
  n = n->child; //第一层VarDec or Id
  while (n->next) {
    ArraySize *arraySize = (ArraySize *)malloc(sizeof(ArraySize));
    assert(n->next->next->type == INT);
    arraySize->size = dataList[n->next->next->dataId].i;
    ++(a->dimension);
    arraySize->next = a->size;
    a->size = arraySize;
    n = n->child;
  } // this part define the order of dimension: from left to right, opposite
    // to cmm
  assert(n->type == ID);
  u32_t nameId;
  assert(n->dataId != -1 && n->dataId != -2 && n->dataId != -3);
  SymbolLinker *symbolLinker = findSymbol(n->dataId);
  if (symbolLinker != NULL &&
      symbolLinker->stackNum == GET_CUR_STACK()->stackNum) {
    if (structType == NULL) {
      printf("Error type 3 at Line %d: Redefined variable\n", n->row);
    } else { // 假设7 若域名重复只能是在同意结构体中
      printf("Error type 15 at Line %d: Redefined field\n", n->row);
    }
  }
  Symbol data;
  data.kind = (structType == NULL) ? VARIBLE : FIELD;
  data.nameId = n->dataId;
  data.typeId = typeId;
  insertSymbol(data);
  if (structType != NULL) {
    ++(structType->structure->fieldNum);
    Field *field = (Field *)malloc(sizeof(Field));
    field->nameId = n->dataId;
    field->typeId = typeId;
    if (structType->structure->field == NULL) {
      field->next = structType->structure->field;
      structType->structure->field = field; // header insert
    } else {
      Field *tmpf = structType->structure->field;
      while (tmpf->next) {
        tmpf = tmpf->next;
      }
      tmpf->next = field;
    }
  }
  if (function != NULL) {
    ++(function->parameterNum);
    ParameterTypeIdList *p =
        (ParameterTypeIdList *)malloc(sizeof(ParameterTypeIdList));
    p->typeId = typeId;
    p->next = function->p;
    function->p = p;
  }
  return typeId;
  // return typeId;
}

u32_t parseArray(node *n, int dimension) {
  assert(n->type == Exp); // root exp
  node *child = n->child; // first child exp
  u32_t indexType;
  if (child->next != NULL && child->next->type == LB) {
    assert(child->next->next->type == Exp);
    indexType = parseExp(child->next->next);
    if (indexType == INF || typeList[indexType].kind != BASIC ||
        typeList[indexType].basic != INT_ID()) {
      printf("Error type 12 at Line %d: index is not an integer.\n",
             child->next->next->row);
      return INF;
    }
    return parseArray(child, dimension + 1);
  }
  u32_t baseType;
  baseType = parseExp(n);
  if (typeList[baseType].kind == ARRAY) {
    if (typeList[baseType].array->dimension != dimension) {
      // will not happen?
      assert(0);
      return INF;
    }
    return typeList[baseType].array->typeId;
  }
  printf("Error type 10 at Line %d:var is not an array\n", n->row);
  return INF;
}

u32_t parseExp(node *n) {
  assert(n != NULL);
  assert(n->type == Exp);
  node *child1 = n->child;
  u32_t typeId, nameId;
  if (child1->next == NULL) {
    switch (child1->type) {
    case INT: {
      return INT_ID();
    }
    case FLOAT: {
      return FLOAT_ID();
    }
    case ID: {
      assert(child1->dataId != -1 && child1->dataId != -2 &&
             child1->dataId != -3);
      SymbolLinker *tmp = findSymbol(child1->dataId);
      if ((tmp == NULL) || (tmp->data.kind != VARIBLE)) {
        printf("Error type 1 at Line %d: Undefined variable\n", child1->row);
        return INF;
      }
      assert(tmp->data.typeId != INF);
      return tmp->data.typeId;
    }
    default:
      assert(0);
    }
  }
  switch (child1->type) {
  case LP: {
    return parseExp(child1->next);
  }
  case MINUS: {
    typeId = parseExp(child1->next);
    if (typeId == INT_ID() || typeId == FLOAT_ID()) {
      return typeId;
    }
    printf("Error type 7 at Line %d: Type cant be used for this operator\n",
           child1->row);
    return INF;
  }
  case NOT: {
    typeId = parseExp(child1->next);
    if (typeId == INT_ID()) {
      return typeId;
    }
    printf("Error type 7 at Line %d: Type cant be used for this operator\n",
           child1->row);
    return INF; //?逻辑非
  }
  default:
    break;
  }
  if (child1->type == ID) {
    u32_t functionId;
    assert(child1->dataId != -1 && child1->dataId != -2 &&
           child1->dataId != -3);
    SymbolLinker *tmp1 = findSymbol(child1->dataId);
    if (tmp1 == NULL) {
      printf("Error type 2 at Line %d: Undefined function\n", child1->row);
      return INF;
    }
    if (tmp1->data.kind == VARIBLE || tmp1->data.kind == FIELD) {
      printf("Error type 11 at Line %d: Not a function\n", child1->row);
      return INF;
    }
    Function *func1 = tmp1->data.function;
    assert(func1 != NULL);
    // printf("%s\n", dataList[child1->dataId].id);
    assert(tmp1->data.kind == FUNCTION);
    u32_t parameterNum = func1->parameterNum;
    node *cur = child1->next->next; // first parameter
    if (cur->type == RP) {
      if (func1->parameterNum != 0) {
        printf("Error type 9 at Line %d: Function with wrong arguments\n",
               cur->row);
        return INF;
      }
      return func1->returnTypeId;
    }
    assert(cur->type == Args);
    cur = cur->child; // first Exp in Args
    u32_t argNum = 1;
    while (cur->next) {
      ++argNum;
      cur = cur->next->next;
      cur = cur->child;
      assert(cur->type == Exp);
    }
    node *args = child1->next->next; // reset now cur is Args
    assert(args != NULL);
    if (argNum != parameterNum) {
      printf("Error type 9 at Line %d: Function with wrong arguments\n",
             args->row);
      return INF; // ?? maybe it should return returntype
    }
    ParameterTypeIdList *p;
    p = func1->p;
    u32_t ida, idb;
    node *exp = args->child; // first Exp;
    for (int i = 0; i < argNum; ++i) {
      ida = p->typeId;
      idb = parseExp(exp);
      if (!typeCompare(ida, idb)) {
        printf("Error type 9 at Line %d: Function with wrong arguments\n",
               cur->row);
        return INF;
      }
      p = p->next;
      if (exp->next) {
        args = exp->next->next;
        exp = args->child;
      }
    }
    return func1->returnTypeId;
  }
  if (child1->next->type == LB) {
    return parseArray(n, 0); // root exp
  }
  if (child1->next->type == DOT) {
    typeId = parseExp(child1);
    if (typeId == INF)
      return INF;
    if (typeList[typeId].kind != STRUCTURE) {
      printf("Error type 13 at Line %d: Illegal dot opeartor\n", child1->row);
      return INF;
    }
    assert(child1->next->next->dataId != -1 &&
           child1->next->next->dataId != -2 &&
           child1->next->next->dataId != -3);
    SymbolLinker *tmp2 = findSymbol(child1->next->next->dataId);
    if (tmp2 == NULL || tmp2->data.kind != FIELD) {
      printf("Error type 14 at Line %d: Non-existent field\n", child1->row);
      return INF;
    }
    Field *field = typeList[typeId].structure->field;
    while (field) {
      if (typeCompare(tmp2->data.typeId,
                      field->typeId)) { //? 根据假设，可以用名相等判断
        return field->typeId;
      } //
      field = field->next;
    }
    printf("Error type 14 at Line %d: Non-existent field\n", child1->row);
    return INF;
  }
  // child1->next->next->next == NULL
  node *op = child1->next;
  u32_t typeIdl = parseExp(child1);
  u32_t typeIdr = parseExp(op->next);
  if (typeIdl == INF || typeIdr == INF) {
    return INF;
  } //?
  if (!typeCompare(typeIdl, typeIdr)) {
    if (op->type == ASSIGNOP) {
      printf("Error type 5 at Line %d: Type mismatched for assignment.\n",
             child1->row);
    } else {
      printf("Error type 7 at Line %d: Type mismatched for operands.\n",
             child1->row);
    }
    return INF;
  }
  switch (op->type) {
  case ASSIGNOP: {
    u32_t childNum = 0; // left Exp's childNum
    for (node *tmpNode = child1->child; tmpNode; tmpNode = tmpNode->next) {
      ++childNum;
    }
    if ((childNum == 1 && child1->child->type == ID) ||
        (childNum == 4 && child1->child->next->type == LB) ||
        (childNum == 3 && child1->child->next->type == DOT)) {
      if (typeCompare(typeIdl, typeIdr)) {
        return typeIdl;
      }
      printf("Error type 5 at Line %d: Type mismatched for assignment.\n",
             child1->row);
      return INF;
    } // left values
    printf("Error type 6 at Line %d: The left-hand side of an assignment must "
           "be a variable\n",
           child1->row);
    return INF;
  }
  case AND:
  case OR: {
    if (typeIdl != INT_ID() || typeIdr != INT_ID()) {
      printf("Error type 7 at Line %d: Type mismatched for operands.\n",
             child1->row);
      return INF;
    }
    return INT_ID();
  }
  case RELOP: {
    if (typeList[typeIdl].kind != BASIC || typeList[typeIdr].kind != BASIC) {
      printf("Error type 7 at Line %d: Type mismatched for operands.\n",
             child1->row);
      return INF;
    }
    return INT_ID();
  }
  case PLUS:
  case MINUS:
  case STAR:
  case DIV: {
    return typeIdl;
  }
  }
  assert(0);
  return INF; // never used
}

void parseDec(u32_t typeId, node *n, Type *structType) {
  assert(n->type == Dec);
  node *varDec = n->child;
  if (varDec->child->type == ID) {
    node *id = varDec->child;
    parseVarDecID(typeId, id, structType, NULL);
    if (varDec->next != NULL) { // assignop
      if (structType != NULL) {
        printf("Error type 15 at Line %d: Initialize field\n", varDec->row);
      } else {
        u32_t rightType = parseExp(varDec->next->next);
        if (rightType != INF && !typeCompare(typeId, rightType)) {
          printf("Error type 5 at Line %d: Type ummatched\n", varDec->row);
        }
      }
    }
    return;
  }
  u32_t ArrayTypeId = parseArrayDec(typeId, varDec, structType, NULL);
  if (n->next) {
    if (structType != NULL) {
      printf("Error type 15 at Line %d: Initialize field\n", varDec->row);
    } else {
      // printf("Error type 7 at Line %d: Type mismatched for operands.\n",
      //        varDec->row);
      // assert(0); // never occur in experiment 2
      // here is experiment 3
    } // no in experiment 2 !! remember to alter me!!!
  }
  //
}

void parseDef(node *n,
              Type *structType) { //对于struct需要指定其域，对于function不需要
  assert(n->type == Def);
  u32_t defType = parseSpecifier(n->child);
  if (defType == INF)
    return;
  node *decList = n->child->next;
  assert(decList->type == DecList);
  node *dec = decList->child;
  while (dec->next != NULL) {
    parseDec(defType, dec, structType);
    decList = dec->next->next;
    assert(decList->type == DecList);
    dec = decList->child;
  }
  parseDec(defType, dec, structType);
}

u32_t parseSpecifier(node *n) {
  assert(n->type == Specifier);
  node *child = n->child;
  if (child->type == TYPE) {
    if (child->dataId == INT) {
      return INT_ID();
    } else {
      return FLOAT_ID();
    }
  }
  child = child->child;
  assert(child->type == STRUCT);
  node *optTag = child->next;
  u32_t nameId, typeId;
  if (optTag->type == OptTag) { // 5元组
    node *defList = optTag->next->next;
    assert(defList->type == DefList);
    node *def;
    if (optTag->dataId != -3) { // not empty
      node *tag = optTag->child;
      nameId = tag->dataId;
    } else {        // name is empty
      nameId = INF; // no name
    }
    typeId = addNewStructure(nameId);
    if (typeId == INF) {
      printf("Error type 16 at Line %d: Redefined struct\n", optTag->row);
      return INF; // return INF if err 16
    }
    while (defList->dataId != -3) { // not NULL
      def = defList->child;
      parseDef(def, typeList + typeId);
      defList = def->next;
      assert(defList->type == DefList);
    }
    return typeId;
  } else { //
    node *tag = optTag;
    assert(tag->child->dataId != -1 && tag->child->dataId != -2 &&
           tag->child->dataId != -3);
    SymbolLinker *symbolLinker = findSymbol(tag->child->dataId);
    if (symbolLinker == NULL || symbolLinker->data.kind != STRUCTURENAME) {
      printf("Error type 17 at Line %d: Undefined structure\n", tag->row);
      return INF;
    }
    return symbolLinker->data.typeId;
  }
} // return typeId

void parseExtDef(node *n) {
  assert(n->type == ExtDef);
  assert(n->child->type == Specifier);
  u32_t typeId = parseSpecifier(n->child);
  if (n->child->next->next == NULL || typeId == INF) {
    return;
  } // specifier semi
  if (n->child->next->type == ExtDecList) {
    node *extDecList = n->child->next;
    node *varDec = extDecList->child;
    assert(varDec->type == VarDec);
    while (varDec->next) {
      assert(extDecList->type == ExtDecList);
      if (varDec->child->type == ID) {
        parseVarDecID(typeId, varDec->child, NULL, NULL);
      } else {
        parseArrayDec(typeId, varDec, NULL, NULL);
      }
      extDecList = varDec->next->next;
      varDec = extDecList->child;
    } // deal with extdeclist
    if (varDec->child->type == ID) {
      parseVarDecID(typeId, varDec->child, NULL, NULL);
    } else {
      parseArrayDec(typeId, varDec, NULL, NULL);
    }
    return;
  }
  if (n->child->next->type == FunDec) {
    node *funDec = n->child->next;
    assert(funDec->type == FunDec);
    u32_t funType;
    node *id = funDec->child;
    assert(id->dataId != -1 && id->dataId != -2 && id->dataId != -3);
    SymbolLinker *tmp = findSymbol(id->dataId);
    if (tmp != NULL) {
      printf("Error type 4 at Line %d: Redefined function\n", id->row);
      return;
    } //不用查作用域，因为无嵌套定义函数
    Symbol symbol;
    symbol.kind = FUNCTION;
    Function *func = (Function *)malloc(sizeof(Function));
    func->nameId = id->dataId;
    func->returnTypeId = typeId;
    func->parameterNum = 0;
    assert(func->parameterNum == 0); //
    func->p = NULL;
    symbol.function = func;
    symbol.nameId = id->dataId;
    insertSymbol(symbol);
    // addNewStack();
    node *varList = id->next->next;
    if (varList->type != RP) {
      assert(varList->type == VarList);
      node *paramDec = varList->child;
      assert(paramDec->type == ParamDec);
      node *varDec = paramDec->child->next;
      assert(varDec->type == VarDec);
      u32_t paraType;
      while (paramDec->next != NULL) {
        assert(paramDec->child->type == Specifier);
        paraType = parseSpecifier(paramDec->child);
        if (paraType == INF)
          return; // should never occur?
        if (varDec->child->type == ID) {
          parseVarDecID(paraType, varDec->child, NULL, func);
        } else {
          assert(varDec->child->type == VarDec);
          parseArrayDec(paraType, varDec, NULL, func);
        }
        varList = paramDec->next->next;
        paramDec = varList->child;
        varDec = paramDec->child->next;
      }
      assert(paramDec->child->type == Specifier);
      paraType = parseSpecifier(paramDec->child);
      if (paraType == INF)
        return;
      if (varDec->child->type == ID) {
        parseVarDecID(paraType, varDec->child, NULL, func);
      } else {
        assert(varDec->child->type == VarDec);
        parseArrayDec(paraType, varDec, NULL, func);
      } // deal with VarList
    }   // have VarList
    assert(n->child->next->next->type == CompSt);
    // commonParser(n->child->next->next, function);
    node *child = n->child->next->next->child;
    while (child) {
      commonParser(child, func);
      child = child->next;
    }
    // deal with Compst
    // 不能直接 commonPaser,否则会再加一层作用域
    // deleteCurStack();
    return;
  }
  assert(0);
  return;
}
