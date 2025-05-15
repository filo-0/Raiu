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
    u8 *Func;
    sz  Size;
} FunctionData;
static inline void FunctionData_Destroy(FunctionData *data) { free(data->Func); }

#define LIST_T FunctionData
#define LIST_T_DTOR FunctionData_Destroy
#define LIST_NAME FunctionList
#include "raiu/list.h"

typedef struct _ModuleData
{
    // all buffers must be freed
    List_Word Words;
    List_DWord DWords;
    List_String Strings;
    List_String InternalFunctions;
    List_String ExternalFunctions;
    FunctionList FunctionDefinitions;
} ModuleData;
static inline void ModuleData_Create(ModuleData *data)
{ 
    List_Word_Create(&data->Words);
    List_DWord_Create(&data->DWords);
    List_String_Create(&data->Strings);
    List_String_Create(&data->InternalFunctions);
    List_String_Create(&data->ExternalFunctions);
    FunctionList_Create(&data->FunctionDefinitions); 
}
static inline void ModuleData_Destroy(ModuleData *data)
{
    List_Word_Destroy(&data->Words);
    List_DWord_Destroy(&data->DWords);
    List_String_Destroy(&data->Strings);
    List_String_Destroy(&data->InternalFunctions);
    List_String_Destroy(&data->ExternalFunctions);
    FunctionList_Destroy(&data->FunctionDefinitions);
}

#define LIST_T ModuleData
#define LIST_T_DTOR ModuleData_Destroy
#define LIST_NAME LinkData
#include "raiu/list.h"

i32 Validate(const String *functionSignature, const FunctionHeader *header, const u8 *instruction, i32 sp);
