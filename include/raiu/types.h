#pragma once

#include <stdint.h>
#include <stddef.h>
#include <assert.h>

typedef void *ref;
typedef size_t sz;

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float  f32;
typedef double f64;
typedef char   ch8; 

typedef union _Byte
{
    i8 Int;
    u8 UInt;
    ch8 Char;
} Byte;
typedef union _HWord
{
    i16 Int;
    u16 UInt;
    Byte Byte[2];
} HWord;
typedef union _Word
{
    i32 Int;
    u32 UInt;
    f32 Float;
    HWord HWord[2];
    Byte  Byte[4];
} Word;
typedef union _DWord
{
    i64 Int;
    u64 UInt;
    f64 Float;
    Byte  Byte[8];
    HWord HWord[4];
    Word  Word[2];
    void *Ptr;
    union _Byte  *BytePtr;
    union _HWord *HWordPtr;
    union _Word  *WordPtr;
    union _DWord *DWordPtr;
} DWord;

#define SIZEOF_BYTE 1
#define SIZEOF_HWORD 2
#define SIZEOF_WORD 4
#define SIZEOF_DWORD 8
#define SIZEOF_PTR 8

static_assert(SIZEOF_BYTE == sizeof(Byte), "Byte size is not 1!");
static_assert(SIZEOF_HWORD == sizeof(HWord), "HWord size is not 2!");
static_assert(SIZEOF_WORD == sizeof(Word), "Word size is not 4!");
static_assert(SIZEOF_DWORD == sizeof(DWord), "DWord size is not 8!");
static_assert(SIZEOF_PTR == sizeof(void*), "Architecture must be 64-bit!");

inline static DWord DereferenceUnallignedDWord(const Word *p) { return *((DWord*)p); }
inline static void AssignUnallignedDWord(Word *dest, const Word *src) { *((DWord*)dest) = *((DWord*)src); }

inline static Word  ByteToHWord(Byte b)  { return (Word)  { .UInt = (u16)b.UInt }; }
inline static Word  ByteToWord(Byte b)   { return (Word)  { .UInt = (u32)b.UInt }; }
inline static DWord ByteToDWord(Byte b)  { return (DWord) { .UInt = (u64)b.UInt }; }

inline static Word  HWordToWord(HWord h)  { return (Word)  { .UInt = (u32)h.UInt }; }
inline static DWord HWordToDWord(HWord h) { return (DWord) { .UInt = (u64)h.UInt }; }

inline static DWord WordToDWord(Word b) { return (DWord) { .UInt = (u64)b.UInt }; }

inline static Word IntToWord(i32 v)   { return (Word) { .Int   = v }; }
inline static Word UIntToWord(u32 v)  { return (Word) { .UInt  = v }; }
inline static Word FloatToWord(f32 v) { return (Word) { .Float = v }; }

inline static DWord IntToDWord(i64 v)   { return (DWord) { .Int   = v }; }
inline static DWord UIntToDWord(u64 v)  { return (DWord) { .UInt  = v }; }
inline static DWord FloatToDWord(f64 v) { return (DWord) { .Float = v }; }
inline static DWord RefToDWord(ref v)   { return (DWord) { .Ptr = v   }; }