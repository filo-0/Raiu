#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "raiu/assert.h"
#include "raiu/types.h"
#include "metadata.h"
#include "raiu/opcodes.h"
#include "rvm.h"


#define PC_OFFSET 0
#define FP_OFFSET 2
#define MT_OFFSET 4
#define AWC_AND_RWC_OFFSET 6
#define LOCALS_OFFSET 7

static inline u8 iNextU8(Byte **pc)  
{ 
    u8 v = (*pc)->UInt; 
    *pc += 1; 
    return v; 
}
static inline u8 iNextU16(Byte **pc) 
{ 
    u16 v = ((*pc + 0)->UInt << 0)| 
            ((*pc + 1)->UInt << 8); 
    *pc += 2; 
    return v; 
}
static inline i8 iNextI8(Byte **pc)  
{ 
    i8 v = (*pc)->UInt; 
    *pc += 1; 
    return v; 
}
static inline i8 iNextI16(Byte **pc) 
{ 
    i16 v = ((*pc + 0)->UInt << 0)| 
            ((*pc + 1)->UInt << 8); 
    *pc += 2; 
    return v; 
}

#pragma region Push

#define LOCAL_PUSH_BYTE(sp, fp, l, b) do { \
    sp->UInt = (u32) (fp + LOCALS_OFFSET + l)->Byte[b].UInt; \
    sp += 1; \
} while(0)
#define LOCAL_PUSH_HWORD(sp, fp, l, h) do { \
    sp->UInt = (u32) (fp + LOCALS_OFFSET + l)->HWord[h].UInt; \
    sp += 1; \
} while(0)
#define LOCAL_PUSH_WORD(sp, fp, l) do { \
    *sp = *(fp + LOCALS_OFFSET + l); \
    sp += 1; \
} while(0)
#define LOCAL_PUSH_DWORD(sp, fp, l) do { \
    *(DWord*)sp = *(DWord*)(fp + LOCALS_OFFSET + l); \
    sp += 2; \
} while(0)
#define LOCAL_PUSH_WORDS(sp, fp, l, n) do { \
    memcpy(sp, fp + LOCALS_OFFSET + l, n); \
    sp += n; \
} while(0)

#define VAL_PUSH_WORD(sp, w) do { \
    *sp = (w); \
    sp += 1; \
} while(0)
#define VAL_PUSH_DWORD(sp, d) do { \
    *(DWord*)sp = (d); \
    sp += 2; \
} while(0)

#pragma endregion
#pragma region Pop

#define POP_BYTE(sp, fp, l, b) do { \
    (fp + LOCALS_OFFSET + l)->UInt = (u32) (sp - 1)->Byte[b].UInt; \
    sp -= 1; \
} while(0)
#define POP_HWORD(sp, fp, l, h) do { \
    (fp + LOCALS_OFFSET + l)->UInt = (u32) (sp - 1)->HWord[h].UInt; \
    sp -= 1; \
} while(0)
#define POP_WORD(sp, fp, l) do { \
    *(fp + LOCALS_OFFSET + l) = *(sp - 1); \
    sp -= 1; \
} while(0)
#define POP_DWORD(sp, fp, l) do { \
    *(DWord*)(fp + LOCALS_OFFSET + l) = *(DWord*)(sp - 2); \
    sp -= 2; \
} while(0)
#define POP_WORDS(sp, fp, l, n) do { \
    memcpy(fp + LOCALS_OFFSET + l, sp - n, n); \
    sp -= n; \
} while(0)

#pragma endregion
#pragma region Operators
#define BINARY_OPERATION_WORD(sp, type, operator) do \
{ \
    Word a, b, res; \
    a = *(sp - 2); \
    b = *(sp - 1); \
    res.type = a.type operator b.type; \
    *(sp - 2) = res; \
    sp -= 1; \
} while (0)
#define BINARY_OPERATION_DWORD(sp, type, operator) do \
{ \
    DWord a, b, res; \
    a = *(DWord*)(sp - 4); \
    b = *(DWord*)(sp - 2); \
    res.type = a.type operator b.type; \
    *(DWord*)(sp - 4) = res; \
    sp -= 2; \
} while (0)
#define UNARY_OPERATION_WORD(sp, type, operator) do \
{ \
    Word a, res; \
    a = *(sp - 1); \
    res.type = operator a.type; \
    *(sp - 1) = res; \
} while (0)
#define UNARY_OPERATION_DWORD(sp, type, operator) do \
{ \
    DWord a, res; \
    a = *(DWord*)(sp - 2); \
    res.type = operator a.type; \
    *(DWord*)(sp - 2) = res; \
} while (0)
#define LOCAL_BINARY_OPERATION_WORD(fp, type, operator, l, v) do \
{ \
    Word loc = (fp + LOCALS_OFFSET)[l]; \
    loc.type = loc.type operator v; \
    (fp + LOCALS_OFFSET)[l] = loc; \
} while(0)
#define CAST_WORD_WORD(sp, typeA, typeB, cast) do \
{ \
    Word from, to; \
    from = *(sp - 1); \
    to.typeB = (cast) from.typeA; \
    *(sp - 1) = to; \
} while(0)
#define CAST_WORD_DWORD(sp, typeA, typeB, cast) do \
{ \
    Word from; \
    DWord to; \
    from = *(sp - 1); \
    to.typeB = (cast) from.typeA; \
    *(DWord*)(sp - 1) = to; \
    sp += 1; \
} while(0)
#define CAST_DWORD_DWORD(sp, typeA, typeB, cast) do \
{ \
    DWord from, to; \
    from = *(DWord*)(sp - 2); \
    to.typeB = (cast) from.typeA; \
    *(DWord*)(sp - 2) = to; \
} while(0)
#define CAST_DWORD_WORD(sp, typeA, typeB, cast) do \
{ \
    DWord from; \
    Word to; \
    from = *(DWord*)(sp - 2); \
    to.typeB = (cast) from.typeA; \
    *(sp - 2) = to; \
    sp -= 1; \
} while(0)

#define LOCAL_BINARY_OPERATION_DWORD(fp, type, operator, l, v) do \
{ \
    DWord loc; \
    loc = *(DWord*)(fp + LOCALS_OFFSET + l); \
    loc.type = loc.type operator v; \
    *(DWord*)(fp + LOCALS_OFFSET + l) = loc; \
} while(0)
#pragma endregion
#pragma region Load & Store
#define LOAD_BYTE(sp, offset, b) do { \
    DWord ref; \
    ref = *(DWord*)(sp - 2); \
    (sp - 2)->Byte[0] = (ref.WordPtr + offset)->Byte[b]; \
    sp -= 1; \
} while(0)
#define LOAD_HWORD(sp, offset, h) do { \
    DWord ref; \
    ref = *(DWord*)(sp - 2); \
    (sp - 2)->HWord[0] = (ref.WordPtr + offset)->HWord[h]; \
    sp -= 1; \
} while(0)
#define LOAD_WORD(sp, offset) do { \
    DWord ref; \
    ref = *(DWord*)(sp - 2); \
    *(sp - 2) = *(ref.WordPtr + offset); \
    sp -= 1; \
} while(0)
#define LOAD_DWORD(sp, offset) do { \
    DWord ref; \
    ref = *(DWord*)(sp - 2); \
    *(DWord*)(sp - 2) = *(DWord*)(ref.WordPtr + offset); \
} while(0)
#define STORE_BYTE(sp, offset, b) do { \
    Word val; \
    DWord ref; \
    ref = *(DWord*)(sp - 3); \
    val         = *(sp - 1); \
    (ref.WordPtr + offset)->Byte[b] = val.Byte[0]; \
    sp -= 3; \
} while(0)
#define STORE_HWORD(sp, offset, h) do { \
    Word val; \
    DWord ref; \
    ref = *(DWord*)(sp - 3); \
    val         = *(sp - 1); \
    (ref.WordPtr + offset)->HWord[h] = val.HWord[0]; \
    sp -= 3; \
} while(0)
#define STORE_WORD(sp, offset) do { \
    Word val; \
    DWord ref; \
    ref = *(DWord*)(sp - 3); \
    val         = *(sp - 1); \
    *(ref.WordPtr + offset) = val; \
    sp -= 3; \
} while(0)
#define STORE_DWORD(sp, offset) do { \
    DWord val; \
    DWord ref; \
    ref = *(DWord*)(sp - 4); \
    val = *(DWord*)(sp - 2); \
    *(DWord*)(ref.WordPtr + offset) = val; \
} while(0)
#define LOAD_BUFF_VAL_WORD(sp, typePtr) do { \
    Word i; \
    DWord ref; \
    ref = *(DWord*)(sp - 3); \
    i   = *(sp - 1); \
    (sp - 3)->UInt = ref.typePtr[i.UInt].UInt; \
    sp -= 2; \
} while(0)
#define LOAD_BUFF_VAL_DWORD(sp) do { \
    Word i; \
    DWord ref; \
    ref = *(DWord*)(sp - 3); \
    i           = *(sp - 1); \
    *(DWord*)(sp - 3) = ref.DWordPtr[i.UInt]; \
    sp -= 1; \
} while(0)
#define LOAD_BUFF_REF(sp, offset) do { \
    Word i; \
    DWord ref; \
    ref = *(DWord*)(sp - 3); \
    i           = *(sp - 1); \
    ref.BytePtr += offset * i.UInt; \
    *(DWord*)(sp - 3) = ref; \
    sp -= 1; \
} while(0)
#define STORE_BUFF_VAL_WORD(sp, typePtr, cast) do {\
    Word i; \
    DWord ref; \
    ref = *(DWord*)(sp - 4); \
    i   = *(sp - 2); \
    ref.typePtr[i.UInt].UInt = (cast) (sp - 1)->UInt; \
    sp -= 4; \
} while(0)
#define STORE_BUFF_VAL_DWORD(sp) do { \
    Word i; \
    DWord ref; \
    ref = *(DWord*)(sp - 5); \
    i   = *(sp - 3); \
    ref.DWordPtr[i.UInt] = *(DWord*)(sp - 2); \
    sp -= 5; \
} while(0)
#pragma endregion
// Continue prefetches the instruction and jumps to the start of the loop
#define CONTINUE do \
{ \
    opcode = pc->UInt; \
    goto LOOP; \
} while (0)

i32 Execute(ProgramContext *context)
{
    Byte            *pc;
    Word            *sp;
    Word            *fp;
    ModuleTable     *mt;
    Word            *wordPool;
    DWord           *dwordPool;
    ch8            **stringPool;
    Function       **functionPool;
    u8               opcode;
    
    pc = (Byte*)(context->EntryPoint + 1);
    sp = context->StackBottom + context->EntryPoint->LWC;
    fp = context->StackBottom;
    mt = context->EntryPoint->MT;
    wordPool     = context->EntryPoint->MT->WordPool;
    dwordPool    = context->EntryPoint->MT->DWordPool;
    stringPool   = context->EntryPoint->MT->StringPool;
    functionPool = context->EntryPoint->MT->FunctionPool;
    opcode       = pc->UInt;
    
#pragma region InstructionTable
    __attribute__((aligned(128)))
    static const void* const InstructionPointers[256] = 
    {
        &&BREAKPOINT,
        &&PUSH_BYTE,
        &&PUSH_BYTE,
        &&PUSH_BYTE,
        &&PUSH_BYTE,
        &&PUSH_HWORD,
        &&PUSH_HWORD,
        &&PUSH_WORD,
        &&PUSH_WORD_L,
        &&PUSH_WORD_L,
        &&PUSH_WORD_L,
        &&PUSH_WORD_L,
        &&PUSH_DWORD,
        &&PUSH_DWORD_L,
        &&PUSH_DWORD_L,
        &&PUSH_DWORD_L,
        &&PUSH_DWORD_L,
        &&PUSH_WORDS,
        
        &&PUSH_I32_V,
        &&PUSH_I32_V,
        &&PUSH_I32_V,
        &&PUSH_I64_V,
        &&PUSH_I64_V,
        &&PUSH_I64_V,
        &&PUSH_F32_1,
        &&PUSH_F32_2,
        &&PUSH_F64_1,
        &&PUSH_F64_2,
        &&PUSH_I32,
        &&PUSH_I64,
    
        &&PUSH_CONST_WORD,
        &&PUSH_CONST_WORD_W,
        &&PUSH_CONST_DWORD,
        &&PUSH_CONST_DWORD_W,
        &&PUSH_CONST_STR,
        &&PUSH_CONST_STR_W,

        &&POP_BYTE,
        &&POP_BYTE,
        &&POP_BYTE,
        &&POP_BYTE,
        &&POP_HWORD,
        &&POP_HWORD,
        &&POP_WORD,
        &&POP_WORD_L,
        &&POP_WORD_L,
        &&POP_WORD_L,
        &&POP_WORD_L,
        &&POP_DWORD,
        &&POP_DWORD_L,
        &&POP_DWORD_L,
        &&POP_DWORD_L,
        &&POP_DWORD_L,
        &&POP_WORDS,

        &&ADD_I32,
        &&ADD_I64,
        &&ADD_F32,
        &&ADD_F64,
        &&INC_I32,
        &&INC_I64,
        &&INC_F32,
        &&INC_F64,

        &&SUB_I32,
        &&SUB_I64,
        &&SUB_F32,
        &&SUB_F64,
        &&DEC_I32,
        &&DEC_I64,
        &&DEC_F32,
        &&DEC_F64,

        &&MUL_I32,
        &&MUL_I64,
        &&MUL_U32,
        &&MUL_U64,
        &&MUL_F32,
        &&MUL_F64,

        &&DIV_I32,
        &&DIV_I64,
        &&DIV_U32,
        &&DIV_U64,
        &&DIV_F32,
        &&DIV_F64,

        &&REM_I32,
        &&REM_I64,
        &&REM_U32,
        &&REM_U64,

        &&NEG_I32,
        &&NEG_I64,
        &&NEG_F32,
        &&NEG_F64,

        &&NOT_WORD,
        &&NOT_DWORD,
        &&AND_WORD,
        &&AND_DWORD,
        &&OR_WORD,
        &&OR_DWORD,
        &&XOR_WORD,
        &&XOR_DWORD,
        &&SHL_WORD,
        &&SHL_DWORD,
        &&SHR_I32,
        &&SHR_I64,
        &&SHR_U32,
        &&SHR_U64,

        &&I32_TO_I8,
        &&I32_TO_I16,
        &&I32_TO_I64,
        &&I32_TO_F32,
        &&I32_TO_F64,
        &&I64_TO_I32,
        &&I64_TO_F32,
        &&I64_TO_F64,
        &&F32_TO_I32,
        &&F32_TO_I64,
        &&F32_TO_F64,
        &&F64_TO_I32,
        &&F64_TO_I64,
        &&F64_TO_F32,

        &&CMP_WORD_EQ,
        &&CMP_DWORD_EQ,
        &&CMP_WORD_NE,
        &&CMP_DWORD_NE,
        &&CMP_I32_GT,
        &&CMP_I64_GT,
        &&CMP_U32_GT,
        &&CMP_U64_GT,
        &&CMP_F32_GT,
        &&CMP_F64_GT,
        &&CMP_I32_LT,
        &&CMP_I64_LT,
        &&CMP_U32_LT,
        &&CMP_U64_LT,
        &&CMP_F32_LT,
        &&CMP_F64_LT,
        &&CMP_I32_GE,
        &&CMP_I64_GE,
        &&CMP_U32_GE,
        &&CMP_U64_GE,
        &&CMP_F32_GE,
        &&CMP_F64_GE,
        &&CMP_I32_LE,
        &&CMP_I64_LE,
        &&CMP_U32_LE,
        &&CMP_U64_LE,
        &&CMP_F32_LE,
        &&CMP_F64_LE,
        &&CMP_NOT,

        &&DUP_WORD,
        &&DUP_DWORD,
        &&DUP_WORD_X1,
        &&DUP_DWORD_X1,
        &&DUP_WORD_X2,
        &&DUP_DWORD_X2,
        &&SWAP_WORD,
        &&SWAP_DWORD,

        &&LOAD_BYTE,
        &&LOAD_BYTE,
        &&LOAD_BYTE,
        &&LOAD_BYTE,
        &&LOAD_HWORD,
        &&LOAD_HWORD,
        &&LOAD_WORD,
        &&LOAD_DWORD,
        &&LOAD_WORDS,

        &&STORE_BYTE,
        &&STORE_BYTE,
        &&STORE_BYTE,
        &&STORE_BYTE,
        &&STORE_HWORD,
        &&STORE_HWORD,
        &&STORE_WORD,
        &&STORE_DWORD,
        &&STORE_WORDS,

        &&LOAD_OFST_BYTE,
        &&LOAD_OFST_BYTE,
        &&LOAD_OFST_BYTE,
        &&LOAD_OFST_BYTE,
        &&LOAD_OFST_HWORD,
        &&LOAD_OFST_HWORD,
        &&LOAD_OFST_WORD,
        &&LOAD_OFST_DWORD,
        &&LOAD_OFST_WORDS,

        &&STORE_OFST_BYTE,
        &&STORE_OFST_BYTE,
        &&STORE_OFST_BYTE,
        &&STORE_OFST_BYTE,
        &&STORE_OFST_HWORD,
        &&STORE_OFST_HWORD,
        &&STORE_OFST_WORD,
        &&STORE_OFST_DWORD,
        &&STORE_OFST_WORDS,

        &&LOAD_BUFF_BYTE_VAL,
        &&LOAD_BUFF_HWORD_VAL,
        &&LOAD_BUFF_WORD_VAL,
        &&LOAD_BUFF_DWORD_VAL,
        &&LOAD_BUFF_WORDS_VAL,

        &&LOAD_BUFF_BYTE_REF,
        &&LOAD_BUFF_HWORD_REF,
        &&LOAD_BUFF_WORD_REF,
        &&LOAD_BUFF_DWORD_REF,
        &&LOAD_BUFF_WORDS_REF,

        &&STORE_BUFF_BYTE,
        &&STORE_BUFF_HWORD,
        &&STORE_BUFF_WORD,
        &&STORE_BUFF_DWORD,
        &&STORE_BUFF_WORDS,

        &&ALLOC,
        &&FREE,

        &&JMP,
        &&JMP_IF,
        &&CALL,
        &&SYSCALL,
        &&RET,
        &&NOT_IMPLEMENTED,
        &&NOT_IMPLEMENTED,
        &&NOT_IMPLEMENTED,
        &&NOT_IMPLEMENTED,
        &&NOT_IMPLEMENTED,
        &&NOT_IMPLEMENTED,
        &&NOT_IMPLEMENTED,
        &&NOT_IMPLEMENTED,
        &&NOT_IMPLEMENTED,
        &&NOT_IMPLEMENTED,
        &&NOT_IMPLEMENTED,
        &&NOT_IMPLEMENTED,
        &&NOT_IMPLEMENTED,
        &&NOT_IMPLEMENTED,
        &&NOT_IMPLEMENTED,
        &&NOT_IMPLEMENTED,
        &&NOT_IMPLEMENTED,
        &&NOT_IMPLEMENTED,
        &&NOT_IMPLEMENTED,
        &&NOT_IMPLEMENTED,
        &&NOT_IMPLEMENTED,
        &&NOT_IMPLEMENTED,
        &&NOT_IMPLEMENTED,
        &&NOT_IMPLEMENTED,
        &&NOT_IMPLEMENTED,
        &&NOT_IMPLEMENTED,
        &&NOT_IMPLEMENTED,
        &&NOT_IMPLEMENTED,
        &&NOT_IMPLEMENTED,
        &&NOT_IMPLEMENTED,
        &&NOT_IMPLEMENTED,
        &&NOT_IMPLEMENTED,
        &&NOT_IMPLEMENTED,
        &&NOT_IMPLEMENTED,
        &&NOT_IMPLEMENTED,
        &&NOT_IMPLEMENTED,
        &&NOT_IMPLEMENTED,
        &&NOT_IMPLEMENTED,
        &&NOT_IMPLEMENTED,
        &&NOT_IMPLEMENTED,
        &&NOT_IMPLEMENTED,
        &&NOT_IMPLEMENTED,
        &&NOT_IMPLEMENTED,
        &&NOT_IMPLEMENTED,
};
#pragma endregion
    const void * const *instructionTable = InstructionPointers;
LOOP:
    pc += 1;
    goto *instructionTable[opcode];
NOT_IMPLEMENTED:
    UNLIKELY(false, "Version not supported!\n");
    exit(EXIT_FAILURE);
BREAKPOINT:
    DEVEL_ASSERT(false, "Breakpoint reached!\n");
    CONTINUE;
#pragma region Push local
PUSH_BYTE:
    {
        sz l = iNextU8(&pc);
        sz b = opcode - OP_PUSH_BYTE_0;
        LOCAL_PUSH_BYTE(sp, fp, l, b);
    }
    CONTINUE;
PUSH_HWORD:
    {
        sz l = iNextU8(&pc);
        sz h = opcode - OP_PUSH_HWORD_0;
        LOCAL_PUSH_HWORD(sp, fp, l, h);
    }
    CONTINUE;
PUSH_WORD:
    {
        sz l = iNextU8(&pc);
        LOCAL_PUSH_WORD(sp, fp, l);
    }
    CONTINUE;
PUSH_WORD_L:
    {
        sz l = opcode - OP_PUSH_WORD_0;
        LOCAL_PUSH_WORD(sp, fp, l);
    }
    CONTINUE;
PUSH_DWORD:
    {
        sz l = iNextU8(&pc);
        LOCAL_PUSH_DWORD(sp, fp, l);
    }
    CONTINUE;
PUSH_DWORD_L:
    {
        sz l = opcode - OP_PUSH_DWORD_0;
        LOCAL_PUSH_DWORD(sp, fp, l);
    }
    CONTINUE;
PUSH_WORDS:
    {
        sz l = iNextU8(&pc);
        sz n = iNextU8(&pc) + 1;
        LOCAL_PUSH_WORDS(sp, fp, l, n);
    }
    CONTINUE;
#pragma endregion
#pragma region Push Immediate
PUSH_I32_V:
    {
        Word w = IntToWord(opcode - OP_PUSH_0_WORD);
        VAL_PUSH_WORD(sp, w);
    }
    CONTINUE;
PUSH_I64_V:
    {
        DWord d = IntToDWord(opcode - OP_PUSH_0_DWORD);
        VAL_PUSH_DWORD(sp, d);
    }
    CONTINUE;
PUSH_F32_1:
    {
        Word w = FloatToWord(1.0f);
        VAL_PUSH_WORD(sp, w);
    }    
    CONTINUE;
PUSH_F32_2:
    {
        Word w = FloatToWord(2.0f);
        VAL_PUSH_WORD(sp, w);
    }    
    CONTINUE;
PUSH_F64_1:
    {
        DWord d = FloatToDWord(1.0);
        VAL_PUSH_DWORD(sp, d);
    }    
    CONTINUE;
PUSH_F64_2:
    {
        DWord d = FloatToDWord(2.0);
        VAL_PUSH_DWORD(sp, d);
    }    
    CONTINUE;
PUSH_I32:
    {
        Word w = IntToWord((i32)iNextI8(&pc));
        VAL_PUSH_WORD(sp, w);
    }
    CONTINUE;
PUSH_I64:
    {
        DWord d = IntToDWord((i64)iNextI8(&pc));
        VAL_PUSH_DWORD(sp, d);
    }
    CONTINUE;
#pragma endregion
#pragma region Push Constant
PUSH_CONST_WORD:
    {
        Word w = *(wordPool + iNextU8(&pc));
        VAL_PUSH_WORD(sp, w);
    }
    CONTINUE;
PUSH_CONST_WORD_W:
    {
        Word w = *(wordPool + iNextU16(&pc));
        VAL_PUSH_WORD(sp, w);
    }    
    CONTINUE;
PUSH_CONST_DWORD:
    {
        DWord d = *(dwordPool + iNextU8(&pc));
        VAL_PUSH_DWORD(sp, d);
    }    
    CONTINUE;
PUSH_CONST_DWORD_W:
    {
        DWord d = *(dwordPool + iNextU16(&pc));
        VAL_PUSH_DWORD(sp, d);
    }    
    CONTINUE;
PUSH_CONST_STR:
    {
        DWord d = RefToDWord(*(stringPool + iNextU8(&pc)));
        VAL_PUSH_DWORD(sp, d);
    }    
    CONTINUE;
PUSH_CONST_STR_W:
    {
        DWord d = RefToDWord(*(stringPool + iNextU16(&pc)));
        VAL_PUSH_DWORD(sp, d);
    }    
    CONTINUE;
#pragma endregion
#pragma region Pop
POP_BYTE:
    {
        sz l = iNextU8(&pc);
        sz b = opcode - OP_POP_BYTE_0;
        POP_BYTE(sp, fp, l, b);
    }
    CONTINUE;
POP_HWORD:
    {
        sz l = iNextU8(&pc);
        sz h = opcode - OP_POP_HWORD_0;
        POP_HWORD(sp, fp, l, h);
    }
    CONTINUE;
POP_WORD:
    {
        sz l = iNextU8(&pc);
        POP_WORD(sp, fp, l);
    }
    CONTINUE;
POP_WORD_L:
    {
        sz l = opcode - OP_POP_WORD_0;
        POP_WORD(sp, fp, l);
    }
    CONTINUE;
POP_DWORD:
    {
        sz l = iNextU8(&pc);
        POP_DWORD(sp, fp, l);
    }
    CONTINUE;
POP_DWORD_L:
    {
        sz l = opcode - OP_POP_DWORD_0;
        POP_DWORD(sp, fp, l);
    }
    CONTINUE;
POP_WORDS:
    {
        sz l = iNextU8(&pc);
        sz n = iNextU8(&pc);
        POP_WORDS(sp, fp, l, n);
    }
    CONTINUE;
#pragma endregion
#pragma region Arithmetic
ADD_I32:
    BINARY_OPERATION_WORD(sp, Int, +);
    CONTINUE;
ADD_I64:
    BINARY_OPERATION_DWORD(sp, Int, +);
    CONTINUE;
ADD_F32:
    BINARY_OPERATION_WORD(sp, Float, +);
    CONTINUE;
ADD_F64:
    BINARY_OPERATION_DWORD(sp, Float, +);
    CONTINUE;
INC_I32:
    {
        sz l = iNextU8(&pc);
        i32 v = (i32) iNextU8(&pc);
        LOCAL_BINARY_OPERATION_WORD(fp, Int, +, l, v);
    }
    CONTINUE;
INC_I64:
    {
        u8 l = iNextU8(&pc);
        i64 v = (i64) iNextU8(&pc);
        LOCAL_BINARY_OPERATION_DWORD(fp, Int, +, l, v);
    }
    CONTINUE;
INC_F32:
    {
        u8 l = iNextU8(&pc);
        f32 v = (f32) iNextU8(&pc);
        LOCAL_BINARY_OPERATION_WORD(fp, Float, +, l, v);
    }
    CONTINUE;
INC_F64:
    {
        u8 l = iNextU8(&pc);
        f64 v = (f64) iNextU8(&pc);
        LOCAL_BINARY_OPERATION_DWORD(fp, Float, +, l, v);
    }
    CONTINUE;
SUB_I32:
    BINARY_OPERATION_WORD(sp, Int, -);
    CONTINUE;
SUB_I64:
    BINARY_OPERATION_DWORD(sp, Int, -);
    CONTINUE;
SUB_F32:
    BINARY_OPERATION_WORD(sp, Float, -);
    CONTINUE;
SUB_F64:
    BINARY_OPERATION_DWORD(sp, Float, -);
    CONTINUE;
DEC_I32:
    {
        u8 l = iNextU8(&pc);
        i32 v = (i32) iNextU8(&pc);
        LOCAL_BINARY_OPERATION_WORD(fp, Int, -, l, v);
    }
    CONTINUE;
DEC_I64:
    {
        u8 l = iNextU8(&pc);
        i64 v = (i64) iNextU8(&pc);
        LOCAL_BINARY_OPERATION_DWORD(fp, Int, -, l, v);
    }
    CONTINUE;
DEC_F32:
    {
        u8 l = iNextU8(&pc);
        f32 v = (f32) iNextU8(&pc);
        LOCAL_BINARY_OPERATION_WORD(fp, Float, -, l, v);
    }
    CONTINUE;
DEC_F64:
    {
        u8 l = iNextU8(&pc);
        f64 v = (f64) iNextU8(&pc);
        LOCAL_BINARY_OPERATION_DWORD(fp, Float, -, l, v);
    }
    CONTINUE;
MUL_I32:
    BINARY_OPERATION_WORD(sp, Int, *);
    CONTINUE;
MUL_I64:
    BINARY_OPERATION_DWORD(sp, Int, *);
    CONTINUE;
MUL_U32:
    BINARY_OPERATION_WORD(sp, UInt, *);
    CONTINUE;
MUL_U64:
    BINARY_OPERATION_DWORD(sp, UInt, *);
    CONTINUE;
MUL_F32:
    BINARY_OPERATION_WORD(sp, Float, *);
    CONTINUE;
MUL_F64:
    BINARY_OPERATION_DWORD(sp, Float,*);
    CONTINUE;
DIV_I32:
    BINARY_OPERATION_WORD(sp, Int, /);
    CONTINUE;
DIV_I64:
    BINARY_OPERATION_DWORD(sp, Int, /);
    CONTINUE;
DIV_U32:
    BINARY_OPERATION_WORD(sp, UInt, /);
    CONTINUE;
DIV_U64:
    BINARY_OPERATION_DWORD(sp, UInt, /);
    CONTINUE;
DIV_F32:
    BINARY_OPERATION_WORD(sp, Float, /);
    CONTINUE;
DIV_F64:
    BINARY_OPERATION_DWORD(sp, Float,/);
    CONTINUE;
REM_I32:
    BINARY_OPERATION_WORD(sp, Int, %);
    CONTINUE;
REM_I64:
    BINARY_OPERATION_DWORD(sp, Int, %);
    CONTINUE;
REM_U32:
    BINARY_OPERATION_WORD(sp, UInt, %);
    CONTINUE;
REM_U64:
    BINARY_OPERATION_DWORD(sp, UInt, %);
    CONTINUE;
NEG_I32:
    UNARY_OPERATION_WORD(sp, Int, -);
    CONTINUE;
NEG_I64:
    UNARY_OPERATION_DWORD(sp, Int, -);
    CONTINUE;
NEG_F32:
    UNARY_OPERATION_WORD(sp, Float, -);
    CONTINUE;
NEG_F64:
    UNARY_OPERATION_DWORD(sp, Float, -);
    CONTINUE;
#pragma endregion
#pragma region Bitwise
NOT_WORD:
    UNARY_OPERATION_WORD(sp, Int, ~);
    CONTINUE;
NOT_DWORD:
    UNARY_OPERATION_DWORD(sp, Int, ~);
    CONTINUE;
AND_WORD:
    BINARY_OPERATION_WORD(sp, Int, &);
    CONTINUE;
AND_DWORD:
    BINARY_OPERATION_DWORD(sp, Int, &);
    CONTINUE;
OR_WORD:
    BINARY_OPERATION_WORD(sp, Int, |);
    CONTINUE;
OR_DWORD:
    BINARY_OPERATION_DWORD(sp, Int, |);
    CONTINUE;
XOR_WORD:
    BINARY_OPERATION_WORD(sp, Int, ^);
    CONTINUE;
XOR_DWORD:
    BINARY_OPERATION_DWORD(sp, Int, ^);
    CONTINUE;
SHL_WORD:
    BINARY_OPERATION_WORD(sp, Int, <<);
    CONTINUE;
SHL_DWORD:
    BINARY_OPERATION_DWORD(sp, Int, <<);
    CONTINUE;
SHR_I32:
    BINARY_OPERATION_WORD(sp, Int, >>);
    CONTINUE;
SHR_I64:
    // TO-DO:FIX
    BINARY_OPERATION_DWORD(sp, Int, >>);
    CONTINUE;
SHR_U32:
    BINARY_OPERATION_WORD(sp, UInt, >>);
    CONTINUE;
SHR_U64:
    // TO-DO:FIX
    BINARY_OPERATION_DWORD(sp, UInt, >>);
    CONTINUE;
#pragma endregion    
#pragma region Cast
I32_TO_I8:
    CAST_WORD_WORD(sp, Int, Int, i8);
    CONTINUE;
I32_TO_I16:
    CAST_WORD_WORD(sp, Int, Int, i16);
    CONTINUE;
I32_TO_I64:
    CAST_WORD_DWORD(sp, Int, Int, i64);
    CONTINUE;
I32_TO_F32:
    CAST_WORD_WORD(sp, Int, Float, f32);
    CONTINUE;
I32_TO_F64:
    CAST_WORD_DWORD(sp, Int, Float, f64);
    CONTINUE;
I64_TO_I32:
    CAST_DWORD_WORD(sp, Int, Int, i32);
    CONTINUE;
I64_TO_F32:
    CAST_DWORD_WORD(sp, Int, Float, f32);
    CONTINUE;
I64_TO_F64:
    CAST_DWORD_DWORD(sp, Int, Float, f64);
    CONTINUE;
F32_TO_I32:
    CAST_WORD_WORD(sp, Float, Int, i32);
    CONTINUE;
F32_TO_I64:
    CAST_WORD_WORD(sp, Float, Int, i64);
    CONTINUE;
F32_TO_F64:
    CAST_WORD_DWORD(sp, Float, Float, f64);
    CONTINUE;
F64_TO_I32:
    CAST_DWORD_WORD(sp, Float, Int, i32);
    CONTINUE;
F64_TO_I64:
    CAST_DWORD_DWORD(sp, Float, Int, i64);
    CONTINUE;
F64_TO_F32:
    CAST_DWORD_WORD(sp, Float, Float, f32);
    CONTINUE;
#pragma endregion
#pragma region Compare
CMP_WORD_EQ:
    BINARY_OPERATION_WORD(sp, Int, ==);
    CONTINUE;
CMP_DWORD_EQ:
    BINARY_OPERATION_DWORD(sp, Int, ==);
    CONTINUE;
CMP_WORD_NE:
    BINARY_OPERATION_WORD(sp, Int, !=);
    CONTINUE;
CMP_DWORD_NE:
    BINARY_OPERATION_DWORD(sp, Int, !=);
    CONTINUE;
CMP_I32_GT:
    BINARY_OPERATION_WORD(sp, Int, >);
    CONTINUE;
CMP_I64_GT:
    BINARY_OPERATION_DWORD(sp, Int, >);
    CONTINUE;
CMP_U32_GT:
    BINARY_OPERATION_WORD(sp, UInt, >);
    CONTINUE;
CMP_U64_GT:
    BINARY_OPERATION_DWORD(sp, UInt, >);
    CONTINUE;
CMP_F32_GT:
    BINARY_OPERATION_WORD(sp, Float, >);
    CONTINUE;
CMP_F64_GT:
    BINARY_OPERATION_DWORD(sp, Float, >);
    CONTINUE;
CMP_I32_LT:
    BINARY_OPERATION_WORD(sp, Int, <);
    CONTINUE;
CMP_I64_LT:
    BINARY_OPERATION_DWORD(sp, Int, <);
    CONTINUE;
CMP_U32_LT:
    BINARY_OPERATION_WORD(sp, UInt, <);
    CONTINUE;
CMP_U64_LT:
    BINARY_OPERATION_DWORD(sp, UInt, <);
    CONTINUE;
CMP_F32_LT:
    BINARY_OPERATION_WORD(sp, Float, <);
    CONTINUE;
CMP_F64_LT:
    BINARY_OPERATION_DWORD(sp, Float, <);
    CONTINUE;
CMP_I32_GE:
    BINARY_OPERATION_WORD(sp, Int, >=);
    CONTINUE;
CMP_I64_GE:
    BINARY_OPERATION_DWORD(sp, Int, >=);
    CONTINUE;
CMP_U32_GE:
    BINARY_OPERATION_WORD(sp, UInt, >=);
    CONTINUE;
CMP_U64_GE:
    BINARY_OPERATION_DWORD(sp, UInt, >=);
    CONTINUE;
CMP_F32_GE:
    BINARY_OPERATION_WORD(sp, Float, >=);
    CONTINUE;
CMP_F64_GE:
    BINARY_OPERATION_DWORD(sp, Float, >=);
    CONTINUE;
CMP_I32_LE:
    BINARY_OPERATION_WORD(sp, Int, <=);
    CONTINUE;
CMP_I64_LE:
    BINARY_OPERATION_DWORD(sp, Int, <=);
    CONTINUE;
CMP_U32_LE:
    BINARY_OPERATION_WORD(sp, UInt, <=);
    CONTINUE;
CMP_U64_LE:
    BINARY_OPERATION_DWORD(sp, UInt, <=);
    CONTINUE;
CMP_F32_LE:
    BINARY_OPERATION_WORD(sp, Float, <=);
    CONTINUE;
CMP_F64_LE:
    BINARY_OPERATION_DWORD(sp, Float, <=);
    CONTINUE;
CMP_NOT:
    UNARY_OPERATION_WORD(sp, Int, !);
    CONTINUE;
#pragma endregion    
#pragma region Stack manipulation
DUP_WORD:
    {
        *(sp - 0) = *(sp - 1);    
        sp += 1;    
    }
    CONTINUE;
DUP_DWORD:
    {
        *(DWord*)(sp + 0) = *(DWord*)(sp - 2);
        sp += 2;
    }  
DUP_WORD_X1:
    {
        Word top = *(sp - 1);
        *(sp - 1) = *(sp - 2);
        *(sp - 2) = top;
        *(sp - 0) = top;    
        sp += 1;    
    }
    CONTINUE;
DUP_DWORD_X1:
    {
        DWord top;
        Word bottom;
        bottom      = *(sp - 3);
        top = *(DWord*)(sp - 2);
        *(DWord*)(sp - 3) = top;
        *(sp - 1) = bottom;
        *(DWord*)(sp + 0) = top;
        sp += 2;
    }  
    CONTINUE;
DUP_WORD_X2:
    {
        Word top  = *(sp - 1);
        *(DWord*)(sp - 2) = *(DWord*)(sp - 3);
        *(sp - 0) = top;   
        *(sp - 3) = top; 
        sp += 1;    
    }
    CONTINUE;
DUP_DWORD_X2:
    {
        DWord top, bottom;
        bottom = *(DWord*)(sp - 4);
        top    = *(DWord*)(sp - 2);
        *(DWord*)(sp - 4) = top;
        *(DWord*)(sp - 2) = bottom;
        *(DWord*)(sp + 0) = top;
        sp += 2;
    }  
    CONTINUE;
SWAP_WORD:
    {
        Word top = *(sp - 1);
        *(sp - 1) = *(sp - 2);
        *(sp - 2) = top;
    }
    CONTINUE;
SWAP_DWORD:
    {
        DWord top = *(DWord*)(sp - 2);
        *(DWord*)(sp - 2) = *(DWord*)(sp - 4);
        *(DWord*)(sp - 4) = top;
    }
    CONTINUE;
#pragma endregion   
#pragma region Standard Load & Store
LOAD_BYTE:
    {
        sz b = opcode - OP_LOAD_BYTE_0;
        LOAD_BYTE(sp, 0, b);
    }
    CONTINUE;
LOAD_HWORD:
    {
        sz h = opcode - OP_LOAD_HWORD_0;
        LOAD_HWORD(sp, 0, h);
    }
    CONTINUE;
LOAD_WORD:
    LOAD_WORD(sp, 0);
    CONTINUE;
LOAD_DWORD:
    LOAD_DWORD(sp, 0);
    CONTINUE;
LOAD_WORDS:
    {
        sz n = iNextU8(&pc) + 1;
        DWord ref;
        ref.Word[0] = *(sp - 2);
        ref.Word[1] = *(sp - 1);
        for (sz i = 0; i < n; i++)
            *(sp - 2 + i) = ref.WordPtr[i];
        sp += n;
        sp -= 2;
    }
STORE_BYTE:
    {
        sz b = opcode - OP_STORE_BYTE_0;
        STORE_BYTE(sp, 0, b);
    }
    CONTINUE;
STORE_HWORD:
    {
        sz h = opcode - OP_STORE_HWORD_0;
        STORE_HWORD(sp, 0, h);
    }
    CONTINUE;
STORE_WORD:
    STORE_WORD(sp, 0);
    CONTINUE;
STORE_DWORD:
    STORE_DWORD(sp, 0);
    CONTINUE;
STORE_WORDS:
    {
        sz n = iNextU8(&pc) + 1;
        DWord ref;
        ref = *(DWord*)(sp - 2 - n);
        for (sz i = 0; i < n; i++)
            *(sp - 2 - n + i) = ref.WordPtr[i];
        sp -= n + 2;
    }
    CONTINUE;
#pragma endregion
#pragma region Offsetted Load & Store
LOAD_OFST_BYTE:
    {
        sz o = iNextU8(&pc);
        sz b = opcode - OP_LOAD_OFST_BYTE_0;
        LOAD_BYTE(sp, o, b);
    }
    CONTINUE;
LOAD_OFST_HWORD:
    {
        sz o = iNextU8(&pc);
        sz h = opcode - OP_LOAD_OFST_HWORD_0;
        LOAD_HWORD(sp, o, h);
    }
    CONTINUE;
LOAD_OFST_WORD:
    {
        sz o = iNextU8(&pc);
        LOAD_WORD(sp, o);
    }
    CONTINUE;
LOAD_OFST_DWORD:
    {
        sz o = iNextU8(&pc);
        LOAD_DWORD(sp, o);
    }
    CONTINUE;
LOAD_OFST_WORDS:
    {
        sz o = iNextU8(&pc);
        sz n = iNextU8(&pc) + 1;
        DWord ref;
        ref.Word[0] = *(sp - 2);
        ref.Word[1] = *(sp - 1);
        for (sz i = 0; i < n; i++)
            *(sp - 2 + i) = ref.WordPtr[i + o];
        sp += n;
        sp -= 2;
    }
STORE_OFST_BYTE:
    {
        sz o = iNextU8(&pc);
        sz b = opcode - OP_STORE_OFST_BYTE_0;
        STORE_BYTE(sp, o, b);
    }
    CONTINUE;
STORE_OFST_HWORD:
    {
        sz o = iNextU8(&pc);
        sz h = opcode - OP_STORE_OFST_HWORD_0;
        STORE_HWORD(sp, o, h);
    }
    CONTINUE;
STORE_OFST_WORD:
    {
        sz o = iNextU8(&pc);
        STORE_WORD(sp, o);
    }
    CONTINUE;
STORE_OFST_DWORD:
    {
        sz o = iNextU8(&pc);
        STORE_DWORD(sp, o);
    }
    CONTINUE;
STORE_OFST_WORDS:
    {
        sz o = iNextU8(&pc);
        sz n = iNextU8(&pc) + 1;
        DWord ref;
        ref = *(DWord*)(sp - 2 - n);
        for (sz i = 0; i < n; i++)
            *(sp - 2 - n + i) = ref.WordPtr[i + o];
        sp -= n + 2;
    }
    CONTINUE;
#pragma endregion
#pragma region Buffer Load & Store
LOAD_BUFF_BYTE_VAL:
    LOAD_BUFF_VAL_WORD(sp, BytePtr);
    CONTINUE;
LOAD_BUFF_HWORD_VAL:
    LOAD_BUFF_VAL_WORD(sp, HWordPtr);
    CONTINUE;
LOAD_BUFF_WORD_VAL:
    LOAD_BUFF_VAL_WORD(sp, WordPtr);
    CONTINUE;   
LOAD_BUFF_DWORD_VAL:
    LOAD_BUFF_VAL_DWORD(sp);
    CONTINUE;   
LOAD_BUFF_WORDS_VAL:
    {
        sz n = iNextU8(&pc) + 1;
        Word i;
        DWord ref;
        ref = *(DWord*)(sp - 3);
        i           = *(sp - 1);
        for (sz j = 0; j < n; j++)
            *(sp - 2 + j) = (ref.WordPtr + i.UInt * n)[j];

        sp += n;
        sp -= 3;
    }
    CONTINUE;
LOAD_BUFF_BYTE_REF:
    LOAD_BUFF_REF(sp, 1);
    CONTINUE;
LOAD_BUFF_HWORD_REF:
    LOAD_BUFF_REF(sp, 2);
    CONTINUE;
LOAD_BUFF_WORD_REF:
    LOAD_BUFF_REF(sp, 4);
    CONTINUE;  
LOAD_BUFF_DWORD_REF:
    LOAD_BUFF_REF(sp, 8);
    CONTINUE;  
LOAD_BUFF_WORDS_REF:
    {
        sz n = iNextU8(&pc) + 1;
        LOAD_BUFF_REF(sp, n * 4);
    }
    CONTINUE;
STORE_BUFF_BYTE:
    STORE_BUFF_VAL_WORD(sp, BytePtr, u8);
    CONTINUE;
STORE_BUFF_HWORD:
    STORE_BUFF_VAL_WORD(sp, HWordPtr, u16);
    CONTINUE;
STORE_BUFF_WORD:
    STORE_BUFF_VAL_WORD(sp, WordPtr, u32);
    CONTINUE;
STORE_BUFF_DWORD:
    STORE_BUFF_VAL_DWORD(sp);
    CONTINUE;
STORE_BUFF_WORDS:
    {
        sz n = iNextU8(&pc) + 1;
        Word i;
        DWord ref;
        ref = *(DWord*)(sp - 3 - n);
        i           = *(sp - 1 - n);
        for (sz j = 0; j < n; j++)
            (ref.WordPtr + n * i.UInt)[j] = *(sp - n + j);
        
        sp -= n + 3;
    }
    CONTINUE;
ALLOC:
    {
        Word sz = *(sp - 1);
        DWord ref = { .Ptr = malloc(sz.UInt) };
        *(DWord*)(sp - 1) = ref;
        sp += 1;
    }
    CONTINUE;
FREE:
    {
        DWord ref = *(DWord*)(sp - 2);
        free(ref.Ptr);
        sp -= 2;
    }
    CONTINUE;
#pragma endregion
#pragma region Control flow
JMP:
    {
        i16 o = iNextI16(&pc);
        pc += o;
    }
    CONTINUE;
JMP_IF:
    {
        i16 o = iNextI16(&pc);
        if((sp - 1)->Int)
            pc += o;
        sp -= 1;
    }
    CONTINUE;
CALL:
/*
    [ pc_low ] [ pc_high ] [ fp_low ] [ fp_high ] [ awc | swc ] 
    [ local0 ] ... [ localN ] 
    [ stack0 ] ... [ stackN ]
*/
    {
        u16 f = iNextU16(&pc);
        FunctionHeader *header = &functionPool[f]->Header;
        ModuleTable *newMT = header->MT;
        u16 awc         = header->AWC;
        u16 lwc         = header->LWC;
        u16 swc         = header->SWC;
        u16 rwc         = header->RWC;

        for (u16 i = 0; i < awc; i++)
            sp[LOCALS_OFFSET  + i] = sp[i - awc];

        // push it right up the sp
        Word *newFP = sp;
        ((DWord*)(newFP + PC_OFFSET))->Ptr          = pc;
        ((DWord*)(newFP + FP_OFFSET))->Ptr          = fp;
        ((DWord*)(newFP + MT_OFFSET))->Ptr          = mt;
        (newFP + AWC_AND_RWC_OFFSET)->HWord[0].UInt = awc;
        (newFP + AWC_AND_RWC_OFFSET)->HWord[1].UInt = rwc;

        fp = newFP;
        sp = newFP + LOCALS_OFFSET + lwc;
        pc = (Byte*)(header + 1);
        mt = newMT;
        wordPool     = newMT->WordPool;
        dwordPool    = newMT->DWordPool;
        functionPool = newMT->FunctionPool;
        stringPool   = newMT->StringPool;

        UNLIKELY(sp + swc >= context->StackTop, "Stack overflow!\n");
    }
    CONTINUE;
SYSCALL:
    {
        static const void *SyscallPointers[] =
        {
            &&HANDLE_SYSCALL_EXIT,
            &&HANDLE_SYSCALL_PRINT,
            &&HANDLE_SYSCALL_PRINTI,
            &&HANDLE_SYSCALL_PRINTF,
            &&HANDLE_SYSCALL_SCAN,
            &&HANDLE_SYSCALL_SCANI,
            &&HANDLE_SYSCALL_SCANF,
            &&HANDLE_SYSCALL_MEMMOV,
            &&HANDLE_SYSCALL_MEMCPY,
            &&HANDLE_SYSCALL_CLOCK,
            &&HANDLE_SYSCALL_SCANF,
            &&HANDLE_SYSCALL_SQRT32,
            &&HANDLE_SYSCALL_SQRT64,
            &&HANDLE_SYSCALL_EXP32,
            &&HANDLE_SYSCALL_EXP64,
            &&HANDLE_SYSCALL_LOG32,
            &&HANDLE_SYSCALL_LOG64,
        };
        const void * const * syscallTable = SyscallPointers;

        u8 f = iNextU8(&pc);
        goto *syscallTable[f];
        
        HANDLE_SYSCALL_EXIT:
        {
            Word exitCode = *(sp - 1);
            return exitCode.Int;
        }
        HANDLE_SYSCALL_PRINT:
        {
            DWord str = *(DWord*)(sp - 2);
            printf(str.Ptr);
            sp -= 2;
            CONTINUE;
        }
        HANDLE_SYSCALL_PRINTI:
        {
            DWord i = *(DWord*)(sp - 2);
            printf("%ld", i.Int);
            sp -= 2;
            CONTINUE;
        }
        HANDLE_SYSCALL_PRINTF:
        {
            DWord f = *(DWord*)(sp - 2);
            printf("%lf", f.Float);
            sp -= 2;
            CONTINUE;
        }
        HANDLE_SYSCALL_SCAN:
        {
            DWord buff = *(DWord*)(sp - 3);
            Word  size = *(sp - 1);
            
            ch8 c = 0;
            sz  i = 0;
            while((c = getc(stdin)) != '\n' && i < size.UInt)
            {
                buff.BytePtr[i].Char = c;
                i++;
            }
            sp -= 3;
            CONTINUE;
        }
        HANDLE_SYSCALL_SCANI:
        {
            DWord i;
            scanf("%ld", &i.Int);
            *(DWord*)sp = i;
            sp += 2;
            CONTINUE;
        }
        HANDLE_SYSCALL_SCANF:
        {
            DWord f;
            scanf("%lf", &f.Float);
            *(DWord*)sp = f;
            sp += 2;
            CONTINUE;
        }
        HANDLE_SYSCALL_MEMMOV:
        {
            DWord dest = *(DWord*)(sp - 5);
            DWord src  = *(DWord*)(sp - 3);
            Word  n    = *        (sp - 1);
            memmove(dest.Ptr, src.Ptr, n.UInt);
            sp -= 5;
            CONTINUE;
        }
        HANDLE_SYSCALL_MEMCPY:
        {
            DWord dest = *(DWord*)(sp - 5);
            DWord src  = *(DWord*)(sp - 3);
            Word  n    = *        (sp - 1);
            memcpy(dest.Ptr, src.Ptr, n.UInt);
            sp -= 5;
            CONTINUE;
        }
        HANDLE_SYSCALL_CLOCK:
        {
            DWord t = { .Int = clock() };
            *(DWord*)sp = t;
            sp += 2;
            CONTINUE;
        }
        HANDLE_SYSCALL_SQRT32:
        {
            Word x = *(sp - 1);
            Word y = { .Float = sqrtf(x.Float) };
            *(sp - 1) = y;
            CONTINUE;
        }
        HANDLE_SYSCALL_SQRT64:
        {
            DWord x = *(DWord*)(sp - 2);
            DWord y = { .Float = sqrt(x.Float) };
            *(DWord*)(sp - 2) = y;
            CONTINUE;
        }
        HANDLE_SYSCALL_EXP32:
        {
            Word x = *(sp - 1);
            Word y = { .Float = expf(x.Float) };
            *(sp - 1) = y;
            CONTINUE;
        }
        HANDLE_SYSCALL_EXP64:
        {
            DWord x = *(DWord*)(sp - 2);
            DWord y = { .Float = exp(x.Float)  };
            *(DWord*)(sp - 2) = y;
            CONTINUE;
        }
        HANDLE_SYSCALL_LOG32:
        {
            Word x = *(sp - 1);
            Word y = { .Float = logf(x.Float)  };
            *(sp - 1) = y;
            CONTINUE;
        }
        HANDLE_SYSCALL_LOG64:
        {
            DWord x = *(DWord*)(sp - 2);
            DWord y = { .Float = log(x.Float) };
            *(DWord*)(sp - 2) = y;
            CONTINUE;
        }
    }
RET:
    Byte *prevPC =        ((DWord*)(fp + PC_OFFSET))->BytePtr; 
    Word *prevFP =        ((DWord*)(fp + FP_OFFSET))->WordPtr;
    ModuleTable *prevMT = ((DWord*)(fp + MT_OFFSET))->Ptr;
    u16   awc    = (fp + AWC_AND_RWC_OFFSET)->HWord[0].UInt;
    u16   rwc    = (fp + AWC_AND_RWC_OFFSET)->HWord[1].UInt;


    Word *prevSP = fp - awc;
    for (u16 i = 0; i < rwc; i++)
        prevSP[i] = sp[i - rwc];
    
    sp = prevSP + rwc;
    fp = prevFP;
    pc = prevPC;
    mt = prevMT;
    wordPool     = mt->WordPool;
    dwordPool    = mt->DWordPool;
    stringPool   = mt->StringPool;
    functionPool = mt->FunctionPool;

    CONTINUE;
#pragma endregion
    return 1;
}