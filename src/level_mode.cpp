// Copyright

#define PLAYER_MAX_VEL_INCREASE_FACTOR 0.5f
#define PLAYER_MAX_VEL_LIMIT           170.0f

#define ROAD_LANE_WIDTH   2
#define LEVEL_PLANE_COUNT 5
#define SHIP_Z            0.2f

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

static int GetNextSpawnLane(level_mode *l, bool isShip = false)
{
    static int lastLane = 0;
    static int lastShipLane = 0;
    int lane = lastLane;
    while (lane == lastLane || (isShip && lane == lastShipLane) || (l->ReservedLanes[lane+2] == 1))
    {
        lane = RandomBetween(l->Entropy, -2, 2);
    }
    lastLane = lane;
    if (isShip)
    {
        lastShipLane = lane;
    }
    return lane;
}

static int GetNextShipColor(level_mode *l)
{
    static int lastColor = 0;
    int color = lastColor;
    while (color == lastColor)
    {
        color = RandomBetween(l->Entropy, 0, 1);
    }
    lastColor = color;
    return color;
}

static void GenerateCrystal(level_gen_params *p, game_state *g, level_mode *l)
{
    int lane = GetNextSpawnLane(l);
    auto ent = CreateCrystalEntity(GetEntry(g->World.CrystalPool), GetCrystalMesh(g));
    ent->Pos().x = lane * ROAD_LANE_WIDTH;
    ent->Pos().y = g->PlayerEntity->Pos().y + 200;
    ent->Pos().z = SHIP_Z + 0.4f;
    ent->Scl().x = 0.3f;
    ent->Scl().y = 0.3f;
    ent->Scl().z = 0.5f;
    ent->Vel().y = PLAYER_MIN_VEL * 0.5f;
    AddEntity(g->World, ent);
}

static void GenerateShip(level_gen_params *p, game_state *g, level_mode *l)
{
    int colorIndex = GetNextShipColor(l);
    color c = IntColor(ShipPalette.Colors[colorIndex]);
    int lane = GetNextSpawnLane(l, true);
    auto ent = CreateShipEntity(GetEntry(g->World.ShipPool), GetShipMesh(g), c, c, false);
    ent->LaneSlot = CreateLaneSlot(ent->PoolEntry, lane);
    ent->Pos().x = lane * ROAD_LANE_WIDTH;
    ent->Pos().y = g->PlayerEntity->Pos().y + 200;
    ent->Pos().z = SHIP_Z;
    ent->Ship->ColorIndex = colorIndex;
    AddFlags(ent, EntityFlag_RemoveOffscreen);
    AddEntity(g->World, ent);
}

static void GenerateRedShip(level_gen_params *p, game_state *g, level_mode *l)
{
    color c = IntColor(ShipPalette.Colors[2]);
    auto ent = CreateShipEntity(GetEntry(g->World.ShipPool), GetShipMesh(g), c, c, false);
    int lane = p->ReservedLane;
    ent->Pos().x = lane * ROAD_LANE_WIDTH;
    ent->Pos().y = g->PlayerEntity->Pos().y + 200;
    ent->Pos().z = SHIP_Z;
    ent->Ship->ColorIndex = 2;
    AddFlags(ent, EntityFlag_RemoveOffscreen);
    AddEntity(g->World, ent);
}

#define ASTEROID_Z (SHIP_Z + 10)
static void GenerateAsteroid(level_gen_params *p, game_state *g, level_mode *l)
{
    //Println("I should be generating an asteroid");
    auto ent = CreateAsteroidEntity(GetEntry(g->World.AsteroidPool), GetAsteroidMesh(g));
    int lane = l->PlayerLaneIndex - 2;
    ent->Pos().x = lane * ROAD_LANE_WIDTH;
    ent->Pos().y = g->PlayerEntity->Pos().y;
    ent->Pos().z = ASTEROID_Z;
    ent->Vel().y = g->PlayerEntity->Vel().y * 1.5f;
    AddFlags(ent, EntityFlag_RemoveOffscreen);
    AddEntity(g->World, ent);
}

static void InitLevel(game_state *g)
{
    g->Mode = GameMode_Level;

    auto l = &g->LevelMode;
    FreeArena(l->Arena);
    l->Entropy = RandomSeed(g->Platform.GetMilliseconds());

    g->RenderState.FogColor = IntColor(FirstPalette.Colors[3]) * 0.5f;
    g->RenderState.FogColor.a = 1.0f;
    ApplyExplosionLight(g->RenderState, IntColor(FirstPalette.Colors[3]));

    g->Gravity = vec3(0, 0, 0);
    g->World.Camera = &g->Camera;

    g->PlayerEntity = CreateShipEntity(&g->World.Arena, GetShipMesh(g), Color_blue, IntColor(FirstPalette.Colors[1]), true);
    g->PlayerEntity->Transform.Position.z = SHIP_Z;
    g->PlayerEntity->Transform.Velocity.y = PLAYER_MIN_VEL;
    AddEntity(g->World, g->PlayerEntity);

    for (int i = 0; i < LEVEL_PLANE_COUNT; i++)
    {
        auto ent = PushStruct<entity>(g->World.Arena);
        ent->Transform.Position.y = LEVEL_PLANE_SIZE * i;
        ent->Transform.Position.z = -0.25f;
        ent->Transform.Scale = vec3{LEVEL_PLANE_SIZE, LEVEL_PLANE_SIZE, 0};
        ent->Model = CreateModel(&g->World.Arena, GetFloorMesh(g));
        ent->Repeat = PushStruct<entity_repeat>(g->World.Arena);
        ent->Repeat->Count = LEVEL_PLANE_COUNT;
        ent->Repeat->Size = LEVEL_PLANE_SIZE;
        ent->Repeat->DistanceFromCamera = LEVEL_PLANE_SIZE/2;
        AddEntity(g->World, ent);
    }
    for (int i = 0; i < LEVEL_PLANE_COUNT; i++)
    {
        auto ent = PushStruct<entity>(g->World.Arena);
        ent->Transform.Position.y = LEVEL_PLANE_SIZE * i;
        ent->Transform.Position.z = 0.0f;
        ent->Transform.Scale = vec3{ 2, LEVEL_PLANE_SIZE, 1 };
        ent->Model = CreateModel(&g->World.Arena, GetRoadMesh(g));
        ent->Repeat = PushStruct<entity_repeat>(g->World.Arena);
        ent->Repeat->Count = LEVEL_PLANE_COUNT;
        ent->Repeat->Size = LEVEL_PLANE_SIZE;
        ent->Repeat->DistanceFromCamera = LEVEL_PLANE_SIZE / 2;
        AddEntity(g->World, ent);
    }

    g->TestFont = FindBitmapFont(g->AssetLoader, "g_type_16");
    l->DraftBoostAudio = CreateAudioSource(g->Arena, FindSound(g->AssetLoader, "boost")->Buffer);

    InitFormat(l->ScoreFormat, "Score: %d\n", 24, &l->Arena);
    InitFormat(l->ScoreNumberFormat, "+%d", 8, &l->Arena);

    auto gen = l->GenParams + LevelGenType_Crystal;
    gen->Flags = LevelGenFlag_Randomize;
    gen->Interval = BASE_CRYSTAL_INTERVAL;
    gen->Func = GenerateCrystal;

    gen = l->GenParams + LevelGenType_Ship;
    gen->Flags = LevelGenFlag_BasedOnVelocity;
    gen->Interval = INITIAL_SHIP_INTERVAL;
    gen->Func = GenerateShip;

    gen = l->GenParams + LevelGenType_RedShip;
    gen->Flags = LevelGenFlag_BasedOnVelocity | LevelGenFlag_ReserveLane;
    gen->Interval = INITIAL_SHIP_INTERVAL;
    gen->Func = GenerateRedShip;

    gen = l->GenParams + LevelGenType_Asteroid;
    gen->Flags = 0;
    gen->Interval = INITIAL_SHIP_INTERVAL;
    gen->Func = GenerateAsteroid;

    for (int i = 0; i < ROAD_LANE_COUNT; i++)
    {
        l->ReservedLanes[i] = 0;
    }
}

static void DebugReset(game_state *g)
{
}

static void AddScoreText(game_state *g, level_mode *l, int score, vec3 pos, color c)
{
    vec4 v = g->Camera.ProjectionView * vec4{pos, 1.0f};
    vec2 screenPos = vec2{v.x/v.w, v.y/v.w};
    screenPos.x = (g->Width/2) * screenPos.x + (g->Width/2);
    screenPos.y = (g->Height/2) * screenPos.y + (g->Height/2);

    auto seq = PushStruct<tween_sequence>(GetEntry(l->SequencePool));
    auto scoreText = PushStruct<level_score_text>(GetEntry(l->ScoreTextPool));
    scoreText->Color = c;
    scoreText->Pos = screenPos;
    scoreText->TargetPos = screenPos + vec2{20.0f, -100.0f};
    scoreText->Score = score;
    scoreText->Sequence = seq;

    seq->Tweens.push_back(tween{&scoreText->TweenValue, 0.0f, 1.0f, 1, 1.0f, TweenEasing_Linear});
    AddSequences(g->TweenState, seq, 1);
    PlaySequence(g->TweenState, seq);
    l->ScoreTextList.push_back(scoreText);
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

#define DRAFT_BOOST       40.0f
inline static void ApplyBoostToShip(entity *ent, float Boost, float Max)
{
    ent->Transform.Velocity.y += Boost;
}

#define SCORE_DESTROY_BLUE_SHIP 10
#define SCORE_MISS_ORANGE_SHIP  10
#define SCORE_DRAFT             5
#define SCORE_CRYSTAL           2

#define ENTITY_IS_PLAYER(e) (e->Flags & EntityFlag_IsPlayer)

#define SHIP_IS_BLUE(s)   (s->ColorIndex == 0)
#define SHIP_IS_ORANGE(s) (s->ColorIndex == 1)
#define SHIP_IS_RED(s)    (s->ColorIndex == 2)
static bool HandleCollision(game_state *g, entity *first, entity *second, float dt)
{
    auto l = &g->LevelMode;
    auto shipEntity = FindEntityOfType(first, second, ColliderType_Ship).Found;
    auto crystalEntity = FindEntityOfType(first, second, ColliderType_Crystal).Found;
    if (first->Collider->Type == ColliderType_TrailPiece || second->Collider->Type == ColliderType_TrailPiece)
    {
        auto trailPieceEntity = FindEntityOfType(first, second, ColliderType_TrailPiece).Found;
        if (shipEntity && ENTITY_IS_PLAYER(shipEntity) && shipEntity != trailPieceEntity->TrailPiece->Owner)
        {
            if (l->NumTrailCollisions == 0 && (!l->DraftActive || l->DraftTarget != trailPieceEntity->TrailPiece->Owner))
            {
                l->CurrentDraftTime += dt;
                l->NumTrailCollisions++;
                l->DraftTarget = trailPieceEntity->TrailPiece->Owner;
                l->DraftActive = false;
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
        if ((ENTITY_IS_PLAYER(otherEntity) && l->DraftActive && l->DraftTarget == entityToExplode) ||
            (SHIP_IS_RED(otherEntity->Ship) || SHIP_IS_RED(entityToExplode->Ship)))
        {
            if (ENTITY_IS_PLAYER(otherEntity))
            {
                l->DraftActive = false;
            }
            else if (ENTITY_IS_PLAYER(entityToExplode))
            {
                entityToExplode = otherEntity;
            }

            if (SHIP_IS_BLUE(entityToExplode->Ship))
            {
                l->Score += SCORE_DESTROY_BLUE_SHIP;
                entityToExplode->Ship->Scored = true;
                AddScoreText(g, l, SCORE_DESTROY_BLUE_SHIP, entityToExplode->Pos(), IntColor(ShipPalette.Colors[SHIP_BLUE]));
            }

            auto exp = CreateExplosionEntity(
                GetEntry(g->World.ExplosionPool),
                entityToExplode->Transform.Position,
                otherEntity->Transform.Velocity,
                entityToExplode->Ship->Color,
                entityToExplode->Ship->OutlineColor,
                vec3{ 0, 1, 1 }
            );
            AddEntity(g->World, exp);
            RemoveEntity(g->World, entityToExplode);
            return false;
        }
        else
        {
            if (otherEntity->Flags & EntityFlag_IsPlayer)
            {
                otherEntity->Vel().y = std::min(PLAYER_MIN_VEL, otherEntity->Vel().y);
            }
        }
        return false;
    }
    if (shipEntity != NULL && crystalEntity != NULL)
    {
        if (shipEntity->Flags & EntityFlag_IsPlayer)
        {
            l->Score += SCORE_CRYSTAL;
            AddScoreText(g, l, SCORE_CRYSTAL, crystalEntity->Pos(), CRYSTAL_COLOR);

            auto pup = CreatePowerupEntity(GetEntry(g->World.PowerupPool), g->LevelMode.Entropy, g->LevelMode.TimeElapsed, crystalEntity->Pos(), shipEntity->Vel(), CRYSTAL_COLOR);
            AddEntity(g->World, pup);
            RemoveEntity(g->World, crystalEntity);
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

static void KeepEntityInsideOfRoad(entity *ent)
{
    const float limit = ROAD_LANE_COUNT * ROAD_LANE_WIDTH / 2;
    ent->Pos().x = glm::clamp(ent->Pos().x, -limit + 0.5f, limit - 0.5f);
}

static void ResetGen(level_gen_params *p)
{
    p->Interval = INITIAL_SHIP_INTERVAL;
    p->Timer = p->Interval;
}

static float GetNextTimer(level_gen_params *p, random_series random)
{
    if (p->Flags & LevelGenFlag_Randomize)
    {
        const float offset = 1.5f;
        return p->Interval + RandomBetween(random, -offset, offset);
    }
    return p->Interval;
}

static void UpdateGen(game_state *g, level_mode *l, level_gen_params *p, float dt)
{
    if (!(p->Flags & LevelGenFlag_Enabled))
    {
        return;
    }

    if (p->Timer <= 0)
    {
        p->Func(p, g, l);
        p->Timer = GetNextTimer(p, l->Entropy);
        if (p->Flags & LevelGenFlag_BasedOnVelocity)
        {
            p->Timer -= (p->Timer * 0.9f) * (g->PlayerEntity->Vel().y / PLAYER_MAX_VEL_LIMIT);
        }
        l->ReservedLanes[p->ReservedLane + 2] = 0;
        p->ReservedLane = NO_RESERVED_LANE;
    }

    if (p->Flags & LevelGenFlag_ReserveLane)
    {
        if (p->Timer <= p->Interval*0.5f && p->ReservedLane == NO_RESERVED_LANE)
        {
            const int maxTries = 5;
            int i = 0;
            int laneIndex = l->PlayerLaneIndex;

            // get the first non-occupied lane, or give up for this frame
            while (l->LaneSlots[laneIndex] > 0 && i < maxTries)
            {
                laneIndex = RandomBetween(l->Entropy, 0, 4);
                i++;
            }
            if (i >= maxTries)
            {
                p->Timer = GetNextTimer(p, l->Entropy);
            }
            else
            {
                p->ReservedLane = laneIndex - 2;
                l->ReservedLanes[laneIndex] = 1;
            }
        }
    }

    p->Timer -= dt;
}

static void RenderLevel(game_state *g, float dt)
{
    auto l = &g->LevelMode;
    auto &updateTime = g->UpdateTime;
    auto &renderTime = g->RenderTime;

    auto &world = g->World;
    auto &input = g->Input;
    auto &cam = g->Camera;
    auto *playerEntity = g->PlayerEntity;
    auto *playerShip = playerEntity->Ship;
    dt *= Global_Game_TimeSpeed;

    l->PlayerMaxVel += PLAYER_MAX_VEL_INCREASE_FACTOR * dt;
    l->PlayerMaxVel = std::min(l->PlayerMaxVel, PLAYER_MAX_VEL_LIMIT);
    l->TimeElapsed += dt;
    updateTime.Begin = g->Platform.GetMilliseconds();

    static bool paused = false;
    if (IsJustPressed(g, Action_debugPause))
    {
        paused = !paused;
    }
    if (paused) return;

    if (g->Input.Keys[SDL_SCANCODE_R])
    {
        DebugReset(g);
    }
    if (Global_Camera_FreeCam)
    {
        UpdateFreeCam(cam, input, dt);
    }

    if (g->Input.Keys[SDL_SCANCODE_E])
    {
        auto exp = CreateExplosionEntity(GetEntry(g->World.ExplosionPool),
                                         g->PlayerEntity->Transform.Position,
                                         g->PlayerEntity->Transform.Velocity,
                                         g->PlayerEntity->Ship->Color,
                                         g->PlayerEntity->Ship->OutlineColor,
                                         vec3{ 0, 1, 1 });
        AddEntity(world, exp);
    }

    // we have a different state each 10 seconds elapsed
    int state = int(std::floor(l->TimeElapsed / 10.0f));
    auto crystals = l->GenParams + LevelGenType_Crystal;
    auto ships = l->GenParams + LevelGenType_Ship;
    auto redShips = l->GenParams + LevelGenType_RedShip;
    auto asteroids = l->GenParams + LevelGenType_Asteroid;
    switch (state)
    {
    case 0:
        crystals->Flags |= LevelGenFlag_Enabled | LevelGenFlag_Randomize;
        asteroids->Flags |= LevelGenFlag_Enabled;
        break;

    case 1:
        ships->Flags |= LevelGenFlag_Enabled;
        break;

    case 6:
        ships->Flags &= ~LevelGenFlag_Enabled;
        ResetGen(ships);
        redShips->Flags |= LevelGenFlag_Enabled;
        break;

    case 8:
        ships->Flags |= LevelGenFlag_Enabled;
        break;
    }
    for (int i = 0; i < LevelGenType_MAX; i++)
    {
        UpdateGen(g, l, l->GenParams + i, dt);
    }

    {
        // player movement
        float moveX = GetAxisValue(input, Action_horizontal);
        float moveY = GetAxisValue(input, Action_vertical);
        if (moveY > 0.0f)
        {
            moveY = 0.0f;
        }
        MoveShipEntity(playerEntity, moveX, moveY, l->PlayerMaxVel, dt);

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

        if (l->DraftCharge == 1.0f && IsPressed(g, Action_boost))
        {
            l->Score += SCORE_DRAFT;
            l->CurrentDraftTime = 0.0f;
            l->DraftActive = true;
            l->DraftTarget->Ship->HasBeenDrafted = true;
            ApplyBoostToShip(playerEntity, DRAFT_BOOST, 0);

            AddScoreText(g, l, SCORE_DRAFT, playerEntity->Pos(), l->DraftTarget->Ship->Color);
        }

        l->PlayerLaneIndex = int(nearestLane)+2;
    }

    size_t FrameCollisionCount = 0;
    DetectCollisions(world.CollisionEntities, l->CollisionCache, FrameCollisionCount);
    for (size_t i = 0; i < FrameCollisionCount; i++)
    {
        auto col = &l->CollisionCache[i];
        col->First->NumCollisions++;
        col->Second->NumCollisions++;

        if (HandleCollision(g, col->First, col->Second, dt))
        {
            ResolveCollision(*col);
        }
    }
    Integrate(world.CollisionEntities, g->Gravity, dt);
    if (l->NumTrailCollisions == 0)
    {
        l->CurrentDraftTime -= dt;
    }
    l->CurrentDraftTime = std::max(0.0f, std::min(l->CurrentDraftTime, Global_Game_DraftChargeTime));
    l->DraftCharge = l->CurrentDraftTime / Global_Game_DraftChargeTime;
    l->NumTrailCollisions = 0;
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
        l->LaneSlots[i] = 0;
    }
    for (auto ent : world.LaneSlotEntities)
    {
        if (!ent) continue;

        l->LaneSlots[ent->LaneSlot->Index]++;
    }
    for (auto ent : world.ShipEntities)
    {
        if (!ent) continue;

        // red ship goes backwards
        if (SHIP_IS_RED(ent->Ship))
        {
            ent->Rot().z = 180.0f;
            ent->Vel().y = -PLAYER_MIN_VEL;
        }
        else
        {
            if (!(l->DraftTarget == ent && l->DraftActive) ||
                (ent->Pos().y < playerEntity->Pos().y && playerEntity->Vel().y < ent->Ship->PassedVelocity))
            {
                ent->Ship->PassedVelocity = playerEntity->Vel().y;
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
                l->Score += SCORE_MISS_ORANGE_SHIP;
                ent->Ship->Scored = true;
                AddScoreText(g, l, SCORE_MISS_ORANGE_SHIP, ent->Pos(), IntColor(ShipPalette.Colors[SHIP_ORANGE]));
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
            RemoveEntity(g->World, ent);
        }
    }
    for (auto ent : world.AsteroidEntities)
    {
        if (!ent) continue;

        if (ent->Pos().z > 0.0f)
        {
            if (ent->Pos().y > playerEntity->Pos().y)
            {
                ent->Vel().z -= 30.0f * dt;
            }
            else
            {
                ent->Vel().z -= 8.0f * dt;
            }
            //ent->Vel().y = playerEntity->Vel().y * 1.2f;
        }
        else
        {
            ent->Pos().z = SHIP_Z;
            ent->Vel().y = 0;
            ent->Vel().z = 0;
            if (!ent->Asteroid->Exploded)
            {
                ent->Asteroid->Exploded = true;
                auto exp = CreateExplosionEntity(GetEntry(g->World.ExplosionPool),
                                                 ent->Pos(),
                                                 vec3(0.0f),
                                                 ASTEROID_COLOR,
                                                 ASTEROID_COLOR,
                                                 vec3(0.0f));
                AddEntity(g->World, exp);
            }
        }
    }

    auto playerPosition = playerEntity->Pos();
    alSourcefv(l->DraftBoostAudio->Source, AL_POSITION, &playerPosition[0]);
    UpdateListener(g->Camera, playerPosition);
    UpdateProjectionView(g->Camera);

    updateTime.End = g->Platform.GetMilliseconds();
    renderTime.Begin = g->Platform.GetMilliseconds();

    PostProcessBegin(g->RenderState);
    RenderBegin(g->RenderState, dt);
    RenderEntityWorld(g->RenderState, g->World, dt);

#ifdef DRAFT_DEBUG
    if (Global_Collision_DrawCollider)
    {
        for (auto ent : world.CollisionEntities)
        {
            if (!ent) continue;

            DrawDebugCollider(g->RenderState, ent->Collider->Box, ent->NumCollisions > 0);
        }
    }
#endif

    RenderEnd(g->RenderState, g->Camera);
    UpdateProjectionView(g->GUICamera);
    Begin(g->GUI, g->GUICamera, 1.0f);
    for (auto text : l->ScoreTextList)
    {
        vec2 p = text->Pos;
        float t = text->TweenValue;
        text->Color.a = 1.0f - t;
        p += (text->TargetPos - p) * t;
        DrawTextCentered(g->GUI, g->TestFont, Format(l->ScoreNumberFormat, text->Score), rect{p.x, p.y, 0, 0}, text->Color);
    }
    End(g->GUI);
    PostProcessEnd(g->RenderState);

    auto lastText = l->ScoreTextList.back();
    if (lastText && lastText->TweenValue >= 1.0f)
    {
        l->ScoreTextList.pop_back();
        DestroySequences(g->TweenState, lastText->Sequence, 1);
        FreeEntryFromData(l->ScoreTextPool, lastText);
        FreeEntryFromData(l->SequencePool, lastText->Sequence);
    }

    Begin(g->GUI, g->GUICamera);
    DrawRect(g->GUI, rect{20,20,200,20},
             IntColor(FirstPalette.Colors[2]), GL_LINE_LOOP, false);

    DrawRect(g->GUI, rect{25,25,190 * l->DraftCharge,10},
             IntColor(FirstPalette.Colors[3]), GL_TRIANGLES, false);

    DrawText(g->GUI, g->TestFont, Format(l->ScoreFormat, l->Score), rect{50, 20, 0, 0}, Color_white);
    End(g->GUI);

    renderTime.End = g->Platform.GetMilliseconds();
}
