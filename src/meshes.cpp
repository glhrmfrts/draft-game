// Copyright

static void
AddLine(vertex_buffer &Buffer, vec3 p1, vec3 p2, color c = Color_white, vec3 n = vec3(0))
{
    PushVertex(Buffer, mesh_vertex{ p1, vec2{ 0, 0 }, c, n });
    PushVertex(Buffer, mesh_vertex{ p2, vec2{ 0, 0 }, c, n });
}

static void
AddQuad(vertex_buffer &Buffer, vec3 p1, vec3 p2, vec3 p3, vec3 p4,
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

static void
AddQuad(vertex_buffer &Buffer, vec3 p1, vec3 p2, vec3 p3, vec3 p4,
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

inline static vec3
GenerateNormal(vec3 p1, vec3 p2, vec3 p3)
{
    vec3 v1 = p2 - p1;
    vec3 v2 = p3 - p1;
    return glm::normalize(glm::cross(v1, v2));
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

static void AddCubeWithRotation(vertex_buffer &Buffer, color c = Color_white,
                                bool NoLight = false, vec3 Center = vec3(0.0f),
                                vec3 Scale = vec3(1.0f), float Angle = 0.0f)
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

static void AddSkyboxFace(mesh *Mesh, vec3 p1, vec3 p2, vec3 p3, vec3 p4, texture *Texture, size_t Index)
{
    AddQuad(Mesh->Buffer, p1, p2, p3, p4, Color_white, vec3(1.0f), true);
    AddPart(Mesh, mesh_part{material{Color_white, 0, 1, Texture}, Index*6, 6, GL_TRIANGLES});
}

#define LEVEL_PLANE_SIZE  512
#define CRYSTAL_COLOR     IntColor(FirstPalette.Colors[1])

mesh *GetFloorMesh(game_main *Game)
{
    auto &m = Game->Meshes;
    if (m.FloorMesh)
    {
        return m.FloorMesh;
    }

    auto *FloorMesh = PushStruct<mesh>(Game->Arena);
    InitMeshBuffer(FloorMesh->Buffer);

    material FloorMaterial = {
        IntColor(SecondPalette.Colors[2]),
        1.0f,
        1,
        FindTexture(Game->AssetLoader, "grid"),
        0,
        vec2{LEVEL_PLANE_SIZE/16,LEVEL_PLANE_SIZE/16}
    };

    float w = 1.0f;
    float l = -w/2;
    float r = w/2;
    AddQuad(FloorMesh->Buffer, vec3(l, l, 0), vec3(r, l, 0), vec3(r, r, 0), vec3(l, r, 0), Color_white, vec3(1, 1, 1));
    AddPart(FloorMesh, {FloorMaterial, 0, FloorMesh->Buffer.VertexCount, GL_TRIANGLES});
    EndMesh(FloorMesh, GL_STATIC_DRAW);

    return m.FloorMesh = FloorMesh;
}

#define ROAD_LANE_COUNT 5
mesh *GetRoadMesh(game_main *Game)
{
    auto &m = Game->Meshes;
    if (m.RoadMesh)
    {
        return m.RoadMesh;
    }

    auto *RoadMesh = PushStruct<mesh>(Game->Arena);
    InitMeshBuffer(RoadMesh->Buffer);

    material RoadMaterial = {Color_black, 0, 0, NULL, 0, vec2{0, 0}};
    material LaneMaterial = {Color_white, 1.0f, 0, NULL, 0, vec2{0, 0}};
    float w = 5.0f;
    float l = -w/2;
    float r = w/2;
    AddQuad(RoadMesh->Buffer, vec3(l, l, 0), vec3(r, l, 0), vec3(r, r, 0), vec3(l, r, 0), Color_white, vec3(1, 1, 1));
    AddPart(RoadMesh, {RoadMaterial, 0, RoadMesh->Buffer.VertexCount, GL_TRIANGLES});
    for (int i = 0; i < ROAD_LANE_COUNT+1; i++)
    {
        AddLine(RoadMesh->Buffer, vec3{l + i, -0.5f, 0.05f}, vec3{l + i, 0.5f, 0.05f});
    }
    AddPart(RoadMesh, {LaneMaterial, 6, RoadMesh->Buffer.VertexCount-6, GL_LINES});
    EndMesh(RoadMesh, GL_STATIC_DRAW);

    return m.RoadMesh = RoadMesh;
}

mesh *GetShipMesh(game_main *Game)
{
    auto &m = Game->Meshes;
    if (m.ShipMesh)
    {
        return m.ShipMesh;
    }

    auto *ShipMesh = PushStruct<mesh>(Game->Arena);
    float h = 0.5f;

    InitMeshBuffer(ShipMesh->Buffer);
    AddTriangle(ShipMesh->Buffer, vec3(-1, 0, 0), vec3(0, 0.1f, h), vec3(0, 1, 0.1f));
    AddTriangle(ShipMesh->Buffer, vec3(1, 0, 0),  vec3(0, 1, 0.1f), vec3(0, 0.1f, h));
    AddTriangle(ShipMesh->Buffer, vec3(-1, 0, 0), vec3(0, 0.1f, 0), vec3(0, 0.1f, h));
    AddTriangle(ShipMesh->Buffer, vec3(1, 0, 0), vec3(0, 0.1f, h), vec3(0, 0.1f, 0));

    material ShipMaterial = {Color_white, 0, 0, NULL};
    material ShipOutlineMaterial = {Color_white, 1, 0, NULL, MaterialFlag_PolygonLines};
    AddPart(ShipMesh, {ShipMaterial, 0, ShipMesh->Buffer.VertexCount, GL_TRIANGLES});
    AddPart(ShipMesh, {ShipOutlineMaterial, 0, ShipMesh->Buffer.VertexCount, GL_TRIANGLES});

    EndMesh(ShipMesh, GL_STATIC_DRAW);

    return m.ShipMesh = ShipMesh;
}

mesh *GetCrystalMesh(game_main *Game)
{
    auto &m = Game->Meshes;
    if (m.CrystalMesh)
    {
        return m.CrystalMesh;
    }

    auto *CrystalMesh = PushStruct<mesh>(Game->Arena);
    InitMeshBuffer(CrystalMesh->Buffer);

    AddTriangle(CrystalMesh->Buffer, vec3{ -1, -1, 0 }, vec3{ 1, -1, 0 }, vec3{ 0, 0, 1 });
    AddTriangle(CrystalMesh->Buffer, vec3{ 1, -1, 0 }, vec3{ 1, 1, 0 }, vec3{ 0, 0, 1 });
    AddTriangle(CrystalMesh->Buffer, vec3{ 1, 1, 0 }, vec3{ -1, 1, 0 }, vec3{ 0, 0, 1 });
    AddTriangle(CrystalMesh->Buffer, vec3{ -1, 1, 0 }, vec3{ -1, -1, 0 }, vec3{ 0, 0, 1 });

    AddTriangle(CrystalMesh->Buffer, vec3{ 0, 0, -1 }, vec3{ 1, -1, 0 }, vec3{ -1, -1, 0 });
    AddTriangle(CrystalMesh->Buffer, vec3{ 0, 0, -1 }, vec3{ 1, 1, 0 }, vec3{ 1, -1, 0 });
    AddTriangle(CrystalMesh->Buffer, vec3{ 0, 0, -1 }, vec3{ -1, 1, 0 }, vec3{ 1, 1, 0 });
    AddTriangle(CrystalMesh->Buffer, vec3{ 0, 0, -1 }, vec3{ -1, -1, 0 }, vec3{ -1, 1, 0 });

    AddPart(CrystalMesh, mesh_part{ material{ CRYSTAL_COLOR, 1.0f, 0, NULL }, 0, CrystalMesh->Buffer.VertexCount, GL_TRIANGLES });
    EndMesh(CrystalMesh, GL_STATIC_DRAW);

    return m.CrystalMesh = CrystalMesh;
}

#define ASTEROID_COLOR IntColor(0x00fa4f, 0.5f)

mesh *GetAsteroidMesh(game_main *game)
{
    auto &m = game->Meshes;
    if (m.AsteroidMesh)
    {
        return m.AsteroidMesh;
    }

    auto astMesh = PushStruct<mesh>(game->Arena);
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

    AddPart(astMesh, mesh_part{material{ASTEROID_COLOR, 0.0f, 0, NULL}, 0, astMesh->Buffer.VertexCount, GL_TRIANGLE_STRIP});
    EndMesh(astMesh, GL_STATIC_DRAW);
    return m.AsteroidMesh = astMesh;
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
    AddPart(Mesh, mesh_part{BlankMaterial, 0, Mesh->Buffer.VertexCount, GL_TRIANGLES});
    EndMesh(Mesh, GL_STATIC_DRAW);
    return Mesh;
}
