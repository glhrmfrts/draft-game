// Copyright
#include <typeinfo>

#define Alignment 4
#define InitialBlockSize 1024*1024

static size_t GetAlignmentOffset(memory_arena &Arena)
{
    size_t ResultPointer = (size_t)Arena.CurrentBlock->Base + Arena.CurrentBlock->Used;
    size_t AlignMask = Alignment - 1;

    if (ResultPointer & AlignMask)
    {
        return Alignment - (ResultPointer & AlignMask);
    }
    return 0;
}

static size_t GetEffectiveSizeFor(memory_arena &Arena, size_t SizeInit)
{
    return SizeInit + GetAlignmentOffset(Arena);
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

    size_t AlignmentOffset = GetAlignmentOffset(Arena);
    Result = (void *)((uintptr_t)Arena.CurrentBlock->Base + Arena.CurrentBlock->Used + AlignmentOffset);

    Arena.CurrentBlock->Used += Size;

    assert(Size >= SizeInit);
    return Result;
}

template<typename T>
T *PushStruct(memory_arena &Arena)
{
    auto Result = (T *)PushSize(Arena, sizeof(T), typeid(T).name());

    // call the constructor manually
    new(Result) T();
    return Result;
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
