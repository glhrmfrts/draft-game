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

struct memory_arena
{
    memory_block *CurrentBlock = NULL;
};

#endif
