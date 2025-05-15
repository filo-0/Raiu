#pragma once
#include <stdio.h>

#ifdef _DEBUG
#define DEVEL_ASSERT(cond, ...) do {\
    if(!(cond)) \
    { \
        printf("[ERROR] "__VA_ARGS__); \
        fflush(stdin); \
        __builtin_trap(); \
    } \
} while(0)
#define UNLIKELY(cond, ...) DEVEL_ASSERT(!(cond), __VA_ARGS__)
#else
#define DEVEL_ASSERT(cond, ...)
#define UNLIKELY(cond, ...) do {\
    if(cond) \
    { \
        printf("[ERROR] "__VA_ARGS__); \
        exit(1); \
    } \
} while(0)
#endif

