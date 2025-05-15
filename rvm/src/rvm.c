#include <stdio.h>
#include <time.h>
#include "rvm.h"
#include "raiu/log.h"

int main(int argc, char **argv)
{
    if(argc > 2)
    {
        printf("Usage: .%s <header>\n", argv[0]);
        return -1;
    }

    String rootpath;
    String_Create(&rootpath, argc == 1 ? "DummyProject" : argv[1]);

    i32 returnValue = Run(&rootpath);
    printf("Program exited with code %d\n", returnValue);
    

    String_Destroy(&rootpath);
    return returnValue;
}

i32 Run(const String *rootpath)
{
    i32 ret = 0;
    ProgramContext context;
    ProgramContext_Init(&context);
    i32 linkError = Link(&context, rootpath);
    if(!linkError)
    {
        LOG_INFO("Linking successful!");
        clock_t t0 = clock();
        ret = Execute(&context);
        clock_t t1 = clock();
        printf("Time elapsed : %ld us\n", t1 - t0);
    }
    else
        ret = linkError;


    Unlink(&context);
    return ret;
}