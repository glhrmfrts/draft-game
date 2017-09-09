// Copyright
#include "lodepng.h"

#define DefaultTextureFilter GL_NEAREST
#define DefaultTextureWrap GL_CLAMP_TO_EDGE

static texture *
LoadTextureFile(asset_cache &AssetCache, const string &Filename, uint32 Flags = 0)
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
        if (Error) {
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
