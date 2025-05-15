#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>

#include "linker.h"
#include "rvm.h"
#include "raiu/raiu.h"
#include "metadata.h"

#define MAP_T char*
#define MAP_K String
#define MAP_K_DTOR String_Destroy
#define MAP_K_COPY String_Copy
#define MAP_NAME StringMap
#define MAP_HASH String_Hash
#define MAP_EQ String_Equal
#include "raiu/map.h"

#define MAP_T Function*
#define MAP_K String
#define MAP_K_DTOR String_Destroy
#define MAP_K_COPY String_Copy
#define MAP_NAME FunctionMap
#define MAP_HASH String_Hash
#define MAP_EQ String_Equal
#include "raiu/map.h"

#define STRLEN_ERROR ((sz) -1)
static sz iSafeStrlen(const char* s, const void *limit)
{
    sz i = 0;
    while(*s != 0)
    {
        if(s >= (char*)limit)
            return STRLEN_ERROR;
        i++;
        s++;
    }
    return i;
}
static inline HWord iReadHWord(const Byte **p) { HWord h = *(HWord*)*p; *p += 2; return h; }
static inline Word  iReadWord(const Byte  **p) { Word  w = *(Word*) *p; *p += 4; return w; }
static inline DWord iReadDWord(const Byte **p) { DWord d = *(DWord*)*p; *p += 8; return d; }

static const Byte *iGetWordPool(const Byte *ptr, const Byte *limitPtr, List_Word *wordList)
{
    if(ptr + 2 > limitPtr)
        return NULL;

    u16 wordCount = iReadHWord(&ptr).UInt;
    List_Word_Reserve(wordList, wordCount);
    for (u16 i = 0; i < wordCount; i++)
    {
        if(ptr + 4 > limitPtr)
            return NULL;

        Word w = iReadWord(&ptr);
        List_Word_PushBack(wordList, &w);
    }
    return ptr;
}
static const Byte *iGetDWordPool(const Byte *ptr, const Byte *limitPtr, List_DWord *dwordList)
{
    if(ptr + 2 > limitPtr)
        return NULL;

    u16 dwordCount = iReadHWord(&ptr).UInt;
    List_DWord_Reserve(dwordList, dwordCount);
    for (u16 i = 0; i < dwordCount; i++)
    {
        if(ptr + 8 > limitPtr)
            return NULL;

        DWord d = iReadDWord(&ptr);
        List_DWord_PushBack(dwordList, &d);
    }
    return ptr;
}
static const Byte *iGetStringPool(const Byte *ptr, const Byte *limitPtr, List_String *stringList)
{
    if(ptr + 2 > limitPtr)
        return NULL;   
    u16 stringCount = iReadHWord(&ptr).UInt;
    List_String_Reserve(stringList, stringCount);
    for (u16 i = 0; i < stringCount; i++)
    {        
        const char *sPtr = (char*)ptr;
        sz len = iSafeStrlen(sPtr, limitPtr);
        if(len == STRLEN_ERROR)
            return NULL;

        String s;
        String_Init(&s);
        String_ConcatStr(&s, sPtr);
        ptr += len + 1;
        List_String_PushBack(stringList, &s);
    }
    return ptr;
}
static const Byte *iGetFunctionSignatures(const Byte *ptr, const Byte *limitPtr, const String *pathSuffix, List_String *intFuncs, List_String *extFuncs) 
{
    if(ptr + 2 > limitPtr)
        return NULL;

    u16 functionCount = iReadHWord(&ptr).UInt;
    sz i = 0;
    List_String_Reserve(intFuncs, functionCount);
    while(i < functionCount)
    {        
        const char *sPtr = (char*) ptr;
        sz len = iSafeStrlen(sPtr, limitPtr);
        if(len == STRLEN_ERROR)
            return NULL;

        String signature;
        String_Copy(&signature, pathSuffix);
        String_PushBack(&signature, '.');
        String_ConcatStr(&signature, sPtr);
        List_String_PushBack(intFuncs, &signature);
        ptr += len + 1;        
        i++;

        if(ptr->Char == '&') // '&' is a separator
        {
            ptr += 2; // skip
            break;
        }
    }

    List_String_Reserve(extFuncs, functionCount - i);
    while(i < functionCount)
    {
        const char *sPtr = (char*) ptr;
        sz len = iSafeStrlen(sPtr, limitPtr);
        if(len == STRLEN_ERROR)
            return NULL;

        String signature;
        String_Init(&signature);
        String_ConcatStr(&signature, sPtr);
        List_String_PushBack(extFuncs, &signature);
        ptr += len + 1;
        i++;
    }

    return ptr;
}
static const Byte *iGetFunctionDefinitions(const Byte *ptr, const Byte *limitPtr, sz functionCount, FunctionList *functionDefinitions)
{
    for (u16 i = 0; i < functionCount; i++)
    {
        if(ptr + 4 > limitPtr)
            return NULL;
        u32 bytes = iReadWord(&ptr).UInt;

        if(ptr + bytes > limitPtr)
            return NULL;
        FunctionData func = { (u8*) malloc(bytes), bytes };
        memcpy(func.Func, ptr, bytes);

        FunctionList_PushBack(functionDefinitions, &func);       

        ptr += bytes;
    }
    return ptr;
}

#define FAILED_TO_OPEN_DIR   -1
#define FAILED_TO_OPEN_FILE  -2
#define LINKING_ERROR_INCOHERENT_FILE 1
#define LINKING_ERROR_FUNCTION_NOT_FOUND 2
#define LINKING_ERROR_NO_MAIN 3

static i32 iUpdateLinkData(LinkData *linkData, const String *filepath)
{
    FILE *file = fopen(String_CStr(filepath), "rb");
    if(!file)
    {
        LOG_ERROR("Linker : Failed to open file %s", String_CStr(filepath));
        return FAILED_TO_OPEN_FILE;
    }
    
    ModuleData moduleData;
    ModuleData_Create(&moduleData);
    
    // get file size
    fseek(file, 0L, SEEK_END);
    sz fileSize = ftell(file);
    fseek(file, 0L, SEEK_SET);

    i32 error = 0;
    Byte *buffer = malloc(fileSize);
    fread(buffer, fileSize, 1, file); // get all file data
    const Byte *ptr = buffer;
    const Byte *bufferLimit = buffer + fileSize;

    ptr = iGetWordPool(ptr, bufferLimit, &moduleData.Words);
    if(!ptr)
    {
        error = LINKING_ERROR_INCOHERENT_FILE;
        goto RET;
    }

    ptr = iGetDWordPool(ptr, bufferLimit, &moduleData.DWords);
    if(!ptr)
    {
        error = LINKING_ERROR_INCOHERENT_FILE;
        goto RET;
    }
    
    ptr = iGetStringPool(ptr, bufferLimit, &moduleData.Strings);
    if(!ptr)
    {
        error = LINKING_ERROR_INCOHERENT_FILE;
        goto RET;
    }


    ptr = iGetFunctionSignatures(ptr, bufferLimit, filepath, &moduleData.InternalFunctions, &moduleData.ExternalFunctions);
    if(!ptr)
    {
        error = LINKING_ERROR_INCOHERENT_FILE;
        goto RET;
    }

    ptr = iGetFunctionDefinitions(ptr, bufferLimit, moduleData.InternalFunctions.Count, &moduleData.FunctionDefinitions);
    if(!ptr)
        error = LINKING_ERROR_INCOHERENT_FILE;

    if(ptr != bufferLimit)
        error = LINKING_ERROR_INCOHERENT_FILE;
RET:
    free(buffer);
    LinkData_PushBack(linkData, &moduleData);
    fclose(file);
    
    switch (error)
    {
    case LINKING_ERROR_INCOHERENT_FILE:
        LOG_ERROR("Linker : Incoherent file");
        break;
    }
    return error;
}
static i32 iGetLinkData(LinkData *linkData, const String *dirpath)
{
    DIR *directory = opendir(String_CStr(dirpath));
    if(!directory)
    {
        LOG_ERROR("Linker : Failed to open directory %s", String_CStr(dirpath));
        return FAILED_TO_OPEN_DIR;
    }

    struct dirent *entry;
    while((entry = readdir(directory)) != NULL)
    {
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        String nextPath;
        String_Copy(&nextPath, dirpath);
        String_PushBack(&nextPath, '/');
        String_ConcatStr(&nextPath, entry->d_name);
        if(entry->d_type == DT_REG) // file
        {
            i32 error = iUpdateLinkData(linkData, &nextPath);
            if(error)
                return error;
        }
        else if (entry->d_type == DT_DIR)
        {

            i32 error = iGetLinkData(linkData, &nextPath);
            if(error)
                return error;
        }

        String_Destroy(&nextPath);
    }

    closedir(directory);
    return 0;
}

static i32  iAllocateContextBuffers(ProgramContext *context, const LinkData *linkData)
{
    #define DEFAULT_STACK_SIZE (1 << 22)
    sz wordBufferSize     = 0;
    sz dwordBufferSize    = 0;
    sz stringBufferSize   = 0;
    sz functionBufferSize = 0;
    for (u32 i = 0; i < linkData->Count; i++)
    {
        const ModuleData *moduleData = LinkData_AtRO(linkData, i);

        wordBufferSize   += moduleData->Words.Count;
        dwordBufferSize  += moduleData->DWords.Count;
        for (u32 j = 0; j < moduleData->Strings.Count; j++)
            stringBufferSize += List_String_AtRO(&moduleData->Strings, j)->Length + 1;
        for (u32 j = 0; j < moduleData->FunctionDefinitions.Count; j++)
        {
            functionBufferSize += functionBufferSize % 8 ? 8 - (context->FunctionsBufferSize % 8) : 0; // allign to 8 bytes
            functionBufferSize += FunctionList_AtRO(&moduleData->FunctionDefinitions, j)->Size + 8; // 8 bytes for module table pointer
        }
    }
    
    
    context->StackBottom = calloc(DEFAULT_STACK_SIZE, sizeof(Word));
    context->StackTop    = context->StackBottom + DEFAULT_STACK_SIZE / sizeof(Word);
    
    context->WordsBuffer        = wordBufferSize     ? (Word*)  calloc(wordBufferSize  , sizeof(Word))   : NULL;
    context->DWordsBuffer       = dwordBufferSize    ? (DWord*) calloc(dwordBufferSize , sizeof(DWord))  : NULL;
    context->StringsBuffer      = stringBufferSize   ? (Byte*)  calloc(stringBufferSize, sizeof(Byte))   : NULL;
    context->FunctionsBuffer    = (Byte*)        calloc(functionBufferSize, sizeof(Byte));
    context->ModuleTablesBuffer = (ModuleTable*) calloc(linkData->Count   , sizeof(ModuleTable));


    context->WordsBufferSize        = wordBufferSize;
    context->DWordsBufferSize       = dwordBufferSize;
    context->StringsBufferSize      = stringBufferSize;
    context->FunctionsBufferSize    = functionBufferSize;
    context->ModuleTablesBufferSize = linkData->Count; 

    if(
        (                                     context->StackBottom        == NULL) ||
        (                                     context->ModuleTablesBuffer == NULL) ||
        (                                     context->FunctionsBuffer    == NULL) ||
        (context->WordsBufferSize     != 0 && context->WordsBuffer        == NULL) ||
        (context->DWordsBufferSize    != 0 && context->DWordsBuffer       == NULL) ||
        (context->StringsBufferSize   != 0 && context->StringsBuffer      == NULL)
    )
    {
        LOG_ERROR("Linked : Failed to allocate application data!");
        return 1;
    }
    return 0;
}
static void iSetWordAndDWordBuffers(ProgramContext *context, const LinkData *linkData)
{
    sz wordIterator = 0;
    sz dwordIterator = 0;

    for (u32 i = 0; i < linkData->Count; i++)
    {
        const ModuleData *moduleData = LinkData_AtRO(linkData, i);
        ModuleTable *moduleTable = context->ModuleTablesBuffer + i;

        moduleTable->WordPool     = context->WordsBuffer + wordIterator;
        moduleTable->WordPoolSize = moduleData->Words.Count;
        memcpy(moduleTable->WordPool, moduleData->Words.Data, moduleData->Words.Count * sizeof(Word));
        wordIterator += moduleData->Words.Count;

        moduleTable->DWordPool     = context->DWordsBuffer + dwordIterator;
        moduleTable->DWordPoolSize = moduleData->DWords.Count;
        memcpy(moduleTable->DWordPool, moduleData->DWords.Data, moduleData->DWords.Count * sizeof(DWord));
        dwordIterator += moduleData->DWords.Count;
    }
}
static void iSetFunctionAndStringBuffer(ProgramContext *context, FunctionMap *functionMap, const LinkData *linkData)
{
    sz functionIterator = 0;
    sz stringIterator   = 0;
    for (u32 i = 0; i < linkData->Count; i++)
    {
        const ModuleData *moduleData = LinkData_AtRO(linkData, i);
        
        for (u32 j = 0; j < moduleData->FunctionDefinitions.Count; j++)
        {
            const String *functionSignature  = List_String_AtRO(&moduleData->InternalFunctions, j);
            const FunctionData *functionData = FunctionList_AtRO(&moduleData->FunctionDefinitions, j);
            Function *functionLoc = (Function*) (context->FunctionsBuffer + functionIterator);
            functionLoc->Header.MT = NULL;
            memcpy((Byte*)functionLoc + 8, functionData->Func, functionData->Size);

            FunctionMap_PutCopy(functionMap, functionSignature, &functionLoc);
            functionIterator += functionData->Size + 8;
            if(functionIterator % 8) // allign
                functionIterator += 8 - (functionIterator % 8);
        }

        for (u32 j = 0; j < moduleData->Strings.Count; j++)
        {
            const String *string = List_String_AtRO(&moduleData->Strings, j);
            char *stringLocation = (char*)context->StringsBuffer + stringIterator; 
            memcpy(stringLocation, String_CStr(string), string->Length);
            stringLocation[string->Length] = 0;   
            stringIterator += string->Length + 1;
        }
    }
}
static i32 iSetFunctionAndStringPool(ProgramContext *context, const FunctionMap *functionMap, const LinkData *linkData)
{
    for (u32 i = 0; i < linkData->Count; i++)
    {
        const ModuleData *moduleData = LinkData_AtRO(linkData, i);
        ModuleTable *moduleTable = context->ModuleTablesBuffer + i;

        moduleTable->StringPoolSize = moduleData->Strings.Count;
        moduleTable->StringPool     = moduleTable->StringPoolSize ? calloc(moduleTable->StringPoolSize, sizeof(char*)) : NULL;
        char *s = (char*)context->StringsBuffer;
        for (u32 j = 0; j < moduleData->Strings.Count; j++)
        {
            moduleTable->StringPool[j] = s;
            s += strlen(s) + 1;
        }

        moduleTable->FunctionPoolSize = moduleData->InternalFunctions.Count + moduleData->ExternalFunctions.Count;
        moduleTable->FunctionPool = moduleTable->FunctionPoolSize ? calloc(moduleTable->FunctionPoolSize, sizeof(Function*)) : NULL;
        for (u32 i = 0; i < moduleData->InternalFunctions.Count; i++)
        {
            const String *funcSignature = List_String_AtRO(&moduleData->InternalFunctions, i);
            moduleTable->FunctionPool[i] = *FunctionMap_AtRO(functionMap, funcSignature);
            moduleTable->FunctionPool[i]->Header.MT = moduleTable;
        }
        for (u32 i = 0; i < moduleData->ExternalFunctions.Count; i++)
        {
            const String *funcSignature = List_String_AtRO(&moduleData->ExternalFunctions, i);
            Function *const *funcLoc = FunctionMap_AtRO(functionMap, funcSignature);
            if(funcLoc == NULL)
            {
                LOG_ERROR("Linker : Cannot find function %s", String_CStr(funcSignature));
                return LINKING_ERROR_FUNCTION_NOT_FOUND;
            }
            moduleTable->FunctionPool[i + moduleData->InternalFunctions.Count] = *funcLoc;
        }
    }
    return 0;
}
i32 iSetEntryPoint(ProgramContext *context, const FunctionMap *functionMap, const String *rootpath)
{
    String mainSignature;
    String_Copy     (&mainSignature, rootpath   );
    String_ConcatStr(&mainSignature, "/Main.Main");

    Function *const *mainLocation = FunctionMap_AtRO(functionMap, &mainSignature);
    if(!mainLocation)
    {
        String_Destroy(&mainSignature);
        return LINKING_ERROR_NO_MAIN;
    }
    
    context->EntryPoint = &(*mainLocation)->Header;

    String_Destroy(&mainSignature);
    return 0;
}

// TODO: make this code mode clear
void ValidateFunction(String *functionName, Function **function, void *valid)
{
    if(*(bool*)valid)
    {
        int validationError = Validate(functionName ,&(*function)->Header, (*function)->Body, 0);    
        if(validationError)
            *(bool*)valid = false; 
    }
}
i32 Link(ProgramContext *context, const String *rootpath)
{
    i32 error = 0;    
    LinkData linkData;
    FunctionMap functionMap;
    LinkData_Create(&linkData);
    FunctionMap_Create(&functionMap);


    i32 getError = iGetLinkData(&linkData, rootpath);
    if(getError)
    {
        error = getError;
        goto RET;
    }

    // calculate buffer sizes
    i32 allocError = iAllocateContextBuffers(context, &linkData);
    if(allocError)
    {
        error = allocError;
        goto RET;
    }

    iSetWordAndDWordBuffers(context, &linkData);

    iSetFunctionAndStringBuffer(context, &functionMap, &linkData);
    i32 functionNotFound = iSetFunctionAndStringPool(context, &functionMap, &linkData);
    if(functionNotFound)
    {
        error = functionNotFound;
        goto RET;
    }

    // find main function
    i32 mainFuncNotFound = iSetEntryPoint(context, &functionMap, rootpath);
    if(mainFuncNotFound)
        error = mainFuncNotFound;
    
    
    bool valid = true;
    // FunctionMap_ForEach(&functionMap, ValidateFunction, &valid);
    // if(!valid)
    // {
    //     error = 1;
    //     goto RET;
    // }

    foreach(FunctionMap, functionMap)
    {
        const FunctionMap_Pair *p = FunctionMap_Iterator_AccessRO(&i);
        int validationError = Validate(&p->Key ,&p->Val->Header, p->Val->Body, 0);    
        if(validationError)
        {
            valid = false; 
            break;
        }
    }
    if(!valid)
    {
        error = 1;
        goto RET;
    }
    
RET:
    FunctionMap_Destroy(&functionMap);
    LinkData_Destroy(&linkData);
    return error;
}

void Unlink(ProgramContext *context)
{
    for (u32 i = 0; i < context->ModuleTablesBufferSize; i++)
    {
        free(context->ModuleTablesBuffer[i].StringPool);
        free(context->ModuleTablesBuffer[i].FunctionPool);
    }

    free(context->ModuleTablesBuffer);
    free(context->WordsBuffer);
    free(context->DWordsBuffer);
    free(context->StringsBuffer);
    free(context->FunctionsBuffer);
    free(context->StackBottom);
}