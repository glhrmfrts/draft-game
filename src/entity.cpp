// Copyright

#define SHIP_Z            0.2f

#define PLAYER_BODY_COLOR     Color_blue
#define PLAYER_OUTLINE_COLOR  IntColor(FirstPalette.Colors[1])

inline static void AddFlags(entity *Entity, uint32 Flags)
{
    Entity->Flags |= Flags;
}

inline float LaneIndexToPitch(int index)
{
	return (index + 2)*0.25f;
}

static model *CreateModel(allocator *alloc, mesh *Mesh)
{
    auto result = PushStruct<model>(alloc);
    result->Mesh = Mesh;
    return result;
}

static collider *CreateCollider(allocator *alloc, collider_type type, vec3 scale = vec3(1.0f))
{
    auto result = PushStruct<collider>(alloc);
    result->Type = type;
    result->Scale = scale;
    return result;
}

#define TRAIL_SIZE (sizeof(trail) + (sizeof(trail_piece)+sizeof(collider))*TRAIL_COUNT)
static trail *CreateTrail(allocator *alloc, entity *Owner, color Color,
						  float radius = 0.5f, bool renderOnly = false)
{
	trail *result = PushStruct<trail>(alloc);
	result->RenderOnly = renderOnly;
	result->Radius = radius;

	for (int i = 0; i < TRAIL_COUNT; i++)
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

#define TRAIL_GROUP_SIZE(n) (sizeof(trail_group) + (sizeof(entity)*n) + (sizeof(material)*3) + (TRAIL_SIZE*n) + (TRAIL_COUNT*4*sizeof(vec3)*n))
static trail_group *CreateTrailGroup(allocator *alloc, entity *owner, color c,
                                     float radius = 0.5f, bool renderOnly = false, size_t count = 1)
{
    trail_group *result = PushStruct<trail_group>(alloc);
    result->RenderOnly = renderOnly;
    result->Radius = radius;
	result->Count = count;
	result->Entities.resize(count);
	result->PointCache.resize(count);

    InitMeshBuffer(result->Mesh.Buffer);
    result->Model.Mesh = &result->Mesh;

    const size_t LineCount = count*TRAIL_COUNT*2;
    const size_t PlaneCount = count*TRAIL_COUNT*6;
    const float emission = 4.0f;

    // planes
    AddPart(&result->Mesh, {CreateMaterial(alloc, c, 0, 0, NULL, MaterialFlag_ForceTransparent), 0, PlaneCount, GL_TRIANGLES});

    // left line
    AddPart(&result->Mesh, { CreateMaterial(alloc, c, emission, 0, NULL), PlaneCount, LineCount, GL_LINES});

    // right line
    AddPart(&result->Mesh, { CreateMaterial(alloc, c, emission, 0, NULL), PlaneCount + LineCount, LineCount, GL_LINES});

	ReserveVertices(result->Mesh.Buffer, PlaneCount + LineCount * 2, GL_DYNAMIC_DRAW);

	for (size_t i = 0; i < count; i++)
	{
		auto ent = PushStruct<entity>(alloc);
		ent->Transform.Position = vec3(0.0f);
		ent->Trail = CreateTrail(alloc, owner, c, radius, renderOnly);

		result->Entities[i] = ent;
		result->PointCache[i] = (vec3 *)PushSize(alloc, TRAIL_COUNT*4*sizeof(vec3), "trail group point cache");
	}

	return result;
}

static lane_slot *CreateLaneSlot(allocator *alloc, int lane, bool occupy = true)
{
    assert(lane >= -2 && lane <= 2);

    auto result = PushStruct<lane_slot>(alloc);
	result->Lane = lane;
    result->Index = lane+2;
	result->Occupy = occupy;
    return result;
}

static audio_source *CreateAudioSource(allocator *alloc, audio_clip *buffer = NULL)
{
	auto result = PushStruct<audio_source>(alloc);
	AudioSourceCreate(result, buffer);
	return result;
}

entity *CreateEntity(allocator *alloc)
{
    auto result = PushStruct<entity>(alloc);
    result->PoolEntry = dynamic_cast<memory_pool_entry *>(alloc);
    return result;
}

#define SHIP_ENTITY_SIZE (sizeof(entity) + sizeof(model) + sizeof(collider) + sizeof(ship) + sizeof(audio_source) + TRAIL_GROUP_SIZE(1) + sizeof(lane_slot) + (sizeof(material)*2))
entity *CreateShipEntity(allocator *alloc, mesh *shipMesh, color c, color outlineColor, audio_clip *clip, bool isPlayer = false, int colorIndex = 0, int lane = 0)
{
    auto ent = CreateEntity(alloc);
	ent->Type = EntityType_Ship;
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
	ent->LaneSlot = CreateLaneSlot(alloc, lane);
    bool trailRenderOnly = isPlayer || (colorIndex == SHIP_RED);
    ent->TrailGroup = CreateTrailGroup(alloc, ent, outlineColor, 0.5f, trailRenderOnly);
	if (clip)
	{
		ent->AudioSource = CreateAudioSource(alloc, clip);
		AudioSourcePlay(ent->AudioSource);
	}

    if (isPlayer)
    {
		ent->Collider->Active = true;
        AddFlags(ent, EntityFlag_IsPlayer);
    }
    else
    {
        AddFlags(ent, EntityFlag_DestroyAudioSource);
    }
    return ent;
}

#define CRYSTAL_ENTITY_SIZE (sizeof(entity) + sizeof(audio_source) + sizeof(lane_slot) + sizeof(model) + sizeof(collider) + sizeof(frame_rotation))
entity *CreateCrystalEntity(allocator *alloc, asset_loader &loader, mesh *crystalMesh, int lane = 0)
{
    auto ent = CreateEntity(alloc);
	ent->Type = EntityType_Crystal;
    ent->Flags |= EntityFlag_RemoveOffscreen;
    ent->Model = CreateModel(alloc, crystalMesh);
    ent->Collider = CreateCollider(alloc, ColliderType_Crystal);
    ent->FrameRotation = PushStruct<frame_rotation>(alloc);
    ent->FrameRotation->Rotation.z = 90.0f;
	ent->AudioSource = CreateAudioSource(alloc, FindSound(loader, "crystal"));
	ent->LaneSlot = CreateLaneSlot(alloc, lane, false);
    return ent;
}

#define MinExplosionRot 1.0f
#define MaxExplosionRot 90.0f
#define MinExplosionVel 0.1f
#define MaxExplosionVel 4.0f
#define RandomRotation(Series)  RandomBetween(Series, MinExplosionRot, MaxExplosionRot)
#define RandomVel(Series, Sign) (Sign * RandomBetween(Series, MinExplosionVel, MaxExplosionVel))
float RandomExplosionVel(random_series &Series, vec3 Sign, int i)
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

#define POWERUP_ENTITY_SIZE (sizeof(entity) + sizeof(powerup) + TRAIL_GROUP_SIZE(1))
entity *CreatePowerupEntity(allocator *alloc, random_series &series, float timeSpawn, vec3 pos, vec3 vel, color c)
{
    auto result = CreateEntity(alloc);
	result->Type = EntityType_Powerup;
    result->Powerup = PushStruct<powerup>(alloc);
    result->Powerup->Color = c;
    result->Powerup->TimeSpawn = timeSpawn;
    result->TrailGroup = CreateTrailGroup(alloc, result, c, 0.1f, true);
    result->SetPos(pos);
    result->Vel().x = RandomBetween(series, -20.0f, 20.0f);
    result->Vel().y = vel.y + (vel.y * 0.3f);
    result->Vel().z = vel.z + 20.0f;
    return result;
}

#define EXPLOSION_ENTITY_SIZE (sizeof(entity) + sizeof(audio_source) + sizeof(lane_slot) + sizeof(explosion) + TRAIL_GROUP_SIZE(EXPLOSION_PARTS_COUNT))
entity *CreateExplosionEntity(allocator *alloc, asset_loader &loader, vec3 pos, vec3 vel, color c, color outlineColor, vec3 sign, int lane = 0)
{
    auto exp = PushStruct<explosion>(alloc);
	auto tg = CreateTrailGroup(alloc, NULL, c, 0.2, true, EXPLOSION_PARTS_COUNT);
    exp->LifeTime = Global_Game_ExplosionLifeTime;
    exp->Color = c;
    InitMeshBuffer(exp->Mesh.Buffer);

    const float count = EXPLOSION_PARTS_COUNT;
    const float theta = M_PI*2 / count;
    float angle = 0;
    for (int i = 0; i < count; i++)
    {
        angle += theta;

        auto ent = tg->Entities[i];
        ent->SetPos(pos);
        ent->Vel().x = std::cos(angle)*10.0f;
        ent->Vel().y = vel.y + std::sin(angle)*10.0f;
    }

    auto result = CreateEntity(alloc);
	result->Type = EntityType_Explosion;
    result->Transform.Position = pos;
    result->Explosion = exp;
	result->TrailGroup = tg;
	result->AudioSource = CreateAudioSource(alloc, FindSound(loader, "explosion"));
	result->LaneSlot = CreateLaneSlot(alloc, lane, false);
    return result;
}

#define ASTEROID_ENTITY_SIZE (sizeof(entity)+sizeof(model)+sizeof(collider)+TRAIL_GROUP_SIZE(1)+sizeof(asteroid))
entity *CreateAsteroidEntity(allocator *alloc, mesh *astMesh)
{
    auto result = CreateEntity(alloc);
	result->Type = EntityType_Asteroid;
    result->Model = CreateModel(alloc, astMesh);
    result->Collider = CreateCollider(alloc, ColliderType_Asteroid, vec3(0.5f));
    result->TrailGroup = CreateTrailGroup(alloc, result, ASTEROID_COLOR, 0.5f, true);
    result->Asteroid = PushStruct<asteroid>(alloc);
    return result;
}

#define CHECKPOINT_ENTITY_SIZE (sizeof(entity)+sizeof(model)+sizeof(audio_source)+sizeof(checkpoint)+TRAIL_GROUP_SIZE(1)+(sizeof(material)*2))
entity *CreateCheckpointEntity(allocator *alloc, asset_loader &loader, mesh *checkpointMesh)
{
    auto result = CreateEntity(alloc);
	result->Type = EntityType_Checkpoint;
    result->Model = CreateModel(alloc, checkpointMesh);
    result->Model->Materials.push_back(CreateMaterial(alloc, CHECKPOINT_COLOR, 0.0f, 0.0f, NULL));
    result->Model->Materials.push_back(CreateMaterial(alloc, CHECKPOINT_OUTLINE_COLOR, 1.0f, 0.0f, NULL));
    result->Checkpoint = PushStruct<checkpoint>(alloc);
    result->TrailGroup = CreateTrailGroup(alloc, result, CHECKPOINT_OUTLINE_COLOR, ROAD_LANE_COUNT);
    result->SetScl(vec3{ROAD_LANE_COUNT, 1.0f, ROAD_LANE_COUNT*2});
	result->AudioSource = CreateAudioSource(alloc, FindSound(loader, "checkpoint"));
    return result;
}

#define FINISH_ENTITY_SIZE (sizeof(entity)+sizeof(model)+sizeof(finish))
entity *CreateFinishEntity(allocator *alloc, asset_loader &loader, mesh *finishMesh)
{
	auto result = CreateEntity(alloc);
	result->Type = EntityType_Finish;
	result->Model = CreateModel(alloc, finishMesh);
    result->Model->SortNumber = 1;
	result->Finish = PushStruct<finish>(alloc);
	result->SetScl(vec3{ ROAD_LANE_COUNT*4, ROAD_LANE_COUNT*4, ROAD_LANE_COUNT*4 });
	return result;
}

#define ENEMY_SKULL_ENTITY_SIZE (sizeof(entity)+sizeof(model)+sizeof(collider)+TRAIL_GROUP_SIZE(1))
entity *CreateEnemySkullEntity(allocator *alloc, mesh *skullMesh)
{
	auto result = CreateEntity(alloc);
	result->Type = EntityType_EnemySkull;
	result->Model = CreateModel(alloc, skullMesh);
	result->Collider = CreateCollider(alloc, ColliderType_EnemySkull);
	result->TrailGroup = CreateTrailGroup(alloc, result, skullMesh->Parts[0].Material->DiffuseColor);
	result->SetScl(vec3(0.02f, 0.01f, 0.02f));
	return result;
}

float Interp(float c, float t, float a, float dt)
{
    if (c == t)
    {
        return t;
    }
    float dir = std::copysign(1, t - c);
    c += a * dir * dt;
    return (dir == std::copysign(1, t - c)) ? c : t;
}

#define SHIP_MIN_VEL            40.0f
#define SHIP_ACCELERATION       20.0f
#define SHIP_BREAK_ACCELERATION 30.0f
#define SHIP_STEER_SPEED        20.0f
#define SHIP_STEER_ACCELERATION 80.0f
#define SHIP_FRICTION           10.0f

void MoveShipEntity(entity *ent, float moveX, float moveY, float minVelArg, float maxVel, float dt)
{
    float minVel = SHIP_MIN_VEL;
    if (ent->Flags & EntityFlag_IsPlayer)
    {
        minVel = minVelArg;
    }
    if (moveY < 0.0f)
    {
        minVel = PLAYER_MIN_VEL_BREAKING;
    }
    if (ent->Transform.Velocity.y < minVel)
    {
        moveY = 0.1f;
    }

    ent->Transform.Velocity.y += moveY * SHIP_ACCELERATION * dt;
    if ((moveY <= 0.0f && ent->Transform.Velocity.y > 0) || ent->Transform.Velocity.y > maxVel)
    {
        ent->Transform.Velocity.y -= SHIP_FRICTION * dt;
    }

    float steerTarget = moveX * SHIP_STEER_SPEED;
    ent->Transform.Velocity.y = std::min(ent->Transform.Velocity.y, maxVel);
    ent->Transform.Velocity.x = Interp(ent->Transform.Velocity.x,
                                          steerTarget,
                                          SHIP_STEER_ACCELERATION,
                                          dt);

    ent->Transform.Rotation.y = 20.0f * (moveX / 1.0f);
    ent->Transform.Rotation.x = Interp(ent->Transform.Rotation.x,
                                          5.0f * moveY,
                                          20.0f,
                                          dt);
}

void PushPosition(trail *t, vec3 pos)
{
    if (t->PositionStackIndex >= TRAIL_COUNT)
    {
        vec3 posSave = t->Entities[TRAIL_COUNT - 1].Transform.Position;
        for (int i = TRAIL_COUNT - 1; i > 0; i--)
        {
            vec3 newPos = posSave;
            posSave = t->Entities[i - 1].Transform.Position;
            t->Entities[i - 1].Transform.Position = newPos;
        }
        t->PositionStackIndex -= 1;
    }
    t->Entities[t->PositionStackIndex++].Transform.Position = pos;
}

void SetRoadPieceBounds(road_piece *piece, float backLeft, float backRight, float frontLeft, float frontRight)
{
	piece->BackLeft = backLeft;
	piece->BackRight = backRight;
	piece->FrontLeft = frontLeft;
	piece->FrontRight = frontRight;
}

void RoadChange(entity_world &w, road_change change)
{
	w.RoadState.ShouldChange = true;
	w.RoadState.Change = change;
}

void RoadRepeatCallback(entity_world *w, entity *ent)
{
	if (w->ShouldSpawnFinish)
	{
		w->ShouldSpawnFinish = false;
		w->DisableRoadRepeat = true;
		w->OnSpawnFinish();
		return;
	}

    if (w->ShouldRoadTangent)
    {
        w->ShouldRoadTangent = false;
        w->RoadTangentPoint = w->PlayerEntity->Pos().y + GEN_PLAYER_OFFSET;
        return;
    }

	if (w->RoadState.ShouldChange)
	{
		w->RoadState.ShouldChange = false;
		switch (w->RoadState.Change)
		{
		case RoadChange_NarrowRight:
			ent->Model->Mesh = GetNarrowRightRoadMesh(w->RoadMeshManager);
			SetRoadPieceBounds(ent->RoadPiece, -2.5f, 2.5f, 0.5f, 2.5f);

			w->RoadState.RoadMesh = GetStraightRoadMesh(w->RoadMeshManager, 0.5f, 2.5f);
			SetRoadPieceBounds(&w->RoadState.NextPiece, 0.5f, 2.5f, 0.5f, 2.5f);

			w->RoadState.MinLaneIndex = 3;
			w->RoadState.MaxLaneIndex = 4;
			break;

		case RoadChange_NarrowLeft:
			ent->Model->Mesh = GetNarrowLeftRoadMesh(w->RoadMeshManager);
			SetRoadPieceBounds(ent->RoadPiece, -2.5f, 2.5f, -2.5f, -0.5f);

			w->RoadState.RoadMesh = GetStraightRoadMesh(w->RoadMeshManager, -2.5f, -0.5f);
			SetRoadPieceBounds(&w->RoadState.NextPiece, -2.5f, -0.5f, -2.5f, -0.5f);

			w->RoadState.MinLaneIndex = 0;
			w->RoadState.MaxLaneIndex = 1;
			break;

		case RoadChange_NarrowCenter:
			ent->Model->Mesh = GetNarrowCenterRoadMesh(w->RoadMeshManager);
			SetRoadPieceBounds(ent->RoadPiece, -2.5f, 2.5f, -0.5f, 0.5f);

			w->RoadState.RoadMesh = GetStraightRoadMesh(w->RoadMeshManager, -0.5f, 0.5f);
			SetRoadPieceBounds(&w->RoadState.NextPiece, -0.5f, 0.5f, -0.5f, 0.5f);

			w->RoadState.MinLaneIndex = 2;
			w->RoadState.MaxLaneIndex = 2;
			break;

		case RoadChange_WidensLeft:
			ent->Model->Mesh = GetWidensLeftRoadMesh(w->RoadMeshManager);
			SetRoadPieceBounds(ent->RoadPiece, 0.5f, 2.5f, -2.5f, 2.5f);

			w->RoadState.RoadMesh = GetStraightRoadMesh(w->RoadMeshManager, -2.5f, 2.5f);
			SetRoadPieceBounds(&w->RoadState.NextPiece, -2.5f, 2.5f, -2.5f, 2.5f);

			w->RoadState.MinLaneIndex = 0;
			w->RoadState.MaxLaneIndex = 4;
			break;

		case RoadChange_WidensRight:
			ent->Model->Mesh = GetWidensRightRoadMesh(w->RoadMeshManager);
			SetRoadPieceBounds(ent->RoadPiece, -2.5f, -0.5f, -2.5f, 2.5f);

			w->RoadState.RoadMesh = GetStraightRoadMesh(w->RoadMeshManager, -2.5f, 2.5f);
			SetRoadPieceBounds(&w->RoadState.NextPiece, -2.5f, 2.5f, -2.5f, 2.5f);

			w->RoadState.MinLaneIndex = 0;
			w->RoadState.MaxLaneIndex = 4;
			break;

		case RoadChange_WidensCenter:
			ent->Model->Mesh = GetWidensCenterRoadMesh(w->RoadMeshManager);
			SetRoadPieceBounds(ent->RoadPiece, -0.5f, 0.5f, -2.5f, 2.5f);

			w->RoadState.RoadMesh = GetStraightRoadMesh(w->RoadMeshManager, -2.5f, 2.5f);
			SetRoadPieceBounds(&w->RoadState.NextPiece, -2.5f, 2.5f, -2.5f, 2.5f);

			w->RoadState.MinLaneIndex = 0;
			w->RoadState.MaxLaneIndex = 4;
			break;
		}
	}
	else
	{
		ent->Model->Mesh = w->RoadState.RoadMesh;
		*ent->RoadPiece = w->RoadState.NextPiece;
	}
}

void InitEntityWorld(game_main *g, entity_world &world)
{
    world.ShipPool.ElemSize = SHIP_ENTITY_SIZE;
    world.CrystalPool.ElemSize = CRYSTAL_ENTITY_SIZE;
    world.PowerupPool.ElemSize = POWERUP_ENTITY_SIZE;
    world.ExplosionPool.ElemSize = EXPLOSION_ENTITY_SIZE;
    world.AsteroidPool.ElemSize = ASTEROID_ENTITY_SIZE;
    world.CheckpointPool.ElemSize = CHECKPOINT_ENTITY_SIZE;
	world.FinishPool.ElemSize = FINISH_ENTITY_SIZE;
	world.EnemySkullPool.ElemSize = ENEMY_SKULL_ENTITY_SIZE;

    world.ShipPool.Arena = &world.Arena;
    world.CrystalPool.Arena = &world.Arena;
    world.PowerupPool.Arena = &world.Arena;
    world.ExplosionPool.Arena = &world.Arena;
    world.AsteroidPool.Arena = &world.Arena;
    world.CheckpointPool.Arena = &world.Arena;
	world.FinishPool.Arena = &world.Arena;
	world.UpdateArgsPool.Arena = &world.Arena;
	world.EnemySkullPool.Arena = &world.Arena;

    world.ShipPool.Name = "ShipPool";
    world.CrystalPool.Name = "CrystalPool";
    world.PowerupPool.Name = "PowerupPool";
    world.ExplosionPool.Name = "ExplosionPool";
    world.AsteroidPool.Name = "AsteroidPool";
    world.CheckpointPool.Name = "CheckpointPool";
	world.FinishPool.Name = "FinishPool";
	world.EnemySkullPool.Name = "EnemySkullPool";

	CreateThreadPool(world.UpdateThreadPool, g, std::thread::hardware_concurrency(), 8);
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
	if (ent->Type == EntityType_EnemySkull)
	{
		AddEntityToList(world.EnemySkullEntities, ent);
	}
	if (ent->RoadPiece)
	{
		AddEntityToList(world.RoadPieceEntities, ent);
	}
	if (ent->AudioSource)
	{
		AddEntityToList(world.AudioEntities, ent);
	}
    if (ent->Model)
    {
        AddEntityToList(world.ModelEntities, ent);
    }
    if (ent->Collider)
    {
		if (ent->Collider->Active)
		{
			AddEntityToList(world.ActiveCollisionEntities, ent);
		}
		else
		{
			AddEntityToList(world.PassiveCollisionEntities, ent);
		}
    }
    if (ent->TrailGroup)
    {
        AddEntityToList(world.TrailGroupEntities, ent);
        if (!ent->TrailGroup->RenderOnly)
        {
            for (int i = 0; i < ent->TrailGroup->Count; i++)
            {
                AddEntity(world, ent->TrailGroup->Entities[i]);
            }
        }
    }
	if (ent->Trail)
	{
		for (int i = 0; i < TRAIL_COUNT; i++)
		{
			AddEntity(world, ent->Trail->Entities + i);
		}
	}
    if (ent->Explosion)
    {
        world.LastExplosion = ent->Explosion;
        AddEntityToList(world.ExplosionEntities, ent);
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
    if (ent->LaneSlot && ent->LaneSlot->Occupy)
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
    if (ent->Collider && ent->Collider->Type == ColliderType_Crystal)
    {
        AddEntityToList(world.CrystalEntities, ent);
    }
    if (ent->Checkpoint)
    {
        AddEntityToList(world.CheckpointEntities, ent);
    }
	if (ent->Finish)
	{
		AddEntityToList(world.FinishEntities, ent);
	}
    if (ent->Flags & EntityFlag_UpdateMovement)
    {
        AddEntityToList(world.MovementEntities, ent);
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
	if (ent->Type == EntityType_EnemySkull)
	{
		RemoveEntityFromList(world.EnemySkullEntities, ent);
	}
	if (ent->RoadPiece)
	{
		RemoveEntityFromList(world.RoadPieceEntities, ent);
	}
	if (ent->AudioSource)
	{
		RemoveEntityFromList(world.AudioEntities, ent);
        if (ent->Flags & EntityFlag_DestroyAudioSource)
        {
            AudioSourceDestroy(ent->AudioSource);
        }
	}
    if (ent->Model)
    {
        RemoveEntityFromList(world.ModelEntities, ent);
    }
    if (ent->Collider)
    {
		if (ent->Collider->Active)
		{
			RemoveEntityFromList(world.ActiveCollisionEntities, ent);
		}
		else
		{
			RemoveEntityFromList(world.PassiveCollisionEntities, ent);
		}
    }
    if (ent->TrailGroup)
    {
        RemoveEntityFromList(world.TrailGroupEntities, ent);
        if (!ent->TrailGroup->RenderOnly)
        {
            for (int i = 0; i < ent->TrailGroup->Count; i++)
            {
                RemoveEntity(world, ent->TrailGroup->Entities[i]);
            }
        }
    }
	if (ent->Trail)
	{
		for (int i = 0; i < TRAIL_COUNT; i++)
		{
			RemoveEntity(world, ent->Trail->Entities + i);
		}
	}
    if (ent->Explosion)
    {
        RemoveEntityFromList(world.ExplosionEntities, ent);
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
    if (ent->LaneSlot && ent->LaneSlot->Occupy)
    {
        RemoveEntityFromList(world.LaneSlotEntities, ent);
    }
    if (ent->FrameRotation)
    {
        RemoveEntityFromList(world.RotatingEntities, ent);
    }
    if (ent->Collider && ent->Collider->Type == ColliderType_Crystal)
    {
        RemoveEntityFromList(world.CrystalEntities, ent);
        FreeEntry(world.CrystalPool, ent->PoolEntry);
    }
    if (ent->Collider && ent->Collider->Type == ColliderType_Asteroid)
    {
        RemoveEntityFromList(world.AsteroidEntities, ent);
        FreeEntry(world.AsteroidPool, ent->PoolEntry);
    }
    if (ent->Checkpoint)
    {
        RemoveEntityFromList(world.CheckpointEntities, ent);
        FreeEntry(world.CheckpointPool, ent->PoolEntry);
    }
	if (ent->Finish)
	{
		RemoveEntityFromList(world.FinishEntities, ent);
		FreeEntry(world.FinishPool, ent->PoolEntry);
	}
    if (ent->Flags & EntityFlag_UpdateMovement)
    {
        RemoveEntityFromList(world.MovementEntities, ent);
    }
    world.NumEntities = std::max(0, world.NumEntities - 1);
}

void InitWorldCommonEntities(entity_world &w, asset_loader *loader, camera *cam)
{
	w.RoadMeshManager.Arena.Free = false;

    FreeArena(w.Arena);
    w.AsteroidEntities.clear();
    w.CrystalEntities.clear();
    w.CheckpointEntities.clear();
    w.ExplosionEntities.clear();
    w.LaneSlotEntities.clear();
    w.ModelEntities.clear();
    w.MovementEntities.clear();
    w.RotatingEntities.clear();
    w.RemoveOffscreenEntities.clear();
    w.RepeatingEntities.clear();
    w.ActiveCollisionEntities.clear();
	w.PassiveCollisionEntities.clear();
    w.ShipEntities.clear();
    w.TrailGroupEntities.clear();
	w.RoadPieceEntities.clear();
	w.AudioEntities.clear();
	w.PowerupEntities.clear();
	w.FinishEntities.clear();

    ResetPool(w.AsteroidPool);
    ResetPool(w.CheckpointPool);
    ResetPool(w.CrystalPool);
    ResetPool(w.ExplosionPool);
    ResetPool(w.ShipPool);
    ResetPool(w.PowerupPool);
	ResetPool(w.UpdateArgsPool);

    w.GenState = PushStruct<gen_state>(w.Arena);
    InitGenState(w.GenState);

    w.AssetLoader = loader;
    w.Camera = cam;
    w.PlayerEntity = CreateShipEntity(&w.Arena, GetSphereMesh(w), PLAYER_BODY_COLOR, PLAYER_OUTLINE_COLOR, NULL, true);
    w.PlayerEntity->Transform.Position.z = SHIP_Z;
    w.PlayerEntity->Transform.Velocity.y = PLAYER_MIN_VEL;
	w.PlayerEntity->AudioSource = CreateAudioSource(&w.Arena);
    AddEntity(w, w.PlayerEntity);

    cam->Position = w.PlayerEntity->Pos() + vec3{0, Global_Camera_OffsetY, Global_Camera_OffsetZ};

	InitFormat(&w.RoadMeshManager.KeyFormat, "%s%f%f", 32, &w.RoadMeshManager.Arena);
	w.RoadState.RoadMesh = GetStraightRoadMesh(w.RoadMeshManager, -2.5f, 2.5f);

    for (int i = 0; i < ROAD_SEGMENT_COUNT; i++)
    {
        auto ent = PushStruct<entity>(w.Arena);
        ent->Pos().y = i*ROAD_SEGMENT_SIZE;
        ent->Pos().z = 0.0f;
        ent->Transform.Scale = vec3{ 2, 1, 1 };
		ent->Model = CreateModel(&w.Arena, w.RoadState.RoadMesh);
        ent->Repeat = PushStruct<entity_repeat>(&w.Arena);
        ent->Repeat->Count = ROAD_SEGMENT_COUNT;
        ent->Repeat->Size = ROAD_SEGMENT_SIZE;
        ent->Repeat->DistanceFromCamera = ROAD_SEGMENT_SIZE;
		ent->Repeat->Callback = RoadRepeatCallback;
		ent->RoadPiece = PushStruct<road_piece>(w.Arena);
		SetRoadPieceBounds(ent->RoadPiece, -2.5f, 2.5f, -2.5f, 2.5f);
        AddEntity(w, ent);
    }
	SetRoadPieceBounds(&w.RoadState.NextPiece, -2.5f, 2.5f, -2.5f, 2.5f);

	w.DisableRoadRepeat = false;

    w.BackgroundState.RandomTexture = FindTexture(*loader, "random");
	w.BackgroundState.Instances.push_back(background_instance{ color(0, 1, 1, 1), vec2(0.0f) });
	w.BackgroundState.Instances.push_back(background_instance{ color(1, 0, 1, 1), vec2(1060, 476) });
}

void SetEntityClip(entity_world &world, gen_type genType, audio_clip *clip)
{
	auto gen = world.GenState->GenParams + genType;
	gen->Clip = clip;
}

void ResetRoadPieces(entity_world &world, float baseY)
{
    world.DisableRoadRepeat = false;
    for (int i = 0; i < ROAD_SEGMENT_COUNT; i++)
    {
        auto ent = world.RoadPieceEntities[i];
        ent->Pos().y = baseY + i*ROAD_SEGMENT_SIZE;
    }
}

entity_update_args *GetUpdateArg(entity_world &world, entity *ent, float dt)
{
	auto entry = GetEntry(world.UpdateArgsPool);
	auto result = PushStruct<entity_update_args>(entry);
	result->World = &world;
	result->Entity = ent;
	result->DeltaTime = dt;
	result->Entry = entry;
	return result;
}

#define WORLD_ARC_RADIUS   500.0f
#define WORLD_ARC_Y_FACTOR 0.005f
vec3 WorldToRenderTransformInner(const vec3 &worldPos, const float radius)
{
	float r = (radius - worldPos.z);
	float d = worldPos.y*WORLD_ARC_Y_FACTOR;
	vec3 result = worldPos;
	result.y = std::cos(d) * r;
	result.z = std::sin(d) * r;
	return result;
}

vec3 WorldToRenderTransform(entity_world &w, const vec3 &worldPos, const float radius)
{
    vec3 tangentWorldPoint = vec3{ worldPos.x, w.RoadTangentPoint, worldPos.z };
	vec3 result = WorldToRenderTransformInner(worldPos, radius);
	vec3 rtdir = glm::normalize(vec3{ 0, -result.z, result.y });

	if (worldPos.y >= tangentWorldPoint.y)
	{
		vec3 tangentPoint = WorldToRenderTransformInner(tangentWorldPoint, radius);
		vec3 tangentDir = glm::normalize(vec3{ 0, -tangentPoint.z, tangentPoint.y });
		result = tangentPoint + (tangentDir * ((worldPos.y - tangentWorldPoint.y) * (radius * WORLD_ARC_Y_FACTOR)));
	}

    return result;
}

static void SetSingleTrailPosition(entity *ent)
{
	ent->TrailGroup->Entities[0]->SetPos(ent->Pos());
}

#define ENTITY_JOB_UNPACK_ARGS(arg) \
	auto args = (entity_update_args *)arg; \
	auto &world = *args->World; \
	auto ent = args->Entity; \
	float dt = args->DeltaTime

#define ENTITY_JOB_FREE_ARGS(arg) FreeEntry(world.UpdateArgsPool, args->Entry)

static void UpdateTrailGroupEntityJob(void *arg)
{
	ENTITY_JOB_UNPACK_ARGS(arg);

	if (!ent->Explosion)
	{
		SetSingleTrailPosition(ent);
	}

	auto tg = ent->TrailGroup;
	if (tg->FirstFrame)
	{
		tg->FirstFrame = false;
		for (int j = 0; j < tg->Count; j++)
		{
			auto tr = tg->Entities[j]->Trail;
			for (int i = 0; i < TRAIL_COUNT; i++)
			{
				tr->Entities[i].Transform.Position = ent->Transform.Position;
			}
		}
	}

	BeginProfileTimer("Trail push position");

	tg->Timer -= dt;
	if (tg->Timer <= 0)
	{
		tg->Timer = Global_Game_TrailRecordTimer;
		for (int i = 0; i < tg->Count; i++)
		{
			PushPosition(tg->Entities[i]->Trail, tg->Entities[i]->Pos());
		}
	}
	EndProfileTimer("Trail push position");

	auto &m = tg->Mesh;
	float *bufferData = MapBuffer(m.Buffer, GL_WRITE_ONLY);

	BeginProfileTimer("Trail build quads");
	for (int j = 0; j < tg->Count; j++)
	{
		// First store the quads, then the lines
		for (int i = 0; i < TRAIL_COUNT; i++)
		{
			auto tr = tg->Entities[j]->Trail;
			auto pieceEntity = tr->Entities + i;
			vec3 c1 = pieceEntity->Transform.Position;
			vec3 c2;
			if (i == TRAIL_COUNT - 1)
			{
				c2 = ent->Transform.Position;
			}
			else
			{
				c2 = tr->Entities[i + 1].Transform.Position;
			}

			float currentTrailTime = tr->Timer / Global_Game_TrailRecordTimer;
			if (i == 0)
			{
				c1 -= (c2 - c1) * currentTrailTime;
			}

			float r = tr->Radius;
			float rm = tr->Radius * 0.8f;
			float min2 = rm * ((TRAIL_COUNT - i) / (float)TRAIL_COUNT);
			float min1 = rm * ((TRAIL_COUNT - i + 1) / (float)TRAIL_COUNT);
			vec3 p1 = c2 - vec3(r - min2, 0, 0);
			vec3 p2 = c2 + vec3(r - min2, 0, 0);
			vec3 p3 = c1 - vec3(r - min1, 0, 0);
			vec3 p4 = c1 + vec3(r - min1, 0, 0);
			color cl2 = color{ 1, 1, 1, 1.0f - (min2 / tr->Radius) };
			color cl1 = color{ 1, 1, 1, 1.0f - (min1 / tr->Radius) };
			bufferData = AddQuad(bufferData, p2, p1, p3, p4, cl2, cl2, cl1, cl1);

			const float lo = 0.05f;
			tg->PointCache[j][i * 4] = p1 - vec3(lo, 0, 0);
			tg->PointCache[j][i * 4 + 1] = p3 - vec3(lo, 0, 0);
			tg->PointCache[j][i * 4 + 2] = p2 + vec3(lo, 0, 0);
			tg->PointCache[j][i * 4 + 3] = p4 + vec3(lo, 0, 0);
			if (pieceEntity->Collider)
			{
				auto &box = pieceEntity->Collider->Box;
				box.Half = vec3(r, (c2.y - c1.y) * 0.5f, 0.5f);
				box.Center = vec3(c1.x, c1.y + box.Half.y, c1.z + box.Half.z);
			}
		}
	}
	EndProfileTimer("Trail build quads");

	BeginProfileTimer("Trail build lines");
	for (int j = 0; j < tg->Count; j++)
	{
		for (int i = 0; i < TRAIL_COUNT; i++)
		{
			vec3 *p = tg->PointCache[j] + i * 4;
			bufferData = AddLine(bufferData, *p++, *p++);
		}
		for (int i = 0; i < TRAIL_COUNT; i++)
		{
			vec3 *p = tg->PointCache[j] + i * 4 + 2;
			bufferData = AddLine(bufferData, *p++, *p++);
		}
	}
	EndProfileTimer("Trail build lines");

	ENTITY_JOB_FREE_ARGS(args);
}

void UpdateExplosionEntityJob(void *arg)
{
	ENTITY_JOB_UNPACK_ARGS(arg);

	auto exp = ent->Explosion;
	if (exp->LifeTime == Global_Game_ExplosionLifeTime)
	{
		AudioSourceSetPitch(ent->AudioSource, LaneIndexToPitch(ent->LaneSlot->Index));
		AudioSourcePlay(ent->AudioSource);
	}

	exp->LifeTime -= dt;
	if (exp->LifeTime <= 0)
	{
		RemoveEntity(world, ent);
		return;
	}

	float alpha = exp->LifeTime / Global_Game_ExplosionLifeTime;
	for (int i = 0; i < EXPLOSION_PARTS_COUNT; i++)
	{
		auto partEnt = ent->TrailGroup->Entities[i];
		partEnt->Transform.Position += partEnt->Transform.Velocity * dt;
	}

	for (auto &meshPart : ent->TrailGroup->Mesh.Parts)
	{
		meshPart.Material->DiffuseColor.a = alpha;
	}

	ENTITY_JOB_FREE_ARGS(args);
}

static bool IsInsideRoadPiece(entity *roadEntity, entity *playerEntity)
{
	return (
		roadEntity->Pos().y < playerEntity->Pos().y &&
		true//roadEntity->Pos().y + ROAD_SEGMENT_SIZE > playerEntity->Pos().y
	);
}

void UpdateLogiclessEntities(entity_world &world, float dt)
{
    for (int i = 0; i < ROAD_LANE_COUNT; i++)
    {
        world.GenState->LaneSlots[i] = 0;
    }
    for (auto ent : world.LaneSlotEntities)
    {
        if (!ent) continue;

        world.GenState->LaneSlots[ent->LaneSlot->Index]++;
    }

    if (world.PlayerEntity->Pos().y < world.RoadTangentPoint)
    {
        float velY = world.PlayerEntity->Vel().y;
        world.BackgroundState.Offset += vec2(0.0f, velY) * dt;
    }
	world.BackgroundState.Time += dt;

	for (auto ent : world.AudioEntities)
	{
		if (!ent) continue;
		AudioSourceSetPosition(ent->AudioSource, ent->Pos());
	}
    for (auto ent : world.MovementEntities)
    {
        if (!ent) continue;
        ent->Transform.Position += ent->Transform.Velocity * dt;
    }
    for (auto ent : world.RemoveOffscreenEntities)
    {
        if (!ent) continue;

        if (ent->Pos().y < world.Camera->Position.y - 50.0f)
        {
            RemoveEntity(world, ent);
        }
    }
    for (auto ent : world.RepeatingEntities)
    {
		if (!ent) continue;

        auto repeat = ent->Repeat;
		if (world.Camera->Position.y - ent->Transform.Position.y > repeat->DistanceFromCamera)
		{
			if (!(ent->RoadPiece && world.DisableRoadRepeat))
			{
				ent->Transform.Position.y += repeat->Size * repeat->Count;
			}
			if (repeat->Callback)
			{
				repeat->Callback(&world, ent);
			}
		}

    }
    for (auto ent : world.RotatingEntities)
    {
        if (!ent) continue;

        ent->Transform.Rotation += ent->FrameRotation->Rotation * dt;
    }

	for (auto ent : world.TrailGroupEntities)
	{
		if (!ent) continue;
		//AddJob(world.UpdateThreadPool, UpdateTrailGroupEntityJob, (void *)GetUpdateArg(world, ent, dt));
		UpdateTrailGroupEntityJob((void *)GetUpdateArg(world, ent, dt));
	}

	for (auto ent : world.ExplosionEntities)
	{
		if (!ent) continue;
		//AddJob(world.UpdateThreadPool, UpdateExplosionEntityJob, (void *)GetUpdateArg(world, ent, dt));
		UpdateExplosionEntityJob((void *)GetUpdateArg(world, ent, dt));
	}

	auto roadEntity = world.RoadPieceEntities[world.RoadState.PlayerActiveEntityIndex];
	while (!IsInsideRoadPiece(roadEntity, world.PlayerEntity))
	{
		if (world.RoadState.PlayerActiveEntityIndex >= world.RoadPieceEntities.size() - 1)
		{
			world.RoadState.PlayerActiveEntityIndex = -1;
		}
		roadEntity = world.RoadPieceEntities[++world.RoadState.PlayerActiveEntityIndex];
	}

	float d = std::min(1.0f, (world.PlayerEntity->Pos().y - roadEntity->Pos().y) / ROAD_SEGMENT_SIZE);
	world.RoadState.Left = Lerp(roadEntity->RoadPiece->BackLeft, d, roadEntity->RoadPiece->FrontLeft);
	world.RoadState.Right = Lerp(roadEntity->RoadPiece->BackRight, d, roadEntity->RoadPiece->FrontRight);
}

void WaitUpdate(entity_world &world)
{
	while (world.UpdateThreadPool.NumJobs > 0) {}
}

void RenderBackground(game_main *g, entity_world &w)
{
    auto &state = w.BackgroundState;
    static auto tex = FindTexture(*w.AssetLoader, "background");
    int numHorizontal = int(std::ceil(float(g->Width) / float(tex->Width)));

    UpdateProjectionView(g->GUICamera);
    Begin(g->GUI, g->GUICamera, 0.0f);

    for (auto &bg : state.Instances)
    {
        for (int i = 0; i < numHorizontal; i++)
        {
            //DrawTexture(g->GUI, rect{i*float(tex->Width),bg.y,float(tex->Width),float(tex->Height)}, tex);
        }
    }
    End(g->GUI);
}

void RenderEntityWorld(render_state &rs, entity_world &world, float dt)
{
	RenderBackground(rs, world.BackgroundState);
    for (auto ent : world.TrailGroupEntities)
    {
        if (!ent) continue;
        auto tg = ent->TrailGroup;
        UnmapBuffer(tg->Mesh.Buffer);
        DrawModel(rs, tg->Model, transform{});
    }
    for (auto ent : world.ModelEntities)
    {
        if (!ent) continue;
        if (ent->Model->Visible)
        {
            DrawModel(rs, *ent->Model, ent->Transform);
        }
    }
    if (world.LastExplosion)
    {
        ApplyExplosionLight(rs, world.LastExplosion->Color);
    }
    world.LastExplosion = NULL;
}
