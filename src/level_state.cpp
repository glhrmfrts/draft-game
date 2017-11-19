// Copyright

#define PLAYER_MAX_VEL_INCREASE_FACTOR 0.5f
#define PLAYER_MAX_VEL_LIMIT           170.0f

#define SCORE_TEXT_CHECKPOINT "CHECKPOINT"
#define SCORE_TEXT_DRAFT      "DRAFT"
#define SCORE_TEXT_BLAST      "BLAST"
#define SCORE_TEXT_MISS       "MISS"
#define SCORE_TEXT_HEALTH    "HEALTH"

#define SCORE_CHECKPOINT        20
#define SCORE_DESTROY_BLUE_SHIP 10
#define SCORE_MISS_ORANGE_SHIP  10
#define SCORE_DRAFT             5
#define SCORE_HEALTH            2

#define GEN_PLAYER_OFFSET 250

// TODO: make possible to share the entity-generation system with the
// menu
//
// TODO: update asteroids velocity (or remove them?)

void PauseMenuCallback(game_main *, menu_data *, int);

static menu_data pauseMenu = {
    "PAUSED", 2, PauseMenuCallback,
    {
        menu_item{"CONTINUE", MenuItemType_Text},
        menu_item{"EXIT", MenuItemType_Text},
    }
};

struct audio_source
{
    ALuint Source;
    float Gain = 1;
};
audio_source *CreateAudioSource(memory_arena &arena, ALuint buffer)
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

void UpdateAudioParams(audio_source *audio)
{
    alSourcef(audio->Source, AL_GAIN, audio->Gain);
}

int GetNextSpawnLane(level_state *l, bool isShip = false)
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

int GetNextShipColor(level_state *l)
{
    static int lastColor = 0;
    int color = lastColor;
    if (l->ForceShipColor > -1)
    {
        return lastColor = l->ForceShipColor;
    }
    while (color == lastColor)
    {
        color = RandomBetween(l->Entropy, 0, 1);
    }
    lastColor = color;
    return color;
}

LEVEL_GEN_FUNC(GenerateCrystal)
{
    int lane = GetNextSpawnLane(l);
    auto ent = CreateCrystalEntity(GetEntry(g->World.CrystalPool), GetCrystalMesh(g->World));
    ent->Pos().x = lane * ROAD_LANE_WIDTH;
    ent->Pos().y = g->World.PlayerEntity->Pos().y + GEN_PLAYER_OFFSET;
    ent->Pos().z = SHIP_Z + 0.4f;
    ent->Scl().x = 0.3f;
    ent->Scl().y = 0.3f;
    ent->Scl().z = 0.5f;
    ent->Vel().y = PLAYER_MIN_VEL * 0.2f;
    AddEntity(g->World, ent);
}

LEVEL_GEN_FUNC(GenerateShip)
{
    int colorIndex = GetNextShipColor(l);
    color c = IntColor(ShipPalette.Colors[colorIndex]);
    int lane = GetNextSpawnLane(l, true);
    auto ent = CreateShipEntity(GetEntry(g->World.ShipPool), GetShipMesh(g->World), c, c, false, colorIndex);
    ent->LaneSlot = CreateLaneSlot(ent->PoolEntry, lane);
    ent->Pos().x = lane * ROAD_LANE_WIDTH;
    ent->Pos().y = g->World.PlayerEntity->Pos().y + GEN_PLAYER_OFFSET;
    ent->Pos().z = SHIP_Z;
    ent->Ship->ColorIndex = colorIndex;
    AddFlags(ent, EntityFlag_RemoveOffscreen);
    AddEntity(g->World, ent);
}

LEVEL_GEN_FUNC(GenerateRedShip)
{
    color c = IntColor(ShipPalette.Colors[SHIP_RED]);
    auto ent = CreateShipEntity(GetEntry(g->World.ShipPool), GetShipMesh(g->World), c, c, false, SHIP_RED);
    int lane = l->PlayerLaneIndex - 2;
    if (p->Flags & LevelGenFlag_ReserveLane)
    {
        lane = p->ReservedLane;
    }
    ent->Pos().x = lane * ROAD_LANE_WIDTH;
    ent->Pos().y = g->World.PlayerEntity->Pos().y + GEN_PLAYER_OFFSET;
    ent->Pos().z = SHIP_Z;
    ent->Ship->ColorIndex = SHIP_RED;
    AddFlags(ent, EntityFlag_RemoveOffscreen);
    AddEntity(g->World, ent);
}

#define ASTEROID_Z (SHIP_Z + 80)
LEVEL_GEN_FUNC(GenerateAsteroid)
{
    //Println("I should be generating an asteroid");
    auto ent = CreateAsteroidEntity(GetEntry(g->World.AsteroidPool), GetAsteroidMesh(g->World));
    int lane = l->PlayerLaneIndex - 2;
    ent->Pos().x = lane * ROAD_LANE_WIDTH;
    ent->Pos().y = g->World.PlayerEntity->Pos().y + (GEN_PLAYER_OFFSET/2) + (100.0f * (g->World.PlayerEntity->Vel().y/PLAYER_MAX_VEL_LIMIT));
    ent->Pos().z = ASTEROID_Z;
    //ent->Vel().y = g->World.PlayerEntity->Vel().y * 1.5f;
    AddFlags(ent, EntityFlag_RemoveOffscreen);
    AddEntity(g->World, ent);
}

LEVEL_GEN_FUNC(GenerateSideTrail)
{
    // TODO: create the side trail pool
    auto ent = CreateEntity(&g->World.Arena);
    ent->Pos().y = g->World.PlayerEntity->Pos().y + GEN_PLAYER_OFFSET;
    ent->Trail = CreateTrail(&g->World.Arena, ent, Color_white, 0.5f, true);
    AddFlags(ent, EntityFlag_RemoveOffscreen | EntityFlag_UpdateMovement);
    AddEntity(g->World, ent);

    ent->Vel().y = RandomBetween(l->Entropy, -25.0f, -75.0f);
    ent->Pos().x = RandomBetween(l->Entropy, 10.0f, 40.0f);
    if (RandomBetween(l->Entropy, 0, 1) == 0)
    {
        ent->Pos().x = -ent->Pos().x;
    }
}

LEVEL_GEN_FUNC(GenerateRandomGeometry)
{
    // TODO: create a pool for this
    mesh *m = NULL;
    vec3 scale = vec3(1.0f);
    color c = Color_white;
    if (RandomBetween(l->Entropy, 0, 1) == 0)
    {
        m = GetCrystalMesh(g->World);
        c = CRYSTAL_COLOR;
        scale = vec3{50.0f, 50.0f, 200.0f};
    }
    else
    {
        m = GetAsteroidMesh(g->World);
        c = ASTEROID_COLOR;
        scale = vec3(50.0f);
    }

    auto ent = CreateEntity(&g->World.Arena);
    ent->Model = CreateModel(&g->World.Arena, m);
    ent->FrameRotation = PushStruct<frame_rotation>(&g->World.Arena);
    ent->FrameRotation->Rotation.x = RandomBetween(l->Entropy, -45.0f, 45.0f);
    ent->FrameRotation->Rotation.y = RandomBetween(l->Entropy, -45.0f, 45.0f);
    ent->FrameRotation->Rotation.z = RandomBetween(l->Entropy, -45.0f, 45.0f);
    //ent->Trail = CreateTrail(&g->World.Arena, ent, c, 0.5f, true);

    ent->Pos().x = RandomBetween(l->Entropy, -1000.0f, 1000.0f);
    ent->Pos().y = g->World.PlayerEntity->Pos().y + GEN_PLAYER_OFFSET*2.0f;
    ent->Pos().z = RandomBetween(l->Entropy, -100.0f, -600.0f);
    ent->Vel().x = RandomBetween(l->Entropy, -20.0f, 20.0f);
    ent->Vel().y = RandomBetween(l->Entropy, -1.0f, -50.0f);
    ent->SetScl(scale);

    AddFlags(ent, EntityFlag_RemoveOffscreen | EntityFlag_UpdateMovement);
    AddEntity(g->World, ent);
}

void InitLevel(game_main *g)
{
    g->State = GameState_Level;

    auto l = &g->LevelState;
    FreeArena(l->Arena);
    l->Entropy = RandomSeed(g->Platform.GetMilliseconds());

    ApplyExplosionLight(g->RenderState, IntColor(FirstPalette.Colors[3]));

    g->Gravity = vec3(0, 0, 0);
    g->World.Camera = &g->Camera;
    l->DraftBoostAudio = CreateAudioSource(g->Arena, FindSound(g->AssetLoader, "boost")->Buffer);

    InitFormat(l->HealthFormat, "Health: %d\n", 24, &l->Arena);
    InitFormat(l->ScoreFormat, "Score: %d\n", 24, &l->Arena);
    InitFormat(l->ScoreNumberFormat, "%s +%d", 16, &l->Arena);

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
    gen->Flags = LevelGenFlag_BasedOnVelocity;// | LevelGenFlag_ReserveLane;
    gen->Interval = INITIAL_SHIP_INTERVAL;
    gen->Func = GenerateAsteroid;

    gen = l->GenParams + LevelGenType_SideTrail;
    gen->Flags = LevelGenFlag_Enabled | LevelGenFlag_Randomize;
    gen->Interval = 0.5f;
    gen->RandomOffset = 0.4f;
    gen->Func = GenerateSideTrail;

    gen = l->GenParams + LevelGenType_RandomGeometry;
    gen->Flags = LevelGenFlag_Enabled | LevelGenFlag_Randomize;
    gen->Interval = 0.5f;
    gen->RandomOffset = 0.4f;
    gen->Func = GenerateRandomGeometry;

    for (int i = 0; i < ROAD_LANE_COUNT; i++)
    {
        l->ReservedLanes[i] = 0;
    }
}

void DebugReset(game_main *g)
{
}

void AddIntroText(game_main *g, level_state *l, const char *text, color c)
{
    auto introText = PushStruct<level_intro_text>(GetEntry(l->IntroTextPool));
    introText->Color = color{c.r, c.g, c.b, 0.0f};
    introText->Text = text;
    introText->Pos = vec2{g->Width*0.5f, g->Height*0.75f};

    auto seq = PushStruct<tween_sequence>(GetEntry(l->SequencePool));
    if (l->IntroTextList.size() > 0)
    {
        for (auto ctext : l->IntroTextList)
        {
            auto cseq = PushStruct<tween_sequence>(GetEntry(l->SequencePool));
            cseq->Tweens.push_back(tween(&ctext->Pos.y)
                                   .SetFrom(ctext->Pos.y)
                                   .SetTo(ctext->Pos.y - GetRealPixels(g, 60.0f))
                                   .SetDuration(0.5f));

            ctext->PosSequence = cseq;
            AddSequences(g->TweenState, cseq, 1);
            PlaySequence(g->TweenState, cseq, true);
        }
        seq->Tweens.push_back(WaitTween(0.5f));
    }

    auto twn = tween(&introText->Color.a)
        .SetFrom(0.0f)
        .SetTo(1.0f)
        .SetDuration(1.0f)
        .SetEasing(TweenEasing_Linear);
    seq->Tweens.push_back(twn);
    seq->Tweens.push_back(WaitTween(3.0f));
    seq->Tweens.push_back(ReverseTween(twn));

    introText->Sequence = seq;
    AddSequences(g->TweenState, seq, 1);
    PlaySequence(g->TweenState, seq, true);

    l->IntroTextList.push_back(introText);
}

void AddScoreText(game_main *g, level_state *l, const char *text, int score, vec3 pos, color c)
{
    vec4 v = g->FinalCamera.ProjectionView * vec4{WorldToRenderTransform(pos), 1.0f};
    vec2 screenPos = vec2{v.x/v.w, v.y/v.w};
    screenPos.x = (g->Width/2) * screenPos.x + (g->Width/2);
    screenPos.y = (g->Height/2) * screenPos.y + (g->Height/2);

    float offsetX = GetRealPixels(g, 100.0f);
    float minOffsetY = GetRealPixels(g, 200.0f);
    float maxOffsetY = GetRealPixels(g, 300.0f);
    vec2 targetPos = vec2{RandomBetween(l->Entropy, -offsetX, offsetX),
                          RandomBetween(l->Entropy, -minOffsetY, -maxOffsetY)};
    auto seq = PushStruct<tween_sequence>(GetEntry(l->SequencePool));
    auto scoreText = PushStruct<level_score_text>(GetEntry(l->ScoreTextPool));
    scoreText->Color = c;
    scoreText->Pos = screenPos;
    scoreText->TargetPos = screenPos + targetPos;
    scoreText->Text = text;
    scoreText->Score = score;
    scoreText->Sequence = seq;
    scoreText->TweenAlphaValue = 1.0f;
    seq->Tweens.push_back(tween(&scoreText->TweenPosValue)
                          .SetFrom(0.0f)
                          .SetTo(1.0f)
                          .SetDuration(1.0f)
                          .SetEasing(TweenEasing_Linear));
    seq->Tweens.push_back(tween(&scoreText->TweenAlphaValue)
                          .SetFrom(1.0f)
                          .SetTo(0.0f)
                          .SetDuration(0.5f)
                          .SetEasing(TweenEasing_Linear));
    AddSequences(g->TweenState, seq, 1);
    PlaySequence(g->TweenState, seq, true);
    l->ScoreTextList.push_back(scoreText);
}

struct find_entity_result
{
    entity *Found = NULL;
    entity *Other = NULL;
};
find_entity_result FindEntityOfType(entity *first, entity *second, collider_type type)
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

#define PLAYER_DAMAGE_TIMER 3.0f

#define ENTITY_IS_PLAYER(e) (e->Flags & EntityFlag_IsPlayer)

#define SHIP_IS_BLUE(s)   (s->ColorIndex == 0)
#define SHIP_IS_ORANGE(s) (s->ColorIndex == 1)
#define SHIP_IS_RED(s)    (s->ColorIndex == 2)

void PlayerExplodeAndLoseHealth(level_state *l, entity_world &w, entity *player, int health)
{
    l->DamageTimer = PLAYER_DAMAGE_TIMER;
    l->Health -= health;

    auto playerExp = CreateExplosionEntity(
        GetEntry(w.ExplosionPool),
        player->Pos(),
        player->Vel(),
        PLAYER_BODY_COLOR,
        PLAYER_OUTLINE_COLOR,
        vec3{0, 1, 1}
    );
    AddEntity(w, playerExp);
}

void PlayerEnemyCollision(level_state *l, entity_world &w,
                          entity *player, entity *enemy, int health)
{
    PlayerExplodeAndLoseHealth(l, w, player, health);
    player->Vel().y = std::min(PLAYER_MIN_VEL, enemy->Vel().y);

    auto enemyExp = CreateExplosionEntity(
        GetEntry(w.ExplosionPool),
        enemy->Pos(),
        enemy->Vel(),
        enemy->Ship->Color,
        enemy->Ship->OutlineColor,
        vec3{0,1,1}
    );
    AddEntity(w, enemyExp);
    RemoveEntity(w, enemy);
}

bool HandleCollision(game_main *g, entity *first, entity *second, float dt)
{
    auto l = &g->LevelState;
    auto shipEntity = FindEntityOfType(first, second, ColliderType_Ship).Found;
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
                AddScoreText(g, l, SCORE_TEXT_BLAST, SCORE_DESTROY_BLUE_SHIP, entityToExplode->Pos(), IntColor(ShipPalette.Colors[SHIP_BLUE]));
            }
            else if (SHIP_IS_ORANGE(entityToExplode->Ship))
            {
                PlayerEnemyCollision(l, g->World, otherEntity, entityToExplode, 25);
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
                PlayerEnemyCollision(l, g->World, otherEntity, entityToExplode, 25);
                return false;
            }
        }
        return false;
    }

    auto crystalEntity = FindEntityOfType(first, second, ColliderType_Crystal).Found;
    if (shipEntity != NULL && crystalEntity != NULL)
    {
        if (shipEntity->Flags & EntityFlag_IsPlayer)
        {
            l->Health += SCORE_HEALTH;
            AddScoreText(g, l, SCORE_TEXT_HEALTH, SCORE_HEALTH, crystalEntity->Pos(), CRYSTAL_COLOR);

            auto pup = CreatePowerupEntity(GetEntry(g->World.PowerupPool),
                                           g->LevelState.Entropy,
                                           g->LevelState.TimeElapsed,
                                           crystalEntity->Pos(),
                                           shipEntity->Vel(),
                                           CRYSTAL_COLOR);
            AddEntity(g->World, pup);
            RemoveEntity(g->World, crystalEntity);
        }
        return false;
    }

    auto asteroidEntity = FindEntityOfType(first, second, ColliderType_Asteroid).Found;
    if (shipEntity != NULL && asteroidEntity != NULL)
    {
        if (ENTITY_IS_PLAYER(shipEntity))
        {
            auto exp = CreateExplosionEntity(
                GetEntry(g->World.ExplosionPool),
                shipEntity->Pos(),
                vec3(0.0f),
                shipEntity->Ship->Color,
                shipEntity->Ship->OutlineColor,
                vec3{ 0, 1, 1 }
            );
            AddEntity(g->World, exp);
            PlayerExplodeAndLoseHealth(l, g->World, shipEntity, 50);
        }
        return false;
    }
    return false;
}

void UpdateListener(camera &cam, vec3 PlayerPosition)
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

void UpdateFreeCam(camera &cam, game_input &input, float dt)
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

void KeepEntityInsideOfRoad(entity *ent)
{
    const float limit = ROAD_LANE_COUNT * ROAD_LANE_WIDTH / 2;
    ent->Pos().x = glm::clamp(ent->Pos().x, -limit + 0.5f, limit - 0.5f);
}

void ResetGen(level_gen_params *p)
{
    p->Interval = INITIAL_SHIP_INTERVAL;
    p->Timer = p->Interval;
}

float GetNextTimer(level_gen_params *p, random_series random)
{
    if (p->Flags & LevelGenFlag_Randomize)
    {
        const float offset = p->RandomOffset;
        return p->Interval + RandomBetween(random, -offset, offset);
    }
    return p->Interval;
}

void UpdateGen(game_main *g, level_state *l, level_gen_params *p, float dt)
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
            p->Timer -= (p->Timer * 0.9f) * (g->World.PlayerEntity->Vel().y / PLAYER_MAX_VEL_LIMIT);
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

void SpawnCheckpoint(game_main *g, level_state *l)
{
    auto ent = CreateCheckpointEntity(GetEntry(g->World.CheckpointPool), GetCheckpointMesh(g->World));
    ent->Pos().y = g->World.PlayerEntity->Pos().y + GEN_PLAYER_OFFSET*2;
    ent->Pos().z = SHIP_Z * 0.5f;
    AddFlags(ent, EntityFlag_RemoveOffscreen);
    AddEntity(g->World, ent);
}

#define FrameSeconds(s) (s * 60)

void UpdateClassicMode(game_main *g, level_state *l)
{
    auto crystals = l->GenParams + LevelGenType_Crystal;
    auto ships = l->GenParams + LevelGenType_Ship;
    auto redShips = l->GenParams + LevelGenType_RedShip;
    auto asteroids = l->GenParams + LevelGenType_Asteroid;
    int frame = l->CurrentCheckpointFrame;
    switch (l->CheckpointNum)
    {
    case 0:
    {
        if (frame == 0)
        {
            AddIntroText(g, l, "COLLECT", CRYSTAL_COLOR);
        }
        if (frame == FrameSeconds(5))
        {
            Enable(crystals);
            Randomize(crystals);
        }
        if (frame == FrameSeconds(25))
        {
            Disable(crystals);
            SpawnCheckpoint(g, l);
        }
        break;
    }

    case 1:
    {
        if (frame == 0)
        {
            AddIntroText(g, l, "DRAFT & BLAST", IntColor(ShipPalette.Colors[SHIP_BLUE]));
        }
        if (frame == FrameSeconds(5))
        {
            l->ForceShipColor = SHIP_BLUE;
            Enable(ships);
        }
        if (frame == FrameSeconds(40))
        {
            Disable(ships);
            SpawnCheckpoint(g, l);
        }
        break;
    }

    case 2:
    {
        if (frame == 0)
        {
            AddIntroText(g, l, "DRAFT & MISS", IntColor(ShipPalette.Colors[SHIP_ORANGE]));
        }
        if (frame == FrameSeconds(5))
        {
            l->ForceShipColor = SHIP_ORANGE;
            Enable(ships);
        }
        if (frame == FrameSeconds(40))
        {
            Disable(ships);
            SpawnCheckpoint(g, l);
        }
        break;
    }

    case 3:
    {
        if (frame == 0)
        {
            AddIntroText(g, l, "DODGE", IntColor(ShipPalette.Colors[SHIP_RED]));
        }
        if (frame == FrameSeconds(5))
        {
            RemoveFlags(redShips, LevelGenFlag_ReserveLane);
            Enable(redShips);
        }
        if (frame == FrameSeconds(25))
        {
            Disable(redShips);
            SpawnCheckpoint(g, l);
        }
        break;
    }

    case 4:
    {
        if (frame == 0)
        {
            AddIntroText(g, l, "DRAFT & BLAST", IntColor(ShipPalette.Colors[SHIP_BLUE]));
        }
        if (frame == FrameSeconds(1))
        {
            AddIntroText(g, l, "DRAFT & MISS", IntColor(ShipPalette.Colors[SHIP_ORANGE]));
        }
        if (frame == FrameSeconds(2))
        {
            AddIntroText(g, l, "ASTEROIDS", ASTEROID_COLOR);
        }
        if (frame == FrameSeconds(5))
        {
            l->ForceShipColor = -1;
            Enable(ships);
            Enable(asteroids);
        }
        break;
    }
    }
    l->CurrentCheckpointFrame++;
}

void UpdateLevel(game_main *g, float dt)
{
    auto l = &g->LevelState;
    auto &updateTime = g->UpdateTime;
    auto &world = g->World;
    auto &input = g->Input;
    auto &cam = g->Camera;
    auto *playerEntity = g->World.PlayerEntity;
    auto *playerShip = playerEntity->Ship;
    updateTime.Begin = g->Platform.GetMilliseconds();
    
    if (IsJustPressed(g, Action_pause))
    {
        if (l->GameplayState == GameplayState_Playing)
        {
            l->GameplayState = GameplayState_Paused;
        }
        else
        {
            l->GameplayState = GameplayState_Playing;
        }
    }

    dt *= Global_Game_TimeSpeed;
    if (l->GameplayState != GameplayState_Paused)
    {
        l->PlayerMaxVel += PLAYER_MAX_VEL_INCREASE_FACTOR * dt;
        l->PlayerMaxVel = std::min(l->PlayerMaxVel, PLAYER_MAX_VEL_LIMIT);
        l->TimeElapsed += dt;
        l->DamageTimer -= dt;
        l->DamageTimer = std::max(l->DamageTimer, 0.0f);

        if (IsJustPressed(g, Action_debugPause))
        {
            l->CheckpointNum++;
            l->CurrentCheckpointFrame = 0;
        }

        if (Global_Camera_FreeCam)
        {
            UpdateFreeCam(cam, input, dt);
        }

        if (g->Input.Keys[SDL_SCANCODE_E])
        {
            auto exp = CreateExplosionEntity(GetEntry(g->World.ExplosionPool),
                                             playerEntity->Transform.Position,
                                             playerEntity->Transform.Velocity,
                                             playerEntity->Ship->Color,
                                             playerEntity->Ship->OutlineColor,
                                             vec3{ 0, 1, 1 });
            AddEntity(world, exp);
        }

        UpdateClassicMode(g, l);
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
                if (std::abs(dif) > ROAD_LANE_WIDTH)
                {
                    playerEntity->Vel().x += ROAD_LANE_WIDTH * glm::normalize(dif);
                }
                else if (std::abs(dif) > 0.05f)
                {
                    playerEntity->Vel().x += dif;
                }
            }

            if (l->DraftCharge == 1.0f && IsPressed(g, Action_boost))
            {
                l->Score += SCORE_DRAFT;
                l->CurrentDraftTime = 0.0f;
                l->DraftActive = true;
                if (l->DraftTarget->Checkpoint)
                {
                    if (l->DraftTarget->Checkpoint->State == CheckpointState_Initial)
                    {
                        l->DraftTarget->Checkpoint->State = CheckpointState_Drafted;
                    }
                    l->DraftTarget = NULL;
                }
                else if (l->DraftTarget->Ship)
                {
                    l->DraftTarget->Ship->HasBeenDrafted = true;
                    AddScoreText(g, l, SCORE_TEXT_DRAFT, SCORE_DRAFT, playerEntity->Pos(), l->DraftTarget->Ship->Color);
                }
                ApplyBoostToShip(playerEntity, DRAFT_BOOST, 0);
            }

            l->PlayerLaneIndex = int(nearestLane)+2;
        }

        size_t frameCollisionCount = 0;
        DetectCollisions(world.CollisionEntities, l->CollisionCache, frameCollisionCount);
        for (size_t i = 0; i < frameCollisionCount; i++)
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
            UpdateCameraToPlayer(g->Camera, playerEntity, dt);
        }

        if (l->DamageTimer > 0.0f)
        {
            int even = int(l->DamageTimer * 3);
            playerEntity->Model->Visible = (even % 2) == 1;
        }
        else
        {
            playerEntity->Model->Visible = true;
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
                    AddScoreText(g, l, SCORE_TEXT_MISS, SCORE_MISS_ORANGE_SHIP, ent->Pos(), IntColor(ShipPalette.Colors[SHIP_ORANGE]));
                }

                KeepEntityInsideOfRoad(ent);
            }
        }
        for (auto ent : world.PowerupEntities)
        {
            if (!ent) continue;

            vec3 distToPlayer = (playerEntity->Pos() + playerEntity->Vel()*dt) - ent->Pos();
            vec3 dirToPlayer = glm::normalize(distToPlayer);
            ent->SetVel(ent->Vel() + dirToPlayer * (1.0f/dt) * dt);
            ent->SetPos(ent->Pos() + ent->Vel() * dt);
            for (auto &meshPart : ent->Trail->Mesh.Parts)
            {
                float &alpha = meshPart.Material.DiffuseColor.a;
                alpha -= 1.0f * dt;
                alpha = std::max(alpha, 0.0f);
            }

            // TODO: temporary i hope
            if (ent->Pos().z < 0.0f)
            {
                RemoveEntity(g->World, ent);
            }
        }
        for (auto ent : world.AsteroidEntities)
        {
            if (!ent) continue;

            if (ent->Pos().z > 0.0f && !ent->Asteroid->Exploded)
            {
                ent->Vel().z -= 80.0f * dt;
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
        for (auto ent : world.CheckpointEntities)
        {
            if (!ent) continue;

            auto cp = ent->Checkpoint;
            switch (cp->State)
            {
            case CheckpointState_Initial:
                if (ent->Pos().y - playerEntity->Pos().y >= 20.0f + (0.1f * playerEntity->Vel().y))
                {
                    ent->Vel().y = playerEntity->Vel().y * 0.2f;
                }
                else
                {
                    ent->Vel().y = playerEntity->Vel().y * 0.8f;
                }

                if (ent->Pos().y < playerEntity->Pos().y)
                {
                    cp->State = CheckpointState_Active;
                    for (auto &mat : ent->Model->Materials)
                    {
                        mat->Emission = 1.0f;
                        mat->DiffuseColor = CHECKPOINT_OUTLINE_COLOR;
                    }

                    l->CheckpointNum++;
                    l->CurrentCheckpointFrame = 0;
                    PlayerExplodeAndLoseHealth(l, g->World, playerEntity, 25);
                }
                break;

            case CheckpointState_Drafted:
                if (ent->Pos().y < playerEntity->Pos().y)
                {
                    cp->State = CheckpointState_Active;
                    for (auto &mat : ent->Model->Materials)
                    {
                        mat->Emission = 1.0f;
                        mat->DiffuseColor = CHECKPOINT_OUTLINE_COLOR;
                    }

                    l->CheckpointNum++;
                    l->CurrentCheckpointFrame = 0;
                    l->Score += SCORE_CHECKPOINT;
                    AddScoreText(g, l, SCORE_TEXT_CHECKPOINT, SCORE_CHECKPOINT, ent->Pos(), IntColor(ShipPalette.Colors[SHIP_BLUE]));
                }
                break;

            case CheckpointState_Active:
            {
                ent->Vel().y = playerEntity->Vel().y * 2.0f;
                if (cp->Timer >= CHECKPOINT_FADE_OUT_DURATION)
                {
                    RemoveEntity(g->World, ent);
                }
                else
                {
                    float alpha = CHECKPOINT_FADE_OUT_DURATION - cp->Timer;
                    for (auto &material : ent->Model->Materials)
                    {
                        material->DiffuseColor.a = alpha;
                    }
                    for (auto &meshPart : ent->Trail->Mesh.Parts)
                    {
                        meshPart.Material.DiffuseColor.a = alpha;
                    }
                }

                cp->Timer += dt;
                break;
            }
            }

            ent->Pos().y += ent->Vel().y * dt;
        }
        
        l->Health = std::max(l->Health, 0);
        if (l->Health == 0 && l->GameplayState == GameplayState_Playing)
        {
            RemoveEntity(g->World, playerEntity);
            l->GameplayState = GameplayState_GameOver;
        }
    }
    else
    {
        float moveY = GetMenuAxisValue(g->Input, g->GUI.VerticalAxis, dt);
        UpdateMenuSelection(g, pauseMenu, moveY);
    }

    auto playerPosition = playerEntity->Pos();
    alSourcefv(l->DraftBoostAudio->Source, AL_POSITION, &playerPosition[0]);
    UpdateListener(g->Camera, playerPosition);

    auto scoreText = l->ScoreTextList.front();
    if (scoreText && scoreText->Sequence->Complete)
    {
        l->ScoreTextList.pop_front();
        DestroySequences(g->TweenState, scoreText->Sequence, 1);
        FreeEntryFromData(l->SequencePool, scoreText->Sequence);
        FreeEntryFromData(l->ScoreTextPool, scoreText);
    }

    auto introText = l->IntroTextList.front();
    if (introText && introText->Sequence->Complete)
    {
        l->IntroTextList.pop_front();
        DestroySequences(g->TweenState, introText->Sequence, 1);
        if (introText->PosSequence)
        {
            DestroySequences(g->TweenState, introText->PosSequence, 1);
            FreeEntryFromData(l->SequencePool, introText->PosSequence);
        }
        FreeEntryFromData(l->SequencePool, introText->Sequence);
        FreeEntryFromData(l->IntroTextPool, introText);
    }

    updateTime.End = g->Platform.GetMilliseconds();
}

void PauseMenuCallback(game_main *g, menu_data *menu, int itemIndex)
{
    auto l = &g->LevelState;
    switch (itemIndex)
    {
    case 0:
    {
        l->GameplayState = GameplayState_Playing;
        break;
    }
    
    case 1:
    {
        Println("EXIT GAME");
        break;
    }
    }
}

void RenderLevel(game_main *g, float dt)
{
    auto l = &g->LevelState;
    auto &renderTime = g->RenderTime;
    auto &world = g->World;
    static auto titleFont = FindBitmapFont(g->AssetLoader, "unispace_48");

    renderTime.Begin = g->Platform.GetMilliseconds();
    UpdateProjectionView(g->Camera);
    PostProcessBegin(g->RenderState);
    RenderBackground(g, g->World);
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
    UpdateFinalCamera(g);
    RenderEnd(g->RenderState, g->FinalCamera);

    const float fontSize = GetRealPixels(g, 24);
    static auto hudFont = FindBitmapFont(g->AssetLoader, "unispace_24");
    static auto textFont = FindBitmapFont(g->AssetLoader, "unispace_32");

    Begin(g->GUI, g->GUICamera, 1.0f);
    for (auto text : l->IntroTextList)
    {
        vec2 p = text->Pos;
        DrawTextCentered(g->GUI, textFont, text->Text, rect{p.x,p.y,0,0}, text->Color);
    }
    for (auto text : l->ScoreTextList)
    {
        vec2 p = text->Pos;
        p += (text->TargetPos - p) * text->TweenPosValue;
        text->Color.a = text->TweenAlphaValue;
        DrawTextCentered(g->GUI, textFont, Format(l->ScoreNumberFormat, text->Text, text->Score), rect{p.x, p.y, 0, 0}, text->Color);
    }
    End(g->GUI);
    PostProcessEnd(g->RenderState);

    Begin(g->GUI, g->GUICamera);
    DrawRect(g->GUI, rect{20,20,200,20},
             IntColor(FirstPalette.Colors[2]), GL_LINE_LOOP, false);

    DrawRect(g->GUI, rect{25,25,190 * l->DraftCharge,10},
             IntColor(FirstPalette.Colors[3]), GL_TRIANGLES, false);

    float left = GetRealPixels(g, 10.0f);
    float top = g->Height - GetRealPixels(g, 10.0f);
    DrawText(g->GUI, hudFont, Format(l->ScoreFormat, l->Score), rect{left, top - fontSize, 0, 0}, Color_white);
    DrawText(g->GUI, hudFont, Format(l->HealthFormat, l->Health), rect{left + 400, top - fontSize, 0, 0}, Color_white);
    
    if (l->GameplayState == GameplayState_Paused)
    {
        DrawMenu(g, pauseMenu, g->GUI.MenuChangeTimer, false);
    }
    
    End(g->GUI);

    renderTime.End = g->Platform.GetMilliseconds();
}
