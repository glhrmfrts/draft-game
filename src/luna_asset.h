#ifndef LUNA_ASSET_H
#define LUNA_ASSET_H

#include "luna_common.h"
#include "luna_render.h"

struct asset_cache
{
    unordered_map<string, texture *> Textures;
};

texture *LoadTextureFile(asset_cache *AssetCache, const string &Filename);

#endif
