#ifndef DRAFT_ASSET_H
#define DRAFT_ASSET_H

struct asset_cache
{
    std::unordered_map<string, texture *> Textures;
    memory_arena Arena;
};

#endif
