#ifndef DRAFT_ASSET_H
#define DRAFT_ASSET_H

#include "common.h"
#include "memory.h"
#include "render.h"

struct asset_cache
{
    unordered_map<string, texture *> Textures;
    memory_arena Arena;
};

texture *LoadTextureFile(asset_cache &AssetCache, const string &Filename);

#endif
