#include <stdio.h>
#include "vm.h"

int main(int argc, char **argv)
{
    if(argc > 2)
    {
        printf("Usage: .%s <header>\n", argv[0]);
        return -1;
    }

    String rootpath;
    String_Create(&rootpath, argc == 1 ? "test_project_root" : argv[1]);

    int returnValue = Run(&rootpath);
    printf("Program exited with code %d\n", returnValue);
    

    String_Destroy(&rootpath);
    return returnValue;
}