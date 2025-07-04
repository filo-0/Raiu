#pragma once

#include "raiu/raiu.h"
#include <stdbool.h>
#include <string.h>
#include "metadata.h"

#define LIST_T Word
#include "raiu/list.h"

#define LIST_T DWord
#include "raiu/list.h"

#define LIST_T String
#define LIST_T_DTOR String_Destroy
#include "raiu/list.h"

typedef struct _FunctionData
{
    u16 AWC;
    u16 LWC;
    u16 SWC;
    u16 RWC;
    u8 *Body;
    sz  Size;
} FunctionData;
static inline void FunctionData_Destroy(FunctionData *data) { free(data->Body); }

#define LIST_T FunctionData
#define LIST_T_DTOR FunctionData_Destroy
#define LIST_NAME FunctionList
#include "raiu/list.h"

#define LIST_T sz
#include "raiu/list.h"

/**
 * A link time struct that holds the data of a module to link
 */
typedef struct _ModuleData
{
    // all buffers must be freed
    List_Word Words;
    List_DWord DWords;
    List_String Strings;
    List_String InternalFunctions;
    List_String ExternalFunctions;
    FunctionList FunctionDefinitions;
    List_String InternalGlobals;
    List_String ExternalGlobals;
    List_sz     GlobalSizes;
} ModuleData;

static inline void ModuleData_Create(ModuleData *data)
{ 
    List_Word_Create(&data->Words);
    List_DWord_Create(&data->DWords);
    List_String_Create(&data->Strings);
    List_String_Create(&data->InternalFunctions);
    List_String_Create(&data->ExternalFunctions);
    FunctionList_Create(&data->FunctionDefinitions); 
    List_String_Create(&data->InternalGlobals);
    List_String_Create(&data->ExternalGlobals);   
    List_sz_Create(&data->GlobalSizes);
}
static inline void ModuleData_Destroy(ModuleData *data)
{
    List_Word_Destroy(&data->Words);
    List_DWord_Destroy(&data->DWords);
    List_String_Destroy(&data->Strings);
    List_String_Destroy(&data->InternalFunctions);
    List_String_Destroy(&data->ExternalFunctions);
    FunctionList_Destroy(&data->FunctionDefinitions);
    List_String_Destroy(&data->InternalGlobals);
    List_String_Destroy(&data->ExternalGlobals);  
    List_sz_Destroy(&data->GlobalSizes);
}

#define LIST_T ModuleData
#define LIST_T_DTOR ModuleData_Destroy
#define LIST_NAME LinkData
#include "raiu/list.h"

/**
 * @brief Checks if a function is valid, the requirements for a function to be valid are:
 * 
 * - The AWC is less than the LWC
 * 
 * - The stack pointer never exceeds the bounds [0, SWC]
 * 
 * - The instructions are valid opcodes
 * 
 * - The system calls are valid opcodes
 * 
 * - The local scope is accessed inside its limits [0, LWC]
 * 
 * - The pools are accessed inside their limits
 * 
 * The parameters instruction and sp are used when the function has branching logic, and all the path must be validated.
 *  
 * @attention The only case where the validity of a function is trusted 
 * is when the function uses the IND_CALL opcode (call through a function pointer)
 * 
 * @param header The function header 
 * @param instruction The instruction to start from, by default should be NULL
 * @param sp The current stack pointer, by default should be 0
 * @return 0 if the function is valid, 1 otherwise
 */
i32 Validate(const Function *function, const u8 *instruction, i32 sp);
