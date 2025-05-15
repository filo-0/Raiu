#pragma once

#include "raiu/types.h"

struct _Function;
typedef struct _ModuleTable
{
    // direct buffers
    Word      *WordPool;
    DWord     *DWordPool;

    // indirect buffers (Need to be freed)
    struct _Function **FunctionPool;
    ch8              **StringPool;
    
    u16 WordPoolSize;
    u16 DWordPoolSize;
    u16 FunctionPoolSize;
    u16 StringPoolSize;
} ModuleTable;

typedef struct _FunctionHeader
{
    ModuleTable *MT;
    u16 AWC;
    u16 LWC;
    u16 SWC;
    u16 RWC;
} FunctionHeader;
typedef struct _Function
{
    FunctionHeader Header;
    u8 Body[];
} Function;

typedef struct _ProgramContext
{
    FunctionHeader *EntryPoint;
    Word *StackBottom; // need to be freed
    Word *StackTop;

    // data buffers (need to be freed)
    Word  *WordsBuffer;
    DWord *DWordsBuffer;
    Byte  *FunctionsBuffer;
    Byte  *StringsBuffer;
    ModuleTable *ModuleTablesBuffer;

    sz WordsBufferSize;
    sz DWordsBufferSize;
    sz StringsBufferSize;
    sz FunctionsBufferSize;
    sz ModuleTablesBufferSize;

} ProgramContext;
static inline void ProgramContext_Init(ProgramContext *context)
{
    context->StackBottom = NULL;
    context->StackTop    = NULL;
    context->EntryPoint  = NULL;
    context->WordsBuffer        = NULL;
    context->DWordsBuffer       = NULL;
    context->StringsBuffer      = NULL;
    context->FunctionsBuffer    = NULL;
    context->ModuleTablesBuffer = NULL;
    context->WordsBufferSize        = 0;
    context->DWordsBufferSize       = 0;
    context->StringsBufferSize      = 0;
    context->FunctionsBufferSize    = 0;
    context->ModuleTablesBufferSize = 0;
}
