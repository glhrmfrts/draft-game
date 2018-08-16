#ifndef DRAFT_ASSET_H
#define DRAFT_ASSET_H

#define CharsPerTexture     1024
#define CharsPerTextureRoot 32
#define MaxCharSupport      128

struct asset_loader;
struct asset_job;
struct material;

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

struct material_lib
{
    std::unordered_map<std::string, material *> Materials;
};

struct mesh_asset_data
{
    std::vector<float> Vertices;
    std::vector<uint32> Indices;
    std::vector<mesh_part> Parts;
    std::vector<std::string> Materials;
    std::string MaterialLib;
    mesh_flags::type Flags = 0;
    size_t VertexSize;
};

struct asset_entry
{
    uint64 LastLoadTime;
    asset_entry_type Type;
    asset_completion Completion;
    string Filename;
    string ID;
    asset_loader *Loader;
    asset_job *Job;
    void *Param;
	  bool OneShot;

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

		struct
		{
			options *Result;
		} Options;

        struct
        {
            level *Result;
        } Level;

		struct
		{
            mesh_asset_data *Data;
			mesh *Result;
		} Mesh;

        struct
        {
            material_lib *Result;
        } MaterialLib;
    };

	asset_entry() {}
	~asset_entry() {}
};

struct asset_job
{
    bool Finished = false;
    fixed_array<asset_entry, 64> Entries;
    std::atomic_int NumLoadedEntries;
    std::string Name;
};

struct platform_api;
struct asset_loader
{
    FT_Library FreeTypeLib;
    memory_arena Arena;
    fixed_array<asset_entry, 512> Entries;
    std::atomic_int NumLoadedEntries;
    asset_job *CurrentJob;
    platform_api *Platform;
    thread_pool Pool;
	bool Active;
};

#endif
