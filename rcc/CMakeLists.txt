
file(GLOB_RECURSE ASSEMBLER_SOURCES "src/*.c")
add_executable(rcc ${ASSEMBLER_SOURCES})

target_include_directories(rcc PRIVATE "src")