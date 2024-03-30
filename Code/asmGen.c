#include "asmGen.h"
#include "ir.h"
#include "symbolTable.h"
#include "syntaxParsingTree.h"
#include <assert.h>
#include <stdio.h>

/*
朴素寄存器分配，对于每个语句，在执行前其操作数的值都在内存中，执行后将result存入内存
对每个临时变量也是如此
所有param都在栈中
为方便操作，只使用寄存器变量，不使用立即数
按param出现的顺序，依次给其分配fp+0 fp+4……
ra is stored at new fp-4
fp is stored at new fp-8
*/

const char *FileHeader = ".data\n\
_prompt: .asciiz \"Enter an integer:\"\n\
_ret: .asciiz \"\\n\"\n\
.globl main\n\
.text\n\
read:\n\
  li $v0, 4\n\
  la $a0, _prompt\n\
  syscall\n\
  li $v0, 5\n\
  syscall\n\
  jr $ra\n\
\n\
write:\n\
  li $v0, 1\n\
  syscall\n\
  li $v0, 4\n\
  la $a0, _ret\n\
  syscall\n\
  move $v0, $0\n\
  jr $ra\n";

i32_t vOffsets[DATA_SIZE];   // index is Var's NO
i32_t tOffsets[DATA_SIZE];   // index is tem's NO
i32_t frameSizes[DATA_SIZE]; // index is function's dataId

void initAsm() {
  fprintf(writeFile, "%s", FileHeader);
  for (i32_t i = 0; i < DATA_SIZE; ++i) {
    vOffsets[i] = -1;
    tOffsets[i] = -1;
  }
}

i32_t *getOffsetP(Operand op) {
  if (op.kind1 == OP_VARIABLE) {
    return &(vOffsets[op.u.varno]);
  } else if (op.kind1 == OP_TEMP) {
    return &(tOffsets[op.u.tempno]);
  } else {
    assert(op.kind1 == OP_CONSTANT);
    return NULL;
  }
}

#define GET_LOCAL_OFFSET(op)                                                   \
  {                                                                            \
    i32_t *offsetP = getOffsetP(op);                                           \
    if (offsetP && *offsetP == -1) {                                           \
      localOffset -= 4;                                                        \
      frameSize += 4;                                                          \
      *offsetP = localOffset;                                                  \
    }                                                                          \
  }

void getFrameSize() {
  i32_t functionId = -1;
  i32_t frameSize;
  i32_t paramOffset;
  i32_t localOffset;
  for (ICLinker *icl = ICHeader->next; icl != ICHeader; icl = icl->next) {
    IC code = icl->code;
    switch (code.kind) {
    case IC_FUNCTION: {
      if (functionId != -1) {
        frameSizes[functionId] = frameSize;
      }
      functionId = code.u.nameId;
      frameSize = 8;
      paramOffset = 0;
      localOffset = -8;
    } break;
    case IC_PARAM: {
      Operand op = code.u.op;
      assert(op.kind1 == OP_VARIABLE);
      vOffsets[op.u.varno] = paramOffset;
      paramOffset += 4;
    } break; // 注意，未加framesize
    case IC_DEC: {
      Operand op = code.u.call.op;
      i32_t value = code.u.call.value;
      assert(op.kind1 == OP_VARIABLE);
      if (vOffsets[op.u.varno] == -1 && op.kind1 != OP_CONSTANT) {
        localOffset -= value;
        frameSize += value;
        vOffsets[op.u.varno] = localOffset;
      }
    } break;
    case IC_ASSIGN:
    case IC_ADDR:
    case IC_LOAD:
    case IC_STORE: {
      Operand left = code.u.assign.left, right = code.u.assign.right;
      GET_LOCAL_OFFSET(left);
      GET_LOCAL_OFFSET(right);
    } break;
    case IC_ADD:
    case IC_SUB:
    case IC_MUL:
    case IC_DIV: {
      Operand op1 = code.u.binop.op1, op2 = code.u.binop.op2,
              result = code.u.binop.result;
      GET_LOCAL_OFFSET(op1);
      GET_LOCAL_OFFSET(op2);
      GET_LOCAL_OFFSET(result);
    } break;
    case IC_EQ:
    case IC_NE:
    case IC_LT:
    case IC_LE:
    case IC_GT:
    case IC_GE: {
      Operand op1 = code.u.binop.op1, op2 = code.u.binop.op2;
      GET_LOCAL_OFFSET(op1);
      GET_LOCAL_OFFSET(op2);
    } break;
    case IC_CALL: {
      Operand op = code.u.call.op;
      GET_LOCAL_OFFSET(op);
    } break;
    case IC_ARG:
    case IC_READ:
    case IC_WRITE:
    case IC_RETURN: {
      Operand op = code.u.op;
      GET_LOCAL_OFFSET(op);
    } break;
    }
  }
  frameSizes[functionId] = frameSize;
}

i32_t regUsed = 0;
#define ALLOC_REG_LOAD(op)                                                     \
  {                                                                            \
    if (op.kind1 == OP_CONSTANT) {                                             \
      fprintf(writeFile, "  li $t%d, %d\n", regUsed++, op.u.instant);          \
    } else {                                                                   \
      fprintf(writeFile, "  lw $t%d, %d($fp)\n", regUsed++, *getOffsetP(op));  \
    }                                                                          \
  }

char ASM_NAME[20][7] = {"add", "sub", "mul", "???", "beq",
                        "bne", "blt", "ble", "bgt", "bge"};

void genAsm() {
  initAsm();
  getFrameSize();
  i32_t functionId;
  i32_t frameSize;
  for (ICLinker *icl = ICHeader->next; icl != ICHeader; icl = icl->next) {
    IC code = icl->code;
    switch (code.kind) {
    case IC_FUNCTION: {
      functionId = code.u.nameId;
      frameSize = frameSizes[functionId];
      fprintf(writeFile, "\n%s:\n\
  sw $ra, -4($sp)\n\
  sw $fp, -8($sp)\n\
  move $fp, $sp\n\
  addi $sp $sp -%d\n",
              dataList[functionId].id, frameSize);
    } break;
    case IC_RETURN: {
      Operand op = code.u.op;
      if (op.kind1 == OP_CONSTANT) {
        fprintf(writeFile, "  li $v0 %d\n", op.u.instant);
      } else {
        fprintf(writeFile, "  lw $v0, %d($fp)\n", *getOffsetP(op));
      }
      fprintf(writeFile, "  lw $ra, -4($fp)\n\
  move $sp, $fp\n\
  lw $fp, -8($fp)\n\
  jr $ra\n");
    } break;
    case IC_LABEL: {
      fprintf(writeFile, "label%d:\n", code.u.op.u.labelno);
    } break;
    case IC_ASSIGN: {
      Operand left = code.u.assign.left, right = code.u.assign.right;
      ALLOC_REG_LOAD(right)
      fprintf(writeFile, "  sw $t0, %d($fp)\n", *getOffsetP(left));
      --regUsed;
    } break;
    case IC_ADD:
    case IC_SUB:
    case IC_MUL: {
      Operand op1 = code.u.binop.op1, op2 = code.u.binop.op2,
              result = code.u.binop.result;
      ALLOC_REG_LOAD(op1)
      ALLOC_REG_LOAD(op2)
      fprintf(writeFile, "  %s $t2, $t0, $t1\n", ASM_NAME[code.kind]);
      fprintf(writeFile, "  sw $t2, %d($fp)\n", *getOffsetP(result));
      --regUsed;
      --regUsed;
    } break;
    case IC_DIV: {
      Operand op1 = code.u.binop.op1, op2 = code.u.binop.op2,
              result = code.u.binop.result;
      ALLOC_REG_LOAD(op1)
      ALLOC_REG_LOAD(op2)
      fprintf(writeFile, "  div $t0, $t1\n");
      fprintf(writeFile, "  mflo $t2\n");
      fprintf(writeFile, "  sw $t2, %d($fp)\n", *getOffsetP(result));
      --regUsed;
      --regUsed;
    } break;
    case IC_ADDR: {
      Operand left = code.u.assign.left, right = code.u.assign.right;
      fprintf(writeFile, "  la $t0, %d($fp)\n", *getOffsetP(right));
      fprintf(writeFile, "  sw $t0, %d($fp)\n", *getOffsetP(left));
    } break;
    case IC_LOAD: {
      Operand left = code.u.assign.left, right = code.u.assign.right;
      ALLOC_REG_LOAD(right)
      fprintf(writeFile, "  lw $t0, 0($t0)\n");
      fprintf(writeFile, "  sw $t0, %d($fp)\n", *getOffsetP(left));
      --regUsed;
    } break;
    case IC_STORE: {
      Operand left = code.u.assign.left, right = code.u.assign.right;
      ALLOC_REG_LOAD(right)
      ALLOC_REG_LOAD(left)
      fprintf(writeFile, "  sw $t0, 0($t1)\n");
      --regUsed;
      --regUsed;
    } break;
    case IC_JUMP: {
      fprintf(writeFile, "  j label%d\n", code.u.op.u.labelno);
    } break;
    case IC_EQ:
    case IC_NE:
    case IC_LT:
    case IC_LE:
    case IC_GT:
    case IC_GE: {
      Operand op1 = code.u.binop.op1, op2 = code.u.binop.op2,
              label = code.u.binop.result;
      ALLOC_REG_LOAD(op1)
      ALLOC_REG_LOAD(op2)
      fprintf(writeFile, "  %s $t0, $t1, label%d\n", ASM_NAME[code.kind],
              label.u.labelno);
      --regUsed;
      --regUsed;
    } break;
    case IC_ARG: {
      Operand op = code.u.op;
      fprintf(writeFile, "  addi $sp, $sp, -4\n"); // realloc space for arg
                                                   // for continuous spaces
      ALLOC_REG_LOAD(op)
      fprintf(writeFile, "  sw $t0, 0($sp)\n");
      --regUsed;
    } break;
    case IC_CALL: {
      Operand op = code.u.call.op;
      i32_t nameId = code.u.call.value;
      fprintf(writeFile, "  jal %s\n", dataList[nameId].id);
      fprintf(writeFile, "  addi $sp, $sp, %d\n",
              4 * (findSymbol(nameId)->data.function->parameterNum));
      fprintf(writeFile, "  sw $v0, %d($fp)\n", *getOffsetP(op));
    } break;
    case IC_READ: {
      Operand op = code.u.op;
      fprintf(writeFile, "  jal read\n");
      fprintf(writeFile, "  sw $v0, %d($fp)\n", *getOffsetP(op));
    } break;
    case IC_WRITE: {
      Operand op = code.u.op;
      if (op.kind1 == OP_CONSTANT) {
        fprintf(writeFile, "  li $a0 %d\n", op.u.instant);
      } else {
        fprintf(writeFile, "  lw $a0, %d($fp)\n", *getOffsetP(op));
      }
      fprintf(writeFile, "  jal write\n");
    } break;
    }
    fprintf(writeFile, "# ");
    _writeIC(icl);
  }
}
