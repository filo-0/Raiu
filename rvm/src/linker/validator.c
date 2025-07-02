#include "linker.h"
#include "raiu/assert.h"
#include "raiu/log.h"


i32 Validate(const String *functionSignature, const FunctionHeader *header, const u8 *instruction, i32 sp)
{
    static const i32 sInstructionsStackOffsets[] = 
    {
        0, // exit
        // push loc
        1, 1, 1, 1, // push byte
        1, 1, // push hword
        1, 1, 1, 1, 1, // push word
        2, 2, 2, 2, 2, // push dword
        INT32_MIN, // push words
        2, // push ref
        // push imm
        1, 1, 1, // push i32
        2, 2, 2, // push i64
        1, 1, // push f32
        2, 2, // push f64
        1, 2, // push i8 as 
        1, 1, // push const word
        2, 2, // push const dword
        2, 2, // push const string
        2, 2, // push glob ref
        2, // push func 
        // pop locals
        -1, -1, -1, -1, // pop byte
        -1, -1, // pop hword
        -1, -1, -1, -1, -1, // pop word
        -2, -2, -2, -2, -2, // pop dword
        INT32_MIN, // pop words
        // arithmetic
        -1, -2, -1, -2, // add
        0, 0, 0, 0, // inc
        -1, -2, -1, -2, // sub
        0, 0, 0, 0, // dec
        -1, -2, -1, -2, -1, -2, // mul
        -1, -2, -1, -2, -1, -2, // div
        -1, -2, -1, -2, // rem
        0, 0, 0, 0, // neg 
        // bitwise
        0, 0, // not
        1, 2, // and
        1, 2, // or
        1, 2, // xor
        1, 1, // shl
        1, 1, 1, 1, // shr
        // casts
        0, 0, 1, 0, 1, // form i32
        -1, -1, 0, // from i64
        0, 1, 1, // from i32
        -1, 0, -1, // from f64
        // compare
        -1, -2, -1, -2, // eq ne
        -1, -2, -1, -2, -1, -2, // gt
        -1, -2, -1, -2, -1, -2, // lt
        -1, -2, -1, -2, -1, -2, // ge
        -1, -2, -1, -2, -1, -2, // le
        0, // not
        // stack manipulation
        1, 2, 1, 2, 1, 2, //dup
        0, 0, // swap
        // load&store
        -1, -1, -1, -1, // load byte
        -1, -1, // load hword
        -1, // load word
        0, // load dword
        INT32_MIN, // load words
        -3, -3, -3, -3, // store byte
        -3, -3, // store hword
        -3, // store word
        -4, // store dword
        INT32_MIN, // store words
        // load&store offset
        -1, -1, -1, -1, // load byte
        -1, -1, // load hword
        -1, // load word
        0, // load dword
        INT32_MIN, // load words
        -3, -3, -3, -3, // store byte
        -3, -3, // store hword
        -3, // store word
        -4, // store dword
        INT32_MIN, // store words
        // load&store buffer
        -2, -2, -2, -1, INT32_MIN, // load val
        -1, -1, -1, -1, -1, // load ref
        -4, -4, -4, -5, INT32_MIN,
        
        // mem
        2, -2,
        // flow
        0, -1, // jump
        INT32_MIN, INT32_MIN, INT32_MIN, // call
        INT32_MIN, // ret
    };
    static const i32 sInstructionsFixedParameterSizes[] = 
    {
        0, // exit
        // push loc
        1, 1, 1, 1, // push byte
        1, 1, // push hword
        1, 0, 0, 0, 0, // push word
        1, 0, 0, 0, 0, // push dword
        2, // push words
        1, // push ref
        // push imm
        0, 0, 0, // push i32
        0, 0, 0, // pusg i64
        0, 0, // push f32
        0, 0, // push f64
        1, 1, // push i8 as 
        1, 2, // push const word
        1, 2, // push const dword
        1, 2, // push const string
        1, 2, // push glob ref
        2, // push func 
        // pop locals
        1, 1, 1, 1, // pop byte
        1, 1, // pop hword
        1, 0, 0, 0, 0, // pop word
        1, 0, 0, 0, 0, // pop dword
        2, // pop words
        // arithmetic
        0, 0, 0, 0, // add
        2, 2, 2, 2, // inc
        0, 0, 0, 0, // sub
        2, 2, 2, 2, // dec
        0, 0, 0, 0, 0, 0, // mul
        0, 0, 0, 0, 0, 0, // div
        0, 0, 0, 0, // rem
        0, 0, 0, 0, // neg 
        // bitwise
        0, 0, // not
        0, 0, // and
        0, 0, // or
        0, 0, // xor
        0, 0, // shl
        0, 0, 0, 0, // shr
        // casts
        0, 0, 0, 0, 0, // form i32
        0, 0, 0, // from i64
        0, 0, 0, // from i32
        0, 0, 0, // from f64
        // compare
        0, 0, 0, 0, // eq ne
        0, 0, 0, 0, 0, 0, // gt
        0, 0, 0, 0, 0, 0, // lt
        0, 0, 0, 0, 0, 0, // ge
        0, 0, 0, 0, 0, 0, // le
        0, // not
        // stack manipulation
        0, 0, 0, 0, 0, 0, //dup
        0, 0, // swap
        // load&store
        0, 0, 0, 0, // load byte
        0, 0, // load hword
        0, // load word
        0, // load dword
        1, // load dwords
        0, 0, 0, 0, // store byte
        0, 0, // store hword
        0, // store word
        0, // store dword
        1, // store words
        
        // load&store offset
        1, 1, 1, 1, // load byte
        1, 1, // load hword
        1, // load word
        1, // load dword
        2, // load dwords
        1, 1, 1, 1, // store byte
        1, 1, // store hword
        1, // store word
        1, // store dword
        2, // store words
        // load&store buffer
        0, 0, 0, 0, 1, // load val
        0, 0, 0, 0, 1, // load ref
        0, 0, 0, 0, 1, // store val
        
        // mem
        0, 0,
        // flow
        0, 0, // jump
        2, 0, 1, // call
        0, // ret
    };
    static const i32 sSysfnStackOffsets[] = 
    {
        -1,
        -2, -2, -2,
        -3, +2, +2,
        -5, -5,
        +2,
        0, 0, 0, 0, 0, 0 
    };
    
    if(header->AWC > header->LWC)
    {
        DEVEL_ASSERT(false, "AWC greater than LWC in function %s\n", String_CStr(functionSignature));
        return 1;   
    }

    u8 opcode = 0;
    const i32 *instrStackOffsets = sInstructionsStackOffsets;
    const i32 *instrParamOffsets = sInstructionsFixedParameterSizes;
    const i32 *sysfnStackOffsets = sSysfnStackOffsets;

    bool exited = false;
    while (!exited)
    {
        opcode = *instruction;
        if(opcode == 0 || opcode > OP_MAX_OPCODE)
        {
            DEVEL_ASSERT(false, "Invalid instruction in function %s! [opcode=%u]\n", String_CStr(functionSignature), opcode);
            return 1;
        }

        if(opcode == OP_JMP_IF)
        {
            u8 offset = instruction[1];
            i32 branchIfTrueError = Validate(functionSignature, header, instruction + offset + 1, sp - 1);
            if(branchIfTrueError)
                return 1;
        }
    
        i32 stackOffset = instrStackOffsets[opcode];
        i32 paramOffset = instrParamOffsets[opcode]; 
        
        switch (opcode)
        {
        case OP_POP_BYTE_0:
        case OP_POP_BYTE_1:
        case OP_POP_BYTE_2:
        case OP_POP_BYTE_3:
        case OP_POP_HWORD_0:
        case OP_POP_HWORD_1:
        case OP_POP_WORD:
        case OP_POP_DWORD:
        case OP_PUSH_BYTE_1:
        case OP_PUSH_BYTE_0:
        case OP_PUSH_BYTE_2:
        case OP_PUSH_BYTE_3:
        case OP_PUSH_HWORD_0:
        case OP_PUSH_HWORD_1:
        case OP_PUSH_WORD:
        case OP_PUSH_DWORD:
        case OP_PUSH_REF:
        case OP_INC_I32:
        case OP_INC_I64:
        case OP_INC_F32:
        case OP_INC_F64:
        case OP_DEC_I32:
        case OP_DEC_I64:
        case OP_DEC_F32:
        case OP_DEC_F64:
            {
                u8 l = instruction[1];
                if(l >= header->LWC)
                {
                    DEVEL_ASSERT(false, "Invalid local scope access in function %s [LWC=%u,l=%u]\n", String_CStr(functionSignature), header->LWC, l);
                    return 1;
                }
            }
            break;
        
        case OP_PUSH_CONST_WORD:
            {
                u8 c = instruction[1];
                if(c >= header->MT->WordPoolSize)
                {
                    DEVEL_ASSERT(false, "Invalid const word access in function %s [MAX=%u,c=%u]\n", String_CStr(functionSignature), header->MT->WordPoolSize, c);
                    return 1;
                }
            }
            break;
        case OP_PUSH_CONST_WORD_W:
            {
                u16 c = *(u16*)(instruction + 1);
                if(c >= header->MT->WordPoolSize)
                {
                    DEVEL_ASSERT(false, "Invalid const word access in function %s [MAX=%u,c=%u]\n", String_CStr(functionSignature), header->MT->WordPoolSize, c);
                    return 1;
                }
            }
            break;
        case OP_PUSH_CONST_DWORD:
            {
                u8 c = instruction[1];
                if(c >= header->MT->DWordPoolSize)
                {
                    DEVEL_ASSERT(false, "Invalid const dword access in function %s [MAX=%u,c=%u]\n", String_CStr(functionSignature), header->MT->DWordPoolSize, c);
                    return 1;
                }
            }
            break;
        case OP_PUSH_CONST_DWORD_W:
            {
                u16 c = *(u16*)(instruction + 1);
                if(c >= header->MT->DWordPoolSize)
                {
                    DEVEL_ASSERT(false, "Invalid const dword access in function %s [MAX=%u,c=%u]\n", String_CStr(functionSignature), header->MT->DWordPoolSize, c);
                    return 1;
                }
            }
            break;
        case OP_PUSH_CONST_STR:
            {
                u8 c = instruction[1];
                if(c >= header->MT->StringPoolSize)
                {
                    DEVEL_ASSERT(false, "Invalid const string access in function %s [MAX=%u,c=%u]\n", String_CStr(functionSignature), header->MT->StringPoolSize, c);
                    return 1;
                }
            }
            break;
        case OP_PUSH_CONST_STR_W:
            {
                u16 c = *(u16*)(instruction + 1);
                if(c >= header->MT->StringPoolSize)
                {
                    DEVEL_ASSERT(false, "Invalid const string access in function %s [MAX=%u,c=%u]\n", String_CStr(functionSignature), header->MT->StringPoolSize, c);
                    return 1;
                }
            }
            break;
        case OP_PUSH_GLOB_REF:
            {
                u8 c = instruction[1];
                if(c >= header->MT->GlobalPoolSize)
                {
                    DEVEL_ASSERT(false, "Invalid global data access in function %s [MAX=%u,c=%u]\n", String_CStr(functionSignature), header->MT->StringPoolSize, c);
                    return 1;
                }
            }
            break;
        case OP_PUSH_GLOB_REF_W:
            {
                u16 c = *(u16*)(instruction + 1);
                if(c >= header->MT->GlobalPoolSize)
                {
                    DEVEL_ASSERT(false, "Invalid global data access in function %s [MAX=%u,c=%u]\n", String_CStr(functionSignature), header->MT->StringPoolSize, c);
                    return 1;
                }
            }
            break;
        case OP_PUSH_FUNC:
            {
                u16 c = *(u16*)(instruction + 1);
                if(c >= header->MT->FunctionPoolSize)
                {
                    DEVEL_ASSERT(false, "Invalid function pointer access in function %s [MAX=%u,c=%u]\n", String_CStr(functionSignature), header->MT->StringPoolSize, c);
                    return 1;
                }
            }
            break;
        case OP_POP_WORDS:
            {
                u8 l = instruction[1];
                if(l >= header->LWC)
                {
                    DEVEL_ASSERT(false, "Invalid local scope access in function %s [LWC=%u,l=%u]\n", String_CStr(functionSignature), header->LWC, l);
                    return 1;
                }

                i32 n = instruction[2] + 1;
                stackOffset = -n;
            }
            break;
        case OP_PUSH_WORDS:
            {
                u8 l = instruction[1];
                if(l >= header->LWC)
                {
                    DEVEL_ASSERT(false, "Invalid local scope access in function %s [LWC=%u,l=%u]",String_CStr(functionSignature), header->LWC, l);
                    return 1;
                }

                i32 n = instruction[2] + 1;
                stackOffset = n;
            }
            break;
        case OP_LOAD_WORDS:
            {
                i32 n = instruction[1] + 1;
                stackOffset = n - 2;
            }
            break;
        case OP_STORE_WORDS:
            {
                i32 n = instruction[1] + 1;
                stackOffset = -n - 2;
            }
            break;
        case OP_LOAD_OFST_WORDS:
            {
                i32 n = instruction[2] + 1;
                stackOffset = n - 2;
            }
            break;
        case OP_STORE_OFST_WORDS:
            {
                i32 n = instruction[2] + 1;
                stackOffset = -n - 2;
            }
            break;
        case OP_LOAD_BUFF_WORDS_VAL:
            {
                i32 n = instruction[1] + 1;
                stackOffset = n - 3;
            }
            break;
        case OP_STORE_BUFF_WORDS:
            {
                i32 n = instruction[1] + 1;
                stackOffset = -n - 3;
            }
            break;
        case OP_CALL:
            {
                u16 f = *(u16*)(instruction + 1);
                if(f >= header->MT->FunctionPoolSize)
                {
                    DEVEL_ASSERT(false, "Invalid function call in function %s [MAX=%u,f=%u]\n", String_CStr(functionSignature), header->MT->FunctionPoolSize, f);
                    return 1;
                }
                Function *function = header->MT->FunctionPool[f];            
                stackOffset = function->Header.RWC - function->Header.AWC;
            }
            break;
        case OP_INDCALL:
            // cannot verify validity, validity is trusted
            stackOffset = 0;
            exited = true;
            break;
        case OP_SYSCALL:
            {
                u8 f = instruction[1];
                if(f > OP_SYS_MAX_OPCODE)
                {
                    DEVEL_ASSERT(false, "Invalid system call in function %s [MAX=%d,f=%u]\n", String_CStr(functionSignature), OP_SYS_MAX_OPCODE, f);
                    return 1;
                }

                if(f == OP_SYS_EXIT)
                    exited = true;
                stackOffset = sysfnStackOffsets[f];
            }
            break;
        case OP_RET:
            stackOffset = header->RWC;
            exited = true;
            break;
        default:
            break;
        }
        DEVEL_ASSERT(stackOffset != INT32_MIN, "Edge case not handled [opcode=%u]\n", opcode);

        sp += stackOffset;
        instruction += paramOffset + 1;

        if(sp < 0 || sp > header->SWC)
        {
            DEVEL_ASSERT(false, "Stack Poiter out of scope in function %s\n", String_CStr(functionSignature));
            return 1;
        }
    } 
    return 0;
}