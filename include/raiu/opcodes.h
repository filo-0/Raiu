#pragma once

#define OP_EXIT 0

#define OP_PUSH_BYTE_0  (u8) 0x01
#define OP_PUSH_BYTE_1  (u8) 0x02
#define OP_PUSH_BYTE_2  (u8) 0x03
#define OP_PUSH_BYTE_3  (u8) 0x04
#define OP_PUSH_HWORD_0 (u8) 0x05
#define OP_PUSH_HWORD_1 (u8) 0x06
#define OP_PUSH_WORD    (u8) 0x07
#define OP_PUSH_WORD_0  (u8) 0x08
#define OP_PUSH_WORD_1  (u8) 0x09
#define OP_PUSH_WORD_2  (u8) 0x0a
#define OP_PUSH_WORD_3  (u8) 0x0b
#define OP_PUSH_DWORD   (u8) 0x0c
#define OP_PUSH_DWORD_0 (u8) 0x0d
#define OP_PUSH_DWORD_1 (u8) 0x0e
#define OP_PUSH_DWORD_2 (u8) 0x0f
#define OP_PUSH_DWORD_3 (u8) 0x10
#define OP_PUSH_WORDS   (u8) 0x11

#define OP_PUSH_0_WORD  (u8) 0x12
#define OP_PUSH_I32_1   (u8) 0x13
#define OP_PUSH_I32_2   (u8) 0x14
#define OP_PUSH_0_DWORD (u8) 0x15
#define OP_PUSH_I64_1   (u8) 0x16
#define OP_PUSH_I64_2   (u8) 0x17
#define OP_PUSH_F32_1   (u8) 0x18
#define OP_PUSH_F32_2   (u8) 0x19
#define OP_PUSH_F64_1   (u8) 0x1a
#define OP_PUSH_F64_2   (u8) 0x1b

#define OP_PUSH_I32     (u8) 0x1c
#define OP_PUSH_I64     (u8) 0x1d
#define OP_PUSH_CONST_WORD    (u8) 0x1e
#define OP_PUSH_CONST_WORD_W  (u8) 0x1f
#define OP_PUSH_CONST_DWORD   (u8) 0x20
#define OP_PUSH_CONST_DWORD_W (u8) 0x21
#define OP_PUSH_CONST_STR     (u8) 0x22
#define OP_PUSH_CONST_STR_W   (u8) 0x23

#define OP_POP_BYTE_0  (u8) 0x24
#define OP_POP_BYTE_1  (u8) 0x25
#define OP_POP_BYTE_2  (u8) 0x26
#define OP_POP_BYTE_3  (u8) 0x27
#define OP_POP_HWORD_0 (u8) 0x28
#define OP_POP_HWORD_1 (u8) 0x29
#define OP_POP_WORD    (u8) 0x2a
#define OP_POP_WORD_0  (u8) 0x2b
#define OP_POP_WORD_1  (u8) 0x2c
#define OP_POP_WORD_2  (u8) 0x2d
#define OP_POP_WORD_3  (u8) 0x2e
#define OP_POP_DWORD   (u8) 0x2f
#define OP_POP_DWORD_0 (u8) 0x30
#define OP_POP_DWORD_1 (u8) 0x31
#define OP_POP_DWORD_2 (u8) 0x32
#define OP_POP_DWORD_3 (u8) 0x33
#define OP_POP_WORDS   (u8) 0x34

#define OP_ADD_I32 (u8) 0x35
#define OP_ADD_I64 (u8) 0x36
#define OP_ADD_F32 (u8) 0x37
#define OP_ADD_F64 (u8) 0x38
#define OP_INC_I32 (u8) 0x39
#define OP_INC_I64 (u8) 0x3A
#define OP_INC_F32 (u8) 0x3B
#define OP_INC_F64 (u8) 0x3C

#define OP_SUB_I32 (u8) 0x3D
#define OP_SUB_I64 (u8) 0x3E
#define OP_SUB_F32 (u8) 0x3F
#define OP_SUB_F64 (u8) 0x40
#define OP_DEC_I32 (u8) 0x41
#define OP_DEC_I64 (u8) 0x42
#define OP_DEC_F32 (u8) 0x43
#define OP_DEC_F64 (u8) 0x44

#define OP_MUL_I32 (u8) 0x45
#define OP_MUL_I64 (u8) 0x46
#define OP_MUL_U32 (u8) 0x47
#define OP_MUL_U64 (u8) 0x48
#define OP_MUL_F32 (u8) 0x49
#define OP_MUL_F64 (u8) 0x4A

#define OP_DIV_I32 (u8) 0x4B
#define OP_DIV_I64 (u8) 0x4C
#define OP_DIV_U32 (u8) 0x4D
#define OP_DIV_U64 (u8) 0x4E
#define OP_DIV_F32 (u8) 0x4F
#define OP_DIV_F64 (u8) 0x50

#define OP_REM_I32 (u8) 0x51
#define OP_REM_I64 (u8) 0x52
#define OP_REM_U32 (u8) 0x53
#define OP_REM_U64 (u8) 0x54

#define OP_NEG_I32 (u8) 0x55
#define OP_NEG_I64 (u8) 0x56
#define OP_NEG_F32 (u8) 0x57
#define OP_NEG_F64 (u8) 0x58

#define OP_NOT_WORD  (u8) 0x59
#define OP_NOT_DWORD (u8) 0x5A
#define OP_AND_WORD  (u8) 0x5B
#define OP_AND_DWORD (u8) 0x5C
#define OP_OR_WORD   (u8) 0x5D
#define OP_OR_DWORD  (u8) 0x5E
#define OP_XOR_WORD  (u8) 0x5F
#define OP_XOR_DWORD (u8) 0x60
#define OP_SHL_WORD  (u8) 0x61
#define OP_SHL_DWORD (u8) 0x62
#define OP_SHR_I32   (u8) 0x63
#define OP_SHR_I64   (u8) 0x64
#define OP_SHR_U32   (u8) 0x65
#define OP_SHR_U64   (u8) 0x66

#define OP_I32_TO_I8  (u8) 0x67
#define OP_I32_TO_I16 (u8) 0x68
#define OP_I32_TO_I64 (u8) 0x69
#define OP_I32_TO_F32 (u8) 0x6A
#define OP_I32_TO_F64 (u8) 0x6B
#define OP_I64_TO_I32 (u8) 0x6C
#define OP_I64_TO_F32 (u8) 0x6D
#define OP_I64_TO_F64 (u8) 0x6E
#define OP_F32_TO_I32 (u8) 0x6F
#define OP_F32_TO_I64 (u8) 0x70
#define OP_F32_TO_F64 (u8) 0x71
#define OP_F64_TO_I32 (u8) 0x72
#define OP_F64_TO_I64 (u8) 0x73
#define OP_F64_TO_F32 (u8) 0x74

#define OP_CMP_WORD_EQ  (u8) 0x75
#define OP_CMP_DWORD_EQ (u8) 0x76
#define OP_CMP_WORD_NE  (u8) 0x77
#define OP_CMP_DWORD_NE (u8) 0x78
#define OP_CMP_I32_GT   (u8) 0x79
#define OP_CMP_I64_GT   (u8) 0x7A
#define OP_CMP_U32_GT   (u8) 0x7B
#define OP_CMP_U64_GT   (u8) 0x7C
#define OP_CMP_F32_GT   (u8) 0x7D
#define OP_CMP_F64_GT   (u8) 0x7E
#define OP_CMP_I32_LT   (u8) 0x7F
#define OP_CMP_I64_LT   (u8) 0x80
#define OP_CMP_U32_LT   (u8) 0x81
#define OP_CMP_U64_LT   (u8) 0x82
#define OP_CMP_F32_LT   (u8) 0x83
#define OP_CMP_F64_LT   (u8) 0x84
#define OP_CMP_I32_GE   (u8) 0x85
#define OP_CMP_I64_GE   (u8) 0x86
#define OP_CMP_U32_GE   (u8) 0x87
#define OP_CMP_U64_GE   (u8) 0x88
#define OP_CMP_F32_GE   (u8) 0x89
#define OP_CMP_F64_GE   (u8) 0x8A
#define OP_CMP_I32_LE   (u8) 0x8B
#define OP_CMP_I64_LE   (u8) 0x8C
#define OP_CMP_U32_LE   (u8) 0x8D
#define OP_CMP_U64_LE   (u8) 0x8E
#define OP_CMP_F32_LE   (u8) 0x8F
#define OP_CMP_F64_LE   (u8) 0x90
#define OP_CMP_NOT      (u8) 0x91

#define OP_DUP_WORD     (u8) 0x92
#define OP_DUP_DWORD    (u8) 0x93
#define OP_DUP_WORD_X1  (u8) 0x94
#define OP_DUP_DWORD_X1 (u8) 0x95
#define OP_DUP_WORD_X2  (u8) 0x96
#define OP_DUP_DWORD_X2 (u8) 0x97
#define OP_SWAP_WORD    (u8) 0x98
#define OP_SWAP_DWORD   (u8) 0x99

#define OP_LOAD_BYTE_0  (u8) 0x9A
#define OP_LOAD_BYTE_1  (u8) 0x9B
#define OP_LOAD_BYTE_2  (u8) 0x9C
#define OP_LOAD_BYTE_3  (u8) 0x9D
#define OP_LOAD_HWORD_0 (u8) 0x9E
#define OP_LOAD_HWORD_1 (u8) 0x9F
#define OP_LOAD_WORD    (u8) 0xA0
#define OP_LOAD_DWORD   (u8) 0xA1
#define OP_LOAD_WORDS   (u8) 0xA2

#define OP_STORE_BYTE_0  (u8) 0xA3
#define OP_STORE_BYTE_1  (u8) 0xA4
#define OP_STORE_BYTE_2  (u8) 0xA5
#define OP_STORE_BYTE_3  (u8) 0xA6
#define OP_STORE_HWORD_0 (u8) 0xA7
#define OP_STORE_HWORD_1 (u8) 0xA8
#define OP_STORE_WORD    (u8) 0xA9
#define OP_STORE_DWORD   (u8) 0xAA
#define OP_STORE_WORDS   (u8) 0xAB

#define OP_LOAD_OFST_BYTE_0  (u8) 0xAC
#define OP_LOAD_OFST_BYTE_1  (u8) 0xAD
#define OP_LOAD_OFST_BYTE_2  (u8) 0xAE
#define OP_LOAD_OFST_BYTE_3  (u8) 0xAF
#define OP_LOAD_OFST_HWORD_0 (u8) 0xB0
#define OP_LOAD_OFST_HWORD_1 (u8) 0xB1
#define OP_LOAD_OFST_WORD    (u8) 0xB2
#define OP_LOAD_OFST_DWORD   (u8) 0xB3
#define OP_LOAD_OFST_WORDS   (u8) 0xB4

#define OP_STORE_OFST_BYTE_0  (u8) 0xB5
#define OP_STORE_OFST_BYTE_1  (u8) 0xB6
#define OP_STORE_OFST_BYTE_2  (u8) 0xB7
#define OP_STORE_OFST_BYTE_3  (u8) 0xB8
#define OP_STORE_OFST_HWORD_0 (u8) 0xB9
#define OP_STORE_OFST_HWORD_1 (u8) 0xBA
#define OP_STORE_OFST_WORD    (u8) 0xBB
#define OP_STORE_OFST_DWORD   (u8) 0xBC
#define OP_STORE_OFST_WORDS   (u8) 0xBD

#define OP_LOAD_BUFF_BYTE_VAL  (u8) 0xBE
#define OP_LOAD_BUFF_HWORD_VAL (u8) 0xBF
#define OP_LOAD_BUFF_WORD_VAL  (u8) 0xC0
#define OP_LOAD_BUFF_DWORD_VAL (u8) 0xC1
#define OP_LOAD_BUFF_WORDS_VAL (u8) 0xC2

#define OP_LOAD_BUFF_BYTE_REF  (u8) 0xC3
#define OP_LOAD_BUFF_HWORD_REF (u8) 0xC4
#define OP_LOAD_BUFF_WORD_REF  (u8) 0xC5
#define OP_LOAD_BUFF_DWORD_REF (u8) 0xC6
#define OP_LOAD_BUFF_WORDS_REF (u8) 0xC7

#define OP_STORE_BUFF_BYTE  (u8) 0xC8
#define OP_STORE_BUFF_HWORD (u8) 0xC9
#define OP_STORE_BUFF_WORD  (u8) 0xCA
#define OP_STORE_BUFF_DWORD (u8) 0xCB
#define OP_STORE_BUFF_WORDS (u8) 0xCC

#define OP_ALLOC (u8) 0xCD
#define OP_FREE  (u8) 0xCE

#define OP_JMP     (u8) 0xCF
#define OP_JMP_IF  (u8) 0xD0
#define OP_CALL    (u8) 0xD1
#define OP_SYSCALL (u8) 0xD2
#define OP_RET     (u8) 0xD3

#define OP_SYS_EXIT   (u8) 0x00
#define OP_SYS_PRINT  (u8) 0x01
#define OP_SYS_PRINTI (u8) 0x02
#define OP_SYS_PRINTF (u8) 0x03
#define OP_SYS_SCAN   (u8) 0x04
#define OP_SYS_SCANI  (u8) 0x05
#define OP_SYS_SCANF  (u8) 0x06
#define OP_SYS_MEMMOV (u8) 0x07
#define OP_SYS_MEMCPY (u8) 0x08
#define OP_SYS_CLOCK  (u8) 0x09

#define OP_SYS_SQRT32 (u8) 0x0A
#define OP_SYS_SQRT64 (u8) 0x0B
#define OP_SYS_EXP32  (u8) 0x0C
#define OP_SYS_EXP64  (u8) 0x0D
#define OP_SYS_LOG32  (u8) 0x0E
#define OP_SYS_LOG64  (u8) 0x0F










