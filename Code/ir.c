#include "ir.h"
#include "symbolTable.h"
#include "syntax.tab.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #define NDEBUG

// undo
// the field of struct?
// how to deal with the assign of struct and arr?
// use store and load!
// *is not a new operand but a new instruct
// things seems to be different when calling a function   para!!
// struct 传值为 addr所以para中的变量为addr类型

char IC_NAME[20][7] = {
    " + ",  " - ", " * ",  " / ",  " == ",   " != ",  " < ",
    " <= ", " > ", " >= ", "ARG ", "PARAM ", "READ ", "WRITE "};

typedef struct S_Args {
  Operand arg;
  struct S_Args *next;
} S_Args;

int var_num = 0, label_num = 0, temp_num = 0;
//变量名为v_{var_num} label, temp以此类推，注意不重名要求
//注意，ic中没有源代码中的变量名，统一以v_命名
// varList[var_num].list == INF iff varilbe havent been assign a name
ICLinker *ICHeader, *ICTail;

#define INSERT_IC(icl)                                                         \
  ICTail->next = icl;                                                          \
  icl->prev = ICTail;                                                          \
  icl->next = ICHeader;                                                        \
  ICHeader->prev = icl;                                                        \
  ICTail = icl;

static void genFunDec(node *funDec);
static void genExp(node *exp, Operand *opp);
static void genDec(node *dec);
static void genDefList(node *defList);
static void genStmt(node *stmt);
static void genStmtList(node *stmtList);
static void genCompSt(node *compSt);
static void genExtDef(node *extDef);
static void genArgs(node *args, ParameterTypeIdList *p);
static void genCond(node *cond, Operand label_true, Operand label_false);
u32_t sizeCounter(u32_t nameId, u32_t structId, u32_t index);

Var varList[DATA_SIZE];
// hash table will be better

u32_t _sizeCounter(u32_t typeId) {
  u32_t size = 0;
  if (typeList[typeId].kind == ARRAY) {
    Array *array = typeList[typeId].array;
    assert(array->dimension == 1);
    size = _sizeCounter(array->typeId) * array->size->size;
  } else if (typeList[typeId].kind == STRUCTURE) {
    Structure *structure = typeList[typeId].structure;
    Field *f = structure->field;
    for (int i = structure->fieldNum; i > 0; --i) {
      size += _sizeCounter(f->typeId);
      assert(f);
      f = f->next;
    }
  } else {
    size = 4;
  }
  return size;
}

u32_t sizeCounter(u32_t nameId, u32_t structId, u32_t index) {
  Symbol s = findSymbol(nameId)->data;
  if ((structId == INF) && (index == INF)) {
    // not filed
    return _sizeCounter(s.typeId);
  } else if (structId != INF) {
    // field
    u32_t size = 0;
    Structure *structure =
        typeList[findSymbol(structId)->data.typeId].structure;
    Field *f = structure->field;
    while (strcmp(dataList[f->nameId].id, dataList[nameId].id) != 0) {
      size += _sizeCounter(f->typeId);
      f = f->next;
    }
    return size;
  } else {
    Array *array = typeList[s.typeId].array;
    assert(array->dimension == 1);
    u32_t size = _sizeCounter(array->typeId) * 1;
    // attenion index may cant be resolved when compile
    return size;
  }
  return 0;
}

ICLinker *ICMalloc() {
  ICLinker *ans = (ICLinker *)malloc(sizeof(ICLinker));
  ans->next = NULL;
  ans->prev = NULL;
  ans->code.kind = IC_INIT;
  return ans;
}

ICLinker *IC_op(Operand op) {
  ICLinker *ans = ICMalloc();
  ans->code.u.op = op;
  return ans;
}

ICLinker *IC_binop(Operand r, Operand op1, Operand op2) {
  ICLinker *ans = ICMalloc();
  ans->code.u.binop.result = r;
  ans->code.u.binop.op1 = op1;
  ans->code.u.binop.op2 = op2;
  return ans;
}

ICLinker *IC_assign(Operand left, Operand right) {
  ICLinker *ans = ICMalloc();
  ans->code.u.assign.left = left;
  ans->code.u.assign.right = right;
  return ans;
}

ICLinker *IC_call(Operand r, u32_t value) {
  ICLinker *ans = ICMalloc();
  ans->code.u.call.op = r;
  ans->code.u.call.value = value;
  return ans;
}

ICLinker *IC_function(u32_t nameId) {
  ICLinker *ans = ICMalloc();
  ans->code.u.nameId = nameId;
  return ans;
}

void initIr() {
  ICTail = ICHeader = ICMalloc();
  ICHeader->next = ICHeader;
  ICHeader->prev = ICHeader;
  ICHeader->code.kind = NULL_HEADER;
  for (int i = 0; i < DATA_SIZE; ++i) {
    varList[i].nameId = INF;
  }
}

#define writeOperand(operand)                                                  \
  switch (operand.kind1) {                                                     \
  case OP_VARIABLE:                                                            \
    fprintf(writeFile, "v%d", operand.u.varno);                                \
    break;                                                                     \
  case OP_CONSTANT:                                                            \
    fprintf(writeFile, "#%d", operand.u.instant);                              \
    break;                                                                     \
  case OP_TEMP:                                                                \
    fprintf(writeFile, "t%d", operand.u.tempno);                               \
    break;                                                                     \
  default:                                                                     \
    assert(0);                                                                 \
    break;                                                                     \
  }

void _writeIC(ICLinker *cur) {
  IC code;
  code = cur->code;
  switch (code.kind) {
  case IC_LABEL: {
    fprintf(writeFile, "LABEL label%d :\n", code.u.op.u.labelno);
    break;
  }
  case IC_FUNCTION: {
    fprintf(writeFile, "FUNCTION %s :\n", dataList[code.u.nameId].id);
    break;
  }
  case IC_ASSIGN: {
    writeOperand(code.u.assign.left);
    fprintf(writeFile, " := ");
    writeOperand(code.u.assign.right);
    fprintf(writeFile, "\n");
    break;
  }
  case IC_ADD:
  case IC_SUB:
  case IC_MUL:
  case IC_DIV: {
    writeOperand(code.u.binop.result);
    fprintf(writeFile, " := ");
    if (code.u.binop.op1.kind2 == OP_ARR ||
        code.u.binop.op1.kind2 == OP_STRUCT) {
      fprintf(writeFile, "&v%d", code.u.binop.op1.u.varno);
    } else {
      writeOperand(code.u.binop.op1);
    }
    fprintf(writeFile, "%s", IC_NAME[code.kind]);
    if (code.u.binop.op2.kind2 == OP_ARR ||
        code.u.binop.op2.kind2 == OP_STRUCT) {
      fprintf(writeFile, "&v%d", code.u.binop.op2.u.varno);
    } else {
      writeOperand(code.u.binop.op2);
    }
    fprintf(writeFile, "\n");
    break;
  }
  case IC_ADDR: {
    writeOperand(code.u.assign.left);
    fprintf(writeFile, " := &");
    writeOperand(code.u.assign.right);
    fprintf(writeFile, "\n");
    break;
  }
  case IC_LOAD: {
    writeOperand(code.u.assign.left);
    fprintf(writeFile, " := *");
    writeOperand(code.u.assign.right);
    fprintf(writeFile, "\n");
    break;
  }
  case IC_STORE: {
    fprintf(writeFile, "*");
    writeOperand(code.u.assign.left);
    fprintf(writeFile, " := ");
    writeOperand(code.u.assign.right);
    fprintf(writeFile, "\n");
    break;
  }
  case IC_JUMP: {
    fprintf(writeFile, "GOTO label%d\n", code.u.op.u.labelno);
    break;
  }
  case IC_EQ:
  case IC_NE:
  case IC_LT:
  case IC_LE:
  case IC_GT:
  case IC_GE: {
    fprintf(writeFile, "IF ");
    writeOperand(code.u.binop.op1);
    fprintf(writeFile, "%s", IC_NAME[code.kind]);
    writeOperand(code.u.binop.op2);
    fprintf(writeFile, " GOTO label%d\n", code.u.binop.result.u.labelno);
    break;
  }
  case IC_RETURN: {
    fprintf(writeFile, "RETURN ");
    writeOperand(code.u.op);
    fprintf(writeFile, "\n");
    break;
  }
  case IC_DEC: {
    fprintf(writeFile, "DEC ");
    writeOperand(code.u.call.op);
    fprintf(writeFile, " %d\n", code.u.call.value);
    break;
  }
  case IC_ARG:
    fprintf(writeFile, "%s", IC_NAME[code.kind]);
    if (code.u.op.kind2 == OP_STRUCT) {
      // deat with struct
      // if struct is not var?
      fprintf(writeFile, "&v%d", code.u.op.u.varno);
    } else {
      writeOperand(code.u.op);
    }
    fprintf(writeFile, "\n");
    break;
  case IC_PARAM:
    fprintf(writeFile, "%s", IC_NAME[code.kind]);
    if (code.u.op.kind2 == OP_ADDRESS) {
      // deal with struct
      fprintf(writeFile, "v%d", code.u.op.u.varno);
    } else {
      writeOperand(code.u.op);
    }
    fprintf(writeFile, "\n");
    break;
  case IC_READ:
  case IC_WRITE: {
    fprintf(writeFile, "%s", IC_NAME[code.kind]);
    writeOperand(code.u.op);
    fprintf(writeFile, "\n");
    break;
  }
  case IC_CALL: {
    writeOperand(code.u.call.op);
    fprintf(writeFile, " := CALL %s\n", dataList[code.u.call.value].id);
    break;
  }
  default:
    assert(0);
  }
}

void writeIC() {
  ICLinker *cur = ICHeader->next;
  while (cur != ICHeader) {
    _writeIC(cur);
    cur = cur->next;
  }
}

Operand newTemp() {
  Operand ans;
  ans.kind1 = OP_TEMP;
  ans.kind2 = OP_NULL2;
  ans.para = 0;
  ans.typeId = INF;
  ans.u.labelno = temp_num++;
  return ans;
}

Operand newLabel() {
  Operand ans;
  ans.kind1 = OP_LABEL;
  ans.kind2 = OP_NULL2;
  ans.para = 0;
  ans.typeId = INF;
  ans.u.labelno = label_num++;
  return ans;
}

Operand newConstant(int instant) {
  Operand ans;
  ans.kind1 = OP_CONSTANT;
  ans.kind2 = OP_NULL2;
  ans.para = 0;
  ans.typeId = INT_ID();
  ans.u.instant = instant;
  return ans;
}

Operand *getVariable(u32_t nameId) {
  // printf("%s %d\n", dataList[nameId].id, var_num);
  // for (int i = 0; i < var_num; ++i) {
  //   printf("%s %d", dataList[varList[i].nameId].id, varList[i].op.kind2);
  // }
  // printf("\n");
  for (int i = 0; i < var_num; ++i) {
    if (strcmp(dataList[varList[i].nameId].id, dataList[nameId].id) == 0) {
      // printf("matched\n");
      // printf("%s, %s, %d\n", dataList[varList[i].nameId].id,
      //        dataList[nameId].id, i);
      return &(varList[i].op);
    }
  }
  SymbolLinker *s = findSymbol(nameId);
  varList[var_num].nameId = nameId;
  varList[var_num].op.para = 0;
  varList[var_num].op.kind1 = OP_VARIABLE;
  varList[var_num].op.kind2 = OP_NULL2;
  varList[var_num].op.typeId = s->data.typeId;
  varList[var_num].op.u.varno = var_num;
  Operand *ans = &(varList[var_num].op);
  ++var_num;
  return ans;
}

static void genFunDec(node *funDec) {
  if (errorFlag)
    return;
  node *id = funDec->child;
  ICLinker *icl1 = IC_function(id->dataId);
  icl1->code.kind = IC_FUNCTION;
  INSERT_IC(icl1);
  node *varList = id->next->next;
  if (varList->type != VarList)
    return;
  SymbolLinker *s = findSymbol(id->dataId);
  ParameterTypeIdList *p = s->data.function->p;
  Operand *op;
  while (p) {
    op = getVariable(p->nameId);
    op->para = 1;
    if (typeList[p->typeId].kind == BASIC) {
      op->kind2 = OP_COM;
    } else if (typeList[p->typeId].kind == STRUCTURE) {
      op->kind2 = OP_ADDRESS;
      // attention!! not structure
    } else {
      // no parameter as arr
      errorFlag = 1;
      printf("Cannot translate: Code contains variables of multi-dimensional "
             "array type or parameters of array type");
      return;
      op->kind2 = OP_ARR;
    }
    ICLinker *icl2 = IC_op(*op);
    icl2->code.kind = IC_PARAM;
    INSERT_IC(icl2);
    p = p->next;
  }
}

static void genArgs(node *args, ParameterTypeIdList *p) {
  //此处var应该全部已经完成初始化
  if (errorFlag)
    return;
  node *exp = args->child;
  Operand op;
  while (exp) {
    if (!(exp->child->next) && (exp->child->type == ID)) {
      op = *getVariable(exp->child->dataId);
      // printf("call arg %s varno %d\n", dataList[exp->child->dataId].id,
      //        op.u.varno);
      assert(op.kind2 != OP_NULL2);
    } else {
      op = newTemp();
      assert(exp->type == Exp);
      genExp(exp, &op);
    }
    ICLinker *icl = IC_op(op);
    icl->code.kind = IC_ARG;
    INSERT_IC(icl);
    if (exp->next) {
      args = exp->next->next;
      exp = args->child;
    } else {
      exp = NULL;
    }
  }
}

// attention if opp is addr, cant use assign, use store
static void genExp(node *exp, Operand *opp) {
  if (errorFlag)
    return;
  node *child = exp->child;
  switch (child->type) {
  case ID: {
    // Operand *op = getVariable(child->dataId);
    // if (child->next == NULL && op->kind2 == OP_ARR) {
    //   errorFlag = 1;
    //   printf("cant assign to an array\n");
    //   return;
    // }
    if (child->next == NULL) {
      if (opp) {
        Operand *op = getVariable(child->dataId);
        if (op->kind2 == OP_ARR || op->kind2 == OP_STRUCT) {
          ICLinker *icl = IC_assign(*opp, *op);
          icl->code.kind = IC_ADDR;
          INSERT_IC(icl);
        } else {
          ICLinker *icl = IC_assign(*opp, *op);
          icl->code.kind = IC_ASSIGN;
          INSERT_IC(icl);
        }
      }
    } else {
      // function
      node *args = child->next->next;
      if (args->type == RP) {
        if (!strcmp(dataList[child->dataId].id, "read")) {
          assert(opp);
          ICLinker *icl1 = IC_op(*opp);
          icl1->code.kind = IC_READ;
          INSERT_IC(icl1);
        } else {
          assert(opp);
          ICLinker *icl2 = IC_call(*opp, child->dataId);
          icl2->code.kind = IC_CALL;
          INSERT_IC(icl2);
        }
      } else {
        if (!strcmp(dataList[child->dataId].id, "write")) {
          node *exp = args->child;
          assert(!(exp->next));
          Operand tmp1 = newTemp();
          assert(exp->type == Exp);
          genExp(exp, &tmp1);
          ICLinker *icl3 = IC_op(tmp1);
          icl3->code.kind = IC_WRITE;
          INSERT_IC(icl3);
          if (opp) {
            Operand ins1 = newConstant(0);
            ICLinker *icl4 = IC_assign(*opp, ins1);
            icl4->code.kind = IC_ASSIGN;
            INSERT_IC(icl4);
            //?
          } else {
          }
        } else {
          genArgs(args, findSymbol(child->dataId)->data.function->p);
          assert(opp);
          ICLinker *icl5 = IC_call(*opp, child->dataId);
          icl5->code.kind = IC_CALL;
          INSERT_IC(icl5);
        }
      }
    }
    break;
  }
  case INT: {
    if (opp) {
      int val = dataList[child->dataId].i;
      Operand op = newConstant(val);
      ICLinker *icl = IC_assign(*opp, op);
      icl->code.kind = IC_ASSIGN;
      INSERT_IC(icl);
    }
    break;
  }
  case NOT:
  case Exp: {
    node *operation = child->next;
    if ((child->type == NOT) || (operation->type == RELOP) ||
        (operation->type == AND) || (operation->type == OR)) {
      Operand label1 = newLabel(), label2 = newLabel();
      Operand ins1 = newConstant(0), ins2 = newConstant(1);
      ICLinker *icl1 = IC_assign(*opp, ins1);
      icl1->code.kind = IC_ASSIGN;
      INSERT_IC(icl1);
      genCond(exp, label1, label2);
      ICLinker *icl2 = IC_op(label1);
      icl2->code.kind = IC_LABEL;
      INSERT_IC(icl2);
      ICLinker *icl3 = IC_assign(*opp, ins2);
      icl3->code.kind = IC_ASSIGN;
      INSERT_IC(icl3);
      ICLinker *icl4 = IC_op(label2);
      icl4->code.kind = IC_LABEL;
      INSERT_IC(icl4);
      break;
    }
    if (operation->type == ASSIGNOP) {
      if (child->child->type == ID) {
        Operand v1 = *getVariable(child->child->dataId);
        if (v1.kind2 != OP_ARR) {
          assert(child->next->next->type == Exp);
          genExp(child->next->next, &v1);
          if (opp) {
            ICLinker *icl1 = IC_assign(*opp, v1);
            icl1->code.kind = IC_ASSIGN;
            INSERT_IC(icl1);
          }
        } else {
          // v1 and v2 are array, not address
          assert(operation->next->child->type == ID);
          Operand v2 = *getVariable(operation->next->child->dataId);
          // this is assign by address
          // getVariable(child->child->dataId)->u.varno = v2.u.varno;
          // this is assign by value
          u32_t len1 = typeList[v1.typeId].array->size->size;
          u32_t len2 = typeList[v2.typeId].array->size->size;
          u32_t len = len1 < len2 ? len1 : len2;
          // u32_t sz = _sizeCounter(typeList[v1.typeId].array->typeId); sz is 4
          assert(opp == NULL);
          Operand tmp1 = newTemp(), tmp2 = newTemp();
          tmp1.kind2 = OP_COM;     // addr1
          tmp2.kind2 = OP_ADDRESS; // addr2
          ICLinker *icl1 = IC_assign(tmp1, v1);
          icl1->code.kind = IC_ADDR;
          INSERT_IC(icl1); // get addr1
          ICLinker *icl2 = IC_assign(tmp2, v2);
          icl2->code.kind = IC_ADDR;
          INSERT_IC(icl2); // get addr1
          Operand tmp3 = newTemp();
          tmp3.kind2 = OP_COM; // value
          ICLinker *icl5 = IC_assign(tmp3, tmp2);
          icl5->code.kind = IC_LOAD;
          INSERT_IC(icl5);
          ICLinker *icl6 = IC_assign(tmp1, tmp3);
          icl6->code.kind = IC_STORE;
          INSERT_IC(icl6);
          for (int i = 0; i < len - 1; ++i) {
            Operand tmp3 = newTemp();
            tmp3.kind2 = OP_COM; // value
            ICLinker *icl3 = IC_binop(tmp1, tmp1, newConstant(4));
            //每次在原来基础上加4
            icl3->code.kind = IC_ADD;
            INSERT_IC(icl3);
            ICLinker *icl4 = IC_binop(tmp2, tmp2, newConstant(4));
            icl4->code.kind = IC_ADD;
            INSERT_IC(icl4);
            ICLinker *icl5 = IC_assign(tmp3, tmp2);
            icl5->code.kind = IC_LOAD;
            INSERT_IC(icl5);
            ICLinker *icl6 = IC_assign(tmp1, tmp3);
            icl6->code.kind = IC_STORE;
            INSERT_IC(icl6);
          }
        }
      } else if (child->child->next->type == DOT) {
        assert(child->child->child->type == ID);
        Operand tmp1 = newTemp(), tmp2 = newTemp();
        tmp1.kind2 = OP_COM;
        tmp2.kind2 = OP_ADDRESS;
        assert(operation->next->type == Exp);
        genExp(operation->next, &tmp1); // get right value
        Operand v1 = *getVariable(child->child->child->dataId);
        Operand v2 = *getVariable(child->child->next->next->dataId);
        ICLinker *icl1 = IC_assign(tmp2, v1);
        if (v1.kind2 == OP_ADDRESS)
          icl1->code.kind = IC_ASSIGN;
        else
          icl1->code.kind = IC_ADDR; // attention
        INSERT_IC(icl1);             // get addr
        u32_t offset = sizeCounter(child->child->next->next->dataId,
                                   child->child->child->dataId, INF);
        ICLinker *icl2 = IC_binop(tmp2, tmp2, newConstant(offset));
        icl2->code.kind = IC_ADD;
        INSERT_IC(icl2);
        ICLinker *icl3 = IC_assign(tmp2, tmp1);
        icl3->code.kind = IC_STORE;
        INSERT_IC(icl3);
        if (opp) {
          ICLinker *icl4 = IC_assign(*opp, tmp1);
          icl4->code.kind = IC_ASSIGN;
          INSERT_IC(icl4);
        }
      } else if (child->child->next->type == LB) {
        node *arrID = child->child->child;
        assert(arrID->type == ID);
        Operand v1 = *getVariable(arrID->dataId);
        Operand tmp1 = newTemp(), tmp2 = newTemp(), tmp3 = newTemp();
        tmp1.kind2 = OP_COM;     // right
        tmp2.kind2 = OP_ADDRESS; // addr
        tmp3.kind2 = OP_COM;     // index
        assert(operation->next->type == Exp);
        genExp(operation->next, &tmp1); // get right value
        ICLinker *icl1 = IC_assign(tmp2, v1);
        if (v1.kind2 == OP_ADDRESS)
          icl1->code.kind = IC_ASSIGN;
        else
          icl1->code.kind = IC_ADDR; // attention
        INSERT_IC(icl1);             // get addr
        assert(child->child->next->next->type == Exp);
        genExp(child->child->next->next, &tmp3); // get index
        tmp3.kind2 = OP_ADDRESS;
        ICLinker *icl2 = IC_binop(tmp3, tmp3, newConstant(4));
        icl2->code.kind = IC_MUL;
        INSERT_IC(icl2); // get offset
        ICLinker *icl3 = IC_binop(tmp2, tmp2, tmp3);
        icl3->code.kind = IC_ADD;
        INSERT_IC(icl3); // now tmp2 is addr of a[i]
        ICLinker *icl4 = IC_assign(tmp2, tmp1);
        icl4->code.kind = IC_STORE;
        INSERT_IC(icl4);
        if (opp) {
          ICLinker *icl5 = IC_assign(*opp, tmp1);
          icl5->code.kind = IC_ASSIGN;
          INSERT_IC(icl5);
        }
      } else {
        assert(0);
      }
    } // attention for load and store
    if (operation->type == DOT) {
      assert(child->child->type == ID);
      Operand v1 = *getVariable(child->child->dataId);
      Operand tmp1 = newTemp();
      tmp1.kind2 = OP_ADDRESS;
      ICLinker *icl1 = IC_assign(tmp1, v1);
      if (v1.kind2 == OP_ADDRESS)
        icl1->code.kind = IC_ASSIGN;
      else
        icl1->code.kind = IC_ADDR;
      INSERT_IC(icl1);
      u32_t offset =
          sizeCounter(operation->next->dataId, child->child->dataId, INF);
      ICLinker *icl2 = IC_binop(tmp1, tmp1, newConstant(offset));
      icl2->code.kind = IC_ADD;
      INSERT_IC(icl2);
      assert(opp);
      ICLinker *icl3 = IC_assign(*opp, tmp1);
      icl3->code.kind = IC_LOAD;
      INSERT_IC(icl3);
    }
    if (operation->type == LB) {
      Operand tmp1 = newTemp(); // index
      tmp1.kind2 = OP_COM;
      Operand tmp2 = newTemp();
      tmp2.kind2 = OP_ADDRESS;
      node *arr = child->child;
      assert(arr->type == ID);
      Operand v1 = *getVariable(arr->dataId);
      assert(operation->next->type == Exp);
      genExp(operation->next, &tmp1);
      Operand ins1 = newConstant(sizeCounter(arr->dataId, INF, 1));
      ICLinker *icl0 = IC_assign(tmp2, v1);
      if (v1.kind2 == OP_ADDRESS)
        icl0->code.kind = IC_ASSIGN;
      else
        icl0->code.kind = IC_ADDR;
      INSERT_IC(icl0);
      ICLinker *icl1 = IC_binop(tmp1, tmp1, ins1);
      icl1->code.kind = IC_MUL;
      INSERT_IC(icl1);
      ICLinker *icl2 = IC_binop(tmp2, tmp2, tmp1);
      icl2->code.kind = IC_ADD;
      INSERT_IC(icl2);
      assert(opp);
      ICLinker *icl3 = IC_assign(*opp, tmp2);
      icl3->code.kind = IC_LOAD;
      INSERT_IC(icl3);
    }
    if ((operation->type == PLUS) || (operation->type == MINUS) ||
        (operation->type == STAR) || (operation->type == DIV)) {
      Operand tmp1 = newTemp(), tmp2 = newTemp();
      assert(child->type == Exp);
      genExp(child, &tmp1);
      genExp(child->next->next, &tmp2);
      ICLinker *icl1 = IC_binop(*opp, tmp1, tmp2);
      switch (operation->type) {
      case PLUS:
        icl1->code.kind = IC_ADD;
        break;
      case MINUS:
        icl1->code.kind = IC_SUB;
        break;
      case STAR:
        icl1->code.kind = IC_MUL;
        break;
      case DIV:
        icl1->code.kind = IC_DIV;
        break;
      }
      INSERT_IC(icl1);
    }
    break;
  }
  case LP: {
    assert(child->next->type == Exp);
    genExp(child->next, opp);
    break;
  }
  case MINUS: {
    Operand tmp1 = newTemp(), ins1 = newConstant(0);
    assert(child->next->type == Exp);
    genExp(child->next, &tmp1);
    ICLinker *icl1 = IC_binop(tmp1, ins1, tmp1);
    icl1->code.kind = IC_SUB;
    INSERT_IC(icl1);
    assert(opp);
    ICLinker *icl2 = IC_assign(*opp, tmp1);
    icl2->code.kind = IC_ASSIGN;
    INSERT_IC(icl2);
    break;
  }
  default:
    assert(0);
  }
}
// use & to alter op's typeId

static void genDec(node *dec) {
  if (errorFlag)
    return;
  node *varDec = dec->child;
  node *id = varDec->child;
  Operand *op;
  if (id->next) {
    id = id->child;
    if (id->next) {
      errorFlag = 1;
      printf("Cannot translate: Code contains variables of multi-dimensional "
             "array type or parameters of array type");
      return;
    }
    op = getVariable(id->dataId);
    op->kind2 = OP_ARR;
    ICLinker *icl = IC_call(*op, sizeCounter(id->dataId, INF, INF));
    icl->code.kind = IC_DEC;
    INSERT_IC(icl);
  } else {
    op = getVariable(id->dataId);
    if (typeList[op->typeId].kind == BASIC) {
      // printf("dec %s varno %d\n", dataList[id->dataId].id, op->u.varno);
      op->kind2 = OP_COM;
    } else {
      assert(typeList[op->typeId].kind == STRUCTURE);
      op->kind2 = OP_STRUCT;
      ICLinker *icl = IC_call(*op, sizeCounter(id->dataId, INF, INF));
      icl->code.kind = IC_DEC;
      INSERT_IC(icl);
    }
  }
  if (varDec->next == NULL)
    return;
  assert(varDec->next->next->type == Exp);
  genExp(varDec->next->next, op);
}

static void genDefList(node *defList) {
  if (errorFlag)
    return;
  if (defList->dataId == -3)
    return;
  node *def;
  node *decList;
  node *dec;
  while (defList->dataId != -3) {
    def = defList->child;
    assert(def->type == Def);
    decList = def->child->next;
    dec = decList->child;
    while (dec->next) {
      assert(dec->type == Dec);
      genDec(dec);
      decList = dec->next->next;
      dec = decList->child;
    }
    genDec(dec);
    defList = defList->child->next;
  }
}

static void genCond(node *exp, Operand label_true, Operand label_false) {
  if (errorFlag)
    return;
  node *child = exp->child;
  if (child->type == NOT) {
    genCond(exp, label_false, label_true);
    return;
  }
  if (child->next) {
    if (child->next->type == AND) {
      Operand label1 = newLabel();
      genCond(child, label1, label_false);
      ICLinker *icl1 = IC_op(label1);
      icl1->code.kind = IC_LABEL;
      INSERT_IC(icl1);
      genCond(child->next->next, label_true, label_false);
      return;
    }
    if (child->next->type == OR) {
      Operand label2 = newLabel();
      genCond(child, label_true, label2);
      ICLinker *icl2 = IC_op(label2);
      icl2->code.kind = IC_LABEL;
      INSERT_IC(icl2);
      genCond(child->next->next, label_true, label_false);
      return;
    }
    if (child->next->type == RELOP) {
      Operand tmp1 = newTemp(), tmp2 = newTemp();
      assert(child->type == Exp);
      genExp(child, &tmp1);
      genExp(child->next->next, &tmp2);
      ICLinker *icl = IC_binop(label_true, tmp1, tmp2);
      switch (child->next->dataId) {
      case EQ:
        icl->code.kind = IC_EQ;
        break;
      case NE:
        icl->code.kind = IC_NE;
        break;
      case LT:
        icl->code.kind = IC_LT;
        break;
      case LE:
        icl->code.kind = IC_LE;
        break;
      case GT:
        icl->code.kind = IC_GT;
        break;
      case GE:
        icl->code.kind = IC_GE;
        break;
      }
      INSERT_IC(icl);
      ICLinker *icl2 = IC_op(label_false);
      icl2->code.kind = IC_JUMP;
      INSERT_IC(icl2);
      return;
    }
  }
  Operand tmp3 = newTemp();
  Operand ins1 = newConstant(0);
  assert(exp->type == Exp);
  genExp(exp, &tmp3);
  ICLinker *icl3 = IC_binop(label_true, ins1, tmp3);
  icl3->code.kind = IC_NE;
  INSERT_IC(icl3);
  ICLinker *icl4 = IC_op(label_false);
  icl4->code.kind = IC_JUMP;
  INSERT_IC(icl4);
  return;
}

static void genStmt(node *stmt) {
  if (errorFlag)
    return;
  node *child = stmt->child;
  switch (child->type) {
  case Exp: {
    assert(child->type == Exp);
    genExp(child, NULL);
    break;
  }
  case CompSt: {
    genCompSt(child);
    break;
  }
  case RETURN: {
    Operand tmp1 = newTemp();
    assert(child->next->type == Exp);
    genExp(child->next, &tmp1);
    ICLinker *icl = IC_op(tmp1);
    icl->code.kind = IC_RETURN;
    INSERT_IC(icl);
    break;
  }
  case WHILE: {
    node *exp = child->next->next;
    assert(exp->type == Exp);
    Operand label_true = newLabel(), label_false = newLabel(),
            label_judge = newLabel();
    ICLinker *icl1 = IC_op(label_judge);
    icl1->code.kind = IC_LABEL;
    INSERT_IC(icl1);
    genCond(exp, label_true, label_false);
    ICLinker *icl2 = IC_op(label_true);
    icl2->code.kind = IC_LABEL;
    INSERT_IC(icl2);
    genStmt(exp->next->next);
    ICLinker *icl3 = IC_op(label_judge);
    icl3->code.kind = IC_JUMP;
    INSERT_IC(icl3);
    ICLinker *icl4 = IC_op(label_false);
    icl4->code.kind = IC_LABEL;
    INSERT_IC(icl4);
    break;
  }
  case IF: {
    node *exp = child->next->next;
    assert(exp->type == Exp);
    Operand label_true = newLabel(), label_false = newLabel();
    genCond(exp, label_true, label_false);
    ICLinker *icl1 = IC_op(label_true);
    icl1->code.kind = IC_LABEL;
    INSERT_IC(icl1);
    genStmt(exp->next->next);
    if (!(exp->next->next->next)) {
      ICLinker *icl2 = IC_op(label_false);
      icl2->code.kind = IC_LABEL;
      INSERT_IC(icl2);
      break;
    }
    Operand label_pass = newLabel();
    ICLinker *icl3 = IC_op(label_pass);
    icl3->code.kind = IC_JUMP;
    INSERT_IC(icl3);
    ICLinker *icl4 = IC_op(label_false);
    icl4->code.kind = IC_LABEL;
    INSERT_IC(icl4);
    genStmt(exp->next->next->next->next);
    ICLinker *icl5 = IC_op(label_pass);
    icl5->code.kind = IC_LABEL;
    INSERT_IC(icl5);
    break;
  }
  default:
    assert(0);
  }
}

static void genStmtList(node *stmtList) {
  if (errorFlag)
    return;
  if (stmtList->dataId == -3)
    return;
  node *stmt;
  while (stmtList->dataId != -3) {
    stmt = stmtList->child;
    assert(stmt->type == Stmt);
    genStmt(stmt);
    stmtList = stmt->next;
  }
}

static void genCompSt(node *compSt) {
  if (errorFlag)
    return;
  node *defList = compSt->child->next;
  assert(defList->type == DefList);
  genDefList(defList);
  genStmtList(defList->next);
}

static void genExtDef(node *extDef) {
  if (errorFlag)
    return;
  if (extDef->child->next->type != FunDec)
    return;
  node *funDec = extDef->child->next;
  assert(funDec->type == FunDec);
  genFunDec(funDec);
  genCompSt(funDec->next);
}

void genIC() {
  if (errorFlag)
    return;
  initIr();
  if (root == NULL)
    return;
  node *extDefList = root->child;
  node *extDef;
  while (extDefList->dataId != -3) {
    assert(extDefList->type == ExtDefList);
    extDef = extDefList->child;
    extDefList = extDefList->child->next;
    genExtDef(extDef);
  }
}
