// Copyright

static void
AddLine(vertex_buffer &Buffer, vec3 p1, vec3 p2, color c = Color_white, vec3 n = vec3(0))
{
    PushVertex(Buffer, {p1.x, p1.y, p1.z, 0, 0,  c.r, c.g, c.b, c.a,  n.x, n.y, n.z});
    PushVertex(Buffer, {p2.x, p2.y, p2.z, 0, 0,  c.r, c.g, c.b, c.a,  n.x, n.y, n.z});
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

void AddTriangle(vertex_buffer &Buffer, vec3 p1, vec3 p2, vec3 p3, vec3 n, color c1 = Color_white)
{
    color c2 = c1;
    color c3 = c1;

    PushVertex(Buffer, {p1.x, p1.y, p1.z, 0, 0,   c1.r, c1.g, c1.b, c1.a,  n.x, n.y, n.z});
    PushVertex(Buffer, {p2.x, p2.y, p2.z, 0, 0,   c2.r, c2.g, c2.b, c2.a,  n.x, n.y, n.z});
    PushVertex(Buffer, {p3.x, p3.y, p3.z, 0, 0,   c3.r, c3.g, c3.b, c3.a,  n.x, n.y, n.z});
}

void AddTriangle(vertex_buffer &Buffer, vec3 p1, vec3 p2, vec3 p3)
{
    AddTriangle(Buffer, p1, p2, p3, GenerateNormal(p1, p2, p3));
}

static void
AddCube(vertex_buffer &Buffer, color c = Color_white, bool NoLight = false)
{
    float z = -0.5;
    float h = z+1;
    float x = -0.5f;
    float w = x+1;
    float y = -0.5f;
    float d = y+1;

	if (NoLight)
	{
		AddQuad(Buffer, vec3(x, y, z), vec3(w, y, z), vec3(w, y, h), vec3(x, y, h), c, vec3(1,1,1));
		AddQuad(Buffer, vec3(w, y, z), vec3(w, d, z), vec3(w, d, h), vec3(w, y, h), c, vec3(1, 1, 1));
		AddQuad(Buffer, vec3(w, d, z), vec3(x, d, z), vec3(x, d, h), vec3(w, d, h), c, vec3(1, 1, 1));
		AddQuad(Buffer, vec3(x, d, z), vec3(x, y, z), vec3(x, y, h), vec3(x, d, h), c, vec3(1, 1, 1));
		AddQuad(Buffer, vec3(x, y, h), vec3(w, y, h), vec3(w, d, h), vec3(x, d, h), c, vec3(1, 1, 1));
		AddQuad(Buffer, vec3(x, d, z), vec3(w, d, z), vec3(w, y, z), vec3(x, y, z), c, vec3(1, 1, 1));
	}
	else
	{
		AddQuad(Buffer, vec3(x, y, z), vec3(w, y, z), vec3(w, y, h), vec3(x, y, h), c, vec3(0, -1, 0));
		AddQuad(Buffer, vec3(w, y, z), vec3(w, d, z), vec3(w, d, h), vec3(w, y, h), c, vec3(1, 0, 0));
		AddQuad(Buffer, vec3(w, d, z), vec3(x, d, z), vec3(x, d, h), vec3(w, d, h), c, vec3(0, 1, 0));
		AddQuad(Buffer, vec3(x, d, z), vec3(x, y, z), vec3(x, y, h), vec3(x, d, h), c, vec3(-1, 0, 0));
		AddQuad(Buffer, vec3(x, y, h), vec3(w, y, h), vec3(w, d, h), vec3(x, d, h), c, vec3(0, 0, 1));
		AddQuad(Buffer, vec3(x, d, z), vec3(w, d, z), vec3(w, y, z), vec3(x, y, z), c, vec3(0, 0, -1));
	}
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
CreateTrail(memory_arena &Arena, entity *Owner, color Color)
{
    trail *Result = PushStruct<trail>(Arena);
    Result->Owner = Owner;

    InitMeshBuffer(Result->Mesh.Buffer);
    Result->Model.Mesh = &Result->Mesh;

    const size_t LineCount = TrailCount*2;
    const size_t PlaneCount = TrailCount*6;
    const float emission = 2.0f;

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
        Entity->Transform.Position = vec3(0.0f);
        Entity->Bounds = PushStruct<collision_bounds>(Arena);
        AddFlags(Entity, Entity_Kinematic);
    }
    return Result;
}

entity *CreateShipEntity(game_state &Game, color Color, color OutlineColor, bool IsPlayer = false)
{
    auto *Entity = PushStruct<entity>(Game.Arena);
    Entity->Type = EntityType_Ship;
    Entity->Model = CreateModel(Game.Arena, &Game.ShipMesh);
    Entity->Model->Materials.push_back(CreateMaterial(Game.Arena, vec4(Color.r, Color.g, Color.b, 1), 0, 0, NULL));
    Entity->Model->Materials.push_back(CreateMaterial(Game.Arena, OutlineColor, 1.0f, 0, NULL, Material_PolygonLines));
    Entity->Transform.Scale.y = 3;
    Entity->Transform.Scale *= 0.75f;
    Entity->Bounds = PushStruct<collision_bounds>(Game.Arena);
    Entity->Ship = PushStruct<ship>(Game.Arena);
    Entity->Ship->Color = Color;
    Entity->Ship->OutlineColor = OutlineColor;
    if (!IsPlayer)
    {
        Entity->Trail = CreateTrail(Game.Arena, Entity, OutlineColor);
    }
    else
    {
        AddFlags(Entity, Entity_IsPlayer);
    }
    return Entity;
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

entity *CreateExplosionEntity(game_state &Game, vec3 Position, vec3 Velocity, color Color, color OutlineColor, vec3 Sign)
{
    auto *Explosion = PushStruct<explosion>(Game.Arena);
	Explosion->LifeTime = Global_Game_ExplosionLifeTime;
	Explosion->Color = Color;
    InitMeshBuffer(Explosion->Mesh.Buffer);
	AddCube(Explosion->Mesh.Buffer, Color_white, true);
	for (int i = 0; i < ExplosionPieceCount; i++)
	{
		vec3 p(0.0f);
		AddTriangle(Explosion->Mesh.Buffer, p, p, p);
	}
	UploadVertices(Explosion->Mesh.Buffer, GL_DYNAMIC_DRAW);

	AddPart(Explosion->Mesh,
			mesh_part{
				material{ Color_white, 0.0f, 0, NULL, 0 },
				0,
				36,
				GL_TRIANGLES });
    AddPart(Explosion->Mesh,
			mesh_part{
				material{Color, 0, 0, NULL, Material_ForceTransparent},
				36,
				3 * ExplosionPieceCount,
				GL_TRIANGLES});
    AddPart(Explosion->Mesh,
			mesh_part{
				material{OutlineColor, 2.0f, 0, NULL, Material_ForceTransparent | Material_PolygonLines},
				36,
				3 * ExplosionPieceCount,
				GL_TRIANGLES});

    for (int i = 0; i < ExplosionPieceCount; i++)
    {
        auto &Piece = Explosion->Pieces[i];
        auto &Series = Game.ExplosionEntropy;
        Piece.Position = Position;
        Piece.Scale = vec3(0.25f);
        for (int i = 0; i < 3; i++)
        {
            Piece.Velocity[i] = RandomExplosionVel(Series, Sign, i);
        }
        Piece.Velocity += Velocity;
        Piece.Rotation = vec3{
            RandomRotation(Series),
            RandomRotation(Series),
            RandomRotation(Series),
        };
    }

    auto *Result = PushStruct<entity>(Game.Arena);
	Result->Transform.Position = Position;
    Result->Explosion = Explosion;
    return Result;
}

entity *CreateCrystalEntity(game_state &Game, vec3 Position)
{
	auto *Result = PushStruct<entity>(Game.Arena);
	Result->Transform.Position = Position;
	Result->Transform.Scale = vec3{0.5f, 0.5f, 0.75f};
	Result->Type = EntityType_Crystal;
	Result->Model = CreateModel(Game.Arena, &Game.CrystalMesh);
	Result->Bounds = PushStruct<collision_bounds>(Game.Arena);
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

#define ShipMaxVel  50.0f
#define PlayerMaxVel 55.0f
#define ShipAcceleration 20.0f
#define ShipBreakAcceleration 30.0f
#define ShipSteerSpeed 10.0f
#define ShipSteerAcceleration 50.0f
#define ShipFriction 2.0f
void MoveShipEntity(entity *Entity, float MoveH, float MoveV, float DeltaTime)
{
    float MaxVel = ShipMaxVel;
    if (Entity->Flags & Entity_IsPlayer)
    {
        MaxVel = PlayerMaxVel;
    }

    if (MoveV > 0.0f && Entity->Transform.Velocity.y < MaxVel)
    {
        Entity->Transform.Velocity.y += MoveV * ShipAcceleration * DeltaTime;
    }
    else if (MoveV < 0.0f && Entity->Transform.Velocity.y > 0)
    {
        Entity->Transform.Velocity.y = std::max(0.0f, Entity->Transform.Velocity.y + (MoveV * ShipBreakAcceleration * DeltaTime));
    }

    if ((MoveV <= 0.0f && Entity->Transform.Velocity.y > 0) || Entity->Transform.Velocity.y > MaxVel)
    {
        Entity->Transform.Velocity.y -= ShipFriction * DeltaTime;
    }

    float SteerTarget = MoveH * ShipSteerSpeed;
    Entity->Transform.Velocity.x = Interp(Entity->Transform.Velocity.x,
										  SteerTarget,
										  ShipSteerAcceleration,
										  DeltaTime);

    Entity->Transform.Rotation.y = 20.0f * (Entity->Transform.Velocity.x / ShipSteerSpeed);
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
