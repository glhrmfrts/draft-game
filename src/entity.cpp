// Copyright

static void
AddLine(vertex_buffer &Buffer, vec3 p1, vec3 p2, color c = Color_white, vec3 n = vec3(0))
{
    PushVertex(Buffer, {p1.x, p1.y, p1.z, 0, 0,  c.r, c.g, c.b, c.a,  n.x, n.y, n.z});
    PushVertex(Buffer, {p2.x, p2.y, p2.z, 0, 0,  c.r, c.g, c.b, c.a,  n.x, n.y, n.z});
}

static void
AddQuad(vertex_buffer &Buffer, vec3 p1, vec3 p2, vec3 p3, vec3 p4,
        color c1 = Color_white, vec3 n = vec3(0), bool FlipV = false)
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
    PushVertex(Buffer, {p1.x, p1.y, p1.z, Uv.u,  Uv.v,   c1.r, c1.g, c1.b, c1.a,  n.x, n.y, n.z});
    PushVertex(Buffer, {p2.x, p2.y, p2.z, Uv.u2, Uv.v,   c2.r, c2.g, c2.b, c2.a,  n.x, n.y, n.z});
    PushVertex(Buffer, {p4.x, p4.y, p4.z, Uv.u,  Uv.v2,  c4.r, c4.g, c4.b, c4.a,  n.x, n.y, n.z});

    PushVertex(Buffer, {p2.x, p2.y, p2.z, Uv.u2, Uv.v,   c2.r, c2.g, c2.b, c2.a,  n.x, n.y, n.z});
    PushVertex(Buffer, {p3.x, p3.y, p3.z, Uv.u2, Uv.v2,  c3.r, c3.g, c3.b, c3.a,  n.x, n.y, n.z});
    PushVertex(Buffer, {p4.x, p4.y, p4.z, Uv.u,  Uv.v2,  c4.r, c4.g, c4.b, c4.a,  n.x, n.y, n.z});
}

inline static vec3
GenerateNormal(vec3 p1, vec3 p2, vec3 p3)
{
    vec3 v1 = p2 - p1;
    vec3 v2 = p3 - p1;
    return glm::normalize(glm::cross(v1, v2));
}

static void
AddTriangle(vertex_buffer &Buffer, vec3 p1, vec3 p2, vec3 p3, color c1 = Color_white)
{
    color c2 = c1;
    color c3 = c1;

    vec3 n = GenerateNormal(p1, p2, p3);
    PushVertex(Buffer, {p1.x, p1.y, p1.z, 0, 0,   c1.r, c1.g, c1.b, c1.a,  n.x, n.y, n.z});
    PushVertex(Buffer, {p2.x, p2.y, p2.z, 0, 0,   c2.r, c2.g, c2.b, c2.a,  n.x, n.y, n.z});
    PushVertex(Buffer, {p3.x, p3.y, p3.z, 0, 0,   c3.r, c3.g, c3.b, c3.a,  n.x, n.y, n.z});
}

static void
AddCube(vertex_buffer &Buffer, float height)
{
    float y = -0.5;
    float h = y + height;
    float x = -0.5f;
    float w = x+1;
    float z = 0.5f;
    float d = z-1;
    AddQuad(Buffer, vec3(x, y, z), vec3(w, y, z), vec3(w, h, z), vec3(x, h, z), Color_white, vec3(0, 0, 1));
    AddQuad(Buffer, vec3(w, y, z), vec3(w, y, d), vec3(w, h, d), vec3(w, h, z), Color_white, vec3(1, 0, 0));
    AddQuad(Buffer, vec3(w, y, d), vec3(x, y, d), vec3(x, h, d), vec3(w, h, d), Color_white, vec3(0, 0, -1));
    AddQuad(Buffer, vec3(x, y, d), vec3(x, y, z), vec3(x, h, z), vec3(x, h, d), Color_white, vec3(-1, 0, 0));
    AddQuad(Buffer, vec3(x, h, z), vec3(w, h, z), vec3(w, h, d), vec3(x, h, d), Color_white, vec3(0, 1, 0));
}

inline static void
AddFlags(entity *Entity, uint32 Flags)
{
    Entity->Flags |= Flags;
}

static material *
CreateMaterial(memory_arena &Arena, color Color, float Emission, float TexWeight, texture *Texture, uint32 Flags = 0)
{
    material *Result = PushStruct<material>(Arena);
    Result->DiffuseColor = Color;
    Result->Emission = Emission;
    Result->TexWeight = TexWeight;
    Result->Texture = Texture;
    Result->Flags = Flags;
    return Result;
}

static model *
CreateModel(memory_arena &Arena, mesh *Mesh)
{
    model *Result = PushStruct<model>(Arena);
    Result->Mesh = Mesh;
    return Result;
}

inline static void
AddPart(mesh &Mesh, const mesh_part &MeshPart)
{
    Mesh.Parts.push_back(MeshPart);
}

static void
AddSkyboxFace(mesh &Mesh, vec3 p1, vec3 p2, vec3 p3, vec3 p4, texture *Texture, size_t Index)
{
    AddQuad(Mesh.Buffer, p1, p2, p3, p4, Color_white, vec3(1.0f), true);
    AddPart(Mesh, mesh_part{material{Color_white, 0, 1, Texture}, Index*6, 6, GL_TRIANGLES});
}

static trail *
CreateTrail(memory_arena &Arena, color Color)
{
    trail *Result = PushStruct<trail>(Arena);
    InitMeshBuffer(Result->Mesh.Buffer);
    Result->Model.Mesh = &Result->Mesh;

    const int LineCount = TrailCount*2;
    const int PlaneCount = TrailCount*6;
    const float emission = 0.3f;

    // Plane parts
    AddPart(Result->Mesh, {{Color, 0, 0, NULL}, 0, 6, GL_TRIANGLES});
    AddPart(Result->Mesh, {{Color, 0, 0, NULL}, 6, TrailCount*6 - 6, GL_TRIANGLES});

    // Left line parts
    AddPart(Result->Mesh, {{Color, emission, 0, NULL}, PlaneCount, 2, GL_LINES});
    AddPart(Result->Mesh, {{Color, emission, 0, NULL}, PlaneCount + 2, LineCount - 2, GL_LINES});

    // Right line parts
    AddPart(Result->Mesh, {{Color, emission, 0, NULL}, PlaneCount + LineCount, 2, GL_LINES});
    AddPart(Result->Mesh, {{Color, emission, 0, NULL}, PlaneCount + LineCount + 2, LineCount - 2, GL_LINES});

    for (int i = 0; i < TrailCount; i++)
    {
        auto Entity = Result->Entities + i;
        Entity->Type = EntityType_TrailPiece;
        Entity->Position = vec3(0.0f);
        Entity->Bounds = PushStruct<bounding_box>(Arena);
        AddFlags(Entity, Entity_Kinematic);
    }
    return Result;
}

static entity *
CreateShipEntity(game_state &Game, color Color, color OutlineColor, bool IsPlayer = false)
{
    auto Entity = PushStruct<entity>(Game.Arena);
    Entity->Type = EntityType_Ship;
    Entity->Model = CreateModel(Game.Arena, &Game.ShipMesh);
    Entity->Model->Materials.push_back(CreateMaterial(Game.Arena, vec4(Color.r, Color.g, Color.b, 1), 0, 0, NULL));
    Entity->Model->Materials.push_back(CreateMaterial(Game.Arena, OutlineColor, 0.1f, 0, NULL, Material_PolygonLines));
    Entity->Size.y = 3;
    Entity->Size *= 0.75f;
    Entity->Bounds = PushStruct<bounding_box>(Game.Arena);
    Entity->Trail = CreateTrail(Game.Arena, OutlineColor);
    return Entity;
}

static float
Interp(float c, float t, float a, float dt)
{
    if (c == t)
    {
        return t;
    }
    float dir = std::copysign(1, t - c);
    c += a * dir * dt;
    return (dir == std::copysign(1, t - c)) ? c : t;
}

#define ShipMaxVel  50.0f
#define PlayerMaxVel 55.0f
#define ShipAcceleration 20.0f
#define ShipBreakAcceleration 30.0f
#define ShipSteerSpeed 10.0f
#define ShipSteerAcceleration 50.0f
#define ShipFriction 2.0f
static void
MoveShipEntity(entity *Entity, float MoveH, float MoveV, float DeltaTime, bool IsPlayer = false)
{
    float MaxVel = ShipMaxVel;
    if (IsPlayer)
    {
        MaxVel = PlayerMaxVel;
    }

    if (MoveV > 0.0f && Entity->Velocity.y < MaxVel)
    {
        Entity->Velocity.y += MoveV * ShipAcceleration * DeltaTime;
    }
    else if (MoveV < 0.0f && Entity->Velocity.y > 0)
    {
        Entity->Velocity.y = std::max(0.0f, Entity->Velocity.y + (MoveV * ShipBreakAcceleration * DeltaTime));
    }

    if (MoveV <= 0.0f)
    {
        Entity->Velocity.y -= ShipFriction * DeltaTime;
    }
    Entity->Velocity.y = std::max(0.0f, std::min(MaxVel, Entity->Velocity.y));

    float SteerTarget = MoveH * ShipSteerSpeed;
    Entity->Velocity.x = Interp(Entity->Velocity.x,
                                SteerTarget,
                                ShipSteerAcceleration,
                                DeltaTime);

    Entity->Rotation.y = 20.0f * (Entity->Velocity.x / ShipSteerSpeed);
    Entity->Rotation.x = Interp(Entity->Rotation.x,
                                5.0f * MoveV,
                                20.0f,
                                DeltaTime);
}

static void
PushPosition(trail *Trail, vec3 Pos)
{
    if (Trail->PositionStackIndex >= TrailCount)
    {
        vec3 PosSave = Trail->Entities[TrailCount - 1].Position;
        for (int i = TrailCount - 1; i > 0; i--)
        {
            vec3 NewPos = PosSave;
            PosSave = Trail->Entities[i - 1].Position;
            Trail->Entities[i - 1].Position = NewPos;
        }
        Trail->PositionStackIndex -= 1;
    }
    Trail->Entities[Trail->PositionStackIndex++].Position = Pos;
}
