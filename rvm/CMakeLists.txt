set(RVM rvm)

if(NOT CMAKE_C_COMPILER_ID STREQUAL "GNU")
    message(FATAL_ERROR "Compiler not supported : ${CMAKE_C_COMPILER}")
endif()
message("Using compiler ${CMAKE_C_COMPILER}")

file(GLOB_RECURSE SOURCES "src/*.c")
add_executable(${RVM} ${SOURCES})

if(CMAKE_BUILD_TYPE STREQUAL "Release")
target_compile_definitions(${RVM} PRIVATE _RELEASE)
target_compile_options(${RVM} PRIVATE -O3 -Wall -Wextra)
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
target_compile_definitions(${RVM} PRIVATE _DEBUG)
target_compile_options(${RVM} PRIVATE -g -Wall -Wextra)
endif()
target_include_directories(${RVM} PRIVATE "src/")

target_link_libraries(${RVM} PRIVATE m) # MATHLIB
target_compile_definitions(${RVM} PRIVATE _DEFAULT_SOURCE) # used by dirent.h