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
        &&HANDLE_BREAKPOINT,
        &&HANDLE_PUSH_BYTE,
        &&HANDLE_PUSH_BYTE,
        &&HANDLE_PUSH_BYTE,
        &&HANDLE_PUSH_BYTE,
        &&HANDLE_PUSH_HWORD,
        &&HANDLE_PUSH_HWORD,
        &&HANDLE_PUSH_WORD,
        &&HANDLE_PUSH_WORD_L,
        &&HANDLE_PUSH_WORD_L,
        &&HANDLE_PUSH_WORD_L,
        &&HANDLE_PUSH_WORD_L,
        &&HANDLE_PUSH_DWORD,
        &&HANDLE_PUSH_DWORD_L,
        &&HANDLE_PUSH_DWORD_L,
        &&HANDLE_PUSH_DWORD_L,
        &&HANDLE_PUSH_DWORD_L,
        &&HANDLE_PUSH_WORDS,
        &&HANDLE_PUSH_REF,
        
        &&HANDLE_PUSH_I32_V,
        &&HANDLE_PUSH_I32_V,
        &&HANDLE_PUSH_I32_V,
        &&HANDLE_PUSH_I64_V,
        &&HANDLE_PUSH_I64_V,
        &&HANDLE_PUSH_I64_V,
        &&HANDLE_PUSH_F32_1,
        &&HANDLE_PUSH_F32_2,
        &&HANDLE_PUSH_F64_1,
        &&HANDLE_PUSH_F64_2,
        &&HANDLE_PUSH_I32,
        &&HANDLE_PUSH_I64,
        
        &&HANDLE_PUSH_CONST_WORD,
        &&HANDLE_PUSH_CONST_WORD_W,
        &&HANDLE_PUSH_CONST_DWORD,
        &&HANDLE_PUSH_CONST_DWORD_W,
        &&HANDLE_PUSH_CONST_STR,
        &&HANDLE_PUSH_CONST_STR_W,
        &&HANDLE_PUSH_GLOB_REF,
        &&HANDLE_PUSH_GLOB_REF_W,
        &&HANDLE_PUSH_FUNC,

        &&HANDLE_POP_BYTE,
        &&HANDLE_POP_BYTE,
        &&HANDLE_POP_BYTE,
        &&HANDLE_POP_BYTE,
        &&HANDLE_POP_HWORD,
        &&HANDLE_POP_HWORD,
        &&HANDLE_POP_WORD,
        &&HANDLE_POP_WORD_L,
        &&HANDLE_POP_WORD_L,
        &&HANDLE_POP_WORD_L,
        &&HANDLE_POP_WORD_L,
        &&HANDLE_POP_DWORD,
        &&HANDLE_POP_DWORD_L,
        &&HANDLE_POP_DWORD_L,
        &&HANDLE_POP_DWORD_L,
        &&HANDLE_POP_DWORD_L,
        &&HANDLE_POP_WORDS,

        &&HANDLE_ADD_I32,
        &&HANDLE_ADD_I64,
        &&HANDLE_ADD_F32,
        &&HANDLE_ADD_F64,
        &&HANDLE_INC_I32,
        &&HANDLE_INC_I64,
        &&HANDLE_INC_F32,
        &&HANDLE_INC_F64,

        &&HANDLE_SUB_I32,
        &&HANDLE_SUB_I64,
        &&HANDLE_SUB_F32,
        &&HANDLE_SUB_F64,
        &&HANDLE_DEC_I32,
        &&HANDLE_DEC_I64,
        &&HANDLE_DEC_F32,
        &&HANDLE_DEC_F64,

        &&HANDLE_MUL_I32,
        &&HANDLE_MUL_I64,
        &&HANDLE_MUL_U32,
        &&HANDLE_MUL_U64,
        &&HANDLE_MUL_F32,
        &&HANDLE_MUL_F64,

        &&HANDLE_DIV_I32,
        &&HANDLE_DIV_I64,
        &&HANDLE_DIV_U32,
        &&HANDLE_DIV_U64,
        &&HANDLE_DIV_F32,
        &&HANDLE_DIV_F64,

        &&HANDLE_REM_I32,
        &&HANDLE_REM_I64,
        &&HANDLE_REM_U32,
        &&HANDLE_REM_U64,

        &&HANDLE_NEG_I32,
        &&HANDLE_NEG_I64,
        &&HANDLE_NEG_F32,
        &&HANDLE_NEG_F64,

        &&HANDLE_NOT_WORD,
        &&HANDLE_NOT_DWORD,
        &&HANDLE_AND_WORD,
        &&HANDLE_AND_DWORD,
        &&HANDLE_OR_WORD,
        &&HANDLE_OR_DWORD,
        &&HANDLE_XOR_WORD,
        &&HANDLE_XOR_DWORD,
        &&HANDLE_SHL_WORD,
        &&HANDLE_SHL_DWORD,
        &&HANDLE_SHR_I32,
        &&HANDLE_SHR_I64,
        &&HANDLE_SHR_U32,
        &&HANDLE_SHR_U64,

        &&HANDLE_I32_TO_I8,
        &&HANDLE_I32_TO_I16,
        &&HANDLE_I32_TO_I64,
        &&HANDLE_I32_TO_F32,
        &&HANDLE_I32_TO_F64,
        &&HANDLE_I64_TO_I32,
        &&HANDLE_I64_TO_F32,
        &&HANDLE_I64_TO_F64,
        &&HANDLE_F32_TO_I32,
        &&HANDLE_F32_TO_I64,
        &&HANDLE_F32_TO_F64,
        &&HANDLE_F64_TO_I32,
        &&HANDLE_F64_TO_I64,
        &&HANDLE_F64_TO_F32,

        &&HANDLE_CMP_WORD_EQ,
        &&HANDLE_CMP_DWORD_EQ,
        &&HANDLE_CMP_WORD_NE,
        &&HANDLE_CMP_DWORD_NE,
        &&HANDLE_CMP_I32_GT,
        &&HANDLE_CMP_I64_GT,
        &&HANDLE_CMP_U32_GT,
        &&HANDLE_CMP_U64_GT,
        &&HANDLE_CMP_F32_GT,
        &&HANDLE_CMP_F64_GT,
        &&HANDLE_CMP_I32_LT,
        &&HANDLE_CMP_I64_LT,
        &&HANDLE_CMP_U32_LT,
        &&HANDLE_CMP_U64_LT,
        &&HANDLE_CMP_F32_LT,
        &&HANDLE_CMP_F64_LT,
        &&HANDLE_CMP_I32_GE,
        &&HANDLE_CMP_I64_GE,
        &&HANDLE_CMP_U32_GE,
        &&HANDLE_CMP_U64_GE,
        &&HANDLE_CMP_F32_GE,
        &&HANDLE_CMP_F64_GE,
        &&HANDLE_CMP_I32_LE,
        &&HANDLE_CMP_I64_LE,
        &&HANDLE_CMP_U32_LE,
        &&HANDLE_CMP_U64_LE,
        &&HANDLE_CMP_F32_LE,
        &&HANDLE_CMP_F64_LE,
        &&HANDLE_CMP_NOT,

        &&HANDLE_DUP_WORD,
        &&HANDLE_DUP_DWORD,
        &&HANDLE_DUP_WORD_X1,
        &&HANDLE_DUP_DWORD_X1,
        &&HANDLE_DUP_WORD_X2,
        &&HANDLE_DUP_DWORD_X2,
        &&HANDLE_SWAP_WORD,
        &&HANDLE_SWAP_DWORD,

        &&HANDLE_LOAD_BYTE,
        &&HANDLE_LOAD_BYTE,
        &&HANDLE_LOAD_BYTE,
        &&HANDLE_LOAD_BYTE,
        &&HANDLE_LOAD_HWORD,
        &&HANDLE_LOAD_HWORD,
        &&HANDLE_LOAD_WORD,
        &&HANDLE_LOAD_DWORD,
        &&HANDLE_LOAD_WORDS,

        &&HANDLE_STORE_BYTE,
        &&HANDLE_STORE_BYTE,
        &&HANDLE_STORE_BYTE,
        &&HANDLE_STORE_BYTE,
        &&HANDLE_STORE_HWORD,
        &&HANDLE_STORE_HWORD,
        &&HANDLE_STORE_WORD,
        &&HANDLE_STORE_DWORD,
        &&HANDLE_STORE_WORDS,

        &&HANDLE_LOAD_OFST_BYTE,
        &&HANDLE_LOAD_OFST_BYTE,
        &&HANDLE_LOAD_OFST_BYTE,
        &&HANDLE_LOAD_OFST_BYTE,
        &&HANDLE_LOAD_OFST_HWORD,
        &&HANDLE_LOAD_OFST_HWORD,
        &&HANDLE_LOAD_OFST_WORD,
        &&HANDLE_LOAD_OFST_DWORD,
        &&HANDLE_LOAD_OFST_WORDS,

        &&HANDLE_STORE_OFST_BYTE,
        &&HANDLE_STORE_OFST_BYTE,
        &&HANDLE_STORE_OFST_BYTE,
        &&HANDLE_STORE_OFST_BYTE,
        &&HANDLE_STORE_OFST_HWORD,
        &&HANDLE_STORE_OFST_HWORD,
        &&HANDLE_STORE_OFST_WORD,
        &&HANDLE_STORE_OFST_DWORD,
        &&HANDLE_STORE_OFST_WORDS,

        &&HANDLE_LOAD_BUFF_BYTE_VAL,
        &&HANDLE_LOAD_BUFF_HWORD_VAL,
        &&HANDLE_LOAD_BUFF_WORD_VAL,
        &&HANDLE_LOAD_BUFF_DWORD_VAL,
        &&HANDLE_LOAD_BUFF_WORDS_VAL,

        &&HANDLE_LOAD_BUFF_BYTE_REF,
        &&HANDLE_LOAD_BUFF_HWORD_REF,
        &&HANDLE_LOAD_BUFF_WORD_REF,
        &&HANDLE_LOAD_BUFF_DWORD_REF,
        &&HANDLE_LOAD_BUFF_WORDS_REF,

        &&HANDLE_STORE_BUFF_BYTE,
        &&HANDLE_STORE_BUFF_HWORD,
        &&HANDLE_STORE_BUFF_WORD,
        &&HANDLE_STORE_BUFF_DWORD,
        &&HANDLE_STORE_BUFF_WORDS,

        &&HANDLE_ALLOC,
        &&HANDLE_FREE,

        &&HANDLE_JMP,
        &&HANDLE_JMP_IF,
        &&HANDLE_CALL,
        &&HANDLE_INDCALL,
        &&HANDLE_SYSCALL,
        &&HANDLE_RET,
        &&HANDLE_NOT_IMPLEMENTED,
        &&HANDLE_NOT_IMPLEMENTED,
        &&HANDLE_NOT_IMPLEMENTED,
        &&HANDLE_NOT_IMPLEMENTED,
        &&HANDLE_NOT_IMPLEMENTED,
        &&HANDLE_NOT_IMPLEMENTED,
        &&HANDLE_NOT_IMPLEMENTED,
        &&HANDLE_NOT_IMPLEMENTED,
        &&HANDLE_NOT_IMPLEMENTED,
        &&HANDLE_NOT_IMPLEMENTED,
        &&HANDLE_NOT_IMPLEMENTED,
        &&HANDLE_NOT_IMPLEMENTED,
        &&HANDLE_NOT_IMPLEMENTED,
        &&HANDLE_NOT_IMPLEMENTED,
        &&HANDLE_NOT_IMPLEMENTED,
        &&HANDLE_NOT_IMPLEMENTED,
        &&HANDLE_NOT_IMPLEMENTED,
        &&HANDLE_NOT_IMPLEMENTED,
        &&HANDLE_NOT_IMPLEMENTED,
        &&HANDLE_NOT_IMPLEMENTED,
        &&HANDLE_NOT_IMPLEMENTED,
        &&HANDLE_NOT_IMPLEMENTED,
        &&HANDLE_NOT_IMPLEMENTED,
        &&HANDLE_NOT_IMPLEMENTED,
        &&HANDLE_NOT_IMPLEMENTED,
        &&HANDLE_NOT_IMPLEMENTED,
        &&HANDLE_NOT_IMPLEMENTED,
        &&HANDLE_NOT_IMPLEMENTED,
        &&HANDLE_NOT_IMPLEMENTED,
        &&HANDLE_NOT_IMPLEMENTED,
        &&HANDLE_NOT_IMPLEMENTED,
        &&HANDLE_NOT_IMPLEMENTED,
        &&HANDLE_NOT_IMPLEMENTED,
        &&HANDLE_NOT_IMPLEMENTED,
        &&HANDLE_NOT_IMPLEMENTED,
        &&HANDLE_NOT_IMPLEMENTED,
        &&HANDLE_NOT_IMPLEMENTED,
        &&HANDLE_NOT_IMPLEMENTED,
        &&HANDLE_NOT_IMPLEMENTED,
};
#pragma endregion
    const void * const *instructionTable = InstructionPointers;
LOOP:
    pc += 1;
    goto *instructionTable[opcode];
HANDLE_NOT_IMPLEMENTED:
    UNLIKELY(false, "Version not supported!\n");
    exit(EXIT_FAILURE);
HANDLE_BREAKPOINT:
    DEVEL_ASSERT(false, "Breakpoint reached!\n");
    CONTINUE;
#pragma region Push local
HANDLE_PUSH_BYTE:
    {
        sz l = iNextU8(&pc);
        sz b = opcode - OP_PUSH_BYTE_0;
        LOCAL_PUSH_BYTE(sp, fp, l, b);
    }
    CONTINUE;
HANDLE_PUSH_HWORD:
    {
        sz l = iNextU8(&pc);
        sz h = opcode - OP_PUSH_HWORD_0;
        LOCAL_PUSH_HWORD(sp, fp, l, h);
    }
    CONTINUE;
HANDLE_PUSH_WORD:
    {
        sz l = iNextU8(&pc);
        LOCAL_PUSH_WORD(sp, fp, l);
    }
    CONTINUE;
HANDLE_PUSH_WORD_L:
    {
        sz l = opcode - OP_PUSH_WORD_0;
        LOCAL_PUSH_WORD(sp, fp, l);
    }
    CONTINUE;
HANDLE_PUSH_DWORD:
    {
        sz l = iNextU8(&pc);
        LOCAL_PUSH_DWORD(sp, fp, l);
    }
    CONTINUE;
HANDLE_PUSH_DWORD_L:
    {
        sz l = opcode - OP_PUSH_DWORD_0;
        LOCAL_PUSH_DWORD(sp, fp, l);
    }
    CONTINUE;
HANDLE_PUSH_WORDS:
    {
        sz l = iNextU8(&pc);
        sz n = iNextU8(&pc) + 1;
        LOCAL_PUSH_WORDS(sp, fp, l, n);
    }
    CONTINUE;
HANDLE_PUSH_REF:
    {
        sz l      = iNextU8(&pc);
        DWord ref = RefToDWord(fp + LOCALS_OFFSET + l); 
        VAL_PUSH_DWORD(sp, ref);
    }
    CONTINUE;
#pragma endregion
#pragma region Push Immediate
HANDLE_PUSH_I32_V:
    {
        Word w = IntToWord(opcode - OP_PUSH_0_WORD);
        VAL_PUSH_WORD(sp, w);
    }
    CONTINUE;
HANDLE_PUSH_I64_V:
    {
        DWord d = IntToDWord(opcode - OP_PUSH_0_DWORD);
        VAL_PUSH_DWORD(sp, d);
    }
    CONTINUE;
HANDLE_PUSH_F32_1:
    {
        Word w = FloatToWord(1.0f);
        VAL_PUSH_WORD(sp, w);
    }    
    CONTINUE;
HANDLE_PUSH_F32_2:
    {
        Word w = FloatToWord(2.0f);
        VAL_PUSH_WORD(sp, w);
    }    
    CONTINUE;
HANDLE_PUSH_F64_1:
    {
        DWord d = FloatToDWord(1.0);
        VAL_PUSH_DWORD(sp, d);
    }    
    CONTINUE;
HANDLE_PUSH_F64_2:
    {
        DWord d = FloatToDWord(2.0);
        VAL_PUSH_DWORD(sp, d);
    }    
    CONTINUE;
HANDLE_PUSH_I32:
    {
        Word w = IntToWord((i32)iNextI8(&pc));
        VAL_PUSH_WORD(sp, w);
    }
    CONTINUE;
HANDLE_PUSH_I64:
    {
        DWord d = IntToDWord((i64)iNextI8(&pc));
        VAL_PUSH_DWORD(sp, d);
    }
    CONTINUE;
#pragma endregion
#pragma region Push Constant
HANDLE_PUSH_CONST_WORD:
    {
        Word w = *(wordPool + iNextU8(&pc));
        VAL_PUSH_WORD(sp, w);
    }
    CONTINUE;
HANDLE_PUSH_CONST_WORD_W:
    {
        Word w = *(wordPool + iNextU16(&pc));
        VAL_PUSH_WORD(sp, w);
    }    
    CONTINUE;
HANDLE_PUSH_CONST_DWORD:
    {
        DWord d = *(dwordPool + iNextU8(&pc));
        VAL_PUSH_DWORD(sp, d);
    }    
    CONTINUE;
HANDLE_PUSH_CONST_DWORD_W:
    {
        DWord d = *(dwordPool + iNextU16(&pc));
        VAL_PUSH_DWORD(sp, d);
    }    
    CONTINUE;
HANDLE_PUSH_CONST_STR:
    {
        DWord d = RefToDWord(*(stringPool + iNextU8(&pc)));
        VAL_PUSH_DWORD(sp, d);
    }    
    CONTINUE;
HANDLE_PUSH_CONST_STR_W:
    {
        DWord d = RefToDWord(*(stringPool + iNextU16(&pc)));
        VAL_PUSH_DWORD(sp, d);
    }    
    CONTINUE;
HANDLE_PUSH_GLOB_REF:
HANDLE_PUSH_GLOB_REF_W:
HANDLE_PUSH_FUNC:
    goto HANDLE_NOT_IMPLEMENTED;
#pragma endregion
#pragma region Pop
HANDLE_POP_BYTE:
    {
        sz l = iNextU8(&pc);
        sz b = opcode - OP_POP_BYTE_0;
        POP_BYTE(sp, fp, l, b);
    }
    CONTINUE;
HANDLE_POP_HWORD:
    {
        sz l = iNextU8(&pc);
        sz h = opcode - OP_POP_HWORD_0;
        POP_HWORD(sp, fp, l, h);
    }
    CONTINUE;
HANDLE_POP_WORD:
    {
        sz l = iNextU8(&pc);
        POP_WORD(sp, fp, l);
    }
    CONTINUE;
HANDLE_POP_WORD_L:
    {
        sz l = opcode - OP_POP_WORD_0;
        POP_WORD(sp, fp, l);
    }
    CONTINUE;
HANDLE_POP_DWORD:
    {
        sz l = iNextU8(&pc);
        POP_DWORD(sp, fp, l);
    }
    CONTINUE;
HANDLE_POP_DWORD_L:
    {
        sz l = opcode - OP_POP_DWORD_0;
        POP_DWORD(sp, fp, l);
    }
    CONTINUE;
HANDLE_POP_WORDS:
    {
        sz l = iNextU8(&pc);
        sz n = iNextU8(&pc);
        POP_WORDS(sp, fp, l, n);
    }
    CONTINUE;
#pragma endregion
#pragma region Arithmetic
HANDLE_ADD_I32:
    BINARY_OPERATION_WORD(sp, Int, +);
    CONTINUE;
HANDLE_ADD_I64:
    BINARY_OPERATION_DWORD(sp, Int, +);
    CONTINUE;
HANDLE_ADD_F32:
    BINARY_OPERATION_WORD(sp, Float, +);
    CONTINUE;
HANDLE_ADD_F64:
    BINARY_OPERATION_DWORD(sp, Float, +);
    CONTINUE;
HANDLE_INC_I32:
    {
        sz l = iNextU8(&pc);
        i32 v = (i32) iNextU8(&pc);
        LOCAL_BINARY_OPERATION_WORD(fp, Int, +, l, v);
    }
    CONTINUE;
HANDLE_INC_I64:
    {
        u8 l = iNextU8(&pc);
        i64 v = (i64) iNextU8(&pc);
        LOCAL_BINARY_OPERATION_DWORD(fp, Int, +, l, v);
    }
    CONTINUE;
HANDLE_INC_F32:
    {
        u8 l = iNextU8(&pc);
        f32 v = (f32) iNextU8(&pc);
        LOCAL_BINARY_OPERATION_WORD(fp, Float, +, l, v);
    }
    CONTINUE;
HANDLE_INC_F64:
    {
        u8 l = iNextU8(&pc);
        f64 v = (f64) iNextU8(&pc);
        LOCAL_BINARY_OPERATION_DWORD(fp, Float, +, l, v);
    }
    CONTINUE;
HANDLE_SUB_I32:
    BINARY_OPERATION_WORD(sp, Int, -);
    CONTINUE;
HANDLE_SUB_I64:
    BINARY_OPERATION_DWORD(sp, Int, -);
    CONTINUE;
HANDLE_SUB_F32:
    BINARY_OPERATION_WORD(sp, Float, -);
    CONTINUE;
HANDLE_SUB_F64:
    BINARY_OPERATION_DWORD(sp, Float, -);
    CONTINUE;
HANDLE_DEC_I32:
    {
        u8 l = iNextU8(&pc);
        i32 v = (i32) iNextU8(&pc);
        LOCAL_BINARY_OPERATION_WORD(fp, Int, -, l, v);
    }
    CONTINUE;
HANDLE_DEC_I64:
    {
        u8 l = iNextU8(&pc);
        i64 v = (i64) iNextU8(&pc);
        LOCAL_BINARY_OPERATION_DWORD(fp, Int, -, l, v);
    }
    CONTINUE;
HANDLE_DEC_F32:
    {
        u8 l = iNextU8(&pc);
        f32 v = (f32) iNextU8(&pc);
        LOCAL_BINARY_OPERATION_WORD(fp, Float, -, l, v);
    }
    CONTINUE;
HANDLE_DEC_F64:
    {
        u8 l = iNextU8(&pc);
        f64 v = (f64) iNextU8(&pc);
        LOCAL_BINARY_OPERATION_DWORD(fp, Float, -, l, v);
    }
    CONTINUE;
HANDLE_MUL_I32:
    BINARY_OPERATION_WORD(sp, Int, *);
    CONTINUE;
HANDLE_MUL_I64:
    BINARY_OPERATION_DWORD(sp, Int, *);
    CONTINUE;
HANDLE_MUL_U32:
    BINARY_OPERATION_WORD(sp, UInt, *);
    CONTINUE;
HANDLE_MUL_U64:
    BINARY_OPERATION_DWORD(sp, UInt, *);
    CONTINUE;
HANDLE_MUL_F32:
    BINARY_OPERATION_WORD(sp, Float, *);
    CONTINUE;
HANDLE_MUL_F64:
    BINARY_OPERATION_DWORD(sp, Float,*);
    CONTINUE;
HANDLE_DIV_I32:
    BINARY_OPERATION_WORD(sp, Int, /);
    CONTINUE;
HANDLE_DIV_I64:
    BINARY_OPERATION_DWORD(sp, Int, /);
    CONTINUE;
HANDLE_DIV_U32:
    BINARY_OPERATION_WORD(sp, UInt, /);
    CONTINUE;
HANDLE_DIV_U64:
    BINARY_OPERATION_DWORD(sp, UInt, /);
    CONTINUE;
HANDLE_DIV_F32:
    BINARY_OPERATION_WORD(sp, Float, /);
    CONTINUE;
HANDLE_DIV_F64:
    BINARY_OPERATION_DWORD(sp, Float,/);
    CONTINUE;
HANDLE_REM_I32:
    BINARY_OPERATION_WORD(sp, Int, %);
    CONTINUE;
HANDLE_REM_I64:
    BINARY_OPERATION_DWORD(sp, Int, %);
    CONTINUE;
HANDLE_REM_U32:
    BINARY_OPERATION_WORD(sp, UInt, %);
    CONTINUE;
HANDLE_REM_U64:
    BINARY_OPERATION_DWORD(sp, UInt, %);
    CONTINUE;
HANDLE_NEG_I32:
    UNARY_OPERATION_WORD(sp, Int, -);
    CONTINUE;
HANDLE_NEG_I64:
    UNARY_OPERATION_DWORD(sp, Int, -);
    CONTINUE;
HANDLE_NEG_F32:
    UNARY_OPERATION_WORD(sp, Float, -);
    CONTINUE;
HANDLE_NEG_F64:
    UNARY_OPERATION_DWORD(sp, Float, -);
    CONTINUE;
#pragma endregion
#pragma region Bitwise
HANDLE_NOT_WORD:
    UNARY_OPERATION_WORD(sp, Int, ~);
    CONTINUE;
HANDLE_NOT_DWORD:
    UNARY_OPERATION_DWORD(sp, Int, ~);
    CONTINUE;
HANDLE_AND_WORD:
    BINARY_OPERATION_WORD(sp, Int, &);
    CONTINUE;
HANDLE_AND_DWORD:
    BINARY_OPERATION_DWORD(sp, Int, &);
    CONTINUE;
HANDLE_OR_WORD:
    BINARY_OPERATION_WORD(sp, Int, |);
    CONTINUE;
HANDLE_OR_DWORD:
    BINARY_OPERATION_DWORD(sp, Int, |);
    CONTINUE;
HANDLE_XOR_WORD:
    BINARY_OPERATION_WORD(sp, Int, ^);
    CONTINUE;
HANDLE_XOR_DWORD:
    BINARY_OPERATION_DWORD(sp, Int, ^);
    CONTINUE;
HANDLE_SHL_WORD:
    BINARY_OPERATION_WORD(sp, Int, <<);
    CONTINUE;
HANDLE_SHL_DWORD:
    BINARY_OPERATION_DWORD(sp, Int, <<);
    CONTINUE;
HANDLE_SHR_I32:
    BINARY_OPERATION_WORD(sp, Int, >>);
    CONTINUE;
HANDLE_SHR_I64:
    // TO-DO:FIX
    BINARY_OPERATION_DWORD(sp, Int, >>);
    CONTINUE;
HANDLE_SHR_U32:
    BINARY_OPERATION_WORD(sp, UInt, >>);
    CONTINUE;
HANDLE_SHR_U64:
    // TO-DO:FIX
    BINARY_OPERATION_DWORD(sp, UInt, >>);
    CONTINUE;
#pragma endregion    
#pragma region Cast
HANDLE_I32_TO_I8:
    CAST_WORD_WORD(sp, Int, Int, i8);
    CONTINUE;
HANDLE_I32_TO_I16:
    CAST_WORD_WORD(sp, Int, Int, i16);
    CONTINUE;
HANDLE_I32_TO_I64:
    CAST_WORD_DWORD(sp, Int, Int, i64);
    CONTINUE;
HANDLE_I32_TO_F32:
    CAST_WORD_WORD(sp, Int, Float, f32);
    CONTINUE;
HANDLE_I32_TO_F64:
    CAST_WORD_DWORD(sp, Int, Float, f64);
    CONTINUE;
HANDLE_I64_TO_I32:
    CAST_DWORD_WORD(sp, Int, Int, i32);
    CONTINUE;
HANDLE_I64_TO_F32:
    CAST_DWORD_WORD(sp, Int, Float, f32);
    CONTINUE;
HANDLE_I64_TO_F64:
    CAST_DWORD_DWORD(sp, Int, Float, f64);
    CONTINUE;
HANDLE_F32_TO_I32:
    CAST_WORD_WORD(sp, Float, Int, i32);
    CONTINUE;
HANDLE_F32_TO_I64:
    CAST_WORD_WORD(sp, Float, Int, i64);
    CONTINUE;
HANDLE_F32_TO_F64:
    CAST_WORD_DWORD(sp, Float, Float, f64);
    CONTINUE;
HANDLE_F64_TO_I32:
    CAST_DWORD_WORD(sp, Float, Int, i32);
    CONTINUE;
HANDLE_F64_TO_I64:
    CAST_DWORD_DWORD(sp, Float, Int, i64);
    CONTINUE;
HANDLE_F64_TO_F32:
    CAST_DWORD_WORD(sp, Float, Float, f32);
    CONTINUE;
#pragma endregion
#pragma region Compare
HANDLE_CMP_WORD_EQ:
    BINARY_OPERATION_WORD(sp, Int, ==);
    CONTINUE;
HANDLE_CMP_DWORD_EQ:
    BINARY_OPERATION_DWORD(sp, Int, ==);
    CONTINUE;
HANDLE_CMP_WORD_NE:
    BINARY_OPERATION_WORD(sp, Int, !=);
    CONTINUE;
HANDLE_CMP_DWORD_NE:
    BINARY_OPERATION_DWORD(sp, Int, !=);
    CONTINUE;
HANDLE_CMP_I32_GT:
    BINARY_OPERATION_WORD(sp, Int, >);
    CONTINUE;
HANDLE_CMP_I64_GT:
    BINARY_OPERATION_DWORD(sp, Int, >);
    CONTINUE;
HANDLE_CMP_U32_GT:
    BINARY_OPERATION_WORD(sp, UInt, >);
    CONTINUE;
HANDLE_CMP_U64_GT:
    BINARY_OPERATION_DWORD(sp, UInt, >);
    CONTINUE;
HANDLE_CMP_F32_GT:
    BINARY_OPERATION_WORD(sp, Float, >);
    CONTINUE;
HANDLE_CMP_F64_GT:
    BINARY_OPERATION_DWORD(sp, Float, >);
    CONTINUE;
HANDLE_CMP_I32_LT:
    BINARY_OPERATION_WORD(sp, Int, <);
    CONTINUE;
HANDLE_CMP_I64_LT:
    BINARY_OPERATION_DWORD(sp, Int, <);
    CONTINUE;
HANDLE_CMP_U32_LT:
    BINARY_OPERATION_WORD(sp, UInt, <);
    CONTINUE;
HANDLE_CMP_U64_LT:
    BINARY_OPERATION_DWORD(sp, UInt, <);
    CONTINUE;
HANDLE_CMP_F32_LT:
    BINARY_OPERATION_WORD(sp, Float, <);
    CONTINUE;
HANDLE_CMP_F64_LT:
    BINARY_OPERATION_DWORD(sp, Float, <);
    CONTINUE;
HANDLE_CMP_I32_GE:
    BINARY_OPERATION_WORD(sp, Int, >=);
    CONTINUE;
HANDLE_CMP_I64_GE:
    BINARY_OPERATION_DWORD(sp, Int, >=);
    CONTINUE;
HANDLE_CMP_U32_GE:
    BINARY_OPERATION_WORD(sp, UInt, >=);
    CONTINUE;
HANDLE_CMP_U64_GE:
    BINARY_OPERATION_DWORD(sp, UInt, >=);
    CONTINUE;
HANDLE_CMP_F32_GE:
    BINARY_OPERATION_WORD(sp, Float, >=);
    CONTINUE;
HANDLE_CMP_F64_GE:
    BINARY_OPERATION_DWORD(sp, Float, >=);
    CONTINUE;
HANDLE_CMP_I32_LE:
    BINARY_OPERATION_WORD(sp, Int, <=);
    CONTINUE;
HANDLE_CMP_I64_LE:
    BINARY_OPERATION_DWORD(sp, Int, <=);
    CONTINUE;
HANDLE_CMP_U32_LE:
    BINARY_OPERATION_WORD(sp, UInt, <=);
    CONTINUE;
HANDLE_CMP_U64_LE:
    BINARY_OPERATION_DWORD(sp, UInt, <=);
    CONTINUE;
HANDLE_CMP_F32_LE:
    BINARY_OPERATION_WORD(sp, Float, <=);
    CONTINUE;
HANDLE_CMP_F64_LE:
    BINARY_OPERATION_DWORD(sp, Float, <=);
    CONTINUE;
HANDLE_CMP_NOT:
    UNARY_OPERATION_WORD(sp, Int, !);
    CONTINUE;
#pragma endregion    
#pragma region Stack manipulation
HANDLE_DUP_WORD:
    {
        *(sp - 0) = *(sp - 1);    
        sp += 1;    
    }
    CONTINUE;
HANDLE_DUP_DWORD:
    {
        *(DWord*)(sp + 0) = *(DWord*)(sp - 2);
        sp += 2;
    }  
HANDLE_DUP_WORD_X1:
    {
        Word top = *(sp - 1);
        *(sp - 1) = *(sp - 2);
        *(sp - 2) = top;
        *(sp - 0) = top;    
        sp += 1;    
    }
    CONTINUE;
HANDLE_DUP_DWORD_X1:
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
HANDLE_DUP_WORD_X2:
    {
        Word top  = *(sp - 1);
        *(DWord*)(sp - 2) = *(DWord*)(sp - 3);
        *(sp - 0) = top;   
        *(sp - 3) = top; 
        sp += 1;    
    }
    CONTINUE;
HANDLE_DUP_DWORD_X2:
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
HANDLE_SWAP_WORD:
    {
        Word top = *(sp - 1);
        *(sp - 1) = *(sp - 2);
        *(sp - 2) = top;
    }
    CONTINUE;
HANDLE_SWAP_DWORD:
    {
        DWord top = *(DWord*)(sp - 2);
        *(DWord*)(sp - 2) = *(DWord*)(sp - 4);
        *(DWord*)(sp - 4) = top;
    }
    CONTINUE;
#pragma endregion   
#pragma region Standard Load & Store
HANDLE_LOAD_BYTE:
    {
        sz b = opcode - OP_LOAD_BYTE_0;
        LOAD_BYTE(sp, 0, b);
    }
    CONTINUE;
HANDLE_LOAD_HWORD:
    {
        sz h = opcode - OP_LOAD_HWORD_0;
        LOAD_HWORD(sp, 0, h);
    }
    CONTINUE;
HANDLE_LOAD_WORD:
    LOAD_WORD(sp, 0);
    CONTINUE;
HANDLE_LOAD_DWORD:
    LOAD_DWORD(sp, 0);
    CONTINUE;
HANDLE_LOAD_WORDS:
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
HANDLE_STORE_BYTE:
    {
        sz b = opcode - OP_STORE_BYTE_0;
        STORE_BYTE(sp, 0, b);
    }
    CONTINUE;
HANDLE_STORE_HWORD:
    {
        sz h = opcode - OP_STORE_HWORD_0;
        STORE_HWORD(sp, 0, h);
    }
    CONTINUE;
HANDLE_STORE_WORD:
    STORE_WORD(sp, 0);
    CONTINUE;
HANDLE_STORE_DWORD:
    STORE_DWORD(sp, 0);
    CONTINUE;
HANDLE_STORE_WORDS:
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
HANDLE_LOAD_OFST_BYTE:
    {
        sz o = iNextU8(&pc);
        sz b = opcode - OP_LOAD_OFST_BYTE_0;
        LOAD_BYTE(sp, o, b);
    }
    CONTINUE;
HANDLE_LOAD_OFST_HWORD:
    {
        sz o = iNextU8(&pc);
        sz h = opcode - OP_LOAD_OFST_HWORD_0;
        LOAD_HWORD(sp, o, h);
    }
    CONTINUE;
HANDLE_LOAD_OFST_WORD:
    {
        sz o = iNextU8(&pc);
        LOAD_WORD(sp, o);
    }
    CONTINUE;
HANDLE_LOAD_OFST_DWORD:
    {
        sz o = iNextU8(&pc);
        LOAD_DWORD(sp, o);
    }
    CONTINUE;
HANDLE_LOAD_OFST_WORDS:
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
HANDLE_STORE_OFST_BYTE:
    {
        sz o = iNextU8(&pc);
        sz b = opcode - OP_STORE_OFST_BYTE_0;
        STORE_BYTE(sp, o, b);
    }
    CONTINUE;
HANDLE_STORE_OFST_HWORD:
    {
        sz o = iNextU8(&pc);
        sz h = opcode - OP_STORE_OFST_HWORD_0;
        STORE_HWORD(sp, o, h);
    }
    CONTINUE;
HANDLE_STORE_OFST_WORD:
    {
        sz o = iNextU8(&pc);
        STORE_WORD(sp, o);
    }
    CONTINUE;
HANDLE_STORE_OFST_DWORD:
    {
        sz o = iNextU8(&pc);
        STORE_DWORD(sp, o);
    }
    CONTINUE;
HANDLE_STORE_OFST_WORDS:
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
HANDLE_LOAD_BUFF_BYTE_VAL:
    LOAD_BUFF_VAL_WORD(sp, BytePtr);
    CONTINUE;
HANDLE_LOAD_BUFF_HWORD_VAL:
    LOAD_BUFF_VAL_WORD(sp, HWordPtr);
    CONTINUE;
HANDLE_LOAD_BUFF_WORD_VAL:
    LOAD_BUFF_VAL_WORD(sp, WordPtr);
    CONTINUE;   
HANDLE_LOAD_BUFF_DWORD_VAL:
    LOAD_BUFF_VAL_DWORD(sp);
    CONTINUE;   
HANDLE_LOAD_BUFF_WORDS_VAL:
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
HANDLE_LOAD_BUFF_BYTE_REF:
    LOAD_BUFF_REF(sp, 1);
    CONTINUE;
HANDLE_LOAD_BUFF_HWORD_REF:
    LOAD_BUFF_REF(sp, 2);
    CONTINUE;
HANDLE_LOAD_BUFF_WORD_REF:
    LOAD_BUFF_REF(sp, 4);
    CONTINUE;  
HANDLE_LOAD_BUFF_DWORD_REF:
    LOAD_BUFF_REF(sp, 8);
    CONTINUE;  
HANDLE_LOAD_BUFF_WORDS_REF:
    {
        sz n = iNextU8(&pc) + 1;
        LOAD_BUFF_REF(sp, n * 4);
    }
    CONTINUE;
HANDLE_STORE_BUFF_BYTE:
    STORE_BUFF_VAL_WORD(sp, BytePtr, u8);
    CONTINUE;
HANDLE_STORE_BUFF_HWORD:
    STORE_BUFF_VAL_WORD(sp, HWordPtr, u16);
    CONTINUE;
HANDLE_STORE_BUFF_WORD:
    STORE_BUFF_VAL_WORD(sp, WordPtr, u32);
    CONTINUE;
HANDLE_STORE_BUFF_DWORD:
    STORE_BUFF_VAL_DWORD(sp);
    CONTINUE;
HANDLE_STORE_BUFF_WORDS:
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
HANDLE_ALLOC:
    {
        Word sz = *(sp - 1);
        DWord ref = { .Ptr = malloc(sz.UInt) };
        *(DWord*)(sp - 1) = ref;
        sp += 1;
    }
    CONTINUE;
HANDLE_FREE:
    {
        DWord ref = *(DWord*)(sp - 2);
        free(ref.Ptr);
        sp -= 2;
    }
    CONTINUE;
#pragma endregion
#pragma region Control flow
HANDLE_JMP:
    {
        i16 o = iNextI16(&pc);
        pc += o;
    }
    CONTINUE;
HANDLE_JMP_IF:
    {
        i16 o = iNextI16(&pc);
        if((sp - 1)->Int)
            pc += o;
        sp -= 1;
    }
    CONTINUE;
HANDLE_CALL:
    FunctionHeader *header;
    {
        u16 f = iNextU16(&pc);
        header = &functionPool[f]->Header;
    }
    goto CALL_HEADER;
HANDLE_INDCALL:
    header = (FunctionHeader*)((DWord*)(sp - 2))->Ptr;
CALL_HEADER:
/*
    [ pc_low ] [ pc_high ] [ fp_low ] [ fp_high ] [ mt_low ] [ mt_high ] [ awc | swc ] 
    [ local0 ] ... [ localN ] 
    [ stack0 ] ... [ stackM ]
*/
    {        
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
HANDLE_SYSCALL:
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
HANDLE_RET:
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