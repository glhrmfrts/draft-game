#ifndef LUNA_MEMORY_H
#define LUNA_MEMORY_H

struct memory_block
{
    memory_block *Prev;
    void *Base;
    size_t Used;
    size_t Size;
};

struct memory_arena
{
    memory_block *CurrentBlock;
};

#define PushStruct(type, Arena) ((type *)PushSize(Arena, sizeof(Type)))
void *PushSize(memory_arena *Arena, size_t Size);

#endif
