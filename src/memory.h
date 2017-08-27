#ifndef DRAFT_MEMORY_H
#define DRAFT_MEMORY_H

#include "common.h"

struct memory_block
{
    memory_block *Prev;
    void *Base;
    size_t Used;
    size_t Size;
};

struct memory_arena
{
    memory_block *CurrentBlock = NULL;
};

void *PushSize(memory_arena &Arena, size_t Size);
void FreeArena(memory_arena &Arena);

template<typename T>
T *PushStruct(memory_arena &Arena)
{
    auto Result = (T *)PushSize(Arena, sizeof(T));

    // call the constructor manually
    new(Result) T();
    return Result;
}

#endif
