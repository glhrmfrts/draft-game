#ifndef DRAFT_MEMORY_H
#define DRAFT_MEMORY_H

struct memory_block
{
    memory_block *Prev = NULL;
    void *Base;
    size_t Used;
    size_t Size;
    const char *Name;
};

struct allocator
{
    virtual ~allocator() {}
};

struct memory_arena : allocator
{
    memory_block *CurrentBlock = NULL;
};

struct memory_pool_entry : allocator
{
    void *Base;
    memory_pool_entry *Next = NULL;
    size_t Used = 0;
};

struct memory_pool
{
    memory_pool_entry *First = NULL;
    memory_pool_entry *FirstFree = NULL;
    size_t ElemSize = 0;

#ifdef DRAFT_DEBUG
    const char *DEBUGName;
#endif
};

template<typename T>
struct generic_pool : memory_pool
{
    generic_pool()
    {
        this->ElemSize = sizeof(T);
#ifdef DRAFT_DEBUG
        this->DEBUGName = typeid(T).name();
#endif
    }
};

struct string_format
{
    const char *Format = NULL;
    char *Result = NULL;
};

#endif
