#include <cmath>
#include <cstdlib>
#include "luna_memory.h"

#if 0
#define Alignment 4
#define InitialBlockSize 1024*1024

using namespace std;

static size_t GetAlignmentOffset(memory_arena *Arena)
{
    size_t ResultPointer = (size_t)Arena->CurrentBlock->Base + Arena->CurrentBlock->Used;
    size_t AlignMask = Alignment - 1;
    if (ResultPointer & AlignMask) {
        return Alignment - (ResultPointer & AlignMask);
    }
    return 0;
}

static size_t GetEffectiveSizeFor(memory_arena *Arena, size_t SizeInit)
{
    return SizeInit + GetAlignmentOffset(Arena);
}

void *PushSize(memory_arena *Arena, size_t SizeInit)
{
    void *Result = 0;
    size_t Size = 0;
    if (Arena->CurrentBlock) {
        Size = GetEffectiveSizeFor(Arena, SizeInit);
    }

    if (!Arena->CurrentBlock ||
        (Arena->CurrentBlock->Used + Size) > Arena->CurrentBlock->Size) {

        Size = SizeInit;
        Size = max(Size, InitialBlockSize);

        memory_block *Block = (memory_block *)calloc(1, sizeof(Block));
        Block->Base = malloc(Size);
        Block->Prev = Arena->CurrentBlock;
        Arena->CurrentBlock = Block;
    }

    assert((Arena->CurrentBlock->Used + Size) <= Arena->CurrentBlock->Size);

    size_t AlignmentOffset = GetAlignmentOffset(Arena);
    Result = Arena->CurrentBlock->Base + Arena->CurrentBlock->Used + AlignmentOffset;
    Arena->CurrentBlock->Used += Size;

    memset(Result, 0, Size);

    assert(Size >= SizeInit);
    return Result;
}
#endif
