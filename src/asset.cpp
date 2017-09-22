// Copyright
#include "lodepng.h"

#define DefaultTextureFilter GL_NEAREST
#define DefaultTextureWrap GL_CLAMP_TO_EDGE

static inline int
NextP2(int n)
{
    int Result = 1;
    while (Result < n)
    {
        Result <<= 1;
    }
    return Result;
}

static long int
GetFileSize(FILE *handle)
{
    long int result = 0;

    fseek(handle, 0, SEEK_END);
    result = ftell(handle);
    rewind(handle);

    return result;
}

static const char *
ReadFile(const char *filename)
{
    FILE *handle;
    fopen_s(&handle, filename, "r");
    long int length = GetFileSize(handle);
    char *buffer = new char[length + 1];
    int i = 0;
    char c = 0;
    while ((c = fgetc(handle)) != EOF) {
        buffer[i++] = c;
        if (i >= length) break;
    }

    buffer[i] = '\0';
    fclose(handle);

    return (const char *)buffer;
}

void InitAssetLoader(asset_loader &Loader, platform_api &Platform)
{
    int Error = FT_Init_FreeType(&Loader.FreeTypeLib);
    if (Error)
    {
        Println("error font lib");
        exit(EXIT_FAILURE);
    }

    CreateThreadPool(Loader.Pool, 1, 32);
    Loader.NumLoadedEntries = 0;
    Loader.Platform = &Platform;
}

void DestroyAssetLoader(asset_loader &Loader)
{
    FT_Done_FreeType(Loader.FreeTypeLib);
    DestroyThreadPool(Loader.Pool);
}

void AddAssetEntry(asset_loader &Loader, asset_type Type, const string &Filename, const string &ID, void *Param)
{
    asset_entry Entry;
    Entry.Type = Type;
    Entry.Filename = Filename;
    Entry.ID = ID;
    Entry.Completion = AssetCompletion_Incomplete;
    Entry.Loader = &Loader;
    Entry.Param = Param;
    switch (Type)
    {
    case AssetType_Texture:
    {
        auto *Result = PushStruct<texture>(Loader.Arena);
        glGenTextures(1, &Result->ID);
        Entry.Texture.Result = Result;
        break;
    }

    case AssetType_Font:
    {
        auto *Result = PushStruct<bitmap_font>(Loader.Arena);
        Result->Texture = PushStruct<texture>(Loader.Arena);
        glGenTextures(1, &Result->Texture->ID);
        Entry.Font.Result = Result;
        break;
    }

    case AssetType_Shader:
    {
        auto *ShaderParam = (shader_asset_param *)Param;
        GLuint Result = glCreateShader(ShaderParam->Type);
        Entry.Shader.Result = Result;
        break;
    }

    case AssetType_Sound:
    {
        auto *Result = PushStruct<sound>(Loader.Arena);
        alGenBuffers(1, &Result->Buffer);
        Entry.Sound.Result = Result;
        break;
    }
    }
    Loader.Entries.push_back(Entry);
}

inline void
AddShaderProgramEntries(asset_loader &Loader, shader_program &Program)
{
    AddAssetEntry(
        Loader,
        AssetType_Shader,
        Program.VertexShaderParam.Path,
        Program.VertexShaderParam.Path,
        (void *)&Program.VertexShaderParam
    );
    AddAssetEntry(
        Loader,
        AssetType_Shader,
        Program.FragmentShaderParam.Path,
        Program.FragmentShaderParam.Path,
        (void *)&Program.FragmentShaderParam
    );
}

inline bitmap_font *
FindBitmapFont(asset_loader &Loader, const string &ID)
{
    for (auto &Entry : Loader.Entries)
    {
        if (Entry.Type == AssetType_Font && Entry.ID == ID)
        {
            return Entry.Font.Result;
        }
    }
    return NULL;
}

inline texture *
FindTexture(asset_loader &Loader, const string &ID)
{
    for (auto &Entry : Loader.Entries)
    {
        if (Entry.Type == AssetType_Texture && Entry.ID == ID)
        {
            return Entry.Texture.Result;
        }
    }
    return NULL;
}

inline sound *
FindSound(asset_loader &Loader, const string &ID)
{
    for (auto &Entry : Loader.Entries)
    {
        if (Entry.Type == AssetType_Sound && Entry.ID == ID)
        {
            return Entry.Sound.Result;
        }
    }
}

static inline ALenum
GetFormatFromChannels(int Count)
{
    switch (Count)
    {
    case 1:
        return AL_FORMAT_MONO16;
    case 2:
        return AL_FORMAT_STEREO16;
    default:
        return 0;
    }
}

static void
LoadAssetThreadSafePart(void *Arg)
{
    auto *Entry = (asset_entry *)Arg;
    switch (Entry->Type)
    {
    case AssetType_Texture:
    {
        uint32 Flags = (uintptr_t)Entry->Param;
        auto *Result = Entry->Texture.Result;
        Result->Filename = Entry->Filename;
        Result->Target = GL_TEXTURE_2D;
        Result->Filters = {DefaultTextureFilter, DefaultTextureFilter};
        Result->Wrap = {DefaultTextureWrap, DefaultTextureWrap};
        Result->Flags = Flags;
        if (Flags & Texture_WrapRepeat)
        {
            Result->Wrap = {GL_REPEAT, GL_REPEAT};
        }
        if (Flags & Texture_Mipmap)
        {
            Result->Filters.Min = GL_LINEAR_MIPMAP_LINEAR;
            Result->Filters.Mag = GL_LINEAR_MIPMAP_LINEAR;
        }

        vector<uint8> Data;
        uint32 Error = lodepng::decode(Data, Result->Width, Result->Height, Entry->Filename);
        if (Error)
        {
            std::cout << "texture: " << Entry->Filename << " " << lodepng_error_text(Error) << std::endl;
            // generate magenta texture if it's missing
            Result->Width = 512;
            Result->Height = 512;

            Data.resize(Result->Width * Result->Height * 4);
            for (uint32 y = 0; y < Result->Height; y++)
            {
                for (uint32 x = 0; x < Result->Width; x++)
                {
                    uint32 i = (y * Result->Width + x) * 4;
                    Data[i] = 255;
                    Data[i+1] = 0;
                    Data[i+2] = 255;
                    Data[i+3] = 255;
                }
            }
        }

        Entry->Texture.TextureData = (uint8 *)PushSize(Entry->Loader->Arena, Data.size(), "texture data");
        memcpy(Entry->Texture.TextureData, &Data[0], Data.size());
        break;
    }

    case AssetType_Font:
    {
        FT_Face Face;
        int Error = FT_New_Face(Entry->Loader->FreeTypeLib, Entry->Filename.c_str(), 0, &Face);
        if (Error)
        {
            Println("error loading font");
            break;
        }

        int Size = (int)(long int)Entry->Param;
        FT_Set_Pixel_Sizes(Face, Size, Size);

        auto *Result = Entry->Font.Result;
        Result->Size = Size;
        Result->SquareSize = NextP2(Size);

        int TextureDataSize = Result->SquareSize * Result->SquareSize * CharsPerTexture;
        Entry->Font.TextureDataSize = TextureDataSize;
        Entry->Font.TextureData = (uint8 *)PushSize(Entry->Loader->Arena, TextureDataSize, "font texture data");
        memset(Entry->Font.TextureData, 0, TextureDataSize);

        for (int i = 0; i < MaxCharSupport; i++)
        {
            FT_Load_Glyph(Face, FT_Get_Char_Index(Face, i), FT_LOAD_DEFAULT);
            FT_Render_Glyph(Face->glyph, FT_RENDER_MODE_NORMAL);
            FT_Bitmap *Bitmap = &Face->glyph->bitmap;
            int Width = Bitmap->width;
            int Height = Bitmap->rows;

            if (Width > Result->SquareSize || Height > Result->SquareSize)
            {
                continue;
            }

            int Row = (i % CharsPerTexture) / CharsPerTextureRoot;
            int Col = (i % CharsPerTexture) % CharsPerTextureRoot;
            int Pitch = CharsPerTextureRoot * Result->SquareSize;

            for (int Char = 0; Char < Height; Char++)
            {
                memcpy(
                    Entry->Font.TextureData + Row*Pitch*Result->SquareSize + Col*Result->SquareSize + Char*Pitch,
                    Bitmap->buffer + (Height - Char - 1) * Width,
                    Width
                );
            }

            Result->AdvX[i] = Face->glyph->advance.x>>6;
            Result->BearingX[i] = Face->glyph->bitmap_left;
            Result->CharWidth[i] = Face->glyph->bitmap.width;
            Result->AdvY[i] = (Face->glyph->metrics.height - Face->glyph->metrics.horiBearingY)>>6;
            Result->BearingY[i] = Face->glyph->bitmap_top;
            Result->CharHeight[i] = Face->glyph->bitmap.rows;
            Result->NewLine = std::max(Result->NewLine, int(Face->glyph->bitmap.rows));

            float Step = 1.0f/float(CharsPerTextureRoot);
            float TextureSize = Result->SquareSize * CharsPerTextureRoot;
            Result->TexRect[i] = texture_rect{
                float(Col)*Step,
                float(Row)*Step,
                float(Col)*Step + (Face->glyph->bitmap.width / TextureSize),
                float(Row)*Step + (Face->glyph->bitmap.rows / TextureSize),
            };
        }

        FT_Done_Face(Face);
        break;
    }

    case AssetType_Shader:
    {
        Entry->Shader.Source = ReadFile(Entry->Filename.c_str());
        break;
    }

    case AssetType_Sound:
    {
        SF_INFO Info;
        SNDFILE *SndFile = sf_open(Entry->Filename.c_str(), SFM_READ, &Info);
        if (!SndFile)
        {
            Println("error reading sound file " + Entry->Filename);
            break;
        }
        sf_seek(SndFile, 0, SEEK_SET);

        int SampleCount = Info.frames * Info.channels;
        short *Data = (short *)PushSize(Entry->Loader->Arena, SampleCount * sizeof(short), "sound data");
        sf_read_short(SndFile, Data, SampleCount);
        sf_close(SndFile);

        auto *Result = Entry->Sound.Result;
        Result->Frames = Info.frames;
        Result->SampleRate = Info.samplerate;
        Result->SampleCount = SampleCount;
        Result->Channels = Info.channels;

        Entry->Sound.Data = Data;
    }
    }

    Entry->LastLoadTime = Entry->Loader->Platform->GetFileLastWriteTime(Entry->Filename.c_str());
    Entry->Completion = AssetCompletion_ThreadSafe;
    Entry->Loader->NumLoadedEntries++;
}

static void
LoadAssetThreadUnsafePart(asset_entry *Entry)
{
    switch (Entry->Type)
    {
    case AssetType_Texture:
    {
        auto *Result = Entry->Texture.Result;
        Bind(*Result, 0);
        UploadTexture(*Result, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, Entry->Texture.TextureData);
        Unbind(*Result, 0);

        ApplyTextureParameters(*Result, 0);
        break;
    }

    case AssetType_Font:
    {
        auto *Result = Entry->Font.Result;
        auto *Texture = Result->Texture;
        Texture->Width = Texture->Height = Result->SquareSize * CharsPerTextureRoot;
        Texture->Target = GL_TEXTURE_2D;
        Texture->Filters = {DefaultTextureFilter, DefaultTextureFilter};
        Texture->Wrap = {DefaultTextureWrap, DefaultTextureWrap};

        GLubyte *RGBA = new GLubyte[Entry->Font.TextureDataSize * 4];
        for (int i = 0; i < Entry->Font.TextureDataSize; i++)
        {
            int Idx = i * 4;
            RGBA[Idx] = 255;
            RGBA[Idx+1] = 255;
            RGBA[Idx+2] = 255;
            RGBA[Idx+3] = Entry->Font.TextureData[i];
        }

        Bind(*Texture, 0);
        UploadTexture(*Texture, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, RGBA);
        Unbind(*Texture, 0);

        ApplyTextureParameters(*Texture, 0);

        delete[] RGBA;
        break;
    }

    case AssetType_Shader:
    {
        auto *Param = (shader_asset_param *)Entry->Param;
        GLuint Result = Entry->Shader.Result;
        CompileShader(Result, Entry->Shader.Source);
        if (Param->Type == GL_VERTEX_SHADER)
        {
            Param->ShaderProgram->VertexShader = Result;
        }
        else if (Param->Type == GL_FRAGMENT_SHADER)
        {
            Param->ShaderProgram->FragmentShader = Result;
        }
        Param->Callback(Param);
        break;
    }

    case AssetType_Sound:
    {
        auto *Result = Entry->Sound.Result;
        alBufferData(
            Result->Buffer,
            GetFormatFromChannels(Result->Channels),
            Entry->Sound.Data,
            Result->SampleCount * sizeof(short),
            Result->SampleRate
        );
        break;
    }
    }

    Entry->Completion = AssetCompletion_ThreadUnsafe;
}

void StartLoading(asset_loader &Loader)
{
    for (auto &Entry : Loader.Entries)
    {
        AddJob(Loader.Pool, LoadAssetThreadSafePart, (void *)&Entry);
    }
}

bool Update(asset_loader &Loader)
{
    if (!int(Loader.Pool.NumJobs))
    {
        for (auto &Entry : Loader.Entries)
        {
            if (Entry.Completion == AssetCompletion_ThreadSafe)
            {
                LoadAssetThreadUnsafePart(&Entry);
            }
        }
        return true;
    }
    return false;
}

void CheckAssetsChange(asset_loader &Loader)
{
    Update(Loader);
    for (auto &Entry : Loader.Entries)
    {
        uint64 LastWriteTime = Loader.Platform->GetFileLastWriteTime(Entry.Filename.c_str());
        int32 Compare = Loader.Platform->CompareFileTime(LastWriteTime, Entry.LastLoadTime);
        if (Compare != 0)
        {
            printf("[asset] Asset changed: %s\n", Entry.Filename.c_str());
            AddJob(Loader.Pool, LoadAssetThreadSafePart, (void *)&Entry);
        }
    }
}
