#pragma once

#include <stdio.h>

#ifdef _DEBUG
#define LOG_INFO(...)  printf("[INFO] "    __VA_ARGS__); printf("\n")
#define LOG_WARN(...)  printf("[WARNING] " __VA_ARGS__); printf("\n")
#define LOG_ERROR(...) printf("[ERROR] "   __VA_ARGS__); printf("\n")
#else
#define LOG_INFO(f, ...) 
#define LOG_WARN(f, ...)
#define LOG_ERROR(f, ...)
#endif
