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

	case AssetEntryType_Level:
	{
		Entry.Level.Result = PushStruct<level>(Loader.Arena);
		break;
	}

	case AssetEntryType_Mesh:
	{
		Entry.Mesh.Result = PushStruct<mesh>(Loader.Arena);
        Entry.Mesh.Data = PushStruct<mesh_asset_data>(Loader.Arena);
		break;
	}

    case AssetEntryType_MaterialLib:
    {
        Entry.MaterialLib.Result = PushStruct<material_lib>(Loader.Arena);
        break;
    }
    }
    Loader.Entries.push_back(Entry);
	Loader.Active = true;

	if (KickLoadingNow)
	{
		AddJob(Loader.Pool, LoadAssetThreadSafePart, (void *)&Loader.Entries[Loader.Entries.size() - 1]);
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
    for (int i = 0; i < Loader.Entries.size(); i++)
	{
		auto &Entry = Loader.Entries[i];
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
	for (int i = 0; i < Loader.Entries.size(); i++)
	{
		auto &Entry = Loader.Entries[i];
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
	for (int i = 0; i < Loader.Entries.size(); i++)
	{
		auto &Entry = Loader.Entries[i];
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
	for (int i = 0; i < Loader.Entries.size(); i++)
	{
		auto &Entry = Loader.Entries[i];
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
	for (int i = 0; i < Loader.Entries.size(); i++)
	{
		auto &Entry = Loader.Entries[i];
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
	for (int i = 0; i < loader.Entries.size(); i++)
	{
		auto &entry = loader.Entries[i];
		if (entry.Type == AssetEntryType_OptionsLoad && entry.ID == id)
		{
			return entry.Options.Result;
		}
	}
	return NULL;
}

inline level *
FindLevel(asset_loader &loader, const string &id)
{
	for (int i = 0; i < loader.Entries.size(); i++)
	{
		auto &entry = loader.Entries[i];
		if (entry.Type == AssetEntryType_Level && entry.ID == id)
		{
			return entry.Level.Result;
		}
	}
	return NULL;
}

inline mesh *
FindMesh(asset_loader &loader, const string &id)
{
    for (int i = 0; i < loader.Entries.size(); i++)
	{
		auto &entry = loader.Entries[i];
		if (entry.Type == AssetEntryType_Mesh && entry.ID == id)
		{
			return entry.Mesh.Result;
		}
	}
	return NULL;
}

inline material_lib *
FindMaterialLib(asset_loader &loader, const string &id)
{
    for (int i = 0; i < loader.Entries.size(); i++)
	{
		auto &entry = loader.Entries[i];
		if (entry.Type == AssetEntryType_MaterialLib && entry.ID == id)
		{
			return entry.MaterialLib.Result;
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
			AddAssetEntry(*Entry->Loader, CreateAssetEntry(AssetEntryType_Stream, trackFile, trackFile, NULL));
		}

		for (json::iterator it = j["clips"].begin(); it != j["clips"].end(); ++it)
		{
			std::string file = it.value();
			result->ClipFiles[hash_string()(it.key())] = file;
			AddAssetEntry(*Entry->Loader, CreateAssetEntry(AssetEntryType_Sound, file, file, NULL));
		}

		for (json::iterator it = j["names"].begin(); it != j["names"].end(); ++it)
		{
			result->Names[hash_string()(it.key())] = it.value();
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

	case AssetEntryType_Level:
	{
		std::ifstream file(Entry->Filename);

		level *result = Entry->Level.Result;
		ParseLevel(file, &Entry->Loader->Arena, result);

		std::string songPath = "data/audio/" + result->SongName + "/song.json";
		AddAssetEntry(*Entry->Loader, CreateAssetEntry(AssetEntryType_Song, songPath, result->SongName, NULL));
		break;
	}

	case AssetEntryType_Mesh:
	{
		std::ifstream file(Entry->Filename);
		if (Entry->Filename.find(".obj") != std::string::npos)
		{
			struct mesh_obj_data
			{
				std::vector<vec3> Vertices;
				std::vector<vec2> Uvs;
				std::vector<vec3> Normals;
				std::vector<color> Colors;
			};

			struct mesh_obj_face_group
			{
				std::vector<int> Data;
				std::string Material;
				int NumFaces = 0;
				bool HasUvs = false;
				bool HasNormals = false;
				bool HasColors = false;
			};

            auto meshData = Entry->Mesh.Data;
			meshData->Flags = (unsigned long)Entry->Param;
			mesh_obj_data data = {};
			mesh_obj_face_group *currentFaceGroup = NULL;
			std::vector<mesh_obj_face_group> faceGroups;
			std::string line;
			while (std::getline(file, line))
			{
				std::istringstream iss(line);
				std::string field;

				float x, y, z;
				if (iss >> field)
				{
					if (field == "mtllib")
					{
						iss >> field;
						std::string path = GetParentPath(Entry->Filename) + "/" + field;
						AddAssetEntry(*Entry->Loader, CreateAssetEntry(AssetEntryType_MaterialLib, path, path, NULL));
						meshData->MaterialLib = path;
					}
					else if (field == "v")
					{
						iss >> x >> y >> z;
						if (meshData->Flags & mesh_flags::UpY)
						{
							data.Vertices.push_back(vec3(x, -z, y));
						}
						else
						{
							data.Vertices.push_back(vec3(x, y, z));
						}
					}
					else if (field == "vt")
					{
						iss >> x >> y;
						data.Uvs.push_back(vec2(x, y));
					}
					else if (field == "vn")
					{
						iss >> x >> y >> z;
						if (meshData->Flags & mesh_flags::UpY)
						{
							data.Normals.push_back(vec3(x, -z, y));
						}
						else
						{
							data.Normals.push_back(vec3(x, y, z));
						}
					}
					else if (field == "usemtl")
					{
						iss >> field;
						faceGroups.emplace_back();
						currentFaceGroup = &faceGroups[faceGroups.size() - 1];
						currentFaceGroup->Material = field;
						meshData->Materials.push_back(field);
					}
					else if (field == "o" || field == "g")
					{
					}
					else if (field == "f")
					{
						std::string parts;
						std::vector<vec3> faceData;

						while (iss >> parts)
						{
							size_t pos = 0;
							size_t index = 0;
							std::string part;
							std::string delim = "/";
							faceData.push_back(vec3());
							auto &vertex = faceData.back();

							while (true)
							{
								pos = parts.find(delim);
								if (pos == string::npos)
								{
									if (parts.empty()) break;
									pos = parts.size();
								}

								part = parts.substr(0, pos);
								if (!part.empty())
								{
									int i = atoi(part.c_str());
									if (index == 0)
									{
										vertex.x = i;
									}
									else if (index == 1)
									{
										vertex.y = i;
										currentFaceGroup->HasUvs = true;
									}
									else if (index == 2)
									{
										vertex.z = i;
										currentFaceGroup->HasNormals = true;
									}
								}

								parts.erase(0, pos + delim.length());
								index++;
							}

							currentFaceGroup->NumFaces++;
						}

						if (faceData.size() > 3)
						{
							std::vector<vec3> newFaceData;

							for (size_t i = 1; i < faceData.size() - 1; i++)
							{
								newFaceData.push_back(faceData[0]);
								newFaceData.push_back(faceData[i]);
								newFaceData.push_back(faceData[i + 1]);
							}

							faceData = newFaceData;
						}

						for (auto const &face : faceData)
						{
							currentFaceGroup->Data.push_back(face.x);
							if (currentFaceGroup->HasUvs) currentFaceGroup->Data.push_back(face.y);
							if (currentFaceGroup->HasNormals) currentFaceGroup->Data.push_back(face.z);
						}
					}
				}
			}

			std::unordered_map<std::string, int> indexMap;
			mesh_obj_data combinedData;
            size_t vertexSize = 0;
			size_t offset = 0;
			int nextIndex = 0;
			int procNormIndex = 0;
			for (size_t i = 0; i < faceGroups.size(); i++)
			{
				auto &fg = faceGroups[i];
				int numElements = fg.Data.size();

				for (int d = 0; d < numElements;)
				{
					std::string indexKey;
					int vertIndex, uvIndex, normIndex;
					vec3 vert;
					vec2 uv;
					vec3 normal;

					vertIndex = fg.Data[d++] - 1;
					vert = data.Vertices[vertIndex];
					indexKey.append(std::to_string(vertIndex));
                    vertexSize = 3;

					if (fg.HasUvs)
					{
						uvIndex = fg.Data[d++] - 1;
						uv = data.Uvs[uvIndex];
						indexKey.append(std::to_string(uvIndex));
						meshData->Flags |= mesh_flags::HasUvs;
                        vertexSize += 2;
					}
					if (fg.HasNormals)
					{
						normIndex = fg.Data[d++] - 1;
						normal = data.Normals[normIndex];
						indexKey.append(std::to_string(normIndex));
						meshData->Flags |= mesh_flags::HasNormals;
                        vertexSize += 3;
					}

					int combinedIndex = 0;
					if (indexMap.find(indexKey) == indexMap.end())
					{
						combinedIndex = nextIndex++;
						indexMap[indexKey] = combinedIndex;

						combinedData.Vertices.push_back(vert);
						if (fg.HasUvs)
						{
							combinedData.Uvs.push_back(uv);
						}
						if (fg.HasNormals)
						{
							combinedData.Normals.push_back(normal);
						}
					}
					else
					{
						combinedIndex = indexMap[indexKey];
					}
					meshData->Indices.push_back(combinedIndex);
				}

				mesh_part part;
				part.PrimitiveType = GL_TRIANGLES;
				part.Offset = offset;
				part.Count = meshData->Indices.size() - offset;
				meshData->Parts.push_back(part);

				offset = meshData->Indices.size();
			}

			if (!(meshData->Flags & mesh_flags::HasNormals))
			{
				vertexSize += 3;
				combinedData.Normals.resize(combinedData.Vertices.size());

				for (size_t i = 0; i < meshData->Indices.size(); i += 3)
				{
					uint32 idx1 = meshData->Indices[i];
					uint32 idx2 = meshData->Indices[i+1];
					uint32 idx3 = meshData->Indices[i+2];
					vec3 v1 = combinedData.Vertices[idx1];
					vec3 v2 = combinedData.Vertices[idx2];
					vec3 v3 = combinedData.Vertices[idx3];
					vec3 normal = glm::cross(v2 - v1, v3 - v1);

					combinedData.Normals[idx1] += normal;
					combinedData.Normals[idx2] += normal;
					combinedData.Normals[idx3] += normal;
				}

				for (size_t i = 0; i < combinedData.Normals.size(); i++)
				{
					combinedData.Normals[i] = glm::normalize(combinedData.Normals[i]);
				}
			}

			for (size_t i = 0; i < combinedData.Vertices.size(); i++)
			{
				vec3 vert = combinedData.Vertices[i];
				meshData->Vertices.insert(meshData->Vertices.end(), &vert[0], &vert[0] + 3);

				if (meshData->Flags & mesh_flags::HasUvs)
				{
					vec2 uv = combinedData.Uvs[i];
					meshData->Vertices.insert(meshData->Vertices.end(), &uv[0], &uv[0] + 2);
				}

				vec3 normal = combinedData.Normals[i];
				meshData->Vertices.insert(meshData->Vertices.end(), &normal[0], &normal[0] + 3);
			}

			meshData->Flags |= mesh_flags::HasIndices | mesh_flags::HasNormals;
            meshData->VertexSize = vertexSize;
		}
		break;
	}

    case AssetEntryType_MaterialLib:
    {
        std::ifstream file(Entry->Filename);
        std::string line;

        auto lib = Entry->MaterialLib.Result;
        material *currentMaterial = NULL;
        while (std::getline(file, line))
        {
			std::istringstream iss(line);
            std::string field;

            if (iss >> field)
            {
                if (field == "newmtl")
                {
                    iss >> field;

					if (lib->Materials.find(field) == lib->Materials.end())
					{
						currentMaterial = PushStruct<material>(Entry->Loader->Arena);
						lib->Materials[field] = currentMaterial;
					}
					else
					{
						currentMaterial = lib->Materials[field];
					}
                }
                else if (field == "Ka" || field == "Kd")
                {
                    float r,g,b;
                    iss >> r >> g >> b;
                    currentMaterial->DiffuseColor = color(r,g,b,1);
                }
				else if (field == "Ks")
				{
					float r, g, b;
					iss >> r >> g >> b;
					currentMaterial->SpecularColor = color(r, g, b, 1);
				}
				else if (field == "Ke")
				{
					float r, g, b;
					iss >> r >> g >> b;
					currentMaterial->EmissiveColor = color(r, g, b, 1);
				}
				else if (field == "Ns")
				{
					float s;
					iss >> s;
					currentMaterial->Shininess = s;
				}
                else if (field == "d")
                {
                    float d;
                    iss >> d;
                    currentMaterial->DiffuseColor.a = d;
                }
				else if (field == "e")
				{
					float e;
					iss >> e;
					currentMaterial->Emission = e;
				}
				else if (field == "cull")
				{
					iss >> field;
					if (field == "on")
					{
						currentMaterial->Flags |= MaterialFlag_CullFace;
					}
				}
            }
        }
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
		for (auto it : result->ClipFiles)
		{
			result->Clips[it.first] = FindSound(*Entry->Loader, it.second);
		}
		break;
	}

	case AssetEntryType_Level:
	{
		auto result = Entry->Level.Result;
		result->Song = FindSong(*Entry->Loader, result->SongName);
		break;
	}

	case AssetEntryType_Mesh:
	{
		auto result = Entry->Mesh.Result;
        auto meshData = Entry->Mesh.Data;
		size_t vertexSize = meshData->VertexSize;
		size_t vertexCount = meshData->Vertices.size() / meshData->VertexSize;
		size_t stride = vertexSize * sizeof(float);
		size_t offset = 0;
		unsigned location = 0;
		std::unordered_map<std::string, std::string> defines;

		std::vector<vertex_attribute> attrs = { vertex_attribute{ location, 3, GL_FLOAT, stride, offset * sizeof(float) } };
		location += 1;
		offset += 3;

		if (meshData->Flags & mesh_flags::HasUvs)
		{
			defines["A_UV"] = std::to_string(location);
			attrs.push_back(vertex_attribute{ location, 2, GL_FLOAT, stride, offset * sizeof(float)});
			location += 1;
			offset += 2;
		}
		if (meshData->Flags & mesh_flags::HasNormals)
		{
			defines["A_NORMAL"] = std::to_string(location);
			attrs.push_back(vertex_attribute{ location, 3, GL_FLOAT, stride, offset * sizeof(float) });
			location += 1;
			offset += 3;
		}
		if (meshData->Flags & mesh_flags::HasColors)
		{
			defines["A_COLOR"] = std::to_string(location);
			attrs.push_back(vertex_attribute{ location, 4, GL_FLOAT, stride, offset * sizeof(float) });
			location += 1;
			offset += 4;
		}

		InitBuffer(result->Buffer, vertexSize, attrs);
		SetIndices(result->Buffer, meshData->Indices);

		for (size_t i = 0; i < vertexSize * vertexCount; i += vertexSize)
		{
			float *data = &meshData->Vertices[i];
			PushVertex(result->Buffer, data);
		}

        auto lib = FindMaterialLib(*Entry->Loader, meshData->MaterialLib);
		for (size_t i = 0; i < meshData->Parts.size(); i++)
		{
			auto &part = meshData->Parts[i];
			part.Program = CompileUberModelProgram(&Entry->Loader->Arena, defines);
			part.Material = lib->Materials[meshData->Materials[i]];
			result->Parts.push_back(part);
		}
		EndMesh(result, GL_STATIC_DRAW);

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
		for (int i = 0; i < Loader.Entries.size(); i++)
		{
			auto &Entry = Loader.Entries[i];
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
	for (int i = 0; i < Loader.Entries.size(); i++)
	{
		auto &Entry = Loader.Entries[i];
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
