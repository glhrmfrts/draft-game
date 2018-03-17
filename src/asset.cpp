// Copyright
#include "lodepng.h"
#include <fstream>

#define DefaultTextureFilter GL_NEAREST
#define DefaultTextureWrap GL_CLAMP_TO_EDGE

static void LoadAssetThreadSafePart(void *Arg);

void InitAssetLoader(game_main *g, asset_loader &Loader, platform_api &Platform)
{
    int Error = FT_Init_FreeType(&Loader.FreeTypeLib);
    if (Error) 
    {
        Println("error font lib");
        exit(EXIT_FAILURE);
    } 

    CreateThreadPool(Loader.Pool, g, 1, 32);
    Loader.NumLoadedEntries = 0; 
    Loader.Platform = &Platform;
	Loader.Arena.Free = false;
}

void DestroyAssetLoader(asset_loader &Loader)
{
    FT_Done_FreeType(Loader.FreeTypeLib);
}

asset_entry CreateAssetEntry(asset_entry_type Type, const string &Filename, const string &ID, void *Param)
{
    asset_entry Result = {};
    Result.Type = Type;
    Result.Filename = Filename;
    Result.ID = ID;
    Result.Param = Param;
    return Result;
}

void AddAssetEntry(asset_loader &Loader, asset_entry Entry, bool KickLoadingNow = true, bool oneShot = false)
{
    Entry.Completion = AssetCompletion_Incomplete;
    Entry.Loader = &Loader;
	Entry.OneShot = oneShot;
    switch (Entry.Type)
    {
    case AssetEntryType_Texture:
    {
        auto *Result = PushStruct<texture>(Loader.Arena);
        glGenTextures(1, &Result->ID);
        Entry.Texture.Result = Result;
        break;
    }

    case AssetEntryType_Font:
    {
        auto *Result = PushStruct<bitmap_font>(Loader.Arena);
        Result->Texture = PushStruct<texture>(Loader.Arena);
        glGenTextures(1, &Result->Texture->ID);
        Entry.Font.Result = Result;
        break;
    }

    case AssetEntryType_Shader:
    {
        auto *ShaderParam = (shader_asset_param *)Entry.Param;
        GLuint Result = glCreateShader(ShaderParam->Type);
        Entry.Shader.Result = Result;
        break;
    }

    case AssetEntryType_Sound:
    {
		auto result = AudioClipCreate(&Loader.Arena, AudioClipType_Sound);
        Entry.Sound.Result = result;
		AudioCheckError();
        break;
    }

	case AssetEntryType_Stream:
	{
		auto result = AudioClipCreate(&Loader.Arena, AudioClipType_Stream);
		Entry.Stream.Result = result;
		AudioCheckError();
		break;
	}

	case AssetEntryType_Song:
	{
		Entry.Song.Result = PushStruct<song>(Loader.Arena);
		break;
	}

	case AssetEntryType_OptionsLoad:
	{
		Entry.Options.Result = PushStruct<options>(Loader.Arena);
		break;
	}
    }
    Loader.Entries.Add(Entry);
	Loader.Active = true;

	if (KickLoadingNow)
	{
		AddJob(Loader.Pool, LoadAssetThreadSafePart, (void *)&Loader.Entries.vec[Loader.Entries.Count() - 1]);
	}
}

inline void
AddShaderProgramEntries(asset_loader &Loader, shader_program &Program)
{
    asset_entry VertexEntry = CreateAssetEntry(
        AssetEntryType_Shader,
        Program.VertexShaderParam.Path,
        Program.VertexShaderParam.Path,
        (void *)&Program.VertexShaderParam
    );
    asset_entry FragmentEntry = CreateAssetEntry(
        AssetEntryType_Shader,
        Program.FragmentShaderParam.Path,
        Program.FragmentShaderParam.Path,
        (void *)&Program.FragmentShaderParam
    );
    AddAssetEntry(Loader, VertexEntry);
    AddAssetEntry(Loader, FragmentEntry);
}

inline bitmap_font *
FindBitmapFont(asset_loader &Loader, const string &ID)
{
    for (int i = 0; i < Loader.Entries.Count(); i++)
	{
		auto &Entry = Loader.Entries.vec[i];
        if (Entry.Type == AssetEntryType_Font && Entry.ID == ID)
        {
            return Entry.Font.Result;
        }
    }
    return NULL;
}

inline texture *
FindTexture(asset_loader &Loader, const string &ID)
{
	for (int i = 0; i < Loader.Entries.Count(); i++)
	{
		auto &Entry = Loader.Entries.vec[i];
        if (Entry.Type == AssetEntryType_Texture && Entry.ID == ID)
        {
            return Entry.Texture.Result;
        }
    }
    return NULL;
}

inline audio_clip *
FindSound(asset_loader &Loader, const string &ID)
{
	for (int i = 0; i < Loader.Entries.Count(); i++)
	{
		auto &Entry = Loader.Entries.vec[i];
        if (Entry.Type == AssetEntryType_Sound && Entry.ID == ID)
        {
            return Entry.Sound.Result;
        }
    }
    return NULL;
}

inline audio_clip *
FindStream(asset_loader &Loader, const string &ID)
{
	for (int i = 0; i < Loader.Entries.Count(); i++)
	{
		auto &Entry = Loader.Entries.vec[i];
		if (Entry.Type == AssetEntryType_Stream && Entry.ID == ID)
		{
			return Entry.Stream.Result;
		}
	}
	return NULL;
}

inline song *
FindSong(asset_loader &Loader, const string &ID)
{
	for (int i = 0; i < Loader.Entries.Count(); i++)
	{
		auto &Entry = Loader.Entries.vec[i];
		if (Entry.Type == AssetEntryType_Song && Entry.ID == ID)
		{
			return Entry.Song.Result;
		}
	}
	return NULL;
}

inline options *
FindOptions(asset_loader &loader, const string &id)
{
	for (int i = 0; i < loader.Entries.Count(); i++)
	{
		auto &entry = loader.Entries.vec[i];
		if (entry.Type == AssetEntryType_OptionsLoad && entry.ID == id)
		{
			return entry.Options.Result;
		}
	}
	return NULL;
}

static void LoadAssetThreadSafePart(void *Arg)
{
    auto *Entry = (asset_entry *)Arg;
	switch (Entry->Type)
	{
	case AssetEntryType_Texture:
	{
		uint32 Flags = (uintptr_t)Entry->Param;
		auto *Result = Entry->Texture.Result;
		Result->Filename = Entry->Filename;
		Result->Target = GL_TEXTURE_2D;
		Result->Filters = { DefaultTextureFilter, DefaultTextureFilter };
		Result->Wrap = { DefaultTextureWrap, DefaultTextureWrap };
		Result->Flags = Flags;
		if (Flags & TextureFlag_WrapRepeat)
		{
			Result->Wrap = { GL_REPEAT, GL_REPEAT };
		}
		if (Flags & TextureFlag_Mipmap)
		{
			Result->Filters.Min = GL_LINEAR_MIPMAP_LINEAR;
			Result->Filters.Mag = GL_LINEAR_MIPMAP_LINEAR;
		}
		if (Flags & TextureFlag_Nearest)
		{
			Result->Filters.Min = GL_NEAREST;
			Result->Filters.Mag = GL_NEAREST;
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
					Data[i + 1] = 0;
					Data[i + 2] = 255;
					Data[i + 3] = 255;
				}
			}
		}

		Entry->Texture.TextureData = (uint8 *)PushSize(Entry->Loader->Arena, Data.size(), "texture data");
		memcpy(Entry->Texture.TextureData, &Data[0], Data.size());
		break;
	}

	case AssetEntryType_Font:
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

			Result->AdvX[i] = Face->glyph->advance.x >> 6;
			Result->BearingX[i] = Face->glyph->bitmap_left;
			Result->CharWidth[i] = Face->glyph->bitmap.width;
			Result->AdvY[i] = (Face->glyph->metrics.height - Face->glyph->metrics.horiBearingY) >> 6;
			Result->BearingY[i] = Face->glyph->bitmap_top;
			Result->CharHeight[i] = Face->glyph->bitmap.rows;
			Result->NewLine = std::max(Result->NewLine, int(Face->glyph->bitmap.rows));

			float Step = 1.0f / float(CharsPerTextureRoot);
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

	case AssetEntryType_Shader:
	{
		Entry->Shader.Source = ReadFile(Entry->Filename.c_str());
		break;
	}

	case AssetEntryType_Sound:
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
		Result->Info = Info;
		Result->SampleCount = SampleCount;

		Entry->Sound.Data = Data;
		break;
	}

	case AssetEntryType_Stream:
	{
		auto result = Entry->Stream.Result;
		result->SndFile = sf_open(Entry->Filename.c_str(), SFM_READ, &result->Info);
		if (!result->SndFile)
		{
			Println("error reading sound file " + Entry->Filename);
			break;
		}
		sf_seek(result->SndFile, 0, SEEK_SET);

		result->SampleCount = result->Info.frames * result->Info.channels;
		break;
	}

	case AssetEntryType_Song:
	{
		using json = nlohmann::json;

		std::string content(ReadFile(Entry->Filename.c_str()));
		json j;
		try
		{
			j = json::parse(content);
		}
		catch (std::exception e)
		{
			Println("error reading json song file " + Entry->Filename);
			throw e;
		}

		song *result = Entry->Song.Result;
		result->Bpm = (int)j["bpm"];

		for (auto trackFile : j["tracks"])
		{
			result->Files.push_back(trackFile);
			AddAssetEntry(*Entry->Loader, CreateAssetEntry(AssetEntryType_Stream, trackFile, trackFile, NULL), true);
		}

		for (json::iterator it = j["names"].begin(); it != j["names"].end(); ++it)
		{
			result->Names[it.key()] = it.value();
		}

		//free((void *)content);
		break;
	}

	case AssetEntryType_OptionsLoad:
	{
		std::string content(ReadFile(Entry->Filename.c_str()));
		
		options *result = Entry->Options.Result;
		ParseOptions(content, &Entry->Loader->Arena, result);
		break;
	}

	case AssetEntryType_OptionsSave:
	{
		using json = nlohmann::json;
		json j = {};
		options *opts = (options *)Entry->Param;
		j["audio/music_gain"] = opts->Values["audio/music_gain"]->Float;
		j["audio/sfx_gain"] = opts->Values["audio/sfx_gain"]->Float;
		j["graphics/resolution"] = opts->Values["graphics/resolution"]->Int;
		j["graphics/fullscreen"] = opts->Values["graphics/fullscreen"]->Bool;
		j["graphics/aa"] = opts->Values["graphics/aa"]->Bool;
		j["graphics/bloom"] = opts->Values["graphics/bloom"]->Bool;

		std::ofstream file;
		file.open(Entry->Filename);
		file << j.dump(4);
		file.close();
		break;
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
    case AssetEntryType_Texture:
    {
        auto *Result = Entry->Texture.Result;
        Bind(*Result, 0);
        UploadTexture(*Result, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, Entry->Texture.TextureData);
        Unbind(*Result, 0);

        ApplyTextureParameters(*Result, 0);
        break;
    }

    case AssetEntryType_Font:
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

    case AssetEntryType_Shader:
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

    case AssetEntryType_Sound:
    {
        auto *Result = Entry->Sound.Result;
		AudioClipSetSoundData(Result, Entry->Sound.Data);
		AudioCheckError();
        break;
    }

	case AssetEntryType_Stream:
	{
		break;
	}

	case AssetEntryType_Song:
	{
		auto result = Entry->Song.Result;
		for (auto &file : result->Files)
		{
			result->Tracks.push_back(FindStream(*Entry->Loader, file));
		}
		break;
	}
    }

    Entry->Completion = AssetCompletion_ThreadUnsafe;
	if (Entry->OneShot)
	{
		Entry->Completion = AssetCompletion_Done;
	}
}

bool Update(asset_loader &Loader)
{
    if (Loader.Active && !int(Loader.Pool.NumJobs))
    {
		Loader.Active = false;
		for (int i = 0; i < Loader.Entries.Count(); i++)
		{
			auto &Entry = Loader.Entries.vec[i];
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
	for (int i = 0; i < Loader.Entries.Count(); i++)
	{
		auto &Entry = Loader.Entries.vec[i];
		if (Entry.Completion == AssetCompletion_Done) continue;

        uint64 LastWriteTime = Loader.Platform->GetFileLastWriteTime(Entry.Filename.c_str());
        int32 Compare = Loader.Platform->CompareFileTime(LastWriteTime, Entry.LastLoadTime);
        if (Compare != 0)
        {
            printf("[asset] Asset changed: %s\n", Entry.Filename.c_str());
            AddJob(Loader.Pool, LoadAssetThreadSafePart, (void *)&Entry);
        }
    }
}
