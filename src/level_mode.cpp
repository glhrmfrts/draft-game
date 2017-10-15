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

    g.PlayerEntity = CreateShipEntity(g.World.Arena, GetShipMesh(g), Color_blue, IntColor(FirstPalette.Colors[1]), true);
    g.PlayerEntity->Transform.Position.z = SHIP_Z;
    g.PlayerEntity->Transform.Velocity.y = PlayerMinVel;
    AddEntity(g.World, g.PlayerEntity);

    for (int i = 0; i < LEVEL_PLANE_COUNT; i++)
    {
        auto ent = PushStruct<entity>(g.World.Arena);
        ent->Transform.Position.y = LEVEL_PLANE_SIZE * i;
        ent->Transform.Position.z = -0.25f;
        ent->Transform.Scale = vec3{LEVEL_PLANE_SIZE, LEVEL_PLANE_SIZE, 0};
        ent->Model = CreateModel(g.World.Arena, GetFloorMesh(g));
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
        ent->Model = CreateModel(g.World.Arena, GetRoadMesh(g));
        ent->Repeat = PushStruct<entity_repeat>(g.World.Arena);
        ent->Repeat->Count = LEVEL_PLANE_COUNT;
        ent->Repeat->Size = LEVEL_PLANE_SIZE;
        ent->Repeat->DistanceFromCamera = LEVEL_PLANE_SIZE / 2;
        AddEntity(g.World, ent);
    }

    g.TestFont = FindBitmapFont(g.AssetLoader, "vcr_16");
    l.DraftBoostAudio = CreateAudioSource(g.Arena, FindSound(g.AssetLoader, "boost")->Buffer);
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
        if (otherEntity->Ship->DraftActive && otherEntity->Ship->DraftTarget == entityToExplode)
        {
            otherEntity->Ship->DraftActive = false;
            if (entityToExplode->Flags & EntityFlag_IsPlayer)
            {
                entityToExplode = otherEntity;
            }

            auto exp = CreateExplosionEntity(
                g.World.Arena,
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
    }
    if (shipEntity != NULL && crystalEntity != NULL)
    {
        if (shipEntity->Flags & EntityFlag_IsPlayer)
        {
            auto exp = CreateExplosionEntity(
                g.World.Arena,
                *crystalEntity->Model->Mesh,
                crystalEntity->Model->Mesh->Parts[0],
                crystalEntity->Pos(),
                shipEntity->Vel(),
                crystalEntity->Scl(),
                CRYSTAL_COLOR,
                CRYSTAL_COLOR,
                vec3{ 0, 1, 1 }
            );
            AddEntity(g.World, exp);
            RemoveEntity(g.World, crystalEntity);
        }
        return false;
    }
    return true;
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

static int GetNextSpawnLane(level_mode &l, bool IsShip = false)
{
    static int LastLane = 0;
    static int LastShipLane = 0;
    int Lane = LastLane;
    while (Lane == LastLane || (IsShip && Lane == LastShipLane))
    {
        Lane = RandomBetween(l.Entropy, -2, 2);
    }
    LastLane = Lane;
    if (IsShip)
    {
        LastShipLane = Lane;
    }
    return Lane;
}

static int GetNextShipColor(level_mode &l)
{
    static int LastColor = 0;
    int Color = LastColor;
    while (Color == LastColor)
    {
        Color = RandomBetween(l.Entropy, 0, 1);
    }
    LastColor = Color;
    return Color;
}

#define INITIAL_SHIP_INTERVAL 4.0f
#define CHANGE_SHIP_TIMER     8.0f
static void GenerateEnemyShips(game_state &g, level_mode &l, float dt)
{
    static float NextShipInterval = INITIAL_SHIP_INTERVAL;
    static float NextShipTimer = INITIAL_SHIP_INTERVAL;
    static float ChangeShipTimer = CHANGE_SHIP_TIMER;

    // @TODO: remove debug key
    if (NextShipTimer <= 0 || g.Input.Keys[SDL_SCANCODE_S])
    {
        if (ChangeShipTimer <= 0)
        {
            ChangeShipTimer = CHANGE_SHIP_TIMER;
            NextShipInterval -= 0.15f;
            NextShipInterval = std::max(1.0f, NextShipInterval);
            NextShipInterval -= 0.0025f * g.PlayerEntity->Vel().y;
            NextShipInterval = std::max(0.05f, NextShipInterval);
        }
        NextShipTimer = NextShipInterval;

        int ColorIndex = GetNextShipColor(l);
        color Color = IntColor(ShipPalette.Colors[ColorIndex]);
        auto ent = CreateShipEntity(g.World.Arena, GetShipMesh(g), Color, Color, false);
        int Lane = GetNextSpawnLane(l, true);
        ent->Pos().x = Lane * ROAD_LANE_WIDTH;
        ent->Pos().y = g.PlayerEntity->Pos().y + 200;
        ent->Pos().z = SHIP_Z;
        ent->Ship->ColorIndex = ColorIndex;
        AddEntity(g.World, ent);
    }

    NextShipTimer -= dt;
    ChangeShipTimer -= dt;
}

static void GenerateCrystals(game_state &g, level_mode &l, float dt)
{
    const float baseCrystalInterval = 2.0f;
    static float nextCrystalTimer = baseCrystalInterval;
    if (nextCrystalTimer <= 0)
    {
        nextCrystalTimer = baseCrystalInterval + RandomBetween(l.Entropy, -1.5f, 1.5f);

        auto ent = CreateCrystalEntity(g.World.Arena, GetCrystalMesh(g));
        ent->Pos().x = GetNextSpawnLane(l) * ROAD_LANE_WIDTH;
        ent->Pos().y = g.PlayerEntity->Pos().y + 200;
        ent->Pos().z = SHIP_Z + 0.4f;
        ent->Scl().x = 0.3f;
        ent->Scl().y = 0.3f;
        ent->Scl().z = 0.5f;
        ent->Vel().y = g.PlayerEntity->Vel().y * 0.5f;
        AddEntity(g.World, ent);
    }

    nextCrystalTimer -= dt;
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

#define PLAYER_MAX_VEL_INCREASE_FACTOR 0.5f
static void RenderLevel(game_state &g, float dt)
{
    auto &l = g.LevelMode;
    auto &updateTime = g.UpdateTime;
    auto &renderTime = g.RenderTime;
    updateTime.Begin = g.Platform.GetMilliseconds();

    auto &world = g.World;
    auto &input = g.Input;
    auto &cam = g.Camera;
    auto *playerEntity = g.PlayerEntity;
    auto *playerShip = playerEntity->Ship;
    dt *= Global_Game_TimeSpeed;

    l.PlayerMaxVel += PLAYER_MAX_VEL_INCREASE_FACTOR * dt;

    static bool Paused = false;
    if (IsJustPressed(g, Action_debugPause))
    {
        Paused = !Paused;
    }
    if (Paused) return;

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
        auto exp = CreateExplosionEntity(g.World.Arena,
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

    GenerateEnemyShips(g, l, dt);
    GenerateCrystals(g, l, dt);

    {
        float moveX = GetAxisValue(input, Action_horizontal);
        MoveShipEntity(playerEntity, moveX, 0, l.PlayerMaxVel, dt);
        KeepEntityInsideOfRoad(playerEntity);

        float &playerX = playerEntity->Pos().x;
        float nearestLane = std::floor(std::ceil(playerX)/ROAD_LANE_WIDTH);
        float targetX = nearestLane * ROAD_LANE_WIDTH;
        if (moveX == 0.0f)
        {
            float dif = targetX - playerX;
            if (std::abs(dif) > 0.05f)
            {
                playerEntity->Transform.Velocity.x = dif*4.0f;
            }
        }

        if (playerShip->DraftCharge == 1.0f && IsJustPressed(g, Action_boost))
        {
            ShipEntityPerformDraft(playerEntity);
        }
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

    for (auto ent : world.ShipEntities)
    {
        if (!ent) continue;

        UpdateShipDraftCharge(ent->Ship, dt);
        if (ent->Pos().y + 1.0f < playerEntity->Pos().y)
        {
            float MoveX = playerEntity->Pos().x - ent->Pos().x;
            MoveX = std::min(MoveX, 1.0f);

            float MoveY = 0.05f + (0.004f * playerEntity->Vel().y);
            if (ent->Vel().y > playerEntity->Vel().y)
            {
                MoveY = 0.0f;
            }
            MoveShipEntity(ent, MoveX, MoveY, l.PlayerMaxVel, dt);

            if (ent->Ship->DraftCharge == 1.0f)
            {
                ShipEntityPerformDraft(ent);
            }
        }
        else if (playerShip->DraftTarget != ent)
        {
            ent->Vel().y = playerEntity->Vel().y * 0.8f;
        }

        KeepEntityInsideOfRoad(ent);
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
        for (auto *ent : world.CollisionEntities)
        {
            if (!ent) continue;

            DrawDebugCollider(g.RenderState, ent->Collider->Box, ent->NumCollisions > 0);
        }
    }
#endif

    RenderEnd(g.RenderState, g.Camera);
    renderTime.End = g.Platform.GetMilliseconds();

    {
        UpdateProjectionView(g.GUICamera);
        Begin(g.GUI, g.GUICamera);
        DrawRect(g.GUI, rect{20,20,200,20},
                 IntColor(FirstPalette.Colors[2]), GL_LINE_LOOP, false);

        DrawRect(g.GUI, rect{25,25,190 * playerShip->DraftCharge,10},
                 IntColor(FirstPalette.Colors[3]), GL_TRIANGLES, false);

        //DrawText(g.GUI, g.TestFont, "Score gui", rect{50, 20, 0, 0}, Color_white);

        End(g.GUI);
    }
}
