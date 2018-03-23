// Copyright
#include <typeinfo>

#define Alignment 4
#define InitialBlockSize 1024*1024

static size_t GetAlignmentOffset(size_t base, size_t used)
{
    size_t resultPointer = base + used;
    size_t alignMask = Alignment - 1;

    if (resultPointer & alignMask)
    {
        return Alignment - (resultPointer & alignMask);
    }
    return 0;
}

static size_t GetEffectiveSizeFor(memory_arena &Arena, size_t SizeInit)
{
    return SizeInit + GetAlignmentOffset((size_t)Arena.CurrentBlock->Base, Arena.CurrentBlock->Used);
}

void *PushSize(memory_arena &Arena, size_t SizeInit, const char *Name)
{
    void *Result = 0;
    size_t Size = SizeInit;
    if (Arena.CurrentBlock)
    {
        Size = GetEffectiveSizeFor(Arena, SizeInit);
    }

    if (!Arena.CurrentBlock ||
        (Arena.CurrentBlock->Used + Size) > Arena.CurrentBlock->Size)
    {
        size_t AllocSize = SizeInit;
        if (InitialBlockSize > AllocSize)
        {
            AllocSize = InitialBlockSize;
        }

        memory_block *Block = (memory_block *)calloc(1, sizeof(memory_block));
        Block->Name = ++Name;
        Block->Base = malloc(AllocSize);
        Block->Prev = Arena.CurrentBlock;
        Block->Size = AllocSize;
        Arena.CurrentBlock = Block;

#ifdef DRAFT_DEBUG
        printf("[memory] Allocating memory block '%s' of %zu bytes at address %p\n", Name, AllocSize, Block->Base);
#endif
    }

    assert((Arena.CurrentBlock->Used + Size) <= Arena.CurrentBlock->Size);

    size_t AlignmentOffset = GetAlignmentOffset((size_t)Arena.CurrentBlock->Base, Arena.CurrentBlock->Used);
    Result = (void *)((uintptr_t)Arena.CurrentBlock->Base + Arena.CurrentBlock->Used + AlignmentOffset);

    Arena.CurrentBlock->Used += Size;

    assert(Size >= SizeInit);
    return Result;
}

template<typename T>
T *PushStruct(memory_arena &arena)
{
    auto result = (T *)PushSize(arena, sizeof(T), typeid(T).name());

    // call the constructor manually
    new(result) T();
    return result;
}

void FreeArena(memory_arena &Arena)
{
    while (Arena.CurrentBlock)
    {
        auto Block = Arena.CurrentBlock;
#ifdef DRAFT_DEBUG
        printf("[memory] Freeing memory block '%s' of %zu bytes at address %p\n", Block->Name, Block->Size, Block->Base);
#endif
        Arena.CurrentBlock = Block->Prev;
        free(Block->Base);
        free(Block);
    }
}

memory_arena::~memory_arena()
{
	if (Free) FreeArena(*this);
}

memory_pool_entry *GetEntry(memory_pool &pool)
{
    assert(pool.ElemSize > 0);
    if (pool.FirstFree)
    {
        auto result = pool.FirstFree;
        pool.FirstFree = result->Next;

#ifdef DRAFT_DEBUG
        //printf("[DEBUG:%s] reuse entry %p\n", pool.Name, result->Base);
#endif
        return result;
    }
    
#ifdef DRAFT_DEBUG
    //printf("[DEBUG:%s] alloc entry\n", pool.Name);
#endif
    
    auto entry = PushStruct<memory_pool_entry>(*pool.Arena);
    entry->Next = pool.First;
    pool.First = entry;
    entry->Base = PushSize(*pool.Arena, pool.ElemSize, pool.Name);
    return entry;
}

void PutEntry(memory_pool &pool, memory_pool_entry *entry, memory_pool_entry *prev)
{
    auto next = entry->Next;
    entry->Next = pool.FirstFree;
    if (prev)
    {
        prev->Next = next;
    }
#ifdef DRAFT_DEBUG
    //printf("[DEBUG:%s] put entry %p\n", pool.Name, entry->Base);
#endif
    entry->Used = 0;
    pool.FirstFree = entry;
}

void FreeEntry(memory_pool &pool, memory_pool_entry *argEntry)
{
    memory_pool_entry *entry = pool.First;
    memory_pool_entry *prev = NULL;

    while (entry)
    {
        if (entry == argEntry)
        {
            PutEntry(pool, entry, prev);
            break;
        }

        prev = entry;
        entry = entry->Next;
    }
}

void FreeEntryFromData(memory_pool &pool, void *data)
{
    memory_pool_entry *entry = pool.First;
    memory_pool_entry *prev = NULL;

    while (entry)
    {
        if (entry->Base == data)
        {
            PutEntry(pool, entry, prev);
            break;
        }

        prev = entry;
        entry = entry->Next;
    }
}

void ResetPool(memory_pool &pool)
{
    pool.First = NULL;
    pool.FirstFree = NULL;
}

void *PushSize(memory_pool_entry *entry, size_t size, const char *name)
{
    size_t alignOffset = GetAlignmentOffset((size_t)entry->Base, entry->Used);
    void *addr = (void *)((uintptr_t)entry->Base + entry->Used + alignOffset);
    entry->Used += size;
    return addr;
}

template<typename T>
T *PushStruct(memory_pool_entry *entry)
{
    auto result = static_cast<T *>(PushSize(entry, sizeof(T), ""));

    // call the constructor manually
    new(result) T();
    return result;
}

void *PushSize(allocator *alloc, size_t size, const char *name)
{
    auto arena = dynamic_cast<memory_arena *>(alloc);
    if (arena)
    {
        return PushSize(*arena, size, name);
    }

    auto entry = dynamic_cast<memory_pool_entry *>(alloc);
    if (entry)
    {
        return PushSize(entry, size, name);
    }

    assert(false);
    return NULL;
}

template<typename T>
T *PushStruct(allocator *alloc)
{
	const char *typeName = typeid(T).name();
    auto arena = dynamic_cast<memory_arena *>(alloc);
    if (arena)
    {
        return PushStruct<T>(*arena);
    }

    auto entry = dynamic_cast<memory_pool_entry *>(alloc);
    if (entry)
    {
        return PushStruct<T>(entry);
    }

    assert(false);
    return NULL;
}

void InitFormat(string_format *format, const char *str, size_t size, allocator *alloc)
{
	format->Size = size;
    format->Format = str;
    format->Result = static_cast<char *>(PushSize(alloc, size, str));
}

void Sprintf(char *result, size_t size, const char *format, va_list args)
{
#ifdef _WIN32
	vsprintf_s(result, size, format, args);
#else
	vsprintf(result, format, args);
#endif
}

char *Format(string_format *format, ...)
{
    assert(format->Result);

    va_list args;
    va_start(args, format);
    Sprintf(format->Result, format->Size, format->Format, args);
    va_end(args);
    return format->Result;
}
