#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>

#include "linker.h"
#include "rvm.h"
#include "raiu/raiu.h"
#include "metadata.h"

#define MAP_T void*
#define MAP_K String
#define MAP_K_DTOR String_Destroy
#define MAP_K_COPY String_Copy
#define MAP_NAME Map_String_Ptr
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
static const Byte *iGetSignatures(const Byte *ptr, const Byte *limitPtr, const String *pathSuffix, List_String *intList, List_String *extList) 
{
    if(ptr + 2 > limitPtr)
        return NULL;

    u16 totalCount = iReadHWord(&ptr).UInt;
    sz i = 0;
    List_String_Reserve(intList, totalCount);
    while(i < totalCount)
    {        
        const char *sPtr = (char*) ptr;
        sz len = iSafeStrlen(sPtr, limitPtr);
        if(len == STRLEN_ERROR)
            return NULL;

        String signature;
        String_Copy(&signature, pathSuffix);
        String_PushBack(&signature, '.');
        String_ConcatStr(&signature, sPtr);
        List_String_PushBack(intList, &signature);
        ptr += len + 1;        
        i++;

        if(ptr->Char == '&') // '&' is a separator
        {
            ptr += 2; // skip
            break;
        }
    }

    List_String_Reserve(extList, totalCount - i);
    while(i < totalCount)
    {
        const char *sPtr = (char*) ptr;
        sz len = iSafeStrlen(sPtr, limitPtr);
        if(len == STRLEN_ERROR)
            return NULL;

        String signature;
        String_Init(&signature);
        String_ConcatStr(&signature, sPtr);
        List_String_PushBack(extList, &signature);
        ptr += len + 1;
        i++;
    }

    return ptr;
}
static const Byte *iGetGlobalSizes(const Byte *ptr, const Byte *limitPtr, sz globalCount, List_sz *globalSizes)
{
    List_sz_Reserve(globalSizes, globalCount);
    for (size_t i = 0; i < globalCount; i++)
    {
        if(ptr + 4 > limitPtr)
            return NULL;
        sz size = (sz)*(u32*)ptr;
        List_sz_PushBack(globalSizes, &size);
        ptr += 4;
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

    ptr = iGetSignatures(ptr, bufferLimit, filepath, &moduleData.InternalGlobals, &moduleData.ExternalGlobals);
    if(!ptr)
    {
        error = LINKING_ERROR_INCOHERENT_FILE;
        goto RET;
    }

    ptr = iGetSignatures(ptr, bufferLimit, filepath, &moduleData.InternalFunctions, &moduleData.ExternalFunctions);
    if(!ptr)
    {
        error = LINKING_ERROR_INCOHERENT_FILE;
        goto RET;
    }

    ptr = iGetGlobalSizes(ptr, bufferLimit, moduleData.InternalGlobals.Count, &moduleData.GlobalSizes);
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
static sz iGetAlignement(sz size)
{
    sz alignement;
    if      (size > 4) alignement = 8;
    else if (size > 2) alignement = 4;
    else if (size > 1) alignement = 2;
    else /* size==1 */ alignement = 1;
    return alignement;
}
static i32  iAllocateContextBuffers(ProgramContext *context, const LinkData *linkData)
{
    #define DEFAULT_STACK_SIZE (1 << 22)
    sz wordBufferSize     = 0;
    sz dwordBufferSize    = 0;
    sz stringBufferSize   = 0;
    sz functionBufferSize = 0;
    sz globalsBufferSize  = 0;
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
        for (u32 j = 0; j < moduleData->GlobalSizes.Count; j++)
        {
            sz globalSize = *List_sz_AtRO(&moduleData->GlobalSizes, j);
            sz alignement = iGetAlignement(globalSize);
            
            globalsBufferSize += globalsBufferSize % alignement ? alignement - (globalsBufferSize % alignement): 0; // align to alignement
            globalsBufferSize += globalSize;
        }   
    }
    
    context->StackBottom = calloc(DEFAULT_STACK_SIZE, sizeof(Word));
    context->StackTop    = context->StackBottom + DEFAULT_STACK_SIZE / sizeof(Word);
    
    context->WordsBuffer        = wordBufferSize     ? (Word*)  calloc(wordBufferSize  , sizeof(Word))   : NULL;
    context->DWordsBuffer       = dwordBufferSize    ? (DWord*) calloc(dwordBufferSize , sizeof(DWord))  : NULL;
    context->StringsBuffer      = stringBufferSize   ? (Byte*)  calloc(stringBufferSize, sizeof(Byte))   : NULL;
    context->GlobalsBuffer      = globalsBufferSize  ? (Byte*)  calloc(globalsBufferSize, sizeof(Byte))  : NULL;
    context->FunctionsBuffer    = (Byte*)        calloc(functionBufferSize, sizeof(Byte));
    context->ModuleTablesBuffer = (ModuleTable*) calloc(linkData->Count   , sizeof(ModuleTable));


    context->WordsBufferSize        = wordBufferSize;
    context->DWordsBufferSize       = dwordBufferSize;
    context->StringsBufferSize      = stringBufferSize;
    context->FunctionsBufferSize    = functionBufferSize;
    context->GlobalsBufferSize      = globalsBufferSize;
    context->ModuleTablesBufferSize = linkData->Count; 

    if(
        (                                     context->StackBottom        == NULL) ||
        (                                     context->ModuleTablesBuffer == NULL) ||
        (                                     context->FunctionsBuffer    == NULL) ||
        (context->WordsBufferSize     != 0 && context->WordsBuffer        == NULL) ||
        (context->DWordsBufferSize    != 0 && context->DWordsBuffer       == NULL) ||
        (context->StringsBufferSize   != 0 && context->StringsBuffer      == NULL) ||
        (context->GlobalsBufferSize   != 0 && context->GlobalsBuffer      == NULL)
    )
    {
        LOG_ERROR("Linked : Failed to allocate application data!");
        return 1;
    }
    return 0;
}
static void iFillBuffers(ProgramContext *context, Map_String_Ptr *functionMap, Map_String_Ptr *globalMap, const LinkData *linkData)
{
    Word  *wordPtr     = context->WordsBuffer;
    DWord *dwordPtr    = context->DWordsBuffer;
    Byte  *stringPtr   = context->StringsBuffer;
    Byte  *globalPtr   = context->GlobalsBuffer;
    Byte  *functionPtr = context->FunctionsBuffer;

    for (u32 i = 0; i < linkData->Count; i++)
    {
        const ModuleData *moduleData = LinkData_AtRO(linkData, i);

        memcpy(wordPtr, moduleData->Words.Data, moduleData->Words.Count * sizeof(Word));
        wordPtr += moduleData->Words.Count;

        memcpy(dwordPtr, moduleData->DWords.Data, moduleData->DWords.Count * sizeof(DWord));
        dwordPtr += moduleData->DWords.Count;

        for (u32 j = 0; j < moduleData->Strings.Count; j++)
        {
            const String *string = List_String_AtRO(&moduleData->Strings, j);
            char *stringLocation = (char*)stringPtr; 
            memcpy(stringLocation, String_CStr(string), string->Length);
            stringLocation[string->Length] = 0;   
            stringPtr += string->Length + 1;
        }

        for (u32 j = 0; j < moduleData->GlobalSizes.Count; j++)
        {
            const String *globalSignature =  List_String_AtRO(&moduleData->InternalGlobals, j);
            sz            globalSize      = *List_sz_AtRO(&moduleData->GlobalSizes, j);
            sz alignement = iGetAlignement(globalSize);
            if((sz)globalPtr % alignement)
                globalPtr += alignement - ((sz)globalPtr % alignement); // align
            Map_String_Ptr_PutCopy(globalMap, globalSignature, (void**)&globalPtr);
            globalPtr += globalSize;
        }
        

        for (u32 j = 0; j < moduleData->FunctionDefinitions.Count; j++)
        {
            const String *functionSignature  = List_String_AtRO(&moduleData->InternalFunctions, j);
            const FunctionData *functionData = FunctionList_AtRO(&moduleData->FunctionDefinitions, j);
            Function *functionLoc = (Function*) functionPtr;
            functionLoc->Header.MT = NULL;
            memcpy((Byte*)functionLoc + 8, functionData->Func, functionData->Size);

            Map_String_Ptr_PutCopy(functionMap, functionSignature, (void**)&functionLoc);
            functionPtr += functionData->Size + 8;
            if((sz)functionPtr % 8) // allign
                functionPtr += 8 - ((sz)functionPtr % 8);
        }
    }
}
static i32  iSetPools(ProgramContext *context, const Map_String_Ptr *functionMap, const Map_String_Ptr *globalMap, const LinkData *linkData)
{
    sz wordIterator  = 0;
    sz dwordIterator = 0;
    for (u32 i = 0; i < linkData->Count; i++)
    {
        const ModuleData *moduleData = LinkData_AtRO(linkData, i);
        ModuleTable *moduleTable = context->ModuleTablesBuffer + i;

        // word and dword pools are direct buffers
        moduleTable->WordPool     = context->WordsBuffer + wordIterator;
        moduleTable->WordPoolSize = moduleData->Words.Count;
        wordIterator += moduleData->Words.Count;

        moduleTable->DWordPool     = context->DWordsBuffer + dwordIterator;
        moduleTable->DWordPoolSize = moduleData->DWords.Count;
        dwordIterator += moduleData->DWords.Count;

        // string, global and function pools are indirect buffers
        moduleTable->StringPoolSize = moduleData->Strings.Count;
        moduleTable->StringPool     = moduleTable->StringPoolSize ? calloc(moduleTable->StringPoolSize, sizeof(char*)) : NULL;
        char *s = (char*)context->StringsBuffer;
        for (u32 j = 0; j < moduleData->Strings.Count; j++)
        {
            moduleTable->StringPool[j] = s;
            s += strlen(s) + 1;
        }

        moduleTable->GlobalPoolSize = moduleData->InternalGlobals.Count + moduleData->ExternalGlobals.Count;
        moduleTable->GlobalPool = moduleTable->GlobalPoolSize ? calloc(moduleTable->GlobalPoolSize, sizeof(void*)) : NULL;
        for (u32 i = 0; i < moduleTable->GlobalPoolSize; i++)
        {
            const String *globSignature = List_String_AtRO(i < moduleData->InternalGlobals.Count ? &moduleData->InternalGlobals : &moduleData->ExternalGlobals, i);
            void *const *globalLoc = Map_String_Ptr_AtRO(globalMap, globSignature);
            if(!globalLoc)
            {
                DEVEL_ASSERT(false, "Linker : Cannot find global %s", String_CStr(globSignature));
                return 1;
            }
            moduleTable->GlobalPool[i] = *globalLoc;
        }

        moduleTable->FunctionPoolSize = moduleData->InternalFunctions.Count + moduleData->ExternalFunctions.Count;
        moduleTable->FunctionPool = moduleTable->FunctionPoolSize ? calloc(moduleTable->FunctionPoolSize, sizeof(void*)) : NULL;
        for (u32 i = 0; i < moduleData->InternalFunctions.Count; i++)
        {
            const String *funcSignature = List_String_AtRO(&moduleData->InternalFunctions, i);
            moduleTable->FunctionPool[i] = *Map_String_Ptr_AtRO(functionMap, funcSignature);
            moduleTable->FunctionPool[i]->Header.MT = moduleTable;
        }
        for (u32 i = 0; i < moduleData->ExternalFunctions.Count; i++)
        {
            const String *funcSignature = List_String_AtRO(&moduleData->ExternalFunctions, i);
            Function *const *funcLoc = (Function**) Map_String_Ptr_AtRO(functionMap, funcSignature);
            if(funcLoc == NULL)
            {
                DEVEL_ASSERT(false, "Linker : Cannot find function %s", String_CStr(funcSignature));
                return LINKING_ERROR_FUNCTION_NOT_FOUND;
            }
            moduleTable->FunctionPool[i + moduleData->InternalFunctions.Count] = *funcLoc;
        }
    }
    return 0;
}
static i32  iSetEntryPoint(ProgramContext *context, const Map_String_Ptr *functionMap, const String *rootpath)
{
    String mainSignature;
    String_Copy     (&mainSignature, rootpath   );
    String_ConcatStr(&mainSignature, "/Main.Main");

    Function *const *mainLocation = (Function**) Map_String_Ptr_AtRO(functionMap, &mainSignature);
    if(!mainLocation)
    {
        String_Destroy(&mainSignature);
        return LINKING_ERROR_NO_MAIN;
    }
    
    context->EntryPoint = &(*mainLocation)->Header;

    String_Destroy(&mainSignature);
    return 0;
}

i32 Link(ProgramContext *context, const String *rootpath)
{
    i32 error = 0;    
    LinkData linkData;
    Map_String_Ptr functionMap;
    Map_String_Ptr globalMap;
    LinkData_Create(&linkData);
    Map_String_Ptr_Create(&functionMap);
    Map_String_Ptr_Create(&globalMap);

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

    iFillBuffers(context, &functionMap, &globalMap, &linkData);

    i32 functionNotFound = iSetPools(context, &functionMap, &globalMap, &linkData);
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
    foreach(Map_String_Ptr, functionMap)
    {
        const Map_String_Ptr_Pair *p = Map_String_Ptr_Iterator_AccessRO(&i);
        Function *f = (Function*)p->Val;
        int validationError = Validate(&p->Key ,&f->Header, f->Body, 0);    
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
    Map_String_Ptr_Destroy(&functionMap);
    Map_String_Ptr_Destroy(&globalMap);
    LinkData_Destroy(&linkData);
    return error;
}

void Unlink(ProgramContext *context)
{
    for (u32 i = 0; i < context->ModuleTablesBufferSize; i++)
    {
        free(context->ModuleTablesBuffer[i].StringPool);
        free(context->ModuleTablesBuffer[i].FunctionPool);
        free(context->ModuleTablesBuffer[i].GlobalPool);
    }

    free(context->ModuleTablesBuffer);
    free(context->WordsBuffer);
    free(context->DWordsBuffer);
    free(context->StringsBuffer);
    free(context->FunctionsBuffer);
    free(context->GlobalsBuffer);
    free(context->StackBottom);
}