// Copyright

struct audio_source
{
    ALuint Source;
    float Gain = 1;
};
static audio_source *CreateAudioSource(memory_arena &arena, ALuint buffer)
{
    auto result = PushStruct<audio_source>(arena);
    alGenSources(1, &result->Source);
    alSourcef(result->Source, AL_PITCH, 1);
    alSourcef(result->Source, AL_GAIN, 1);
    alSource3f(result->Source, AL_POSITION, 0,0,0);
    alSource3f(result->Source, AL_VELOCITY, 0,0,0);
    alSourcei(result->Source, AL_LOOPING, AL_FALSE);
    alSourcei(result->Source, AL_BUFFER, buffer);
    return result;
}

static void UpdateAudioParams(audio_source *audio)
{
    alSourcef(audio->Source, AL_GAIN, audio->Gain);
}

#define ROAD_LANE_WIDTH   2
#define LEVEL_PLANE_COUNT 5
#define SHIP_Z            0.2f
static void InitLevel(game_state &g)
{
    g.Mode = GameMode_Level;

    auto &l = g.LevelMode;
    FreeArena(l.Arena);
    l.Entropy = RandomSeed(g.Platform.GetMilliseconds());

    g.RenderState.FogColor = IntColor(FirstPalette.Colors[3]) * 0.5f;
    g.RenderState.FogColor.a = 1.0f;
    ApplyExplosionLight(g.RenderState, IntColor(FirstPalette.Colors[3]));

    g.Gravity = vec3(0, 0, 0);
    g.World.Camera = &g.Camera;

    g.PlayerEntity = CreateShipEntity(&g.World.Arena, GetShipMesh(g), Color_blue, IntColor(FirstPalette.Colors[1]), true);
    g.PlayerEntity->Transform.Position.z = SHIP_Z;
    g.PlayerEntity->Transform.Velocity.y = PLAYER_MIN_VEL;
    AddEntity(g.World, g.PlayerEntity);

    for (int i = 0; i < LEVEL_PLANE_COUNT; i++)
    {
        auto ent = PushStruct<entity>(g.World.Arena);
        ent->Transform.Position.y = LEVEL_PLANE_SIZE * i;
        ent->Transform.Position.z = -0.25f;
        ent->Transform.Scale = vec3{LEVEL_PLANE_SIZE, LEVEL_PLANE_SIZE, 0};
        ent->Model = CreateModel(&g.World.Arena, GetFloorMesh(g));
        ent->Repeat = PushStruct<entity_repeat>(g.World.Arena);
        ent->Repeat->Count = LEVEL_PLANE_COUNT;
        ent->Repeat->Size = LEVEL_PLANE_SIZE;
        ent->Repeat->DistanceFromCamera = LEVEL_PLANE_SIZE/2;
        AddEntity(g.World, ent);
    }
    for (int i = 0; i < LEVEL_PLANE_COUNT; i++)
    {
        auto ent = PushStruct<entity>(g.World.Arena);
        ent->Transform.Position.y = LEVEL_PLANE_SIZE * i;
        ent->Transform.Position.z = 0.0f;
        ent->Transform.Scale = vec3{ 2, LEVEL_PLANE_SIZE, 1 };
        ent->Model = CreateModel(&g.World.Arena, GetRoadMesh(g));
        ent->Repeat = PushStruct<entity_repeat>(g.World.Arena);
        ent->Repeat->Count = LEVEL_PLANE_COUNT;
        ent->Repeat->Size = LEVEL_PLANE_SIZE;
        ent->Repeat->DistanceFromCamera = LEVEL_PLANE_SIZE / 2;
        AddEntity(g.World, ent);
    }

    g.TestFont = FindBitmapFont(g.AssetLoader, "g_type_16");
    l.DraftBoostAudio = CreateAudioSource(g.Arena, FindSound(g.AssetLoader, "boost")->Buffer);

    InitFormat(l.ScoreFormat, "Score: %d\n", 24, &l.Arena);
}

static void
DebugReset(game_state &g)
{
}

struct find_entity_result
{
    entity *Found = NULL;
    entity *Other = NULL;
};
static find_entity_result FindEntityOfType(entity *first, entity *second, collider_type type)
{
    find_entity_result result;
    if (first->Collider->Type == type)
    {
        result.Found = first;
        result.Other = second;
    }
    else if (second->Collider->Type == type)
    {
        result.Found = second;
        result.Other = first;
    }
    return result;
}

#define CrystalBoost      10.0f
#define DraftBoost        40.0f
#define CrystalBoostScore 10
#define DraftBoostScore   100
inline static void ApplyBoostToShip(entity *ent, float Boost, float Max)
{
    ent->Transform.Velocity.y += Boost;
}

#define SCORE_DESTROY_BLUE_SHIP 10
#define SCORE_MISS_ORANGE_SHIP  10
#define SCORE_DRAFT             5
#define SCORE_CRYSTAL           2

#define SHIP_IS_BLUE(s)   (s->ColorIndex == 0)
#define SHIP_IS_ORANGE(s) (s->ColorIndex == 1)
#define SHIP_IS_RED(s)    (s->ColorIndex == 2)
static bool HandleCollision(game_state &g, entity *first, entity *second, float dt)
{
    auto shipEntity = FindEntityOfType(first, second, ColliderType_Ship).Found;
    auto crystalEntity = FindEntityOfType(first, second, ColliderType_Crystal).Found;
    if (first->Collider->Type == ColliderType_TrailPiece || second->Collider->Type == ColliderType_TrailPiece)
    {
        auto trailPieceEntity = FindEntityOfType(first, second, ColliderType_TrailPiece).Found;
        if (shipEntity && shipEntity != trailPieceEntity->TrailPiece->Owner)
        {
            auto s = shipEntity->Ship;
            if (s && s->NumTrailCollisions == 0 && (!s->DraftActive || s->DraftTarget != trailPieceEntity->TrailPiece->Owner))
            {
                s->CurrentDraftTime += dt;
                s->NumTrailCollisions++;
                s->DraftTarget = trailPieceEntity->TrailPiece->Owner;
                s->DraftActive = false;
            }
        }
        return false;
    }
    if (first->Collider->Type == ColliderType_Ship && second->Collider->Type == ColliderType_Ship)
    {
        vec3 rv = first->Transform.Velocity - second->Transform.Velocity;
        entity *entityToExplode = NULL;
        entity *otherEntity = NULL;
        if (rv.y < 0.0f)
        {
            entityToExplode = first;
            otherEntity = second;
        }
        else
        {
            entityToExplode = second;
            otherEntity = first;
        }
        if ((otherEntity->Ship->DraftActive && otherEntity->Ship->DraftTarget == entityToExplode) ||
            (SHIP_IS_RED(otherEntity->Ship) || SHIP_IS_RED(entityToExplode->Ship)))
        {
            otherEntity->Ship->DraftActive = false;
            if (entityToExplode->Flags & EntityFlag_IsPlayer)
            {
                entityToExplode = otherEntity;
            }
            else if (SHIP_IS_BLUE(entityToExplode->Ship))
            {
                g.LevelMode.Score += SCORE_DESTROY_BLUE_SHIP;
                entityToExplode->Ship->Scored = true;
            }

            auto exp = CreateExplosionEntity(
                GetEntry(g.World.ExplosionPool),
                *entityToExplode->Model->Mesh,
                entityToExplode->Model->Mesh->Parts[0],
                entityToExplode->Transform.Position,
                otherEntity->Transform.Velocity,
                entityToExplode->Transform.Scale,
                entityToExplode->Ship->Color,
                entityToExplode->Ship->OutlineColor,
                vec3{ 0, 1, 1 }
            );
            AddEntity(g.World, exp);
            RemoveEntity(g.World, entityToExplode);
            return false;
        }
        else
        {
            if (otherEntity->Flags & EntityFlag_IsPlayer)
            {
                otherEntity->Vel().y -= 20.0f;
            }
        }
        return false;
    }
    if (shipEntity != NULL && crystalEntity != NULL)
    {
        if (shipEntity->Flags & EntityFlag_IsPlayer)
        {
            g.LevelMode.Score += SCORE_CRYSTAL;

            auto pup = CreatePowerupEntity(GetEntry(g.World.PowerupPool), g.LevelMode.Entropy, g.LevelMode.TimeElapsed, crystalEntity->Pos(), shipEntity->Vel(), CRYSTAL_COLOR);
            AddEntity(g.World, pup);
            RemoveEntity(g.World, crystalEntity);
        }
        return false;
    }
    return false;
}

static void UpdateListener(camera &cam, vec3 PlayerPosition)
{
    static float *orient = new float[6];
    orient[0] = cam.View[2][0];
    orient[1] = cam.View[2][1];
    orient[2] = cam.View[2][2];
    orient[3] = cam.View[0][0];
    orient[4] = cam.View[0][1];
    orient[5] = cam.View[0][2];
    alListenerfv(AL_ORIENTATION, orient);
    alListenerfv(AL_POSITION, &cam.Position[0]);
    alListener3f(AL_VELOCITY, 0, 0, 0);
}

static void UpdateFreeCam(camera &cam, game_input &input, float dt)
{
    static float pitch;
    static float yaw;
    const float speed = 50.0f;
    float axisValue = GetAxisValue(input, Action_camVertical);
    vec3 camDir = glm::normalize(cam.LookAt - cam.Position);

    cam.Position += camDir * axisValue * speed * dt;

    if (input.MouseState.Buttons & MouseButton_Middle)
    {
        yaw += input.MouseState.dX * dt;
        pitch -= input.MouseState.dY * dt;
        pitch = glm::clamp(pitch, -1.5f, 1.5f);
    }

    camDir.x = sin(yaw);
    camDir.y = cos(yaw);
    camDir.z = pitch;
    cam.LookAt = cam.Position + camDir * 50.0f;

    float strafeYaw = yaw + (M_PI / 2);
    float hAxisValue = GetAxisValue(input, Action_camHorizontal);
    cam.Position += vec3(sin(strafeYaw), cos(strafeYaw), 0) * hAxisValue * speed * dt;
}

static int GetNextSpawnLane(level_mode &l, bool isShip = false)
{
    static int lastLane = 0;
    static int lastShipLane = 0;
    int lane = lastLane;
    while (lane == lastLane || (isShip && lane == lastShipLane) || (lane == l.RedShipReservedLane))
    {
        lane = RandomBetween(l.Entropy, -2, 2);
    }
    lastLane = lane;
    if (isShip)
    {
        lastShipLane = lane;
    }
    return lane;
}

static int GetNextShipColor(level_mode &l)
{
    static int lastColor = 0;
    int color = lastColor;
    while (color == lastColor)
    {
        color = RandomBetween(l.Entropy, 0, 1);
    }
    lastColor = color;
    return color;
}

static float GetNextTimer(random_series series, uint64 flags, uint8 randFlag, float baseInterval, float offset)
{
    if (flags & randFlag)
    {
        return baseInterval + RandomBetween(series, -offset, offset);
    }
    return baseInterval;
}

static float DecreaseInterval(float interval, float playerVelY)
{
    float res = interval;
    res -= 0.5f;
    res = std::max(1.0f, res);
    res -= 0.0025f * playerVelY;
    res = std::max(0.05f, res);
    return res;
}

#define CRYSTAL_INTERVAL_OFFSET 1.5f
static void GenerateCrystals(game_state &g, level_mode &l, float dt)
{
    if (l.NextCrystalTimer <= 0)
    {
        int lane = GetNextSpawnLane(l);
        auto ent = CreateCrystalEntity(GetEntry(g.World.CrystalPool), GetCrystalMesh(g));
        ent->Pos().x = lane * ROAD_LANE_WIDTH;
        ent->Pos().y = g.PlayerEntity->Pos().y + 200;
        ent->Pos().z = SHIP_Z + 0.4f;
        ent->Scl().x = 0.3f;
        ent->Scl().y = 0.3f;
        ent->Scl().z = 0.5f;
        ent->Vel().y = PLAYER_MIN_VEL * 0.5f;
        AddEntity(g.World, ent);

        l.NextCrystalTimer = GetNextTimer(l.Entropy, l.RandFlags, LevelFlag_Crystals, BASE_CRYSTAL_INTERVAL, CRYSTAL_INTERVAL_OFFSET);
    }

    l.NextCrystalTimer -= dt;
}

static void GenerateShips(game_state &g, level_mode &l, float dt)
{
    // TODO: remove debug key
    if (l.NextShipTimer <= 0 || g.Input.Keys[SDL_SCANCODE_S])
    {
        if (l.ChangeShipTimer <= 0)
        {
            l.ChangeShipTimer = CHANGE_SHIP_TIMER;
            l.NextShipInterval = DecreaseInterval(l.NextShipInterval, g.PlayerEntity->Vel().y);
        }

        int colorIndex = GetNextShipColor(l);
        color c = IntColor(ShipPalette.Colors[colorIndex]);
        int lane = GetNextSpawnLane(l, true);
        auto ent = CreateShipEntity(GetEntry(g.World.ShipPool), GetShipMesh(g), c, c, false);
        ent->LaneSlot = CreateLaneSlot(ent->PoolEntry, lane);
        ent->Pos().x = lane * ROAD_LANE_WIDTH;
        ent->Pos().y = g.PlayerEntity->Pos().y + 200;
        ent->Pos().z = SHIP_Z;
        ent->Ship->ColorIndex = colorIndex;
        AddFlags(ent, EntityFlag_RemoveOffscreen);
        AddEntity(g.World, ent);

        l.NextShipTimer = GetNextTimer(l.Entropy, l.RandFlags, LevelFlag_Ships, l.NextShipInterval, 1.0f);
    }

    l.NextShipTimer -= dt;
    if (l.IncFlags & LevelFlag_Ships)
    {
        l.ChangeShipTimer -= dt;
    }
}

#define RED_SHIP_INTERVAL_OFFSET 1.0f
static void GenerateRedShips(game_state &g, level_mode &l, float dt)
{
    if (l.NextRedShipTimer <= 0)
    {
        if (l.ChangeRedShipTimer <= 0)
        {
            l.ChangeRedShipTimer = CHANGE_SHIP_TIMER;
            l.NextRedShipInterval = DecreaseInterval(l.NextRedShipInterval, g.PlayerEntity->Vel().y);
        }

        color c = IntColor(ShipPalette.Colors[2]);
        auto ent = CreateShipEntity(GetEntry(g.World.ShipPool), GetShipMesh(g), c, c, false);
        int lane = l.RedShipReservedLane;
        ent->Pos().x = lane * ROAD_LANE_WIDTH;
        ent->Pos().y = g.PlayerEntity->Pos().y + 200;
        ent->Pos().z = SHIP_Z;
        ent->Ship->ColorIndex = 2;
        AddFlags(ent, EntityFlag_RemoveOffscreen);
        AddEntity(g.World, ent);

        l.RedShipReservedLane = NO_RESERVED_LANE;
        l.NextRedShipTimer = GetNextTimer(l.Entropy, l.RandFlags, LevelFlag_RedShips, l.NextRedShipInterval, RED_SHIP_INTERVAL_OFFSET);
    }
    if (l.NextRedShipTimer <= l.NextRedShipInterval*0.5f && l.RedShipReservedLane == NO_RESERVED_LANE)
    {
        const int maxTries = 5;
        int i = 0;
        int laneIndex = l.PlayerLaneIndex;

        // get the first non-occupied lane, or give up for this frame
        while (l.LaneSlots[laneIndex] > 0 && i < maxTries)
        {
            laneIndex = RandomBetween(l.Entropy, 0, 4);
            i++;
        }
        if (i >= maxTries)
        {
            l.NextRedShipTimer = GetNextTimer(l.Entropy, l.RandFlags, LevelFlag_RedShips, l.NextRedShipInterval, RED_SHIP_INTERVAL_OFFSET);
        }
        else
        {
            l.RedShipReservedLane = laneIndex - 2;
        }
    }

    l.NextRedShipTimer -= dt;
    if (l.IncFlags & LevelFlag_RedShips)
    {
        l.ChangeRedShipTimer -= dt;
    }
}

static void UpdateShipDraftCharge(ship *s, float dt)
{
    if (s->NumTrailCollisions == 0)
    {
        s->CurrentDraftTime -= dt;
    }
    s->CurrentDraftTime = std::max(0.0f, std::min(s->CurrentDraftTime, Global_Game_DraftChargeTime));
    s->DraftCharge = s->CurrentDraftTime / Global_Game_DraftChargeTime;
    s->NumTrailCollisions = 0;
}

static void ShipEntityPerformDraft(entity *ent)
{
    ent->Ship->CurrentDraftTime = 0.0f;
    ent->Ship->DraftActive = true;
    ApplyBoostToShip(ent, DraftBoost, 0);
}

static void KeepEntityInsideOfRoad(entity *ent)
{
    const float limit = ROAD_LANE_COUNT * ROAD_LANE_WIDTH / 2;
    ent->Pos().x = glm::clamp(ent->Pos().x, -limit + 0.5f, limit - 0.5f);
}

inline static void AddFlags(uint64 &flags, uint64 f)
{
    flags |= f;
}

inline static void RemoveFlags(uint64 &flags, uint64 f)
{
    flags = flags &~ f;
}

static void ResetShipsGen(level_mode &l)
{
    l.NextShipInterval = INITIAL_SHIP_INTERVAL;
    l.NextShipTimer = INITIAL_SHIP_INTERVAL;
    l.ChangeShipTimer = CHANGE_SHIP_TIMER;
}

static void ResetRedShipsGen(level_mode &l)
{
    l.RedShipReservedLane = NO_RESERVED_LANE;
    l.NextRedShipTimer = INITIAL_SHIP_INTERVAL;
    l.NextRedShipInterval = INITIAL_SHIP_INTERVAL;
    l.ChangeRedShipTimer = CHANGE_SHIP_TIMER;
}

#define PLAYER_MAX_VEL_INCREASE_FACTOR 0.5f
void RenderLevel(game_state &g, float dt)
{
    auto &l = g.LevelMode;
    auto &updateTime = g.UpdateTime;
    auto &renderTime = g.RenderTime;

    auto &world = g.World;
    auto &input = g.Input;
    auto &cam = g.Camera;
    auto *playerEntity = g.PlayerEntity;
    auto *playerShip = playerEntity->Ship;
    dt *= Global_Game_TimeSpeed;

    l.PlayerMaxVel += PLAYER_MAX_VEL_INCREASE_FACTOR * dt;
    l.TimeElapsed += dt;
    updateTime.Begin = g.Platform.GetMilliseconds();

    static bool paused = false;
    if (IsJustPressed(g, Action_debugPause))
    {
        paused = !paused;
    }
    if (paused) return;

    if (g.Input.Keys[SDL_SCANCODE_R])
    {
        DebugReset(g);
    }
    if (Global_Camera_FreeCam)
    {
        UpdateFreeCam(cam, input, dt);
    }

    if (g.Input.Keys[SDL_SCANCODE_E])
    {
        auto exp = CreateExplosionEntity(GetEntry(g.World.ExplosionPool),
                                         *g.PlayerEntity->Model->Mesh,
                                         g.PlayerEntity->Model->Mesh->Parts[0],
                                         g.PlayerEntity->Transform.Position,
                                         g.PlayerEntity->Transform.Velocity,
                                         g.PlayerEntity->Transform.Scale,
                                         g.PlayerEntity->Ship->Color,
                                         g.PlayerEntity->Ship->OutlineColor,
                                         vec3{ 0, 1, 1 });
        AddEntity(world, exp);
    }

    // we have a different state each 10 seconds elapsed
    int state = int(std::floor(l.TimeElapsed / 10.0f));
    switch (state)
    {
    case 0:
        AddFlags(l.GenFlags, LevelFlag_Crystals);
        AddFlags(l.RandFlags, LevelFlag_Crystals);
        break;

    case 1:
        AddFlags(l.GenFlags, LevelFlag_Ships);
        AddFlags(l.IncFlags, LevelFlag_Ships);
        break;

    case 6:
        RemoveFlags(l.GenFlags, LevelFlag_Ships);
        RemoveFlags(l.IncFlags, LevelFlag_Ships);
        ResetShipsGen(l);
        AddFlags(l.GenFlags, LevelFlag_RedShips);
        AddFlags(l.IncFlags, LevelFlag_RedShips);
        break;

    case 8:
        AddFlags(l.GenFlags, LevelFlag_Ships);
        AddFlags(l.IncFlags, LevelFlag_Ships);
        break;
    }
    if (l.GenFlags & LevelFlag_Crystals)
    {
        GenerateCrystals(g, l, dt);
    }
    if (l.GenFlags & LevelFlag_Ships)
    {
        GenerateShips(g, l, dt);
    }
    if (l.GenFlags & LevelFlag_RedShips)
    {
        GenerateRedShips(g, l, dt);
    }

    {
        // player movement
        float moveX = GetAxisValue(input, Action_horizontal);
        float moveY = GetAxisValue(input, Action_vertical);
        if (moveY > 0.0f)
        {
            moveY = 0.0f;
        }
        MoveShipEntity(playerEntity, moveX, moveY, l.PlayerMaxVel, dt);
        //KeepEntityInsideOfRoad(playerEntity);

        float &playerX = playerEntity->Pos().x;
        float nearestLane = std::floor(std::ceil(playerX)/ROAD_LANE_WIDTH);
        nearestLane = glm::clamp(nearestLane, -2.0f, 2.0f);
        float targetX = nearestLane * ROAD_LANE_WIDTH;
        if (moveX == 0.0f)
        {
            float dif = targetX - playerX;
            if (std::abs(dif) > 0.05f)
            {
                playerEntity->Transform.Velocity.x = dif*4.0f;
            }
        }

        if (playerShip->DraftCharge == 1.0f && IsPressed(g, Action_boost))
        {
            auto draftTarget = playerEntity->Ship->DraftTarget;
            draftTarget->Ship->HasBeenDrafted = true;
            l.Score += SCORE_DRAFT;
            ShipEntityPerformDraft(playerEntity);
        }

        l.PlayerLaneIndex = int(nearestLane)+2;
    }

    size_t FrameCollisionCount = 0;
    DetectCollisions(world.CollisionEntities, l.CollisionCache, FrameCollisionCount);
    for (size_t i = 0; i < FrameCollisionCount; i++)
    {
        auto &Col = l.CollisionCache[i];
        Col.First->NumCollisions++;
        Col.Second->NumCollisions++;

        if (HandleCollision(g, Col.First, Col.Second, dt))
        {
            ResolveCollision(Col);
        }
    }
    Integrate(world.CollisionEntities, g.Gravity, dt);
    UpdateShipDraftCharge(playerShip, dt);
    if (!Global_Camera_FreeCam)
    {
        const float CAMERA_LERP = 10.0f;
        vec3 d = playerEntity->Pos() - cam.LookAt;
        cam.Position += d * CAMERA_LERP * dt;
        cam.LookAt = cam.Position + vec3{0, -Global_Camera_OffsetY, -Global_Camera_OffsetZ};
    }

    UpdateLogiclessEntities(world, dt);
    for (int i = 0; i < ROAD_LANE_COUNT; i++)
    {
        l.LaneSlots[i] = 0;
    }
    for (auto ent : world.LaneSlotEntities)
    {
        if (!ent) continue;

        l.LaneSlots[ent->LaneSlot->Index]++;
    }
    for (auto ent : world.ShipEntities)
    {
        if (!ent) continue;

        // red ship goes backwards
        if (SHIP_IS_RED(ent->Ship))
        {
            ent->Rot().z = 180.0f;
            ent->Vel().y = playerEntity->Vel().y * -0.8f;
        }
        else
        {
            UpdateShipDraftCharge(ent->Ship, dt);
            if (playerEntity->Ship->DraftTarget != ent || ent->Pos().y < playerEntity->Pos().y)
            {
                if (ent->Pos().y - playerEntity->Pos().y >= 20.0f + (0.1f * playerEntity->Vel().y))
                {
                    ent->Vel().y = playerEntity->Vel().y * 0.2f;
                }
                else
                {
                    ent->Vel().y = playerEntity->Vel().y * 0.8f;
                }
            }
            if (SHIP_IS_ORANGE(ent->Ship) &&
                ent->Pos().y+1.0f < playerEntity->Pos().y &&
                ent->Ship->HasBeenDrafted &&
                !ent->Ship->Scored)
            {
                l.Score += SCORE_MISS_ORANGE_SHIP;
                ent->Ship->Scored = true;
            }

            KeepEntityInsideOfRoad(ent);
        }
    }
    for (auto ent : world.PowerupEntities)
    {
        if (!ent) continue;

        vec3 distToPlayer = (playerEntity->Pos() + playerEntity->Vel()*dt) - ent->Pos();
        vec3 dirToPlayer = glm::normalize(distToPlayer);
        ent->SetVel(ent->Vel() + dirToPlayer);
        ent->SetPos(ent->Pos() + ent->Vel() * dt);

        // TODO: temporary i hope
        if (ent->Pos().z < 0.0f)
        {
            RemoveEntity(g.World, ent);
        }
    }

    auto playerPosition = playerEntity->Pos();
    alSourcefv(l.DraftBoostAudio->Source, AL_POSITION, &playerPosition[0]);
    UpdateListener(g.Camera, playerPosition);
    UpdateProjectionView(g.Camera);

    updateTime.End = g.Platform.GetMilliseconds();
    renderTime.Begin = g.Platform.GetMilliseconds();

    RenderBegin(g.RenderState, dt);
    RenderEntityWorld(g.RenderState, g.World, dt);

#ifdef DRAFT_DEBUG
    if (Global_Collision_DrawCollider)
    {
        for (auto ent : world.CollisionEntities)
        {
            if (!ent) continue;

            DrawDebugCollider(g.RenderState, ent->Collider->Box, ent->NumCollisions > 0);
        }
    }
#endif

    RenderEnd(g.RenderState, g.Camera);
    renderTime.End = g.Platform.GetMilliseconds();

    UpdateProjectionView(g.GUICamera);
    Begin(g.GUI, g.GUICamera);
    DrawRect(g.GUI, rect{20,20,200,20},
             IntColor(FirstPalette.Colors[2]), GL_LINE_LOOP, false);

    DrawRect(g.GUI, rect{25,25,190 * playerShip->DraftCharge,10},
             IntColor(FirstPalette.Colors[3]), GL_TRIANGLES, false);

    DrawText(g.GUI, g.TestFont, Format(l.ScoreFormat, l.Score), rect{50, 20, 0, 0}, Color_white);

    End(g.GUI);
}
