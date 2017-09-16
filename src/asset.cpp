// Copyright
#include "lodepng.h"

#define DefaultTextureFilter GL_NEAREST
#define DefaultTextureWrap GL_CLAMP_TO_EDGE

inline void InitFreeType(asset_cache &AssetCache)
{
    int Error = FT_Init_FreeType(&AssetCache.FreeTypeLib);
    if (Error)
    {
        Println("error font lib");
        exit(EXIT_FAILURE);
    }
}

texture *LoadTextureFile(asset_cache &AssetCache, const string &Filename, uint32 Flags = 0)
{
    texture *Result;
    if (AssetCache.Textures.find(Filename) == AssetCache.Textures.end())
    {
        Result = PushStruct<texture>(AssetCache.Arena);
        Result->Filename = Filename;
        Result->Target = GL_TEXTURE_2D;
        Result->Filters = {DefaultTextureFilter, DefaultTextureFilter};
        Result->Wrap = {DefaultTextureWrap, DefaultTextureWrap};
        Result->Flags = Flags;
        if (Flags & Texture_WrapRepeat)
        {
            Result->Wrap = {GL_REPEAT, GL_REPEAT};
        }
        ApplyTextureParameters(*Result, 0);

        vector<uint8> Data;
        uint32 Error = lodepng::decode(Data, Result->Width, Result->Height, Filename);
        if (Error)
        {
            std::cout << "texture: " << Filename << " " << lodepng_error_text(Error) << std::endl;

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

        Bind(*Result, 0);
        UploadTexture(*Result, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, &Data[0]);
        Unbind(*Result, 0);

        if (Flags & Texture_Mipmap)
        {
            Result->Filters.Min = GL_LINEAR_MIPMAP_LINEAR;
            Result->Filters.Mag = GL_LINEAR_MIPMAP_LINEAR;
            ApplyTextureParameters(*Result, 0);
        }

        AssetCache.Textures[Filename] = Result;
    }
    else
    {
        Result = AssetCache.Textures[Filename];
    }

    return Result;
}

static inline int NextP2(int n)
{
    int Result = 1;
    while (Result < n)
    {
        Result <<= 1;
    }
    return Result;
}

bitmap_font *LoadBitmapFontFromTTF(asset_cache &AssetCache, const string &Filename, int Size)
{
    FT_Face Face;
    int Error = FT_New_Face(AssetCache.FreeTypeLib, Filename.c_str(), 0, &Face);
    if (Error)
    {
        Println("error loading font");
        exit(EXIT_FAILURE);
    }

    FT_Set_Pixel_Sizes(Face, Size, Size);

    auto *Result = PushStruct<bitmap_font>(AssetCache.Arena);
    Result->Size = Size;
    Result->SquareSize = NextP2(Size);

    int TextureDataSize = Result->SquareSize * Result->SquareSize * CharsPerTexture;
    GLubyte *TextureData[TextureDataSize];
    memset(TextureData, 0, TextureDataSize);

    for (int i = 0; i < MaxCharSupport; i++)
    {
        FT_Load_Glyph(Face, FT_Get_Char_Index(Face, i), FT_LOAD_DEFAULT);
        FT_Render_Glyph(Face->glyph, FT_RENDER_MODE_NORMAL);
        FT_Bitmap *Bitmap = &Face->glyph->bitmap;
        int Width = Bitmap->Width;
        int Height = Bitmap->Rows;

        if (Width > Result->SquareSize || Height > Result->SquareSize)
        {
            return;
        }

        int Row = (i % CharsPerTexture) / CharsPerTextureRoot;
        int Col = (i % CharsPerTexture) % CharsPerTextureRoot;
        int Pitch = CharsPerTextureRoot * Result->SquareSize;

        for (int Char = 0; Char < Height; Char++)
        {
            memcpy(
                TextureData + Row*Pitch*Result->SquareSize + Col*Result->SquareSize + Char*Pitch,
                Bitmap->buffer + (Height - Char - 1) * Width,
                Width
            );
        }

        Result->AdvX[i] = Face->glyph->advance.x>>6;
        Result->BearingX[i] = Face->glyph->bitmap_left;
        Result->CharWidth[i] = Face->glyph->bitmap.width;
        Result->AdvY[i] = (Face->glyph->metrics.height - face->glyph->metrics.horiBearingY)>>6;
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

    auto *Texture = PushStruct<texture>(AssetCache.Arena);
    Texture->Width = Texture->Height = Result->SquareSize * CharsPerTextureRoot;
    Texture->Target = GL_TEXTURE_2D;
    Texture->Filters = {DefaultTextureFilter, DefaultTextureFilter};
    Texture->Wrap = {DefaultTextureWrap, DefaultTextureWrap};
    ApplyTextureParameters(*Texture, 0);

    GLubyte RGBA[TextureDataSize*4];
    for (int i = 0; i < TextureDataSize; i++)
    {
        int Idx = i * 4;
        RGBA[Idx] = 255;
        RGBA[Idx+1] = 255;
        RGBA[Idx+2] = 255;
        RGBA[Idx+3] = TextureData[i];
    }

    Bind(*Texture, 0);
    UploadTexture(*Texture, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, RGBA);
    Unbind(*Texture, 0);

    Result->Texture = Texture;
    return Result;
}
