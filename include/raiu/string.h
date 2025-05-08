#pragma once

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "types.h"

#define SMALL_STRING_LEN 23

typedef struct
{
    union StringData 
    {
        struct HeapBuffer
        {
            char *Data;
            sz    Capacity;
        } Heap;
        char Stack[SMALL_STRING_LEN + 1];
    } Data;
    sz         Length;
} String;

static inline void String_Create(String *this, const char *s)
{
    if(s == NULL)
    {
        this->Data.Stack[0] = 0;
        this->Length = 0;
    }

    sz len = strlen(s);
    char *thisBuffer;
    if(len <= SMALL_STRING_LEN)
        thisBuffer = this->Data.Stack;
    else
    {
        this->Data.Heap.Data     = (char*) malloc(sizeof(char) * (len + 1));
        this->Data.Heap.Capacity = len + 1;
        thisBuffer = this->Data.Heap.Data;
    }
    memcpy(thisBuffer, s, len * sizeof(char));
    thisBuffer[len] = 0;
    this->Length = len;
}
static inline void String_Copy(String *this, const String *other)
{
    if(other->Length <= SMALL_STRING_LEN)
        *this = *other;
    else
    {
        this->Data.Heap.Data     = (char*) malloc(other->Data.Heap.Capacity * sizeof(char));
        this->Data.Heap.Capacity = other->Data.Heap.Capacity;
        this->Length             = other->Length;

        memcpy(this->Data.Heap.Data, other->Data.Heap.Data, other->Length);
        this->Data.Heap.Data[this->Length] = 0;
    }
}
static inline void String_Destroy(String *this)
{
    if(this->Length <= SMALL_STRING_LEN)    
        return;
    free(this->Data.Heap.Data);
}
static inline char String_At(const String *this, sz i) 
{
    if(this->Length <= SMALL_STRING_LEN)
    {
        return this->Data.Stack[i];
    }
    else
    {
        return this->Data.Heap.Data[i];
    }
}
static inline sz String_Capacity(const String *this)
{
    return this->Length <= SMALL_STRING_LEN ? SMALL_STRING_LEN + 1 : this->Data.Heap.Capacity;
}
static inline char *String_Str(String *this) 
{ 
    return this->Length <= SMALL_STRING_LEN ? this->Data.Stack : this->Data.Heap.Data; 
}
static inline const char *String_CStr(const String *this) 
{ 
    return this->Length <= SMALL_STRING_LEN ? this->Data.Stack : this->Data.Heap.Data; 
}
static inline void String_Reserve(String *this, sz characters)
{
    sz newCapacity = characters + 1;
    if(String_Capacity(this) >= newCapacity)
        return;

    if(this->Length <= SMALL_STRING_LEN)
    {
        if(newCapacity < SMALL_STRING_LEN)
            return;
        
        char *buffer = (char*) malloc(sizeof(char) * newCapacity);
        memcpy(buffer, this->Data.Stack, this->Length * sizeof(char));
        buffer[this->Length] = 0;
        this->Data.Heap.Data     = buffer;
        this->Data.Heap.Capacity = newCapacity;
    }
    else
    {
        this->Data.Heap.Data     = (char*) realloc(this->Data.Heap.Data, newCapacity);
        this->Data.Heap.Capacity = newCapacity;
    }
}

static inline void String_PushBack(String *this, char c)
{
    if(this->Length == String_Capacity(this))
        String_Reserve(this, this->Length ? this->Length * 2 : 1);
    sz newLength = this->Length + 1;
    char *thisBuffer = newLength <= SMALL_STRING_LEN ? this->Data.Stack : this->Data.Heap.Data;
    thisBuffer[newLength - 1] = c;
    thisBuffer[newLength + 0] = 0;
    this->Length = newLength;
}
static inline void String_Concat(String *this, const String *other)
{
    sz newLength = this->Length + other->Length;
    String_Reserve(this, newLength);

    char *thisBuffer  = newLength <= SMALL_STRING_LEN ? this->Data.Stack : this->Data.Heap.Data;
    const char *otherBuffer = String_CStr(other);
    memcpy(thisBuffer + this->Length, otherBuffer, other->Length * sizeof(char));
    thisBuffer[newLength] = 0;
    this->Length += other->Length;
}
static inline void String_ConcatStr(String *this, const char *other)
{
    sz otherLen = strlen(other);
    sz newLength = this->Length + otherLen;
    String_Reserve(this, newLength);

    char *thisBuffer  = newLength <= SMALL_STRING_LEN ? this->Data.Stack : this->Data.Heap.Data;
    memcpy(thisBuffer + this->Length, other, otherLen * sizeof(char));
    thisBuffer[newLength] = 0;
    this->Length += otherLen;
}

static inline bool String_Equal(const String *this, const String *other) { return strcmp(String_CStr(this), String_CStr(other)) == 0; }
static inline bool String_EqualStr(const String *this, const char *str) { return strcmp(String_CStr(this), str) == 0; }
static inline sz String_Hash(const String *this)
{
    const char *str = String_CStr(this);
    sz hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return hash;
}
