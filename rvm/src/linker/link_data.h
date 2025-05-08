#pragma once

#include <stdbool.h>
#include <string.h>
#include "metadata.h"


#define LIST_T Word
#include "raiu/list.h"
#undef LIST_T

#define LIST_T DWord
#include "raiu/list.h"
#undef LIST_T

#define LIST_T char
#include "raiu/list.h"
#undef LIST_T

#define LIST_T Byte
#include "raiu/list.h"
#undef LIST_T

typedef struct _FunctionData
{
    u8 *Func;
    sz  Size;
} FunctionData;
typedef struct _ModuleData
{
    // all buffers must be freed
    List_Word Words;
    List_DWord DWords;
    List_char Strings;
    List_char InternalFunctions;
    List_char ExternalFunctions;
    List_Byte FunctionDefinitions;
} ModuleData;
static inline void ModuleData_Create(ModuleData *data)
{ 
    List_Word_Create(&data->Words);
    List_DWord_Create(&data->DWords);
    List_char_Create(&data->Strings);
    List_char_Create(&data->InternalFunctions);
    List_char_Create(&data->ExternalFunctions);
    List_Byte_Create(&data->FunctionDefinitions); 
}
static inline void ModuleData_Destroy(ModuleData *data)
{
    List_Word_Destroy(&data->Words);
    List_DWord_Destroy(&data->DWords);
    List_char_Destroy(&data->Strings);
    List_char_Destroy(&data->InternalFunctions);
    List_char_Destroy(&data->ExternalFunctions);
    List_Byte_Destroy(&data->FunctionDefinitions);
}

#define LIST_T ModuleData
#define LIST_T_DTOR ModuleData_Destroy
#include <raiu/list.h>

static inline bool StrEQ(const char **a, const char **b) { return strcmp(*a, *b) == 0; }
static inline u32 StrHash(const char **s)
{
    const char *str = *s;
    u32 hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return hash;
}
typedef const char *str;
typedef Byte       *addr;

#define MAP_K str
#define MAP_T addr
#define MAP_HASH StrHash
#define MAP_EQ StrEQ
#include <raiu/map.h>

typedef struct _LinkData
{
    List_ModuleData    ModuleList;
    Map_str_addr       FunctionMap;
} LinkData;
static inline void LinkData_Create(LinkData *data)
{
    List_ModuleData_Create(&data->ModuleList);
    Map_str_addr_Create(&data->FunctionMap);
}
static inline void LinkData_Destroy(LinkData *data)
{
    List_ModuleData_Destroy(&data->ModuleList);
    Map_str_addr_Destroy(&data->FunctionMap);
}