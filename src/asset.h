#ifndef DRAFT_ASSET_H
#define DRAFT_ASSET_H

#include <ft2build.h>
#include FT_FREETYPE_H

#define CharsPerTexture     1024
#define CharsPerTextureRoot 32
#define MaxCharSupport      128
struct bitmap_font
{
    int Size;
    int SquareSize;
    texture *Texture;

    int          AdvX[MaxCharSupport];
    int          AdvY[MaxCharSupport];
    int          BearingX[MaxCharSupport];
    int          BearingY[MaxCharSupport];
    int          CharWidth[MaxCharSupport];
    int          CharHeight[MaxCharSupport];
    texture_rect TexRect[MaxCharSupport];
};

struct asset_cache
{
    FT_Lib FreeTypeLib;
    std::unordered_map<string, texture *> Textures;
    memory_arena Arena;
};

#endif
