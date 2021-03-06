// Copyright

#define ASTEROID_COLOR  IntColor(0x00fa4f, 0.5f)

#define PUSH_MESH_VERTEX(buffer, p1, u, v, c1, n) \
	*(buffer++) = p1.x; *(buffer++) = p1.y; *(buffer++) = p1.z; \
	*(buffer++) = u; *(buffer++) = v; \
	*(buffer++) = c1.r; *(buffer++) = c1.g; *(buffer++) = c1.b; *(buffer++) = c1.a; \
	*(buffer++) = n.x; *(buffer++) = n.y; *(buffer++) = n.z

static void AddLine(vertex_buffer &Buffer, vec3 p1, vec3 p2, color c = Color_white, vec3 n = vec3(0))
{
    PushVertex(Buffer, mesh_vertex{ p1, vec2{ 0, 0 }, c, n });
    PushVertex(Buffer, mesh_vertex{ p2, vec2{ 0, 0 }, c, n });
}

static float *AddLine(float *buffer, vec3 p1, vec3 p2, color c = Color_white, vec3 n = vec3(0))
{
	PUSH_MESH_VERTEX(buffer, p1, 0, 0, c, n);
	PUSH_MESH_VERTEX(buffer, p2, 0, 0, c, n);
	return buffer;
}

static void AddQuad(vertex_buffer &Buffer, vec3 p1, vec3 p2, vec3 p3, vec3 p4,
                    color c1 = Color_white, vec3 n = vec3(1), bool FlipV = false)
{
    color c2 = c1;
    color c3 = c1;
    color c4 = c1;

    texture_rect Uv = {0, 0, 1, 1};
    if (FlipV)
    {
        Uv.v = 1;
        Uv.v2 = 0;
    }

    PushVertex(Buffer, mesh_vertex{ p1, vec2{ Uv.u, Uv.v }, c1, n });
    PushVertex(Buffer, mesh_vertex{ p2, vec2{ Uv.u2, Uv.v }, c2, n });
    PushVertex(Buffer, mesh_vertex{ p4, vec2{ Uv.u, Uv.v2 }, c4, n });

    PushVertex(Buffer, mesh_vertex{ p2, vec2{ Uv.u2, Uv.v }, c2, n });
    PushVertex(Buffer, mesh_vertex{ p3, vec2{ Uv.u2, Uv.v2 }, c3, n });
    PushVertex(Buffer, mesh_vertex{ p4, vec2{ Uv.u, Uv.v2 }, c4, n });
}

static void AddQuad(vertex_buffer &Buffer, vec3 p1, vec3 p2, vec3 p3, vec3 p4,
                    color c1, color c2, color c3, color c4,
                    vec3 n = vec3(1), bool FlipV = false)
{
    texture_rect Uv = { 0, 0, 1, 1 };
    if (FlipV)
    {
        Uv.v = 1;
        Uv.v2 = 0;
    }

    PushVertex(Buffer, mesh_vertex{ p1, vec2{ Uv.u, Uv.v }, c1, n });
    PushVertex(Buffer, mesh_vertex{ p2, vec2{ Uv.u2, Uv.v }, c2, n });
    PushVertex(Buffer, mesh_vertex{ p4, vec2{ Uv.u, Uv.v2 }, c4, n });

    PushVertex(Buffer, mesh_vertex{ p2, vec2{ Uv.u2, Uv.v }, c2, n });
    PushVertex(Buffer, mesh_vertex{ p3, vec2{ Uv.u2, Uv.v2 }, c3, n });
    PushVertex(Buffer, mesh_vertex{ p4, vec2{ Uv.u, Uv.v2 }, c4, n });
}

static float *AddQuad(float *buffer, vec3 p1, vec3 p2, vec3 p3, vec3 p4,
	color c1, color c2, color c3, color c4,
	vec3 n = vec3(1), bool FlipV = false)
{
	texture_rect uv = { 0, 0, 1, 1 };
	if (FlipV)
	{
		uv.v = 1;
		uv.v2 = 0;
	}

	PUSH_MESH_VERTEX(buffer, p1, uv.u, uv.v, c1, n);
	PUSH_MESH_VERTEX(buffer, p2, uv.u2, uv.v, c2, n);
	PUSH_MESH_VERTEX(buffer, p4, uv.u, uv.v2, c4, n);

	PUSH_MESH_VERTEX(buffer, p2, uv.u2, uv.v, c2, n);
	PUSH_MESH_VERTEX(buffer, p3, uv.u2, uv.v2, c3, n);
	PUSH_MESH_VERTEX(buffer, p4, uv.u, uv.v2, c4, n);
	return buffer;
}

void AddTriangle(vertex_buffer &Buffer, vec3 p1, vec3 p2, vec3 p3, vec3 n, color c1 = Color_white)
{
    color c2 = c1;
    color c3 = c1;

    PushVertex(Buffer, mesh_vertex{ p1, vec2{ 0, 0 }, c1, n });
    PushVertex(Buffer, mesh_vertex{ p2, vec2{ 0, 0 }, c2, n });
    PushVertex(Buffer, mesh_vertex{ p3, vec2{ 0, 0 }, c3, n });
}

void AddTriangle(vertex_buffer &Buffer, vec3 p1, vec3 p2, vec3 p3)
{
    AddTriangle(Buffer, p1, p2, p3, GenerateNormal(p1, p2, p3));
}

void AddCubeWithRotation(vertex_buffer &Buffer, color c, bool NoLight, vec3 Center,
                         vec3 Scale, float Angle)
{
    float z = -0.5f;
    float h = 0.5f;
    float x = -0.5f;
    float w = 0.5f;
    float y = -0.5f;
    float d = 0.5f;

    transform Transform;
    Transform.Position = Center;
    Transform.Scale = Scale;
    Transform.Rotation.z = glm::degrees(Angle);
    mat4 TransformMatrix = GetTransformMatrix(Transform);
    auto r = [TransformMatrix](float x, float y, float z){
        vec4 v4 = vec4{x, y, z, 1.0f};
        v4 = TransformMatrix * v4;
        return vec3{v4.x, v4.y, v4.z};
    };

    if (NoLight)
    {
        AddQuad(Buffer, r(x, y, z), r(w, y, z), r(w, y, h), r(x, y, h), c, r(1, 1, 1));
        AddQuad(Buffer, r(w, y, z), r(w, d, z), r(w, d, h), r(w, y, h), c, r(1, 1, 1));
        AddQuad(Buffer, r(w, d, z), r(x, d, z), r(x, d, h), r(w, d, h), c, r(1, 1, 1));
        AddQuad(Buffer, r(x, d, z), r(x, y, z), r(x, y, h), r(x, d, h), c, r(1, 1, 1));
        AddQuad(Buffer, r(x, y, h), r(w, y, h), r(w, d, h), r(x, d, h), c, r(1, 1, 1));
        AddQuad(Buffer, r(x, d, z), r(w, d, z), r(w, y, z), r(x, y, z), c, r(1, 1, 1));
    }
    else
    {
        AddQuad(Buffer, r(x, y, z), r(w, y, z), r(w, y, h), r(x, y, h), c, r(0, -1, 0));
        AddQuad(Buffer, r(w, y, z), r(w, d, z), r(w, d, h), r(w, y, h), c, r(1, 0, 0));
        AddQuad(Buffer, r(w, d, z), r(x, d, z), r(x, d, h), r(w, d, h), c, r(0, 1, 0));
        AddQuad(Buffer, r(x, d, z), r(x, y, z), r(x, y, h), r(x, d, h), c, r(-1, 0, 0));
        AddQuad(Buffer, r(x, y, h), r(w, y, h), r(w, d, h), r(x, d, h), c, r(0, 0, 1));
        AddQuad(Buffer, r(x, d, z), r(w, d, z), r(w, y, z), r(x, y, z), c, r(0, 0, -1));
    }
}

inline static void AddPart(mesh *Mesh, const mesh_part &MeshPart)
{
    Mesh->Parts.push_back(MeshPart);
}

#if 0
static void AddSkyboxFace(mesh *Mesh, vec3 p1, vec3 p2, vec3 p3, vec3 p4, texture *Texture, size_t Index)
{
    AddQuad(Mesh->Buffer, p1, p2, p3, p4, Color_white, vec3(1.0f), true);
    AddPart(Mesh, mesh_part{material{Color_white, 0, 1, Texture}, Index*6, 6, GL_TRIANGLES});
}
#endif

static material *CreateMaterial(allocator *alloc, color Color, float Emission, float TexWeight, texture *Texture, uint32 Flags = 0)
{
	auto result = PushStruct<material>(alloc);
	result->DiffuseColor = Color;
	result->Emission = Emission;
	result->TexWeight = TexWeight;
	result->Texture = Texture;
	result->Flags = Flags;
	return result;
}

struct mesh_part_scope
{
    mesh *Mesh;
    size_t Offset;

    mesh_part_scope(mesh *m)
    {
        Mesh = m;
        Offset = m->Buffer.VertexCount;
    }

    void Commit(material *m, GLuint primType, float lineWidth = DEFAULT_LINE_WIDTH)
    {
        size_t count = Mesh->Buffer.VertexCount - Offset;
        AddPart(this->Mesh, mesh_part{m, Offset, count, primType, lineWidth});
    }
};

#define ROAD_SEGMENT_COUNT 12
#define ROAD_SEGMENT_SIZE  24
#define ROAD_LANE_WIDTH    2
#define ROAD_LANE_COUNT    5
#define ROAD_BORDER_COLOR  IntColor(FirstPalette.Colors[2])
#define ROAD_FLOOR_COLOR   Color_black
#define ROAD_LANE_COLOR    color{1,1,1,1.0f}

mesh *BuildStraightRoadMesh(memory_arena &arena, float l, float r)
{
	auto roadMesh = PushStruct<mesh>(arena);
	InitMeshBuffer(roadMesh->Buffer);

	auto roadMaterial = CreateMaterial(&arena, ROAD_FLOOR_COLOR, 0, 0.0f, NULL);
	auto borderMaterial = CreateMaterial(&arena, ROAD_BORDER_COLOR, 1.0f, 0, NULL);
	auto laneMaterial = CreateMaterial(&arena, ROAD_LANE_COLOR, 0.0f, 0, NULL);
	float width = r - l;

	mesh_part_scope roadScope(roadMesh);
	for (int y = 0; y < ROAD_SEGMENT_SIZE; y++)
	{
		AddQuad(roadMesh->Buffer, vec3{ l, y, 0 }, vec3{ r, y, 0 }, vec3{ r, y + 1, 0 }, vec3{ l, y + 1, 0 }, Color_white, vec3(1, 1, 1));
	}
	roadScope.Commit(roadMaterial, GL_TRIANGLES);

	mesh_part_scope borderScope(roadMesh);
	for (int y = 0; y < ROAD_SEGMENT_SIZE; y++)
	{
		for (int i = 0; i < (int)width + 1; i++)
		{
			if (i == 0 || i == (int)width)
			{
				AddLine(roadMesh->Buffer, vec3{ l + i, y, 0.05f }, vec3{ l + i, y + 1, 0.05f });
			}
		}
	}
	borderScope.Commit(borderMaterial, GL_LINES, 4);

	mesh_part_scope laneScope(roadMesh);
	for (int y = 0; y < ROAD_SEGMENT_SIZE; y++)
	{
		for (int i = 0; i < (int)width + 1; i++)
		{
			if (y % 2 == 0)
			{
				color c = Color_white;
				AddLine(roadMesh->Buffer, vec3{ l + i, y, 0.05f }, vec3{ l + i, y + 1, 0.05f }, c);
			}
		}
	}
	laneScope.Commit(laneMaterial, GL_LINES, 2);
	EndMesh(roadMesh, GL_STATIC_DRAW);

	return roadMesh;
}

mesh *BuildVariantRoadMesh(memory_arena &arena, float backLeft, float backRight, float frontLeft, float frontRight)
{
	auto roadMesh = PushStruct<mesh>(arena);
	InitMeshBuffer(roadMesh->Buffer);

	//auto roadTexture = FindTexture(*w.AssetLoader, "grid");
	auto roadMaterial = CreateMaterial(&arena, ROAD_FLOOR_COLOR, 0, 0.0f, NULL);
	auto borderMaterial = CreateMaterial(&arena, ROAD_BORDER_COLOR, 1.0f, 0, NULL);
	auto laneMaterial = CreateMaterial(&arena, ROAD_LANE_COLOR, 0.0f, 0, NULL);
	float backWidth = backRight - backLeft;
	float frontWidth = frontRight - frontLeft;
	float bl = backLeft;
	float br = backRight;
	float fl = frontLeft;
	float fr = frontRight;
	float dleft = (frontLeft - backLeft) / (float)ROAD_SEGMENT_SIZE;
	float dright = (frontRight - backRight) / (float)ROAD_SEGMENT_SIZE;
	float left = 0;
	float right = 0;
	float maxLeft = 0;
	float maxRight = 0;
	if (std::abs(bl) > std::abs(fl))
	{
		maxLeft = bl;
	}
	else
	{
		maxLeft = fl;
	}

	if (std::abs(br) > std::abs(fr))
	{
		maxRight = br;
	}
	else
	{
		maxRight = fr;
	}

	mesh_part_scope roadScope(roadMesh);
	left = bl;
	right = br;
	for (int y = 0; y < ROAD_SEGMENT_SIZE; y++)
	{
		AddQuad(roadMesh->Buffer, vec3{ left, y, 0 }, vec3{ right, y, 0 }, vec3{ right + dright, y + 1, 0 }, vec3{ left + dleft, y + 1, 0 }, Color_white, vec3(1, 1, 1));
		left += dleft;
		right += dright;
	}
	roadScope.Commit(roadMaterial, GL_TRIANGLES);

	mesh_part_scope borderScope(roadMesh);
	left = bl;
	right = br;
	for (int y = 0; y < ROAD_SEGMENT_SIZE; y++)
	{
		AddLine(roadMesh->Buffer, vec3{ left, y, 0.05f }, vec3{ left + dleft, y + 1, 0.05f });
		AddLine(roadMesh->Buffer, vec3{ right, y, 0.05f }, vec3{ right + dright, y + 1, 0.05f });
		left += dleft;
		right += dright;
	}
	borderScope.Commit(borderMaterial, GL_LINES, 4);

	mesh_part_scope laneScope(roadMesh);
	left = bl;
	right = br;
	for (int y = 0; y < ROAD_SEGMENT_SIZE; y++)
	{
		if (y % 2 == 0)
		{
			for (float x = maxLeft; x < maxRight; x += 1)
			{
				if (x > left && x < right)
				{
					color c = Color_white;
					AddLine(roadMesh->Buffer, vec3{ x, y, 0.05f }, vec3{ x, y + 1, 0.05f }, c);
				}
			}
		}
		left += dleft;
		right += dright;
	}
	laneScope.Commit(laneMaterial, GL_LINES, 2);
	EndMesh(roadMesh, GL_STATIC_DRAW);

	return roadMesh;
}

mesh *GetStraightRoadMesh(road_mesh_manager &manager, float left, float right)
{
	char *keyChars = Format(&manager.KeyFormat, "straight", left, right);
	manager.Key.assign(keyChars);

	if (manager.Meshes.find(manager.Key) == manager.Meshes.end())
	{
		auto result = BuildStraightRoadMesh(manager.Arena, left, right);
		manager.Meshes[manager.Key] = result;
		return result;
	}
	return manager.Meshes[manager.Key];
}

mesh *GetNarrowRightRoadMesh(road_mesh_manager &manager)
{
	char *keyChars = Format(&manager.KeyFormat, "narrowR", -2.5f, 0.5f);
	manager.Key.assign(keyChars);

	if (manager.Meshes.find(manager.Key) == manager.Meshes.end())
	{
		auto result = BuildVariantRoadMesh(manager.Arena, -2.5f, 2.5f, 0.5f, 2.5f);
		manager.Meshes[manager.Key] = result;
		return result;
	}
	return manager.Meshes[manager.Key];
}

mesh *GetNarrowLeftRoadMesh(road_mesh_manager &manager)
{
	char *keyChars = Format(&manager.KeyFormat, "narrowL", 2.5f, -0.5f);
	manager.Key.assign(keyChars);

	if (manager.Meshes.find(manager.Key) == manager.Meshes.end())
	{
		auto result = BuildVariantRoadMesh(manager.Arena, -2.5f, 2.5f, -2.5f, -0.5f);
		manager.Meshes[manager.Key] = result;
		return result;
	}
	return manager.Meshes[manager.Key];
}

mesh *GetNarrowCenterRoadMesh(road_mesh_manager &manager)
{
	char *keyChars = Format(&manager.KeyFormat, "narrowC", 5.0f, 1.0f);
	manager.Key.assign(keyChars);

	if (manager.Meshes.find(manager.Key) == manager.Meshes.end())
	{
		auto result = BuildVariantRoadMesh(manager.Arena, -2.5f, 2.5f, -0.5f, 0.5f);
		manager.Meshes[manager.Key] = result;
		return result;
	}
	return manager.Meshes[manager.Key];
}

mesh *GetWidensLeftRoadMesh(road_mesh_manager &manager)
{
	char *keyChars = Format(&manager.KeyFormat, "widensL", 0.5f, -2.5f);
	manager.Key.assign(keyChars);

	if (manager.Meshes.find(manager.Key) == manager.Meshes.end())
	{
		auto result = BuildVariantRoadMesh(manager.Arena, 0.5f, 2.5f, -2.5f, 2.5f);
		manager.Meshes[manager.Key] = result;
		return result;
	}
	return manager.Meshes[manager.Key];
}

mesh *GetWidensRightRoadMesh(road_mesh_manager &manager)
{
	char *keyChars = Format(&manager.KeyFormat, "widensR", -0.5f, 2.5f);
	manager.Key.assign(keyChars);

	if (manager.Meshes.find(manager.Key) == manager.Meshes.end())
	{
		auto result = BuildVariantRoadMesh(manager.Arena, -2.5f, -0.5f, -2.5f, 2.5f);
		manager.Meshes[manager.Key] = result;
		return result;
	}
	return manager.Meshes[manager.Key];
}

mesh *GetWidensCenterRoadMesh(road_mesh_manager &manager)
{
	char *keyChars = Format(&manager.KeyFormat, "widensC", 1.0f, 5.0f);
	manager.Key.assign(keyChars);

	if (manager.Meshes.find(manager.Key) == manager.Meshes.end())
	{
		auto result = BuildVariantRoadMesh(manager.Arena, -0.5f, 0.5f, -2.5f, 2.5f);
		manager.Meshes[manager.Key] = result;
		return result;
	}
	return manager.Meshes[manager.Key];
}

mesh *GetShipMesh(entity_world &w)
{
    if (w.ShipMesh)
    {
        return w.ShipMesh;
    }

    auto *ShipMesh = PushStruct<mesh>(w.PersistentArena);
    float h = 0.5f;

    InitMeshBuffer(ShipMesh->Buffer);
    AddTriangle(ShipMesh->Buffer, vec3(-1, 0, 0), vec3(0, 0.1f, h), vec3(0, 1, 0.1f));
    AddTriangle(ShipMesh->Buffer, vec3(1, 0, 0),  vec3(0, 1, 0.1f), vec3(0, 0.1f, h));
    AddTriangle(ShipMesh->Buffer, vec3(-1, 0, 0), vec3(0, 0.1f, 0), vec3(0, 0.1f, h));
    AddTriangle(ShipMesh->Buffer, vec3(1, 0, 0), vec3(0, 0.1f, h), vec3(0, 0.1f, 0));

    auto shipMaterial = CreateMaterial(&w.PersistentArena, Color_white, 0, 0, NULL);
	auto shipOutlineMaterial = CreateMaterial(&w.PersistentArena, Color_white, 1, 0, NULL, MaterialFlag_PolygonLines);
    AddPart(ShipMesh, {shipMaterial, 0, ShipMesh->Buffer.VertexCount, GL_TRIANGLES});
    AddPart(ShipMesh, {shipOutlineMaterial, 0, ShipMesh->Buffer.VertexCount, GL_TRIANGLES});

    EndMesh(ShipMesh, GL_STATIC_DRAW);

    return w.ShipMesh = ShipMesh;
}

mesh *GetCrystalMesh(entity_world &w)
{
    if (w.CrystalMesh)
    {
        return w.CrystalMesh;
    }

    auto *CrystalMesh = PushStruct<mesh>(w.PersistentArena);
    InitMeshBuffer(CrystalMesh->Buffer);

    AddTriangle(CrystalMesh->Buffer, vec3{ -1, -1, 0 }, vec3{ 1, -1, 0 }, vec3{ 0, 0, 1 });
    AddTriangle(CrystalMesh->Buffer, vec3{ 1, -1, 0 }, vec3{ 1, 1, 0 }, vec3{ 0, 0, 1 });
    AddTriangle(CrystalMesh->Buffer, vec3{ 1, 1, 0 }, vec3{ -1, 1, 0 }, vec3{ 0, 0, 1 });
    AddTriangle(CrystalMesh->Buffer, vec3{ -1, 1, 0 }, vec3{ -1, -1, 0 }, vec3{ 0, 0, 1 });

    AddTriangle(CrystalMesh->Buffer, vec3{ 0, 0, -1 }, vec3{ 1, -1, 0 }, vec3{ -1, -1, 0 });
    AddTriangle(CrystalMesh->Buffer, vec3{ 0, 0, -1 }, vec3{ 1, 1, 0 }, vec3{ 1, -1, 0 });
    AddTriangle(CrystalMesh->Buffer, vec3{ 0, 0, -1 }, vec3{ -1, 1, 0 }, vec3{ 1, 1, 0 });
    AddTriangle(CrystalMesh->Buffer, vec3{ 0, 0, -1 }, vec3{ -1, -1, 0 }, vec3{ -1, 1, 0 });

    AddPart(CrystalMesh, mesh_part{ CreateMaterial(&w.PersistentArena, CRYSTAL_COLOR, 1.0f, 0, NULL, MaterialFlag_TransformUniform ), 0, CrystalMesh->Buffer.VertexCount, GL_TRIANGLES });
    EndMesh(CrystalMesh, GL_STATIC_DRAW);

    return w.CrystalMesh = CrystalMesh;
}

#ifndef M_PI_2
#define M_PI_2 (M_PI/2)
#endif

mesh *GetAsteroidMesh(entity_world &w)
{
    if (w.AsteroidMesh)
    {
        return w.AsteroidMesh;
    }

    auto astMesh = PushStruct<mesh>(w.PersistentArena);
    InitMeshBuffer(astMesh->Buffer);

    const int p = 8;
    for (int i = 0; i < p/2; i++)
    {
        float theta1 = i * (M_PI*2) / p - M_PI_2;
        float theta2 = (i+1) * (M_PI*2) / p - M_PI_2;
        for (int j = 0; j <= p; j++)
        {
            float theta3 = j * (M_PI*2) / p;
            float x = std::cos(theta2) * std::cos(theta3);
            float y = std::sin(theta2);
            float z = std::cos(theta2) * std::sin(theta3);
            PushVertex(astMesh->Buffer, mesh_vertex{vec3{x, y, z}, vec2{0,0}, Color_white, vec3{x, y, z}});

            x = std::cos(theta1) * std::cos(theta3);
            y = std::sin(theta1);
            z = std::cos(theta1) * std::sin(theta3);
            PushVertex(astMesh->Buffer, mesh_vertex{vec3{x, y, z}, vec2{0,0}, Color_white, vec3{x, y, z}});
        }
    }

    AddPart(astMesh, mesh_part{CreateMaterial(&w.PersistentArena, ASTEROID_COLOR, 0.0f, 0, NULL, MaterialFlag_TransformUniform), 0, astMesh->Buffer.VertexCount, GL_TRIANGLE_STRIP});
    EndMesh(astMesh, GL_STATIC_DRAW);
    return w.AsteroidMesh = astMesh;
}

#define CHECKPOINT_COLOR         IntColor(FirstPalette.Colors[3], 0.5f)
#define CHECKPOINT_OUTLINE_COLOR IntColor(ShipPalette.Colors[SHIP_BLUE])

mesh *GetCheckpointMesh(entity_world &w)
{
    if (w.CheckpointMesh)
    {
        return w.CheckpointMesh;
    }

    auto cpMesh = PushStruct<mesh>(w.PersistentArena);
    InitMeshBuffer(cpMesh->Buffer);
    PushVertex(cpMesh->Buffer, mesh_vertex{vec3{-1.0f, 0.0f, 0.0f}, vec2{0,0}, Color_white, vec3{1,1,1}});
    PushVertex(cpMesh->Buffer, mesh_vertex{vec3{1.0f, 0.0f, 0.0f}, vec2{0,0}, Color_white, vec3{1,1,1}});
    PushVertex(cpMesh->Buffer, mesh_vertex{vec3{0.0f, 0.0f, 1.0f}, vec2{0,0}, Color_white, vec3{1,1,1}});
    AddPart(cpMesh, mesh_part{CreateMaterial(&w.PersistentArena, CHECKPOINT_COLOR, 0.0f, 0, NULL), 0, cpMesh->Buffer.VertexCount, GL_TRIANGLES});
    AddPart(cpMesh, mesh_part{CreateMaterial(&w.PersistentArena, CHECKPOINT_OUTLINE_COLOR, 1.0f, 0, NULL), 0, cpMesh->Buffer.VertexCount, GL_LINE_LOOP});
    EndMesh(cpMesh, GL_STATIC_DRAW);
    return w.CheckpointMesh = cpMesh;
}

mesh *GetFinishMesh(entity_world &w)
{
	if (w.FinishMesh)
	{
		return w.FinishMesh;
	}

	auto finishMesh = PushStruct<mesh>(w.PersistentArena);
	InitMeshBuffer(finishMesh->Buffer);
	PushVertex(finishMesh->Buffer, mesh_vertex{ vec3{0.0f, 0.0f, 0.0f}, vec2{0.5f, 0.5f}, Color_white, vec3{1,1,1} });

	const float radius = 1.0f;
	const int segments = 48;
	const float theta = M_PI * 2 / float(segments);
	float angle = 0;
	for (int i = 0; i < segments + 1; i++)
	{
		float x = cos(angle) * radius;
		float z = sin(angle) * radius;

		PushVertex(finishMesh->Buffer, mesh_vertex{ vec3{x, 0, z}, vec2{x + 0.5f, z + 0.5f}, Color_white, vec3{1,1,1} });

		angle += theta;
	}

	AddPart(finishMesh, mesh_part{ CreateMaterial(&w.PersistentArena, Color_white, 1.0f, 0, NULL), 0, finishMesh->Buffer.VertexCount, GL_TRIANGLE_FAN });
	EndMesh(finishMesh, GL_STATIC_DRAW);

	return w.FinishMesh = finishMesh;
}

mesh *GetBackgroundMesh(entity_world &w)
{
    if (w.BackgroundMesh)
    {
        return w.BackgroundMesh;
    }

    auto bgMesh = PushStruct<mesh>(w.PersistentArena);
    auto mat = CreateMaterial(&w.PersistentArena, Color_white, 1.0f, 1.0f, FindTexture(*w.AssetLoader, "background", "main_assets"));
    mat->FogWeight = 0.0f;

    InitMeshBuffer(bgMesh->Buffer);
    AddQuad(bgMesh->Buffer, vec3{-1.0f, 0.0f, 0.0f}, vec3{1.0f, 0.0f, 0.0f}, vec3{1.0f, 0.0f, 1.0f}, vec3{-1.0f, 0.0f, 1.0f});
    AddPart(bgMesh, mesh_part{mat, 0, bgMesh->Buffer.VertexCount, GL_TRIANGLES});
    EndMesh(bgMesh, GL_STATIC_DRAW);
    return w.BackgroundMesh = bgMesh;
}

#define WALL_HEIGHT 2.0f
#define WALL_WIDTH  0.5f
mesh *GenerateWallMesh(memory_arena &Arena, const std::vector<vec2> &Points)
{
    auto *Mesh = PushStruct<mesh>(Arena);
    InitMeshBuffer(Mesh->Buffer);

    size_t VertCount = Points.size();
    assert(VertCount > 0);
    for (size_t i = 0; i < VertCount - 1; i++)
    {
        vec2 First = Points[i];
        vec2 Second = Points[i + 1];
        vec2 Dif = Second - First;
        vec2 Center = First + Dif*0.5f;
        float Angle = std::atan2(First.y - Second.y, First.x - Second.x);
        AddCubeWithRotation(Mesh->Buffer,
                            Color_white,
                            false,
                            vec3{Center.x, Center.y, 0.0f},
                            vec3{glm::length(Dif) + 0.25f, WALL_WIDTH, WALL_HEIGHT},
                            Angle);
    }
    AddPart(Mesh, mesh_part{&BlankMaterial, 0, Mesh->Buffer.VertexCount, GL_TRIANGLES});
    EndMesh(Mesh, GL_STATIC_DRAW);
    return Mesh;
}

mesh *GetSphereMesh(entity_world &w)
{
    if (w.SphereMesh)
    {
        return w.SphereMesh;
    }

    auto result = PushStruct<mesh>(w.PersistentArena);
    InitMeshBuffer(result->Buffer);

    const float rings = 24;
    const float pointsPerRing = 24;
    const float deltaTheta = M_PI / rings;
    const float deltaPhi = 2*M_PI / (pointsPerRing + 2);
    float theta = 0;
    float phi = 0;
    for (int r = 0; r < rings; r++)
    {
        theta += deltaTheta;
        for (int p = 0; p < pointsPerRing; p++)
        {
            phi += deltaPhi;
            float x = std::sin(theta) * std::cos(phi);
            float y = std::sin(theta) * std::sin(phi);
            float z = std::cos(theta);
            PushVertex(result->Buffer, mesh_vertex{vec3{x, y, z}, vec2{}, vec4{1, 1, 1, 1}, vec3{x, y, z}});
        }
    }

    AddPart(result, mesh_part{CreateMaterial(&w.PersistentArena, Color_white, 1.0f, 0.0f, NULL), 0, result->Buffer.VertexCount, GL_TRIANGLE_FAN});
    EndMesh(result, GL_STATIC_DRAW);
    return w.SphereMesh = result;
}
