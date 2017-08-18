#include <iostream>
#include "lodepng.h"
#include "luna_asset.h"

#define DefaultTextureFilter GL_NEAREST
#define DefaultTextureWrap GL_CLAMP_TO_EDGE

texture *LoadTextureFile(asset_cache &AssetCache, const string &Filename)
{
    texture *Result;
    if (AssetCache.Textures.find(Filename) == AssetCache.Textures.end()) {
        Result = new texture;
        Result->Target = GL_TEXTURE_2D;
        Result->Filters = {DefaultTextureFilter, DefaultTextureFilter};
        Result->Wrap = {DefaultTextureWrap, DefaultTextureWrap};
        ApplyTextureParameters(*Result, 0);

        vector<uint8> Data;
        uint32 Error = lodepng::decode(Data, Result->Width, Result->Height, Filename);
        if (Error) {
            std::cout << "texture: " << Filename << " " << lodepng_error_text(Error) << std::endl;
            exit(EXIT_FAILURE);
        }

        Bind(*Result, 0);
        UploadTexture(*Result, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, &Data[0]);
        Unbind(*Result, 0);

        AssetCache.Textures[Filename] = Result;
    }
    else {
        Result = AssetCache.Textures[Filename];
    }

    return Result;
}
