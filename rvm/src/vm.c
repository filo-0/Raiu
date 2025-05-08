#include <stdio.h>
#include <time.h>
#include "vm.h"

int Run(const String *rootpath)
{
    ProgramContext context;

    int linkingResult = Link(&context, rootpath);
    if(linkingResult)
    {
        printf("Linking failed with code %d\n", linkingResult);
        return linkingResult;
    }
    // clock_t t0 = clock();
    // i32 ret = Execute(&context);
    // clock_t t1 = clock();
    // printf("Time elapsed : %ld us\n", t1 - t0);

    Unlink(&context);
    // return ret;
    return 0;
}