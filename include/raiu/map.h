#include <raiu/types.h>
#include <stdbool.h>
#include <stdlib.h>

#ifndef MAP_T
#error "MAP_T (Value type) parameter not defined!"
#endif
#ifndef MAP_K
#error "MAP_K (Key type) parameter not defined!"
#endif
#if defined(MAP_T) && defined(MAP_K)

#define T MAP_T
#define K MAP_K

#ifdef MAP_HASH
#define H MAP_HASH
#else
#define H(x) (*(x))
#endif

#ifdef MAP_EQ
#define EQ MAP_EQ
#else
#define EQ(a, b) (*(a) == *(b))
#endif

#ifdef MAP_T_DTOR
#define T_DTOR MAP_T_DTOR
#else
#define T_DTOR(x) 
#endif

#ifdef MAP_K_DTOR
#define K_DTOR MAP_K_DTOR
#else
#define K_DTOR(x) 
#endif

#ifdef MAP_T_COPY
#define T_COPY MAP_T_COPY
#else
#define T_COPY(x, y) *(x) = *(y)
#endif

#ifdef MAP_K_COPY
#define K_COPY MAP_K_COPY
#else
#define K_COPY(x, y) *(x) = *(y)
#endif

#define CAT(a, b) a##b
#define PASTE(a, b) CAT(a, b)
#define JOIN(prefix, name) PASTE(prefix, PASTE(_, name))

#ifdef MAP_NAME
#define MAP(K, T) MAP_NAME
#define MPAIR(K, T) JOIN(MAP_NAME, Pair)
#define MBUCK(K, T) JOIN(MAP_NAME, Bucket)
#define MITER(K, T) JOIN(MAP_NAME, Iterator)
#else
#define MAP_NAME MAP(MAP_K, MAP_T)
#define MAP(K, T) JOIN(Map, JOIN(K, T))
#define MPAIR(K, T) JOIN(MAP_NAME, Pair)
#define MBUCK(K, T) JOIN(MAP_NAME, Bucket)
#define MITER(K, T) JOIN(MAP_NAME, Iterator)
#endif

typedef struct
{
    K Key;
    T Val;
} MPAIR(K, T);

typedef struct
{
    MPAIR(K, T) *Data;
    u32 Capacity;
    u32 Count;
} MBUCK(K, T);

static inline void JOIN(MBUCK(K, T), Create)(MBUCK(K, T) *bucket) 
{ 
    bucket->Data     = NULL;
    bucket->Capacity = 0U;
    bucket->Count    = 0U;
}
static inline void JOIN(MBUCK(K, T), Add)(MBUCK(K, T) *bucket, MPAIR(K, T) *pair)
{
    if(bucket->Capacity == bucket->Count)
    {
        u32 newCapacity = bucket->Capacity ? bucket->Capacity * 2 : 1;
        bucket->Data = (MPAIR(K, T)*) realloc(bucket->Data, newCapacity * sizeof(MPAIR(K, T)));
        bucket->Capacity = newCapacity;
    }
    bucket->Data[bucket->Count] = *pair;
    bucket->Count += 1;
}


typedef struct 
{
    MBUCK(K, T) *BucketBuffer;
    u32          BucketCapacity;
    u32          Count;
} MAP(K, T);

static inline void JOIN(MAP(K, T), Create)(MAP(K, T) *map) 
{ 
    map->BucketBuffer   = NULL;
    map->BucketCapacity = 0U;
    map->Count          = 0U;
}

static inline void JOIN(MAP(K, T), Destroy)(MAP(K, T) *map) 
{
    for (u32 i = 0; i < map->BucketCapacity; i++)
    {
        MBUCK(K, T) *bucket = map->BucketBuffer + i;
        for (u32 j = 0; j < bucket->Count; j++)
        {
            MPAIR(K, T) *pair = bucket->Data + j;
            K_DTOR(&pair->Key);
            T_DTOR(&pair->Val);
            (void)pair;
        }
        
        free(bucket->Data);
    }
    free(map->BucketBuffer);
}

static inline T *JOIN(MAP(K, T), AtRW)(MAP(K, T) *map, K *key)
{
    u32 bucketID = H(key) % map->BucketCapacity;
    MBUCK(K, T) *bucket = map->BucketBuffer + bucketID;
    for (u32 i = 0; i < bucket->Count; i++)
    {
        if(EQ(&bucket->Data[i].Key, key))
            return &bucket->Data[i].Val;
    }
    return NULL;
}
static inline T const *JOIN(MAP(K, T), AtRO)(const MAP(K, T) *map, K const *key)
{
    u32 bucketID = H(key) % map->BucketCapacity;
    MBUCK(K, T) *bucket = map->BucketBuffer + bucketID;
    for (u32 i = 0; i < bucket->Count; i++)
    {
        if(EQ(&bucket->Data[i].Key, key))
            return &bucket->Data[i].Val;
    }
    return NULL;
}

static inline void JOIN(MAP(K, T), Reserve)(MAP(K, T) *map, u32 capacity)
{
    if(map->BucketCapacity >= capacity)
        return;

    MBUCK(K, T) *newBuffer = (MBUCK(K, T)*) calloc(capacity, sizeof(MBUCK(K, T)));
    
    for (u32 i = 0; i < map->BucketCapacity; i++)
    {
        MBUCK(K, T) *bucketFrom = map->BucketBuffer + i;
        for (u32 j = 0; j < bucketFrom->Count; j++)
        {
            MPAIR(K, T) *pair = bucketFrom->Data + j;
            u32 bucketID = H(&pair->Key) % capacity;
            
            MBUCK(K, T) *bucketTo = newBuffer + bucketID;
            JOIN(MBUCK(K, T), Add)(bucketTo, pair);
        }
        free(bucketFrom->Data);
    }
    free(map->BucketBuffer);
    map->BucketBuffer = newBuffer;
    map->BucketCapacity = capacity;
}

static inline void JOIN(MAP(K, T), Put)(MAP(K, T) *map, K *key, T *val)
{
    if(map->Count == map->BucketCapacity)
        JOIN(MAP(K, T), Reserve)(map, map->BucketCapacity ? map->BucketCapacity * 2 : 1);
    
    u32 bucketID = H(key) % map->BucketCapacity;
    MBUCK(K, T) *bucket = map->BucketBuffer + bucketID;
    for (u32 i = 0; i < bucket->Count; i++)
    {
        if(!EQ(&bucket->Data[i].Key, key))
            continue;
        MPAIR(K, T) *p = bucket->Data + i;
        p->Val = *val;
        return;
    }

    // not found
    MPAIR(K, T) pair = { *key, *val };
    JOIN(MBUCK(K, T), Add)(map->BucketBuffer + bucketID, &pair);
    map->Count += 1;
}
static inline void JOIN(MAP(K, T), PutCopy)(MAP(K, T) *map, K const *key, T const *val)
{
    if(map->Count == map->BucketCapacity)
        JOIN(MAP(K, T), Reserve)(map, map->BucketCapacity ? map->BucketCapacity * 2 : 1);
    
    u32 bucketID = H(key) % map->BucketCapacity;
    MBUCK(K, T) *bucket = map->BucketBuffer + bucketID;
    for (u32 i = 0; i < bucket->Count; i++)
    {
        if(!EQ(&bucket->Data[i].Key, key))
            continue;
        MPAIR(K, T) *p = bucket->Data + i;
        T_COPY(&p->Val, val);
        return;
    }

    // not found
    MPAIR(K, T) pair;
    K_COPY(&pair.Key, key);
    T_COPY(&pair.Val, val);
    JOIN(MBUCK(K, T), Add)(map->BucketBuffer + bucketID, &pair);
    map->Count += 1;
}

typedef struct 
{
    MAP(K, T)   *Map;
    MBUCK(K, T) *Bucket;
    sz           PairIndex;
} MITER(K, T);
static inline MITER(K, T) JOIN(MITER(K, T), Begin)(MAP(K, T) *map) 
{
    MBUCK(K, T) *bucket = map->BucketBuffer;
    while(bucket != map->BucketBuffer + map->BucketCapacity && !bucket->Data)
        bucket++;
    return (MITER(K, T)) { map, bucket, 0 }; 
}
static inline bool JOIN(MITER(K, T), End)(const MITER(K, T) *i, const MAP(K, T) *map) { return i->Bucket == map->BucketBuffer + map->BucketCapacity; }
static inline void JOIN(MITER(K, T), Inc)(MITER(K, T) *i)
{
    i->PairIndex++;
    if(i->PairIndex == i->Bucket->Count)
    {
        do i->Bucket++;
        while(i->Bucket != i->Map->BucketBuffer + i->Map->BucketCapacity && !i->Bucket->Data);

        i->PairIndex = 0;
    }
}
static inline MPAIR(K, T) const *JOIN(MITER(K, T), AccessRO)(MITER(K, T) *i) { return i->Bucket->Data + i->PairIndex; }
static inline MPAIR(K, T)       *JOIN(MITER(K, T), AccessRW)(MITER(K, T) *i) { return i->Bucket->Data + i->PairIndex; }

static inline void JOIN(MAP(K, T), CreateFromList)(MAP(K, T) *map, const MPAIR(K, T) *list, u32 count)
{
    JOIN(MAP(K, T), Create)(map);
    JOIN(MAP(K, T), Reserve)(map, count);
    for (u32 i = 0; i < count; i++)
    {
        const MPAIR(K, T) *p = list + i;
        JOIN(MAP(K, T), PutCopy)(map, &p->Key, &p->Val);
    }
}

#define foreach(MAP_TYPE, MAP) for(MAP_TYPE##_Iterator i = MAP_TYPE##_Iterator##_Begin(&(MAP)); !MAP_TYPE##_Iterator##_End(&i, &(MAP)); MAP_TYPE##_Iterator##_Inc(&i))

#undef T
#undef K
#undef H
#undef EQ
#undef T_DTOR
#undef K_DTOR

#undef MAP_T
#undef MAP_K
#undef MAP_T_DTOR
#undef MAP_K_DTOR
#undef MAP_T_COPY
#undef MAP_K_COPY
#undef MAP_HASH
#undef MAP_EQ
#undef MAP_NAME
#endif
