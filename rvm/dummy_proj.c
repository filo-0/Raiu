#include <stdio.h>
#include <dirent.h>
#include <raiu/raiu.h>
#include <sys/stat.h>

static i32 iCreateDummyProject(const char *root)
{

    /*
        Root -> Main
                Subdir -> SubFile
    */

    String rootPath, mainPath, subdirPath, subfilePath;
    String_Create(&rootPath, root);
    String_Create(&mainPath, root); String_ConcatStr(&mainPath, "/Main");
    String_Create(&subdirPath, root); String_ConcatStr(&subdirPath, "/Subdir");
    String_Create(&subfilePath, root); String_ConcatStr(&subfilePath, "/Subdir/Subfile");

    mkdir(String_CStr(&rootPath), 0700);
    mkdir(String_CStr(&subdirPath), 0700);

    // Main
    {
        FILE *mainFile = fopen(String_CStr(&mainPath), "wb");
        if(!mainFile)
        {
            perror("Error creating file");
            return -1;
        }
        const Word WORD_POOL[] = { IntToWord(100000) };
        const DWord DWORD_POOL[] = { };
        const ch8 *STRING_POOL[] = { "The result of the operation is : ", "\n" };
        const ch8 *FUNCTION_POOL[] = { "Main", "&", "DummyProject/Subdir/Subfile.PrintInt" };
        const ch8 *GLOBAL_POOL[]   = { };
        const Byte MAIN_BODY[] = 
        {
            { 0 }, { 0 },
            { 0 }, { 0 },
            { 2 }, { 0 },
            { 0 }, { 0 },
            { OP_PUSH_FUNC }, { 1 }, { 0 },
            { OP_INDCALL },
            { OP_PUSH_FUNC }, { 1 }, { 0 },
            { OP_INDCALL },
            { OP_PUSH_CONST_STR }, { 1 },
            { OP_SYSCALL }, { OP_SYS_PRINT },
            { OP_PUSH_0_WORD },
            { OP_SYSCALL }, { OP_SYS_EXIT }
        };
        const u16 WORD_POOL_SIZE = sizeof(WORD_POOL) / sizeof(Word);
        fwrite(&WORD_POOL_SIZE, 2, 1, mainFile);
        fwrite(&WORD_POOL, WORD_POOL_SIZE, 4, mainFile);

        const u16 DWORD_POOL_SIZE = sizeof(DWORD_POOL) / sizeof(DWord);
        fwrite(&DWORD_POOL_SIZE, 2, 1, mainFile);
        fwrite(&DWORD_POOL, DWORD_POOL_SIZE, 8, mainFile);

        const u16 STRING_POOL_SIZE = sizeof(STRING_POOL) / sizeof(char*);
        fwrite(&STRING_POOL_SIZE, 2, 1, mainFile);
        for (u16 i = 0; i < STRING_POOL_SIZE; i++)
            fwrite(STRING_POOL[i], strlen(STRING_POOL[i]) + 1, 1, mainFile);

        const u16 GLOBAL_POOL_SIZE = sizeof(GLOBAL_POOL) / sizeof(char*);
        fwrite(&GLOBAL_POOL_SIZE, 2, 1, mainFile);
        for (u16 i = 0; i < GLOBAL_POOL_SIZE; i++)
            fwrite(GLOBAL_POOL[i], strlen(GLOBAL_POOL[i]) + 1, 1, mainFile);

        const u16 FUNCTION_POOL_SIZE = sizeof(FUNCTION_POOL) / sizeof(char*) - 1;
        fwrite(&FUNCTION_POOL_SIZE, 2, 1, mainFile);
        for(u16 i = 0; i < FUNCTION_POOL_SIZE + 1; i++)
            fwrite(FUNCTION_POOL[i], strlen(FUNCTION_POOL[i]) + 1, 1, mainFile);

        const u32 MAIN_BODY_SIZE = sizeof(MAIN_BODY) - 8;
        fwrite(&MAIN_BODY_SIZE, 4, 1, mainFile);
        fwrite(&MAIN_BODY, MAIN_BODY_SIZE + 8, 1, mainFile);

        fclose(mainFile);
    }


    // Subfile
    {
        FILE *subFile = fopen(String_CStr(&subfilePath), "wb");
        if(!subFile)
        {
            perror("Error creating file");
            return -1;
        }

        const Word  WORD_POOL[] = {};
        const DWord DWORD_POOL[] = {};
        const ch8 *STRING_POOL[] = {};
        const ch8 *FUNCTION_POOL[] = { "PrintInt", };
        const ch8 *GLOBAL_POOL[] = { "Integer" };
        const u32 GLOBAL_SIZES[] = { 4 };
        const Byte PRINTINT_BODY[] = 
        {
            { 0 }, { 0 },
            { 0 }, { 0 },
            { 4 }, { 0 },
            { 0 }, { 0 },
            { OP_PUSH_GLOB_REF }, { 0 },
            { OP_LOAD_WORD },
            { OP_I32_TO_I64 },
            { OP_SYSCALL }, { OP_SYS_PRINTI },
            { OP_PUSH_GLOB_REF }, { 0 },
            { OP_DUP_DWORD },
            { OP_LOAD_WORD },
            { OP_PUSH_I32_1 },
            { OP_ADD_I32 },
            { OP_STORE_WORD },
            { OP_RET }
        };

        const u16 WORD_POOL_SIZE = sizeof(WORD_POOL) / sizeof(Word);
        fwrite(&WORD_POOL_SIZE, sizeof(u16), 1, subFile);

        const u16 DWORD_POOL_SIZE = sizeof(DWORD_POOL) / sizeof(DWord);
        fwrite(&DWORD_POOL_SIZE, sizeof(u16), 1, subFile);

        const u16 STRING_POOL_SIZE = sizeof(STRING_POOL) / sizeof(ch8*);
        fwrite(&STRING_POOL_SIZE, sizeof(u16), 1, subFile);

        const u16 GLOBAL_POOL_SIZE = sizeof(GLOBAL_POOL) / sizeof(ch8*);
        fwrite(&GLOBAL_POOL_SIZE, sizeof(u16), 1, subFile);
        for (u16 i = 0; i < GLOBAL_POOL_SIZE; i++)
            fwrite(GLOBAL_POOL[i], strlen(GLOBAL_POOL[i]) + 1, 1, subFile);

        const u16 FUNCTION_POOL_SIZE = sizeof(FUNCTION_POOL) / sizeof(ch8*);
        fwrite(&FUNCTION_POOL_SIZE, sizeof(u16), 1, subFile);
        for (u16 i = 0; i < FUNCTION_POOL_SIZE; i++)
            fwrite(FUNCTION_POOL[i], strlen(FUNCTION_POOL[i]) + 1, 1, subFile);
        
        fwrite(GLOBAL_SIZES, 4, 1, subFile);

        const u32 PRINTINT_BODY_SIZE = sizeof(PRINTINT_BODY) - 8;
        fwrite(&PRINTINT_BODY_SIZE, sizeof(u32), 1, subFile);
        fwrite(&PRINTINT_BODY, PRINTINT_BODY_SIZE + 8, 1, subFile);


        fclose(subFile);
    }

    String_Destroy(&rootPath);
    String_Destroy(&mainPath);
    String_Destroy(&subdirPath);
    String_Destroy(&subfilePath);
    return 0;
}
int main() { return iCreateDummyProject("DummyProject"); }