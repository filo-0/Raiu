#pragma once

#include "raiu/types.h"

struct _Function;

/**
 * @brief The metatable of a module, contains all the pools associated to a module.
 * 
 * @param WordPool A pointer to the first word in the global word buffer
 * @param DWrodPool A pointer to the first dword in the global dword buffer
 * @param FunctionPool A buffer of pointers the the functions of the module
 * @param StringPool A buffer of pointers to the constant strings of the module
 * @param GlobalPool A buffer of pointers to the global variables ot the module
 * @param WordPoolSize The size of the word pool
 * @param DWordPoolSize The size of the dword pool
 * @param FunctionPoolSize The size of the function pool
 * @param StringPoolSize The size of the string pool
 * @param GlobalPoolSize The size of the global pool
 */
typedef struct _ModuleTable
{
    // direct buffers
    Word  *WordPool;
    DWord *DWordPool;

    // indirect buffers (Need to be freed)
    struct _Function **FunctionPool;
    char             **StringPool;
    void             **GlobalPool;
     
    u16 WordPoolSize;
    u16 DWordPoolSize;
    u16 FunctionPoolSize;
    u16 StringPoolSize;
    u16 GlobalPoolSize;
} ModuleTable;

/**
 * @brief The header of a function.
 * @param Signature A reference to the function signature in the global debug string buffer
 * @param MT A reference to the module metadata table
 * @param AWC The Argument Word Count of the function
 * @param LWC The Local Word Count of the function
 * @param SWC The Stack Word Count of the function
 * @param RWC The Return Word Count of the function
 */
typedef struct _FunctionHeader
{
    ModuleTable *MT;
    char *Signature;
    // u64 ValidationCertificate
    u16 AWC;
    u16 LWC;
    u16 SWC;
    u16 RWC;
} FunctionHeader;

/**
 * @brief This struct represents the layout of a functon, first there is the header, then the bytecode instructions,
 * the body is considered to be immediately after the structure in memory.
 */
typedef struct _Function
{
    FunctionHeader Header;
    u8 Body[];
} Function;

/**
 * @brief The program context is the root that contains all the data buffers of the program, therefore is responsible for the 
 * creation and delition of them
 * 
 * @param EntryPoint A reference to the header of the main function, this is used only at startup
 * @param StackBottom The stack buffer
 * @param StackTop The upper limit of the stack
 * @param WordsBuffer The global word buffer
 * @param DWordsBuffer The global dword buffer
 * @param FunctionsBuffer The global functions buffer
 * @param StringsBuffer The global string buffer
 * @param GlobalsBuffer The global data buffer
 * @param ModuleTablesBuffer The buffer of all module metadata tables
 * @param WordsBufferSize The size in words of the word buffer
 * @param DWordsBufferSize The size in double words of the dword buffer
 * @param FunctionsBufferSize The size in bytes of the function buffer
 * @param GlobalsBufferSize The size in bytes ot the global buffer
 * @param MouduleTableSize The amount of modules that the program has loaded
 */
typedef struct _ProgramContext
{
    Function *EntryPoint;
    Word *StackBottom; // need to be freed
    Word *StackTop;

    // data buffers (need to be freed)
    Word  *WordsBuffer;
    DWord *DWordsBuffer;
    Byte  *FunctionsBuffer;
    Byte  *StringsBuffer;
    Byte  *GlobalsBuffer;
    ModuleTable *ModuleTablesBuffer;

    sz WordsBufferSize;
    sz DWordsBufferSize;
    sz StringsBufferSize;
    sz FunctionsBufferSize;
    sz GlobalsBufferSize;
    sz ModuleTablesBufferSize;

    Byte *DebugStringsBuffer;
    sz    DebugStringsBufferSize;
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
    context->GlobalsBuffer      = NULL;
    context->ModuleTablesBuffer = NULL;
    context->WordsBufferSize        = 0;
    context->DWordsBufferSize       = 0;
    context->StringsBufferSize      = 0;
    context->FunctionsBufferSize    = 0;
    context->GlobalsBufferSize      = 0;
    context->ModuleTablesBufferSize = 0;
}
