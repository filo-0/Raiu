#include <raiu/types.h>
#include <stdbool.h>
#include <stdlib.h>

#ifndef MAP_T
#error "MAP_T (Value type) parameter not defined!"
#endif
#ifndef MAP_K
#error "MAM_K (Key type) parameter not defined!"
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

#define CAT(a, b) a##b
#define PASTE(a, b) CAT(a, b)
#define JOIN(prefix, name) PASTE(prefix, PASTE(_, name))

#define MAP(K, T) JOIN(Map, JOIN(K, T))
#define MPAIR(K, T) JOIN(MPair, JOIN(K, T))
#define MBUCK(K, T) JOIN(MBucket, JOIN(K, T))

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
            MPAIR(K, T) *pair = bucket->Data + i;
            K_DTOR(&pair->Key);
            T_DTOR(&pair->Val);
            (void)pair;
        }
        
        free(bucket->Data);
    }
    free(map->BucketBuffer);
}

static inline T *JOIN(MAP(K, T), At)(MAP(K, T) *map, K *key)
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
    MPAIR(K, T) pair = { *key, *val };
    JOIN(MBUCK(K, T), Add)(map->BucketBuffer + bucketID, &pair);
    map->Count += 1;
}

#undef T
#undef K
#undef H
#undef EQ
#endif
