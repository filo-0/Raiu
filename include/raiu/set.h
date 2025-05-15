#include <raiu/types.h>
#include <stdbool.h>
#include <stdlib.h>

#ifndef SET_T
#error "SET_T (Value type) parameter not defined!"
#endif

#if defined(SET_T)

#define T SET_T

#ifdef SET_HASH
#define H SET_HASH
#else
#define H(x) (*(x))
#endif

#ifdef SET_EQ
#define EQ SET_EQ
#else
#define EQ(a, b) (*(a) == *(b))
#endif

#ifdef SET_T_DTOR
#define T_DTOR SET_T_DTOR
#else
#define T_DTOR(x) 
#endif

#ifdef SET_T_COPY
#define T_COPY SET_T_COPY
#else
#define T_COPY(x, y) *(x) = *(y)
#endif

#define CAT(a, b) a##b
#define PASTE(a, b) CAT(a, b)
#define JOIN(prefix, name) PASTE(prefix, PASTE(_, name))

#ifdef SET_NAME
#define SET(T) SET_NAME
#define MBUCK(T) JOIN(SET_NAME, Bucket)
#define MITER(T) JOIN(SET_NAME, Iterator)
#else
#define SET_NAME SET(SET_T)
#define SET(T) JOIN(Set, T)
#define MBUCK(T) JOIN(SET_NAME, Bucket)
#define MITER(T) JOIN(SET_NAME, Iterator)
#endif

typedef struct
{
    T *Data;
    u32 Capacity;
    u32 Count;
} MBUCK(T);

static inline void JOIN(MBUCK(T), Create)(MBUCK(T) *bucket) 
{ 
    bucket->Data     = NULL;
    bucket->Capacity = 0U;
    bucket->Count    = 0U;
}
static inline void JOIN(MBUCK(T), Add)(MBUCK(T) *bucket, T *elem)
{
    if(bucket->Capacity == bucket->Count)
    {
        u32 newCapacity = bucket->Capacity ? bucket->Capacity * 2 : 1;
        bucket->Data = (T*) realloc(bucket->Data, newCapacity * sizeof(T));
        bucket->Capacity = newCapacity;
    }
    bucket->Data[bucket->Count] = *elem;
    bucket->Count += 1;
}


typedef struct 
{
    MBUCK(T) *BucketBuffer;
    u32          BucketCapacity;
    u32          Count;
} SET(T);

static inline void JOIN(SET(T), Create)(SET(T) *set) 
{ 
    set->BucketBuffer   = NULL;
    set->BucketCapacity = 0U;
    set->Count          = 0U;
}

static inline void JOIN(SET(T), Destroy)(SET(T) *set) 
{
    for (u32 i = 0; i < set->BucketCapacity; i++)
    {
        MBUCK(T) *bucket = set->BucketBuffer + i;
        for (u32 j = 0; j < bucket->Count; j++)
        {
            T *elem = bucket->Data + j;
            T_DTOR(&elem);
            (void)elem;
        }
        
        free(bucket->Data);
    }
    free(set->BucketBuffer);
}

static inline bool JOIN(SET(T), Has)(SET(T) *set, T *elem)
{
    u32 bucketID = H(elem) % set->BucketCapacity;
    MBUCK(T) *bucket = set->BucketBuffer + bucketID;
    for (u32 i = 0; i < bucket->Count; i++)
    {
        if(EQ(&bucket->Data[i], elem))
            return true;
    }
    return false;
}

static inline void JOIN(SET(T), Reserve)(SET(T) *set, u32 capacity)
{
    if(set->BucketCapacity >= capacity)
        return;

    MBUCK(T) *newBuffer = (MBUCK(T)*) calloc(capacity, sizeof(MBUCK(T)));
    
    for (u32 i = 0; i < set->BucketCapacity; i++)
    {
        MBUCK(T) *bucketFrom = set->BucketBuffer + i;
        for (u32 j = 0; j < bucketFrom->Count; j++)
        {
            T *elem = bucketFrom->Data + j;
            u32 bucketID = H(elem) % capacity;
            
            MBUCK(T) *bucketTo = newBuffer + bucketID;
            JOIN(MBUCK(T), Add)(bucketTo, elem);
        }
        free(bucketFrom->Data);
    }
    free(set->BucketBuffer);
    set->BucketBuffer = newBuffer;
    set->BucketCapacity = capacity;
}

static inline void JOIN(SET(T), Put)(SET(T) *set, T *elem)
{
    if(set->Count == set->BucketCapacity)
        JOIN(SET(T), Reserve)(set, set->BucketCapacity ? set->BucketCapacity * 2 : 1);
    
    u32 bucketID = H(elem) % set->BucketCapacity;
    JOIN(MBUCK(T), Add)(set->BucketBuffer + bucketID, elem);
    set->Count += 1;
}
static inline void JOIN(SET(T), PutCopy)(SET(T) *set, T const *elem)
{
    if(set->Count == set->BucketCapacity)
        JOIN(SET(T), Reserve)(set, set->BucketCapacity ? set->BucketCapacity * 2 : 1);
    
    u32 bucketID = H(elem) % set->BucketCapacity;
    JOIN(MBUCK(T), Add)(set->BucketBuffer + bucketID, elem);
    set->Count += 1;
}

typedef struct 
{
    SET(T)   *Set;
    MBUCK(T) *Bucket;
    sz           PairIndex;
} MITER(T);
static inline MITER(T) JOIN(MITER(T), Begin)(SET(T) *set) 
{
    MBUCK(T) *bucket = set->BucketBuffer;
    while(bucket != set->BucketBuffer + set->BucketCapacity && !bucket->Data)
        bucket++;
    return (MITER(T)) { set, bucket, 0 }; 
}
static inline bool JOIN(MITER(T), End)(const MITER(T) *i, const SET(T) *set) { return i->Bucket == set->BucketBuffer + set->BucketCapacity; }
static inline void JOIN(MITER(T), Inc)(MITER(T) *i)
{
    i->PairIndex++;
    if(i->PairIndex == i->Bucket->Count)
    {
        do i->Bucket++;
        while(i->Bucket != i->Set->BucketBuffer + i->Set->BucketCapacity && !i->Bucket->Data);

        i->PairIndex = 0;
    }
}
static inline T const *JOIN(MITER(T), AccessRO)(MITER(T) *i) { return i->Bucket->Data + i->PairIndex; }
static inline T       *JOIN(MITER(T), AccessRW)(MITER(T) *i) { return i->Bucket->Data + i->PairIndex; }

#define foreach(SET_TYPE, SET) for(SET_TYPE##_Iterator i = SET_TYPE##_Iterator##_Begin(&(SET)); !SET_TYPE##_Iterator##_End(&i, &(SET)); SET_TYPE##_Iterator##_Inc(&i))

#undef T
#undef H
#undef EQ

#undef SET_T
#undef SET_T_DTOR
#undef SET_T_COPY
#undef SET_HASH
#undef SET_EQ
#undef SET_NAME
#endif
