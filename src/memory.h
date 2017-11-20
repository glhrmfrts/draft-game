#ifndef DRAFT_MEMORY_H
#define DRAFT_MEMORY_H

struct memory_block
{
    const char *Name;
    memory_block *Prev = NULL;
    void *Base;
    size_t Used;
    size_t Size;
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
    const char *Name;
    memory_arena *Arena = NULL;
    memory_pool_entry *First = NULL;
    memory_pool_entry *FirstFree = NULL;
    size_t ElemSize = 0;
};

template<typename T>
struct generic_pool : memory_pool
{
    generic_pool()
    {
        this->ElemSize = sizeof(T);
        this->Name = typeid(T).name();
    }
};

struct string_format
{
    const char *Format = NULL;
    char *Result = NULL;
};

#endif
