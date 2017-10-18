// Copyright

inline static void AddFlags(entity *Entity, uint32 Flags)
{
    Entity->Flags |= Flags;
}

static material *CreateMaterial(memory_arena &Arena, color Color, float Emission, float TexWeight, texture *Texture, uint32 Flags = 0)
{
    material *Result = PushStruct<material>(Arena);
    Result->DiffuseColor = Color;
    Result->Emission = Emission;
    Result->TexWeight = TexWeight;
    Result->Texture = Texture;
    Result->Flags = Flags;
    return Result;
}

static model *CreateModel(memory_arena &Arena, mesh *Mesh)
{
    model *Result = PushStruct<model>(Arena);
    Result->Mesh = Mesh;
    return Result;
}

static collider *CreateCollider(memory_arena &Arena, collider_type Type)
{
    collider *Result = PushStruct<collider>(Arena);
    Result->Type = Type;
    return Result;
}

static trail *CreateTrail(memory_arena &Arena, entity *Owner, color Color, float radius = 0.5f, bool renderOnly = false)
{
    trail *result = PushStruct<trail>(Arena);
    result->RenderOnly = renderOnly;
    result->Radius = radius;

    InitMeshBuffer(result->Mesh.Buffer);
    result->Model.Mesh = &result->Mesh;

    const size_t LineCount = TrailCount*2;
    const size_t PlaneCount = TrailCount*6;
    const float emission = 4.0f;

    // Plane parts
    AddPart(&result->Mesh, {{Color, 0, 0, NULL, MaterialFlag_ForceTransparent}, 0, 6, GL_TRIANGLES});
    AddPart(&result->Mesh, {{Color, 0, 0, NULL, MaterialFlag_ForceTransparent}, 6, TrailCount*6 - 6, GL_TRIANGLES});

    // Left line parts
    AddPart(&result->Mesh, {{Color, emission, 0, NULL}, PlaneCount, 2, GL_LINES});
    AddPart(&result->Mesh, {{Color, emission, 0, NULL}, PlaneCount + 2, LineCount - 2, GL_LINES});

    // Right line parts
    AddPart(&result->Mesh, {{Color, emission, 0, NULL}, PlaneCount + LineCount, 2, GL_LINES});
    AddPart(&result->Mesh, {{Color, emission, 0, NULL}, PlaneCount + LineCount + 2, LineCount - 2, GL_LINES});

    ReserveVertices(result->Mesh.Buffer, PlaneCount + LineCount*2, GL_DYNAMIC_DRAW);

    for (int i = 0; i < TrailCount; i++)
    {
        auto Entity = result->Entities + i;
        Entity->Transform.Position = vec3(0.0f);
        if (!renderOnly)
        {
            Entity->TrailPiece = PushStruct<trail_piece>(Arena);
            Entity->TrailPiece->Owner = Owner;
            Entity->Collider = PushStruct<collider>(Arena);
            Entity->Collider->Type = ColliderType_TrailPiece;
            AddFlags(Entity, EntityFlag_Kinematic);
        }
    }
    return result;
}

static lane_slot *CreateLaneSlot(memory_arena &arena, int lane)
{
    assert(lane >= -2 && lane <= 2);

    auto result = PushStruct<lane_slot>(arena);
    result->Index = lane+2;
    return result;
}

entity *CreateShipEntity(memory_arena &arena, mesh *shipMesh, color c, color outlineColor, int lane, bool isPlayer = false)
{
    auto ent = PushStruct<entity>(arena);
    ent->Model = CreateModel(arena, shipMesh);
    ent->Model->Materials.push_back(CreateMaterial(arena, vec4(c.r, c.g, c.b, 1), 0, 0, NULL));
    ent->Model->Materials.push_back(CreateMaterial(arena, outlineColor, 1.0f, 0, NULL, MaterialFlag_PolygonLines));
    ent->Transform.Scale.y = 3;
    ent->Transform.Scale *= 0.75f;
    ent->Collider = PushStruct<collider>(arena);
    ent->Collider->Type = ColliderType_Ship;
    ent->Ship = PushStruct<ship>(arena);
    ent->Ship->Color = c;
    ent->Ship->OutlineColor = outlineColor;
    ent->Trail = CreateTrail(arena, ent, outlineColor);
    ent->LaneSlot = CreateLaneSlot(arena, lane);
    if (isPlayer)
    {
        ent->PlayerState = PushStruct<player_state>(arena);
        AddFlags(ent, EntityFlag_IsPlayer);
    }
    return ent;
}

entity *CreateCrystalEntity(memory_arena &arena, mesh *crystalMesh, int lane)
{
    auto ent = PushStruct<entity>(arena);
    ent->Flags |= EntityFlag_RemoveOffscreen;
    ent->Model = CreateModel(arena, crystalMesh);
    ent->Collider = CreateCollider(arena, ColliderType_Crystal);
    ent->LaneSlot = CreateLaneSlot(arena, lane);
    return ent;
}

#define MinExplosionRot 1.0f
#define MaxExplosionRot 90.0f
#define MinExplosionVel 0.1f
#define MaxExplosionVel 4.0f
#define RandomRotation(Series)  RandomBetween(Series, MinExplosionRot, MaxExplosionRot)
#define RandomVel(Series, Sign) (Sign * RandomBetween(Series, MinExplosionVel, MaxExplosionVel))
inline static float RandomExplosionVel(random_series &Series, vec3 Sign, int i)
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

entity *CreatePowerupEntity(memory_arena &arena, random_series &series, float timeSpawn, vec3 pos, vec3 vel, color c)
{
    auto result = PushStruct<entity>(arena);
    result->Powerup = PushStruct<powerup>(arena);
    result->Powerup->Color = c;
    result->Powerup->TimeSpawn = timeSpawn;
    result->Trail = CreateTrail(arena, result, c, 0.1f, true);
    result->SetPos(pos);
    result->Vel().x = RandomBetween(series, -20.0f, 20.0f);
    result->Vel().y = vel.y + (vel.y * 0.3f);
    result->Vel().z = vel.z + 20.0f;
    return result;
}

entity *CreateExplosionEntity(memory_arena &arena, mesh &baseMesh, mesh_part &part, vec3 pos, vec3 vel, vec3 scale, color c, color outlineColor, vec3 sign)
{
    assert(part.PrimitiveType == GL_TRIANGLES);

    auto exp = PushStruct<explosion>(arena);
    exp->LifeTime = Global_Game_ExplosionLifeTime;
    exp->Color = c;
    InitMeshBuffer(exp->Mesh.Buffer);

    const float count = EXPLOSION_PARTS_COUNT;
    const float theta = M_PI*2 / count;
    float angle = 0;
    for (int i = 0; i < count; i++)
    {
        angle += theta;

        auto ent = exp->Entities + i;
        ent->Trail = CreateTrail(arena, ent, outlineColor, 0.2f, true);
        ent->SetPos(pos);
        ent->Vel().x = std::cos(angle)*10.0f;
        ent->Vel().y = vel.y + std::sin(angle)*10.0f;
    }
    /*
    size_t pieceCount = part.Count / 3;
    static vector<vec3> normals;
    if (normals.size() < PieceCount)
    {
        normals.resize(PieceCount);
    }

    exp->Pieces.resize(pieceCount);
    exp->Triangles.resize(part.Count);
    auto &Vertices = baseMesh.Buffer.Vertices;
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
                material{Color, 0, 0, NULL, MaterialFlag_ForceTransparent},
                0,
                Explosion->Triangles.size(),
                GL_TRIANGLES});
    AddPart(&Explosion->Mesh,
            mesh_part{
                material{OutlineColor, 2.0f, 0, NULL, MaterialFlag_ForceTransparent | MaterialFlag_PolygonLines},
                0,
                Explosion->Triangles.size(),
                GL_TRIANGLES});

    for (size_t i = 0; i < PieceCount; i++)
    {
        auto &Piece = Explosion->Pieces[i];
        Piece.Position = Position;
        Piece.Scale = Scale;
        Piece.Velocity = Velocity*1.1f + Normals[i]*1.5f;
    }
    */

    auto result = PushStruct<entity>(arena);
    result->Transform.Position = pos;
    result->Explosion = exp;
    return result;
}

static float Interp(float c, float t, float a, float dt)
{
    if (c == t)
    {
        return t;
    }
    float dir = std::copysign(1, t - c);
    c += a * dir * dt;
    return (dir == std::copysign(1, t - c)) ? c : t;
}

#define ShipMinVel              40.0f
#define PlayerMinVel            50.0f
#define ShipAcceleration        20.0f
#define ShipBreakAcceleration   30.0f
#define ShipSteerSpeed          20.0f
#define ShipSteerAcceleration   80.0f
#define ShipFriction            10.0f
void MoveShipEntity(entity *ent, float moveH, float moveV, float maxVel, float dt)
{
    float MinVel = ShipMinVel;
    if (ent->Flags & EntityFlag_IsPlayer)
    {
        MinVel = PlayerMinVel;
    }
    if (ent->Transform.Velocity.y < MinVel)
    {
        moveV = 0.1f;
    }

    ent->Transform.Velocity.y += moveV * ShipAcceleration * dt;
    if ((moveV <= 0.0f && ent->Transform.Velocity.y > 0) || ent->Transform.Velocity.y > maxVel)
    {
        ent->Transform.Velocity.y -= ShipFriction * dt;
    }

    float steerTarget = moveH * ShipSteerSpeed;
    ent->Transform.Velocity.y = std::min(ent->Transform.Velocity.y, maxVel);
    ent->Transform.Velocity.x = Interp(ent->Transform.Velocity.x,
                                          steerTarget,
                                          ShipSteerAcceleration,
                                          dt);

    ent->Transform.Rotation.y = 20.0f * (moveH / 1.0f);
    ent->Transform.Rotation.x = Interp(ent->Transform.Rotation.x,
                                          5.0f * moveV,
                                          20.0f,
                                          dt);
}

void PushPosition(trail *t, vec3 pos)
{
    if (t->PositionStackIndex >= TrailCount)
    {
        vec3 posSave = t->Entities[TrailCount - 1].Transform.Position;
        for (int i = TrailCount - 1; i > 0; i--)
        {
            vec3 newPos = posSave;
            posSave = t->Entities[i - 1].Transform.Position;
            t->Entities[i - 1].Transform.Position = newPos;
        }
        t->PositionStackIndex -= 1;
    }
    t->Entities[t->PositionStackIndex++].Transform.Position = pos;
}

// Finds the first free slot on the list and insert the entity
void AddEntityToList(std::vector<entity *> &list, entity *ent)
{
    for (auto it = list.begin(), end = list.end(); it != end; it++)
    {        if (!(*it))
        {
            *it = ent;
            return;
        }
    }

    // no free slot, insert at the end
    list.push_back(ent);
}

void AddEntity(entity_world &world, entity *ent)
{
    if (ent->Model)
    {
        AddEntityToList(world.ModelEntities, ent);
    }
    if (ent->Collider)
    {
        AddEntityToList(world.CollisionEntities, ent);
    }
    if (ent->Trail)
    {
        AddEntityToList(world.TrailEntities, ent);
        if (!ent->Trail->RenderOnly)
        {
            for (int i = 0; i < TrailCount; i++)
            {
                AddEntity(world, ent->Trail->Entities + i);
            }
        }
    }
    if (ent->Explosion)
    {
        world.LastExplosion = ent->Explosion;
        AddEntityToList(world.ExplosionEntities, ent);
        for (int i = 0; i < EXPLOSION_PARTS_COUNT; i++)
        {
            AddEntity(world, ent->Explosion->Entities + i);
        }
    }
    if (ent->Ship && !(ent->Flags & EntityFlag_IsPlayer))
    {
        AddEntityToList(world.ShipEntities, ent);
    }
    if (ent->Repeat)
    {
        AddEntityToList(world.RepeatingEntities, ent);
    }
    if (ent->Flags & EntityFlag_RemoveOffscreen)
    {
        AddEntityToList(world.RemoveOffscreenEntities, ent);
    }
    if (ent->Powerup)
    {
        AddEntityToList(world.PowerupEntities, ent);
    }
    if (ent->LaneSlot)
    {
        AddEntityToList(world.LaneSlotEntities, ent);
    }
    world.NumEntities++;
}

void RemoveEntityFromList(std::vector<entity *> &list, entity *ent)
{
    for (auto it = list.begin(), end = list.end(); it != end; it++)
    {
        if (*it == ent)
        {
            *it = NULL;
            return;
        }
    }
}

void RemoveEntity(entity_world &world, entity *ent)
{
    if (ent->Model)
    {
        RemoveEntityFromList(world.ModelEntities, ent);
    }
    if (ent->Collider)
    {
        RemoveEntityFromList(world.CollisionEntities, ent);
    }
    if (ent->Trail)
    {
        RemoveEntityFromList(world.TrailEntities, ent);
        if (!ent->Trail->RenderOnly)
        {
            for (int i = 0; i < TrailCount; i++)
            {
                RemoveEntity(world, ent->Trail->Entities + i);
            }
        }
    }
    if (ent->Explosion)
    {
        RemoveEntityFromList(world.ExplosionEntities, ent);
        for (int i = 0; i < EXPLOSION_PARTS_COUNT; i++)
        {
            RemoveEntity(world, ent->Explosion->Entities + i);
        }
    }
    if (ent->Ship)
    {
        RemoveEntityFromList(world.ShipEntities, ent);
    }
    if (ent->Repeat)
    {
        RemoveEntityFromList(world.RepeatingEntities, ent);
    }
    if (ent->Flags & EntityFlag_RemoveOffscreen)
    {
        RemoveEntityFromList(world.RemoveOffscreenEntities, ent);
    }
    if (ent->Powerup)
    {
        RemoveEntityFromList(world.PowerupEntities, ent);
    }
    if (ent->LaneSlot)
    {
        RemoveEntityFromList(world.LaneSlotEntities, ent);
    }
    world.NumEntities = std::max(0, world.NumEntities - 1);
}

void UpdateLogiclessEntities(entity_world &world, float dt)
{
    for (auto ent : world.RemoveOffscreenEntities)
    {
        if (!ent) continue;

        if (ent->Pos().y < world.Camera->Position.y)
        {
            RemoveEntity(world, ent);
        }
    }

    for (auto ent : world.RepeatingEntities)
    {
        auto repeat = ent->Repeat;
        if (world.Camera->Position.y - ent->Transform.Position.y > repeat->DistanceFromCamera)
        {
            ent->Transform.Position.y += repeat->Size * repeat->Count;
        }
    }
}

void RenderEntityWorld(render_state &rs, entity_world &world, float dt)
{
    static vec3 PointCache[TrailCount*4];
    for (auto ent : world.TrailEntities)
    {
        if (!ent) continue;

        auto tr = ent->Trail;
        if (tr->FirstFrame)
        {
            tr->FirstFrame = false;
            for (int i = 0; i < TrailCount; i++)
            {
                tr->Entities[i].Transform.Position = ent->Transform.Position;
            }
        }

        tr->Timer -= dt;
        if (tr->Timer <= 0)
        {
            tr->Timer = Global_Game_TrailRecordTimer;
            PushPosition(tr, ent->Transform.Position);
        }

        auto &m = tr->Mesh;
        ResetBuffer(m.Buffer);

        // First store the quads, then the lines
        for (int i = 0; i < TrailCount; i++)
        {
            auto *pieceEntity = tr->Entities + i;
            vec3 c1 = pieceEntity->Transform.Position;
            vec3 c2;
            if (i == TrailCount - 1)
            {
                c2 = ent->Transform.Position;
            }
            else
            {
                c2 = tr->Entities[i + 1].Transform.Position;
            }

            float CurrentTrailTime = tr->Timer/Global_Game_TrailRecordTimer;
            if (i == 0)
            {
                c1 -= (c2 - c1) * CurrentTrailTime;
            }

            float r = tr->Radius;
            float rm = tr->Radius * 0.8f;
            float min2 =  rm * ((TrailCount - i) / (float)TrailCount);
            float min1 =  rm * ((TrailCount - i+1) / (float)TrailCount);
            vec3 p1 = c2 - vec3(r - min2, 0, 0);
            vec3 p2 = c2 + vec3(r - min2, 0, 0);
            vec3 p3 = c1 - vec3(r - min1, 0, 0);
            vec3 p4 = c1 + vec3(r - min1, 0, 0);
            color cl2 = color{ 1, 1, 1, 1.0f - min2 * 2 };
            color cl1 = color{ 1, 1, 1, 1.0f - min1 * 2 };
            AddQuad(m.Buffer, p2, p1, p3, p4, cl2, cl2, cl1, cl1);

            const float lo = 0.05f;
            PointCache[i*4] = p1 - vec3(lo, 0, 0);
            PointCache[i*4 + 1] = p3 - vec3(lo, 0, 0);
            PointCache[i*4 + 2] = p2 + vec3(lo, 0, 0);
            PointCache[i*4 + 3] = p4 + vec3(lo, 0, 0);
            if (pieceEntity->Collider)
            {
                auto &box = pieceEntity->Collider->Box;
                box.Half = vec3(r, (c2.y-c1.y) * 0.5f, 0.5f);
                box.Center = vec3(c1.x, c1.y + box.Half.y, c1.z + box.Half.z);
            }
        }
        for (int i = 0; i < TrailCount; i++)
        {
            vec3 *p = PointCache + i*4;
            AddLine(m.Buffer, *p++, *p++);
        }
        for (int i = 0; i < TrailCount; i++)
        {
            vec3 *p = PointCache + i*4 + 2;
            AddLine(m.Buffer, *p++, *p++);
        }
        UploadVertices(m.Buffer, 0, m.Buffer.VertexCount);

        DrawModel(rs, tr->Model, transform{});
    }

    for (auto ent : world.ExplosionEntities)
    {
        if (!ent) continue;

        auto exp = ent->Explosion;
        exp->LifeTime -= dt;
        if (exp->LifeTime <= 0)
        {
            RemoveEntity(world, ent);
            continue;
        }

        float alpha = exp->LifeTime / Global_Game_ExplosionLifeTime;
        for (int i = 0; i < EXPLOSION_PARTS_COUNT; i++)
        {
            auto partEnt = exp->Entities + i;
            partEnt->Transform.Position += partEnt->Transform.Velocity * dt;

            for (auto &meshPart : partEnt->Trail->Mesh.Parts)
            {
                meshPart.Material.DiffuseColor.a = alpha;
            }
        }
/*
        ResetBuffer(exp->Mesh.Buffer);
        for (size_t i = 0; i < exp->Pieces.size(); i++)
        {
            auto &Piece = exp->Pieces[i];
            Piece.Position += Piece.Velocity * dt;
            mat4 Transform = GetTransformMatrix(Piece);

            size_t TriangleIndex = i * 3;
            vec3 p1 = vec3(Transform * vec4{ exp->Triangles[TriangleIndex], 1.0f });
            vec3 p2 = vec3(Transform * vec4{ exp->Triangles[TriangleIndex + 1], 1.0f });
            vec3 p3 = vec3(Transform * vec4{ exp->Triangles[TriangleIndex + 2], 1.0f });
            AddTriangle(exp->Mesh.Buffer, p1, p2, p3, vec3{ 1, 1, 1 }, color{1, 1, 1, alpha});
        }
        UploadVertices(exp->Mesh.Buffer, GL_DYNAMIC_DRAW);

        DrawMeshPart(rs, exp->Mesh, exp->Mesh.Parts[0], transform{});
        DrawMeshPart(rs, exp->Mesh, exp->Mesh.Parts[1], transform{});
*/
    }

    for (auto ent : world.ModelEntities)
    {
        if (!ent) continue;

        DrawModel(rs, *ent->Model, ent->Transform);
    }

    if (world.LastExplosion)
    {
        ApplyExplosionLight(rs, world.LastExplosion->Color);
    }
    world.LastExplosion = NULL;
}
