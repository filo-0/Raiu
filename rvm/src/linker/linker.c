#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>

#include "vm.h"
#include "raiu/types.h"
#include "raiu/opcodes.h"
#include "metadata.h"
#include "link_data.h"
#include "raiu/string.h"

Byte SIMPLE_PROGRAM[] = 
{
    { OP_CALL }, { 0 }, { 0 },
    { OP_SYSCALL }, { OP_SYS_EXIT },
    { 0 }, { 0 }, { 0 }, // padding

    // header
    { 0 }, { 0 }, // awc
    { 3 }, { 0 }, // lwc
    { 2 }, { 0 }, // swc
    { 0 }, { 0 }, // rwc
    { OP_PUSH_0_WORD },
    { OP_POP_WORD_0 },
    { OP_PUSH_0_DWORD },
    { OP_POP_DWORD_1 },
    { OP_JMP }, { 8 }, { 0 },
    { OP_PUSH_DWORD_1 },
    { OP_PUSH_I64 }, { 32 },
    { OP_ADD_I64 },
    { OP_POP_DWORD_1 },
    { OP_INC_I32 }, { 0 }, { 1 },
    { OP_PUSH_WORD_0 },
    { OP_PUSH_CONST_WORD }, { 0 },
    { OP_CMP_I32_LT },
    { OP_JMP_IF }, { 0xf1 }, { 0xff },
    { OP_PUSH_DWORD_1 },
    { OP_I64_TO_I32 },
    { OP_SYSCALL }, { OP_SYS_EXIT }
};
Byte SIMPLE_FOR[] = 
{
    { OP_CALL }, { 0 }, { 0 },
    { OP_SYSCALL }, { OP_SYS_EXIT },
    { 0 }, { 0 }, { 0 }, // padding

    // header
    { 0 }, { 0 }, // awc
    { 1 }, { 0 }, // lwc
    { 2 }, { 0 }, // swc
    { 0 }, { 0 }, // rwc
    { OP_PUSH_0_WORD },
    { OP_POP_WORD_0 },
    { OP_JMP }, { 3 }, { 0 },
    { OP_INC_I32 }, { 0 }, { 1 },
    { OP_PUSH_WORD_0 },
    { OP_PUSH_CONST_WORD }, { 0 },
    { OP_CMP_I32_LT },
    { OP_JMP_IF }, { 0xf6 }, { 0xff },
    { OP_PUSH_WORD_0 },
    { OP_SYSCALL }, { OP_SYS_EXIT }
};
Byte SIMPLE_REC[] = 
{
    { OP_PUSH_CONST_WORD }, { 0 },
    { OP_CALL }, { 0 }, { 0 },
    { OP_SYSCALL }, { OP_SYS_EXIT },
    { 0 },  // padding

    // header
    { 1 }, { 0 }, // awc
    { 1 }, { 0 }, // lwc
    { 3 }, { 0 }, // swc
    { 1 }, { 0 }, // rwc
    { OP_PUSH_WORD_0 },
    { OP_PUSH_0_WORD },
    { OP_CMP_I32_LE },
    { OP_JMP_IF }, { 9 }, { 0 },
    { OP_PUSH_WORD_0 },
    { OP_PUSH_WORD_0 },
    { OP_PUSH_I32_1 },
    { OP_SUB_I32 },
    { OP_CALL }, { 0 }, { 0 },
    { OP_ADD_I32 },
    { OP_RET },
    { OP_PUSH_0_WORD },
    { OP_RET }
};

static inline HWord iReadHWord(Byte *buffer, sz i) { return *(HWord*)(buffer + i); }
static inline Word  iReadWord(Byte *buffer, sz i)  { return *(Word*)(buffer + i); }
static inline DWord iReadDWord(Byte *buffer, sz i) { return *(DWord*)(buffer + i); }

static inline void iWriteHWord(Byte *buffer, sz i, HWord h) { *(HWord*)(buffer + i) = h; }
static inline void iWriteWord (Byte *buffer, sz i, Word  w) { *(Word* )(buffer + i) = w; }
static inline void iWriteDWord(Byte *buffer, sz i, DWord d) { *(DWord*)(buffer + i) = d; }

static int iCreateDummyFile(const char *filepath)
{
    FILE *file = fopen(filepath, "wb");
    if(!file)
        return -1;


    const u16 WORD_POOL_SIZE = 1; 
    const Word WORD_POOL[] = { IntToWord(1000) };
    const u16 DWORD_POOL_SIZE = 1; 
    const DWord DWORD_POOL[] = { IntToDWord(1000000000000LL) };
    const u16 STRING_POOL_SIZE = 1; 
    const ch8 *STRING_POOL[] = { "Hello world!\n" };
    const u16 FUNCTION_POOL_SIZE = 2; 
    const ch8 *FUNCTION_POOL[] = { "@Main", "@Rec" };
    const u32 MAIN_SIZE = 15;
    const Byte MAIN_BODY[] = 
    {
        { 0 }, { 0 },
        { 0 }, { 0 },
        { 1 }, { 0 },
        { 0 }, { 0 },
        { OP_PUSH_CONST_WORD }, { 0 },
        { OP_CALL }, { 1 }, { 1 }, 
        { OP_SYSCALL }, { OP_SYS_EXIT }
    };
    const u32 REC_SIZE = 25;
    const Byte REC_BODY[] = 
    {
        { 1 }, { 0 },
        { 1 }, { 0 },
        { 3 }, { 0 },
        { 1 }, { 0 },
        { OP_PUSH_WORD_0 },
        { OP_PUSH_0_WORD },
        { OP_CMP_I32_LE },
        { OP_JMP_IF }, { 9 }, { 0 },
        { OP_PUSH_WORD_0 },
        { OP_PUSH_WORD_0 },
        { OP_PUSH_I32_1 },
        { OP_SUB_I32 },
        { OP_CALL }, { 1 }, { 0 },
        { OP_ADD_I32 },
        { OP_RET },
        { OP_PUSH_0_WORD },
        { OP_RET }
    };
    

    fwrite(&WORD_POOL_SIZE, 2, 1, file);
    fwrite(&WORD_POOL, WORD_POOL_SIZE, 4, file);

    fwrite(&DWORD_POOL_SIZE, 2, 1, file);
    fwrite(&DWORD_POOL, DWORD_POOL_SIZE, 8, file);

    fwrite(&STRING_POOL_SIZE, 2, 1, file);
    fwrite(STRING_POOL[0], strlen(STRING_POOL[0]) + 1, 1, file);

    fwrite(&FUNCTION_POOL_SIZE, 2, 1, file);
    fwrite(FUNCTION_POOL[0], strlen(FUNCTION_POOL[0]) + 1, 1, file);
    fwrite(FUNCTION_POOL[1], strlen(FUNCTION_POOL[1]) + 1, 1, file);

    fwrite(&MAIN_SIZE, 4, 1, file);
    fwrite(&MAIN_BODY, MAIN_SIZE, 1, file);

    fwrite(&REC_SIZE, 4, 1, file);
    fwrite(&REC_BODY, REC_SIZE, 1, file);

    fclose(file);
    return 0;
}

static int iUpdateLinkData(LinkData *linkData, const String *filepath)
{
    FILE *file = fopen(String_CStr(filepath), "rb");
    if(!file)
    {
        perror("Error opening file");
        return -1;
    }
    ModuleData moduleData;
    ModuleData_Create(&moduleData);
    
    // get file size
    fseek(file, 0L, SEEK_END);
    sz fileSize = ftell(file);
    fseek(file, 0L, SEEK_SET);

    Byte *buffer = malloc(fileSize);
    fread(buffer, fileSize, 1, file); // get all file data

    sz i = 0;

    u16 wordCount = iReadHWord(buffer, i).UInt; i += 2;
    List_Word_Reserve(&moduleData.Words, wordCount);
    for (u16 j = 0; j < wordCount; j++)
    {
        Word w = iReadWord(buffer, i);
        List_Word_PushBack(&moduleData.Words, &w);
        i += 4;
    }

    u16 dwordCount = iReadHWord(buffer, i).UInt; i += 2;
    List_DWord_Reserve(&moduleData.DWords, dwordCount);
    for (u16 j = 0; j < dwordCount; j++)
    {
        DWord d = iReadDWord(buffer, i);
        List_DWord_PushBack(&moduleData.DWords, &d);
        i += 8;
    }
    
    u16 stringCount = iReadHWord(buffer, i).UInt; i += 2;
    {
        sz k = 0; 
        for (u16 j = 0; j < stringCount; j++)
        {        
            while (buffer[i + k].Int != 0)
            {
                List_char_PushBack(&moduleData.Strings, &buffer[i + k].Char);
                k++;
            }
            List_char_PushBack(&moduleData.Strings, "");
            k++;
        }
        i += k;
    }


    u16 internalFunctionCount = 0;
    u16 functionCount = iReadHWord(buffer, i).UInt; i += 2;
    {
        sz j = 0;
        sz k = 0;
        while(j < functionCount)
        {
            while (buffer[i + k].Int != 0)
            {
                List_char_PushBack(&moduleData.InternalFunctions, &buffer[i + k].Char);
                k++;
            }
            List_char_PushBack(&moduleData.Strings, "");
            j++;
            k++;
            internalFunctionCount++;

            if(buffer[i + k].Char == '&') // '&' is a separator
            {
                k++; // skip
                break;
            }
        }
        while(j < functionCount)
        {
            while (buffer[i + k].Int != 0)
            {
                List_char_PushBack(&moduleData.ExternalFunctions, &buffer[i + k].Char);
                k++;
            }
            List_char_PushBack(&moduleData.Strings, "");
            j++;
            k++;
        }
        i += k;
    }

    for (u16 j = 0; j < internalFunctionCount; j++)
    {
        u32 bytes = iReadWord(buffer, i).UInt; i += 4;
        List_Byte_Reserve(&moduleData.FunctionDefinitions, moduleData.FunctionDefinitions.Count + bytes);
        for (u32 k = 0; k < bytes; k++)
            List_Byte_PushBack(&moduleData.FunctionDefinitions, &buffer[i + k]);
        
        i += bytes;
    }
    
    free(buffer);

    List_ModuleData_PushBack(&linkData->ModuleList, &moduleData);
    if(fclose(file) == -1)
    {
        perror("Error closing file");
        return -1;
    }
    return 0;
}
static int iGetLinkData(LinkData *linkData, const String *dirpath)
{
    DIR *directory = opendir(String_CStr(dirpath));
    if(!directory)
    {
        perror("Error opening directory");
        return -1;
    }

    struct dirent *entry;
    while((entry = readdir(directory)) != NULL)
    {
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        String nextPath;
        String_Create(&nextPath, String_CStr(dirpath));
        String_PushBack(&nextPath, '/');
        String_ConcatStr(&nextPath, entry->d_name);
        if(entry->d_type == DT_REG) // file
        {
            int updateError = iUpdateLinkData(linkData, &nextPath);
            if(updateError)
                return -1;
        }
        else if (entry->d_type == DT_DIR)
        {

            int getError = iGetLinkData(linkData, &nextPath);
            if(getError)
                return -1;
        }

        String_Destroy(&nextPath);
    }


    if(closedir(directory) == -1)
    {
        perror("Error closing directory");
        return -1;
    }

    return 0;
}

#define DEFAULT_STACK_SIZE (1 << 22)
int Link(ProgramContext *context, const String *rootpath)
{
    iCreateDummyFile("test/f1/dummy.bin");
    LinkData data;
    LinkData_Create(&data);

    iGetLinkData(&data, rootpath);

    // init context
    context->StackBottom = malloc(DEFAULT_STACK_SIZE);
    context->StackTop    = context->StackBottom + DEFAULT_STACK_SIZE / sizeof(Word);
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

    // update context

    LinkData_Destroy(&data);
    return 0;
}

void Unlink(ProgramContext *context)
{
    for (u32 i = 0; i < context->ModuleTablesBufferSize / sizeof(ModuleTable); i++)
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