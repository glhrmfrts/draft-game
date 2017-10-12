// Copyright

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

static trail *
CreateTrail(memory_arena &Arena, entity *Owner, color Color)
{
    trail *Result = PushStruct<trail>(Arena);

    InitMeshBuffer(Result->Mesh.Buffer);
    Result->Model.Mesh = &Result->Mesh;

    const size_t LineCount = TrailCount*2;
    const size_t PlaneCount = TrailCount*6;
    const float emission = 4.0f;

    // Plane parts
    AddPart(&Result->Mesh, {{Color, 0, 0, NULL, Material_ForceTransparent}, 0, 6, GL_TRIANGLES});
    AddPart(&Result->Mesh, {{Color, 0, 0, NULL, Material_ForceTransparent}, 6, TrailCount*6 - 6, GL_TRIANGLES});

    // Left line parts
    AddPart(&Result->Mesh, {{Color, emission, 0, NULL}, PlaneCount, 2, GL_LINES});
    AddPart(&Result->Mesh, {{Color, emission, 0, NULL}, PlaneCount + 2, LineCount - 2, GL_LINES});

    // Right line parts
    AddPart(&Result->Mesh, {{Color, emission, 0, NULL}, PlaneCount + LineCount, 2, GL_LINES});
    AddPart(&Result->Mesh, {{Color, emission, 0, NULL}, PlaneCount + LineCount + 2, LineCount - 2, GL_LINES});

    ReserveVertices(Result->Mesh.Buffer, PlaneCount + LineCount*2, GL_DYNAMIC_DRAW);

    for (int i = 0; i < TrailCount; i++)
    {
        auto Entity = Result->Entities + i;
        Entity->TrailPiece = PushStruct<trail_piece>(Arena);
        Entity->TrailPiece->Owner = Owner;
        Entity->Transform.Position = vec3(0.0f);
        Entity->Collider = PushStruct<collider>(Arena);
        Entity->Collider->Type = ColliderType_TrailPiece;
        AddFlags(Entity, EntityFlag_Kinematic);
    }
    return Result;
}

entity *CreateShipEntity(game_state &Game, color Color, color OutlineColor, bool IsPlayer = false)
{
    auto *Entity = PushStruct<entity>(Game.Arena);
    Entity->Model = CreateModel(Game.Arena, GetShipMesh(Game));
    Entity->Model->Materials.push_back(CreateMaterial(Game.Arena, vec4(Color.r, Color.g, Color.b, 1), 0, 0, NULL));
    Entity->Model->Materials.push_back(CreateMaterial(Game.Arena, OutlineColor, 1.0f, 0, NULL, Material_PolygonLines));
    Entity->Transform.Scale.y = 3;
    Entity->Transform.Scale *= 0.75f;
    Entity->Collider = PushStruct<collider>(Game.Arena);
    Entity->Collider->Type = ColliderType_Ship;
    Entity->Ship = PushStruct<ship>(Game.Arena);
    Entity->Ship->Color = Color;
    Entity->Ship->OutlineColor = OutlineColor;
    Entity->Trail = CreateTrail(Game.Arena, Entity, OutlineColor);
    if (IsPlayer)
    {
        Entity->PlayerState = PushStruct<player_state>(Game.Arena);
        AddFlags(Entity, EntityFlag_IsPlayer);
    }
    return Entity;
}

#define DefaultEnemyColor   IntColor(SecondPalette.Colors[3])
#define ExplosiveEnemyColor Color_red
entity *CreateEnemyShipEntity(game_state &Game, vec3 Position, vec3 Velocity, enemy_type Type)
{
    color Color;
    switch (Type)
    {
    case EnemyType_Default:
        Color = DefaultEnemyColor;
        break;

    case EnemyType_Explosive:
        Color = ExplosiveEnemyColor;
        break;
    }

    auto *Result = CreateShipEntity(Game, Color, Color);
    Result->Transform.Position = Position;
    Result->Transform.Velocity = Velocity;
    Result->Ship->EnemyType = Type;
    return Result;
}

#define MinExplosionRot 1.0f
#define MaxExplosionRot 90.0f
#define MinExplosionVel 0.1f
#define MaxExplosionVel 4.0f
#define RandomRotation(Series)  RandomBetween(Series, MinExplosionRot, MaxExplosionRot)
#define RandomVel(Series, Sign) (Sign * RandomBetween(Series, MinExplosionVel, MaxExplosionVel))
inline static float
RandomExplosionVel(random_series &Series, vec3 Sign, int i)
{
    if (Sign[i] == 0)
    {
        int s = RandomBetween(Series, 0, 1);
        int rs = 1;
        if (s == 0)
        {
            rs = -1;
        }
        return RandomVel(Series, rs);
    }
    else
    {
        return RandomVel(Series, Sign[i]);
    }
}

entity *CreateExplosionEntity(game_state &Game, mesh &Mesh, mesh_part &Part,
                              vec3 Position, vec3 Velocity, vec3 Scale,
                              color Color, color OutlineColor, vec3 Sign)
{
    assert(Part.PrimitiveType == GL_TRIANGLES);

    auto *Explosion = PushStruct<explosion>(Game.Arena);
    Explosion->LifeTime = Global_Game_ExplosionLifeTime;
    Explosion->Color = Color;
    InitMeshBuffer(Explosion->Mesh.Buffer);

    size_t PieceCount = Part.Count / 3;
    static vector<vec3> Normals;
    if (Normals.size() < PieceCount)
    {
        Normals.resize(PieceCount);
    }

    Explosion->Pieces.resize(PieceCount);
    Explosion->Triangles.resize(Part.Count);
    auto &Vertices = Mesh.Buffer.Vertices;
    for (size_t i = 0; i < PieceCount; i++)
    {
        size_t Index = i * 3;
        size_t vi_1 = Index * (sizeof(mesh_vertex) / sizeof(float));
        size_t vi_2 = (Index + 1) * (sizeof(mesh_vertex) / sizeof(float));
        size_t vi_3 = (Index + 2) * (sizeof(mesh_vertex) / sizeof(float));
        vec3 p1 = vec3{ Vertices[vi_1], Vertices[vi_1 + 1], Vertices[vi_1 + 2] };
        vec3 p2 = vec3{ Vertices[vi_2], Vertices[vi_2 + 1], Vertices[vi_2 + 2] };
        vec3 p3 = vec3{ Vertices[vi_3], Vertices[vi_3 + 1], Vertices[vi_3 + 2] };
        Explosion->Triangles[Index] = p1;
        Explosion->Triangles[Index + 1] = p2;
        Explosion->Triangles[Index + 2] = p3;

        // @TODO: do not regenerate normals
        Normals[i] = GenerateNormal(p1, p2, p3);
    }

    AddPart(&Explosion->Mesh,
            mesh_part{
                material{Color, 0, 0, NULL, Material_ForceTransparent},
                0,
                Explosion->Triangles.size(),
                GL_TRIANGLES});
    AddPart(&Explosion->Mesh,
            mesh_part{
                material{OutlineColor, 2.0f, 0, NULL, Material_ForceTransparent | Material_PolygonLines},
                0,
                Explosion->Triangles.size(),
                GL_TRIANGLES});

    for (size_t i = 0; i < PieceCount; i++)
    {
        auto &Piece = Explosion->Pieces[i];
        Piece.Position = Position;
        Piece.Scale = Scale;
        Piece.Velocity = Velocity * 1.1f + Normals[i];
    }

    auto *Result = PushStruct<entity>(Game.Arena);
    Result->Transform.Position = Position;
    Result->Explosion = Explosion;
    return Result;
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

#define ShipMinVel              20.0f
#define ShipMaxVel              100.0f
#define PlayerMinVel            25.0f
#define PlayerMaxVel            120.0f
#define ShipAcceleration        20.0f
#define ShipBreakAcceleration   30.0f
#define ShipSteerSpeed          20.0f
#define ShipSteerAcceleration   80.0f
#define ShipFriction            5.0f
void MoveShipEntity(entity *Entity, float MoveH, float MoveV, float DeltaTime)
{
    float MinVel = ShipMinVel;
    if (Entity->Flags & EntityFlag_IsPlayer)
    {
        //MinVel = PlayerMinVel;
    }
    if (Entity->Transform.Velocity.y < MinVel)
    {
        //MoveV = 0.1f;
    }

    float MaxVel = ShipMaxVel;
    if (Entity->Flags & EntityFlag_IsPlayer)
    {
        MaxVel = PlayerMaxVel;
    }

    Entity->Transform.Velocity.y += MoveV * ShipAcceleration * DeltaTime;
    if ((MoveV <= 0.0f && Entity->Transform.Velocity.y > 0) || Entity->Transform.Velocity.y > MaxVel)
    {
        Entity->Transform.Velocity.y -= ShipFriction * DeltaTime;
    }

    float SteerTarget = MoveH * ShipSteerSpeed;
    Entity->Transform.Velocity.x = Interp(Entity->Transform.Velocity.x,
                                          SteerTarget,
                                          ShipSteerAcceleration,
                                          DeltaTime);

    Entity->Transform.Rotation.y = 20.0f * (MoveH / 1.0f);
    Entity->Transform.Rotation.x = Interp(Entity->Transform.Rotation.x,
                                          5.0f * MoveV,
                                          20.0f,
                                          DeltaTime);
}

void PushPosition(trail *Trail, vec3 Pos)
{
    if (Trail->PositionStackIndex >= TrailCount)
    {
        vec3 PosSave = Trail->Entities[TrailCount - 1].Transform.Position;
        for (int i = TrailCount - 1; i > 0; i--)
        {
            vec3 NewPos = PosSave;
            PosSave = Trail->Entities[i - 1].Transform.Position;
            Trail->Entities[i - 1].Transform.Position = NewPos;
        }
        Trail->PositionStackIndex -= 1;
    }
    Trail->Entities[Trail->PositionStackIndex++].Transform.Position = Pos;
}
