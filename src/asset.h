#ifndef DRAFT_ASSET_H
#define DRAFT_ASSET_H

#define CharsPerTexture     1024
#define CharsPerTextureRoot 32
#define MaxCharSupport      128
struct bitmap_font
{
    int Size;
    int SquareSize;
    int NewLine;
    texture *Texture;

    int          AdvX[MaxCharSupport];
    int          AdvY[MaxCharSupport];
    int          BearingX[MaxCharSupport];
    int          BearingY[MaxCharSupport];
    int          CharWidth[MaxCharSupport];
    int          CharHeight[MaxCharSupport];
    texture_rect TexRect[MaxCharSupport];
};

enum asset_type
{
    AssetType_Texture,
    AssetType_Font,
    AssetType_Shader,
    AssetType_Sound,
	AssetType_Stream,
	AssetType_Song,
};
enum asset_completion
{
    AssetCompletion_Incomplete,
    AssetCompletion_ThreadSafe,
    AssetCompletion_ThreadUnsafe,
};

struct asset_loader;
struct asset_entry
{
    uint64 LastLoadTime;
    asset_type Type;
    asset_completion Completion;
    string Filename;
    string ID;
    asset_loader *Loader;
    void *Param;

    union
    {
        struct
        {
            uint8 *TextureData;
            texture *Result;
        } Texture;

        struct
        {
            uint8 *TextureData;
            int TextureDataSize;
            bitmap_font *Result;
        } Font;

        struct
        {
            const char *Source;
            GLuint Result;
        } Shader;

        struct
        {
            short *Data;
            audio_clip *Result;
        } Sound;

		struct
		{
			audio_clip *Result;
		} Stream;

		struct
		{
			song *Result;
		} Song;
    };
};

struct platform_api;
struct asset_loader
{
    FT_Library FreeTypeLib;
    memory_arena Arena;
    fixed_array<asset_entry, 64> Entries;

    platform_api *Platform;
    thread_pool Pool;
    std::atomic_int NumLoadedEntries;
};

#endif
