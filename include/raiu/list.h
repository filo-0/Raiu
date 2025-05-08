#include <stdlib.h>
#include <raiu/types.h>

#if defined(LIST_T)
#define T LIST_T

#define CAT(a, b) a##b
#define PASTE(a, b) CAT(a, b)
#define JOIN(prefix, name) PASTE(prefix, PASTE(_, name))

#if defined(LIST_T_DTOR)
#define T_DTOR LIST_T_DTOR
#else
#define T_DTOR(x) ;
#endif 

#if defined(LIST_T_CPY)
#define T_CPY LIST_T_CPY
#else
#define T_CPY(x, y) *(x) = *(y)
#endif


#define LIST(type) JOIN(List, type)

typedef struct
{
    T   *Data;
    u32  Capacity;
    u32  Count;
} LIST(T);

static inline void JOIN(LIST(T), Create)(LIST(T) *list) 
{ 
    list->Data     = NULL;  
    list->Capacity = 0U;
    list->Count    = 0U;
}
static inline void JOIN(LIST(T), Destroy)(LIST(T) *list) 
{
    for (u32 i = 0; i < list->Count; i++)
        T_DTOR(list->Data + i); 
    free(list->Data); 
}

static inline T *JOIN(LIST(T), At)(LIST(T) *list, u32 i) { return list->Data + i; }
static inline T *JOIN(LIST(T), Back)(LIST(T) *list) { return list->Data + list->Count - 1; }
static inline T *JOIN(LIST(T), Front)(LIST(T) *list) { return list->Data; }

static inline void JOIN(LIST(T), Reserve)(LIST(T) *list, u32 capacity)
{
    if(capacity <= list->Capacity)
        return;
    list->Data = (T*) realloc(list->Data, capacity * sizeof(T));
    list->Capacity = capacity;
}

static inline void JOIN(LIST(T), PushBackCopy)(LIST(T) *list, T *ref) 
{
    if(list->Capacity == list->Count)
        JOIN(LIST(T), Reserve)(list, list->Count ? list->Count * 2 : 1);
    T_CPY(list->Data + list->Count, ref);
    list->Count += 1;
}
static inline void JOIN(LIST(T), PushBack)(LIST(T) *list, T *ref) 
{
    if(list->Capacity == list->Count)
        JOIN(LIST(T), Reserve)(list, list->Count ? list->Count * 2 : 1);
    list->Data[list->Count] = *ref;
    list->Count += 1;
}



#undef T
#undef T_DTOR
#undef T_CPY
#endif // LIST_T