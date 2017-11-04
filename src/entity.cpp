// Copyright

inline static void AddFlags(entity *Entity, uint32 Flags)
{
    Entity->Flags |= Flags;
}

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

static model *CreateModel(allocator *alloc, mesh *Mesh)
{
    auto result = PushStruct<model>(alloc);
    result->Mesh = Mesh;
    return result;
}

static collider *CreateCollider(allocator *alloc, collider_type Type)
{
    auto result = PushStruct<collider>(alloc);
    result->Type = Type;
    return result;
}

#define TRAIL_SIZE (sizeof(trail) + (sizeof(trail_piece)+sizeof(collider))*TrailCount)
static trail *CreateTrail(allocator *alloc, entity *Owner, color Color, float radius = 0.5f, bool renderOnly = false)
{
    trail *result = PushStruct<trail>(alloc);
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
        auto ent = result->Entities + i;
        ent->Transform.Position = vec3(0.0f);
        if (!renderOnly)
        {
            ent->TrailPiece = PushStruct<trail_piece>(alloc);
            ent->TrailPiece->Owner = Owner;
            ent->Collider = PushStruct<collider>(alloc);
            ent->Collider->Type = ColliderType_TrailPiece;
            AddFlags(ent, EntityFlag_Kinematic);
        }
    }
    return result;
}

static lane_slot *CreateLaneSlot(allocator *alloc, int lane)
{
    assert(lane >= -2 && lane <= 2);

    auto result = PushStruct<lane_slot>(alloc);
    result->Index = lane+2;
    return result;
}

entity *CreateEntity(allocator *alloc)
{
    auto result = PushStruct<entity>(alloc);
    result->PoolEntry = dynamic_cast<memory_pool_entry *>(alloc);
    return result;
}

#define SHIP_ENTITY_SIZE (sizeof(entity) + sizeof(model) + sizeof(collider) + sizeof(ship) + TRAIL_SIZE + sizeof(lane_slot) + (sizeof(material)*2))
entity *CreateShipEntity(allocator *alloc, mesh *shipMesh, color c, color outlineColor, bool isPlayer = false)
{
    auto ent = CreateEntity(alloc);
    ent->Model = CreateModel(alloc, shipMesh);
    ent->Model->Materials.push_back(CreateMaterial(alloc, vec4(c.r, c.g, c.b, 1), 0, 0, NULL));
    ent->Model->Materials.push_back(CreateMaterial(alloc, outlineColor, 1.0f, 0, NULL, MaterialFlag_PolygonLines));
    ent->Transform.Scale.y = 3;
    ent->Transform.Scale *= 0.75f;
    ent->Collider = PushStruct<collider>(alloc);
    ent->Collider->Type = ColliderType_Ship;
    ent->Ship = PushStruct<ship>(alloc);
    ent->Ship->Color = c;
    ent->Ship->OutlineColor = outlineColor;
    ent->Trail = CreateTrail(alloc, ent, outlineColor);
    if (isPlayer)
    {
        AddFlags(ent, EntityFlag_IsPlayer);
    }
    return ent;
}

#define CRYSTAL_ENTITY_SIZE (sizeof(entity) + sizeof(model) + sizeof(collider) + sizeof(frame_rotation))
entity *CreateCrystalEntity(allocator *alloc, mesh *crystalMesh)
{
    auto ent = CreateEntity(alloc);
    ent->Flags |= EntityFlag_RemoveOffscreen;
    ent->Model = CreateModel(alloc, crystalMesh);
    ent->Collider = CreateCollider(alloc, ColliderType_Crystal);
    ent->FrameRotation = PushStruct<frame_rotation>(alloc);
    ent->FrameRotation->Rotation.z = 90.0f;
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

#define POWERUP_ENTITY_SIZE (sizeof(entity) + sizeof(powerup) + TRAIL_SIZE)
entity *CreatePowerupEntity(allocator *alloc, random_series &series, float timeSpawn, vec3 pos, vec3 vel, color c)
{
    auto result = CreateEntity(alloc);
    result->Powerup = PushStruct<powerup>(alloc);
    result->Powerup->Color = c;
    result->Powerup->TimeSpawn = timeSpawn;
    result->Trail = CreateTrail(alloc, result, c, 0.1f, true);
    result->SetPos(pos);
    result->Vel().x = RandomBetween(series, -20.0f, 20.0f);
    result->Vel().y = vel.y + (vel.y * 0.3f);
    result->Vel().z = vel.z + 20.0f;
    return result;
}

#define EXPLOSION_ENTITY_SIZE (sizeof(entity) + sizeof(explosion) + (TRAIL_SIZE*EXPLOSION_PARTS_COUNT))
entity *CreateExplosionEntity(allocator *alloc, vec3 pos, vec3 vel, color c, color outlineColor, vec3 sign)
{
    auto exp = PushStruct<explosion>(alloc);
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
        ent->Trail = CreateTrail(alloc, ent, outlineColor, 0.2f, true);
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

        // TODO: do not regenerate normals
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

    auto result = CreateEntity(alloc);
    result->Transform.Position = pos;
    result->Explosion = exp;
    return result;
}

#define ASTEROID_ENTITY_SIZE (sizeof(entity)+sizeof(model)+sizeof(collider)+TRAIL_SIZE+sizeof(asteroid))
static entity *CreateAsteroidEntity(allocator *alloc, mesh *astMesh)
{
    auto result = PushStruct<entity>(alloc);
    result->Model = CreateModel(alloc, astMesh);
    result->Collider = CreateCollider(alloc, ColliderType_Asteroid);
    result->Trail = CreateTrail(alloc, result, ASTEROID_COLOR, 0.5f, true);
    result->Asteroid = PushStruct<asteroid>(alloc);
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
#define PLAYER_MIN_VEL          50.0f
#define PLAYER_MIN_VEL_BREAKING 20.0f
#define ShipAcceleration        20.0f
#define ShipBreakAcceleration   30.0f
#define ShipSteerSpeed          20.0f
#define ShipSteerAcceleration   80.0f
#define ShipFriction            10.0f
void MoveShipEntity(entity *ent, float moveX, float moveY, float maxVel, float dt)
{
    float minVel = ShipMinVel;
    if (ent->Flags & EntityFlag_IsPlayer)
    {
        minVel = PLAYER_MIN_VEL;
    }
    if (moveY < 0.0f)
    {
        minVel = PLAYER_MIN_VEL_BREAKING;
    }
    if (ent->Transform.Velocity.y < minVel)
    {
        moveY = 0.1f;
    }

    ent->Transform.Velocity.y += moveY * ShipAcceleration * dt;
    if ((moveY <= 0.0f && ent->Transform.Velocity.y > 0) || ent->Transform.Velocity.y > maxVel)
    {
        ent->Transform.Velocity.y -= ShipFriction * dt;
    }

    float steerTarget = moveX * ShipSteerSpeed;
    ent->Transform.Velocity.y = std::min(ent->Transform.Velocity.y, maxVel);
    ent->Transform.Velocity.x = Interp(ent->Transform.Velocity.x,
                                          steerTarget,
                                          ShipSteerAcceleration,
                                          dt);

    ent->Transform.Rotation.y = 20.0f * (moveX / 1.0f);
    ent->Transform.Rotation.x = Interp(ent->Transform.Rotation.x,
                                          5.0f * moveY,
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

void InitEntityWorld(entity_world &world)
{
    world.ShipPool.ElemSize = SHIP_ENTITY_SIZE;
    world.CrystalPool.ElemSize = CRYSTAL_ENTITY_SIZE;
    world.PowerupPool.ElemSize = POWERUP_ENTITY_SIZE;
    world.ExplosionPool.ElemSize = EXPLOSION_ENTITY_SIZE;
    world.AsteroidPool.ElemSize = ASTEROID_ENTITY_SIZE;

#ifdef DRAFT_DEBUG
    world.ShipPool.DEBUGName = "ShipPool";
    world.CrystalPool.DEBUGName = "CrystalPool";
    world.PowerupPool.DEBUGName = "PowerupPool";
    world.ExplosionPool.DEBUGName = "ExplosionPool";
    world.AsteroidPool.DEBUGName = "AsteroidPool";
#endif
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
    if (ent->FrameRotation)
    {
        AddEntityToList(world.RotatingEntities, ent);
    }
    if (ent->Collider && ent->Collider->Type == ColliderType_Asteroid)
    {
        AddEntityToList(world.AsteroidEntities, ent);
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
        FreeEntry(world.ExplosionPool, ent->PoolEntry);
    }
    if (ent->Ship)
    {
        RemoveEntityFromList(world.ShipEntities, ent);
        FreeEntry(world.ShipPool, ent->PoolEntry);
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
        FreeEntry(world.PowerupPool, ent->PoolEntry);
    }
    if (ent->LaneSlot)
    {
        RemoveEntityFromList(world.LaneSlotEntities, ent);
    }
    if (ent->FrameRotation)
    {
        RemoveEntityFromList(world.RotatingEntities, ent);
    }
    if (ent->Collider && ent->Collider->Type == ColliderType_Crystal)
    {
        FreeEntry(world.CrystalPool, ent->PoolEntry);
    }
    if (ent->Collider && ent->Collider->Type == ColliderType_Asteroid)
    {
        FreeEntry(world.AsteroidPool, ent->PoolEntry);
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
    for (auto ent : world.RotatingEntities)
    {
        if (!ent) continue;

        ent->Transform.Rotation += ent->FrameRotation->Rotation * dt;
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
        }/*
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
