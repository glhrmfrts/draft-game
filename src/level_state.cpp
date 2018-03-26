// Copyright

#define SCORE_TEXT_CHECKPOINT "CHECKPOINT"
#define SCORE_TEXT_DRAFT      "DRAFT"
#define SCORE_TEXT_BLAST      "BLAST"
#define SCORE_TEXT_MISS       "MISS"
#define SCORE_TEXT_HEALTH     "HEALTH"

#define SCORE_CHECKPOINT        20
#define SCORE_DESTROY_BLUE_SHIP 10
#define SCORE_MISS_ORANGE_SHIP  10
#define SCORE_DRAFT             5
#define SCORE_HEALTH            2

// TODO(20/11/2017): asteroids can stay in the game, but not with high velocity

MENU_FUNC(PauseMenuCallback);
MENU_FUNC(GameOverMenuCallback);

static menu_data pauseMenu = {
    "PAUSED", 2, PauseMenuCallback,
    {
        {"CONTINUE", MenuItemType_Text, true},
        {"EXIT", MenuItemType_Text, true},
    }
};

static menu_data gameOverMenu = {
    "GAME OVER", 3, GameOverMenuCallback,
    {
        {"CONTINUE", MenuItemType_Text, true },
        {"RESTART", MenuItemType_Text, true},
        {"EXIT", MenuItemType_Text, true},
    }
};

static song *firstSong;

// sets the level and it's initial state
void ResetLevelState(game_main *g, level_state *l, const std::string &levelNumber)
{
	g->State = GameState_Level;

    l->Health = 50;
    l->CheckpointNum = 0;
    l->CurrentCheckpointFrame = 0;
    l->GameplayState = GameplayState_Playing;
    l->Level = FindLevel(g->AssetLoader, levelNumber);
    ResetLevel(l->Level);

    l->PlayerMinVel = l->Level->Checkpoints[0].PlayerMinVel;
    l->PlayerMaxVel = l->Level->Checkpoints[0].PlayerMaxVel;

    g->MusicMaster.StepBeat = true;

    auto gen = g->World.GenState->GenParams + GenType_SideTrail;
    gen->Flags |= GenFlag_Enabled;

	// get sounds
	l->DraftBoostSound = FindSound(g->AssetLoader, "boost");

	// get fonts to gui
	static auto smallFont = FindBitmapFont(g->AssetLoader, "vcr_16");
	static auto textFont = FindBitmapFont(g->AssetLoader, "unispace_32");
	l->ScoreTextGroup.Items = {
		text_group_item(Color_white, textFont),
		text_group_item(Color_white, smallFont)
	};

	l->TargetTextGroup.Items = {
		text_group_item(Color_white, textFont),
		text_group_item(Color_white, smallFont)
	};

	auto ent = PushStruct<entity>(l->Arena);
	ent->Pos().x = 0;
	ent->Pos().y = 250;
	ent->Pos().z = -0;
	ent->SetScl(vec3(0.05f));
	ent->SetRot(vec3(0, 0, -90));
	ent->Model = CreateModel(&l->Arena, FindMesh(g->AssetLoader, "deer"));
	AddEntity(g->World, ent);
}

void RemoveGameplayEntities(entity_world &w)
{
	for (auto ent : w.CheckpointEntities)
	{
		if (!ent) continue;
		RemoveEntity(w, ent);
	}
	for (auto ent : w.AsteroidEntities)
	{
		if (!ent) continue;
		RemoveEntity(w, ent);
	}
	for (auto ent : w.PowerupEntities)
	{
		if (!ent) continue;
		RemoveEntity(w, ent);
	}
	for (auto ent : w.ShipEntities)
	{
		if (!ent) continue;
		RemoveEntity(w, ent);
	}
	for (auto ent : w.CrystalEntities)
	{
		if (!ent) continue;
		RemoveEntity(w, ent);
	}
	for (auto ent : w.FinishEntities)
	{
		if (!ent) continue;
		RemoveEntity(w, ent);
	}
}

void CleanupLevel(game_main *g, level_state *l)
{
	DestroySequence(g->TweenState, l->StatsScreenSequence);
	DestroySequence(g->TweenState, l->GameOverMenuSequence);
	DestroySequence(g->TweenState, l->ChangeLevelSequence);
	DestroySequence(g->TweenState, l->ExitSequence);
}

void InitLevelState(game_main *g, level_state *l)
{
    // clean memory
    FreeArena(l->Arena);
    ResetPool(l->IntroTextPool);
    ResetPool(l->ScoreTextPool);
	ResetPool(l->TrackArgsPool);

    l->AssetLoader = &g->AssetLoader;
    l->Entropy = RandomSeed(g->Platform.GetMilliseconds());
    l->IntroTextPool.Arena = &l->Arena;
    l->ScoreTextPool.Arena = &l->Arena;
    l->TrackArgsPool.Arena = &l->Arena;
    g->Gravity = vec3(0, 0, 0);
    g->World.Camera = &g->Camera;

    // init string formats
    InitFormat(&l->ScorePercentFormat, "%s: %d%%\n", 24, &l->Arena);
    InitFormat(&l->ScoreRatioFormat, " (%d / %d)", 24, &l->Arena);
    InitFormat(&l->ScoreNumberFormat, "%s +%d", 16, &l->Arena);

    // create sequences
	l->GameOverMenuSequence = CreateSequence(g->TweenState);
    l->GameOverMenuSequence->Tweens.push_back(
        tween(&l->GameOverAlpha)
            .SetFrom(0.0f)
            .SetTo(1.0f)
            .SetDuration(1.0f)
            .SetEasing(TweenEasing_Linear)
    );

    l->StatsScreenSequence = CreateSequence(g->TweenState);
	l->StatsScreenSequence->Tweens.push_back(CallbackTween([l]()
	{ 
		l->StatsAlpha[0] = 0;
		l->StatsAlpha[1] = 0;
		l->StatsAlpha[2] = 0;
		l->DrawStats = true;
	}));
    l->StatsScreenSequence->Tweens.push_back(WaitTween(1.0f));
    l->StatsScreenSequence->Tweens.push_back(
        tween(l->StatsAlpha)
            .SetFrom(0.0f)
            .SetTo(1.0f)
            .SetDuration(FAST_TWEEN_DURATION)
            .SetEasing(TweenEasing_Linear)
    );
    l->StatsScreenSequence->Tweens.push_back(
        tween(l->StatsAlpha + 1)
            .SetFrom(0.0f)
            .SetTo(1.0f)
            .SetDuration(FAST_TWEEN_DURATION)
            .SetEasing(TweenEasing_Linear)
    );
    l->StatsScreenSequence->Tweens.push_back(
        tween(l->StatsAlpha + 2)
            .SetFrom(0.0f)
            .SetTo(1.0f)
            .SetDuration(FAST_TWEEN_DURATION)
            .SetEasing(TweenEasing_Linear)
    );

    l->ChangeLevelSequence = CreateSequence(g->TweenState);
    l->ChangeLevelSequence->Tweens.push_back(CallbackTween([l](){ l->GameplayState = GameplayState_ChangingLevel; }));
    l->ChangeLevelSequence->Tweens.push_back(FadeInTween(&g->ScreenRectAlpha, FAST_TWEEN_DURATION).SetCallback([g, l]()
        {
			l->DrawStats = false;
            RemoveGameplayEntities(g->World);
			ResetLevelState(g, l, l->Level->Next);
            ResetRoadPieces(g->World, g->World.PlayerEntity->Pos().y);
        }));
    l->ChangeLevelSequence->Tweens.push_back(WaitTween(FAST_TWEEN_DURATION));
    l->ChangeLevelSequence->Tweens.push_back(FadeOutTween(&g->ScreenRectAlpha, FAST_TWEEN_DURATION).SetCallback([l](){ l->GameplayState = GameplayState_Playing; }));

	l->ExitSequence = CreateSequence(g->TweenState);
	l->ExitSequence->Tweens.push_back(FadeInTween(&g->ScreenRectAlpha, FAST_TWEEN_DURATION).SetCallback([g, l]()
	{
		RemoveGameplayEntities(g->World);
		ResetMenuState(g);
	}));
}

void RestartLevel(game_main *g)
{
    g->LevelState.Health = 50;
    RemoveGameplayEntities(g->World);
    AddEntity(g->World, g->World.PlayerEntity);
    InitLevelState(g, &g->LevelState);
}

void AddIntroText(game_main *g, level_state *l, const char *text, color c)
{
    auto introText = PushStruct<level_intro_text>(GetEntry(l->IntroTextPool));
    introText->Color = color{c.r, c.g, c.b, 0.0f};
    introText->Text = text;
    introText->Pos = vec2{g->Width*0.5f, g->Height*0.75f};

	auto seq = CreateSequence(g->TweenState);
	seq->OneShot = true;
    if (l->IntroTextList.size() > 0)
    {
        for (auto ctext : l->IntroTextList)
        {
			auto cseq = CreateSequence(g->TweenState);
			cseq->OneShot = true;
            cseq->Tweens.push_back(tween(&ctext->Pos.y)
                                   .SetFrom(ctext->Pos.y)
                                   .SetTo(ctext->Pos.y - GetRealPixels(g, 60.0f))
                                   .SetDuration(0.5f));

            ctext->PosSequence = cseq;
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
    PlaySequence(g->TweenState, seq, true);

    l->IntroTextList.push_back(introText);
}

void AddScoreText(game_main *g, level_state *l, const char *text, int score, vec3 pos, color c)
{
    vec4 v = g->FinalCamera.ProjectionView * vec4{WorldToRenderTransform(g->World, pos, g->RenderState.BendRadius), 1.0f};
    vec2 screenPos = vec2{v.x/v.w, v.y/v.w};
    screenPos.x = (g->Width/2) * screenPos.x + (g->Width/2);
    screenPos.y = (g->Height/2) * screenPos.y + (g->Height/2);

    float offsetX = GetRealPixels(g, 100.0f);
    float minOffsetY = GetRealPixels(g, 200.0f);
    float maxOffsetY = GetRealPixels(g, 300.0f);
    vec2 targetPos = vec2{RandomBetween(l->Entropy, -offsetX, offsetX),
                          RandomBetween(l->Entropy, -minOffsetY, -maxOffsetY)};

	auto seq = CreateSequence(g->TweenState);
	seq->OneShot = true;

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
		*l->AssetLoader,
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
    player->Vel().y = std::min(PLAYER_MIN_VEL, player->Vel().y);

    auto enemyExp = CreateExplosionEntity(
        GetEntry(w.ExplosionPool),
		*l->AssetLoader,
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
                return false;
            }
            else if (ENTITY_IS_PLAYER(otherEntity) && SHIP_IS_RED(entityToExplode->Ship))
            {
                PlayerEnemyCollision(l, g->World, otherEntity, entityToExplode, 50);
                return false;
            }

            auto exp = CreateExplosionEntity(
                GetEntry(g->World.ExplosionPool),
				*l->AssetLoader,
                entityToExplode->Transform.Position,
                otherEntity->Transform.Velocity,
                entityToExplode->Ship->Color,
                entityToExplode->Ship->OutlineColor,
                vec3{ 0, 1, 1 },
				entityToExplode->LaneSlot->Lane
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

			Println(crystalEntity->LaneSlot->Index);
			AudioSourceSetPitch(crystalEntity->AudioSource, LaneIndexToPitch(crystalEntity->LaneSlot->Index));
			AudioSourcePlay(crystalEntity->AudioSource);

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
				*l->AssetLoader,
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

void SpawnCheckpoint(game_main *g, level_state *l)
{
    auto ent = CreateCheckpointEntity(GetEntry(g->World.CheckpointPool), g->AssetLoader, GetCheckpointMesh(g->World));
    ent->Pos().y = g->World.PlayerEntity->Pos().y + GEN_PLAYER_OFFSET;
    ent->Pos().z = SHIP_Z * 0.5f;
    AddFlags(ent, EntityFlag_RemoveOffscreen);
    AddEntity(g->World, ent);
}

void SpawnFinish(game_main *g, level_state *l)
{
	g->World.ShouldSpawnFinish = true;
	g->World.OnSpawnFinish = [g]()
	{
		auto ent = CreateFinishEntity(GetEntry(g->World.FinishPool), g->AssetLoader, GetFinishMesh(g->World));
		ent->Pos().y = g->World.PlayerEntity->Pos().y + GEN_PLAYER_OFFSET + ROAD_SEGMENT_SIZE*1.5f;
		ent->Pos().z = SHIP_Z * 0.5f;
		AddFlags(ent, EntityFlag_RemoveOffscreen);
		AddEntity(g->World, ent);
	};
}

void RoadTangent(game_main *g, level_state *l)
{
    g->World.ShouldRoadTangent = true;
}

track_args *GetTrackArg(game_main *g, level_state *l, hash_string::result_type trackHash)
{
	auto entry = GetEntry(l->TrackArgsPool);
	auto result = PushStruct<track_args>(entry);
	result->Game = g;
	result->TrackHash = trackHash;
	return result;
}

static void PlayTrackFunc(void *arg)
{
	auto args = (track_args *)arg;
	auto sng = args->Game->LevelState.Level->Song;
	MusicMasterPlayTrack(args->Game->MusicMaster, sng->Names[args->TrackHash]);
}

void PlayTrack(game_main *g, level_state *l, hash_string::result_type trackHash, int divisor)
{
	if (divisor > 0)
	{
		auto arg = GetTrackArg(g, l, trackHash);
		MusicMasterOnNextBeat(g->MusicMaster, PlayTrackFunc, (void *)arg, divisor);
	}
	else
	{
		MusicMasterPlayTrack(g->MusicMaster, l->Level->Song->Names[trackHash]);
	}
}

static void StopTrackFunc(void *arg)
{
	auto args = (track_args *)arg;
	auto sng = args->Game->LevelState.Level->Song;
	MusicMasterStopTrack(args->Game->MusicMaster, sng->Names[args->TrackHash]);
}

void StopTrack(game_main *g, level_state *l, hash_string::result_type trackHash, int divisor)
{
	if (divisor > 0)
	{
		auto arg = GetTrackArg(g, l, trackHash);
		MusicMasterOnNextBeat(g->MusicMaster, StopTrackFunc, (void *)arg, divisor);
	}
	else
	{
		MusicMasterStopTrack(g->MusicMaster, l->Level->Song->Names[trackHash]);
	}
}

static void LevelStateToStats(game_main *g, level_state *l)
{
	l->GameplayState = GameplayState_Stats;
	PlaySequence(g->TweenState, l->StatsScreenSequence);
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

	static float myPitch = 1.0f;
	float targetPitch = std::max(std::round(playerEntity->Vel().y) / 50.0f * 0.7f, 1.0f);
	targetPitch = std::min(targetPitch, 2.5f*0.7f);

	myPitch = Interp(myPitch, targetPitch, 0.5f, dt);
	MusicMasterSetPitch(g->MusicMaster, myPitch);

    if (IsJustPressed(g, Action_pause))
    {
        if (l->GameplayState == GameplayState_Playing)
        {
            l->GameplayState = GameplayState_Paused;
        }
        else if (l->GameplayState == GameplayState_Paused)
        {
            l->GameplayState = GameplayState_Playing;
        }
    }
	if (IsJustPressed(g, Action_select))
	{
		if (l->GameplayState == GameplayState_Stats)
		{
            if (!l->ChangeLevelSequence->Active)
            {
                PlaySequence(g->TweenState, l->ChangeLevelSequence);
            }
			return;
		}
	}

    dt *= Global_Game_TimeSpeed;
    if (l->GameplayState != GameplayState_Paused)
    {
        if (false/*is infinite mode*/)
        {
            l->PlayerMaxVel += PLAYER_MAX_VEL_INCREASE_FACTOR * dt;
            l->PlayerMaxVel = std::min(l->PlayerMaxVel, PLAYER_MAX_VEL_LIMIT);
        }

        l->TimeElapsed += dt;
        l->DamageTimer -= dt;
        l->DamageTimer = std::max(l->DamageTimer, 0.0f);

        if (IsJustPressed(g, Action_debugPause))
        {
            l->CheckpointNum++;
            l->CurrentCheckpointFrame = 0;
        }

#ifdef DRAFT_DEBUG
        if (Global_Camera_FreeCam)
        {
            UpdateFreeCam(cam, input, dt);
        }
        if (g->Input.Keys[SDL_SCANCODE_E])
        {
            auto exp = CreateExplosionEntity(GetEntry(g->World.ExplosionPool),
											*l->AssetLoader,
                                             playerEntity->Transform.Position,
                                             playerEntity->Transform.Velocity,
                                             playerEntity->Ship->Color,
                                             playerEntity->Ship->OutlineColor,
                                             vec3{ 0, 1, 1 });
            AddEntity(world, exp);
        }
        if (g->Input.Keys[SDL_SCANCODE_R])
        {
            RestartLevel(g);
        }
#endif

        if (l->GameplayState == GameplayState_Playing || l->GameplayState == GameplayState_Stats)
        {
            //UpdateClassicMode(g, l);
			LevelUpdate(l->Level, g, l, dt);
        }

		BeginProfileTimer("Gen state");
        UpdateGenState(g, world.GenState, (void *)l, dt);
		EndProfileTimer("Gen state");

        {
            // player movement
            float moveX = GetAxisValue(input, Action_horizontal);
            float moveY = GetAxisValue(input, Action_vertical);
            if (moveY > 0.0f)
            {
                moveY = 0.0f;
            }
            MoveShipEntity(playerEntity, moveX, moveY, l->PlayerMinVel, l->PlayerMaxVel, dt);

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

			// check if player is out of bounds
			float playerRight = playerX + ROAD_LANE_WIDTH / 2;
			float playerLeft = playerX - ROAD_LANE_WIDTH / 2;
			if (playerRight < world.RoadState.Left*ROAD_LANE_WIDTH || playerLeft > world.RoadState.Right*ROAD_LANE_WIDTH)
			{
				playerEntity->Vel().y = std::min(PLAYER_MIN_VEL, playerEntity->Vel().y);
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
				AudioSourcePlay(playerEntity->AudioSource, l->DraftBoostSound);
            }

            world.GenState->PlayerLaneIndex = int(nearestLane)+2;
        }

		BeginProfileTimer("Collision");
		{
			size_t frameCollisionCount = 0;
			DetectCollisions(world.ActiveCollisionEntities, world.PassiveCollisionEntities, l->CollisionCache, &frameCollisionCount);
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

			Integrate(world.ActiveCollisionEntities, g->Gravity, dt);
			Integrate(world.PassiveCollisionEntities, g->Gravity, dt);
			if (l->NumTrailCollisions == 0)
			{
				l->CurrentDraftTime -= dt;
			}
		}
		EndProfileTimer("Collision");

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

		//BeginProfileTimer(g->Platform.GetMilliseconds(), "World Entities");
        UpdateLogiclessEntities(world, dt);
		//EndProfileTimer(g->Platform.GetMilliseconds(), "World Entities");

		BeginProfileTimer("Game Entities");
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
            for (auto &meshPart : ent->TrailGroup->Mesh.Parts)
            {
                float &alpha = meshPart.Material->DiffuseColor.a;
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
						                             *l->AssetLoader,
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
					l->CurrentCheckpointFrame = -1;
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
					l->CurrentCheckpointFrame = -1;
                    l->Score += SCORE_CHECKPOINT;
                    AddScoreText(g, l, SCORE_TEXT_CHECKPOINT, SCORE_CHECKPOINT, ent->Pos(), IntColor(ShipPalette.Colors[SHIP_BLUE]));

					AudioSourcePlay(ent->AudioSource);
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
                    for (auto &meshPart : ent->TrailGroup->Mesh.Parts)
                    {
                        meshPart.Material->DiffuseColor.a = alpha;
                    }
                }

                cp->Timer += dt;
                break;
            }
            }

            ent->Pos().y += ent->Vel().y * dt;
        }
		for (auto ent : world.FinishEntities)
		{
			if (!ent) continue;

			if (ent->Finish->Finished)
			{
				ent->Pos().y = g->Camera.Position.y + 1;
			}
			else if (g->Camera.Position.y + 5.0f >= ent->Pos().y)
			{
				ent->Finish->Finished = true;
				LevelStateToStats(g, l);
			}
		}
		for (auto ent : world.EnemySkullEntities)
		{
			if (!ent) continue;

			// horizontal movement
			if (ent->Vel().x > 0 && ent->Pos().x-1.0f > g->World.RoadState.Right)
			{
				ent->Vel().x = -ent->Vel().x;
			}
			else if (ent->Vel().x < 0 && ent->Pos().x+1.0f < g->World.RoadState.Left)
			{
				ent->Vel().x = -ent->Vel().x;
			}

			ent->Vel().y = -PLAYER_MIN_VEL * 0.5f;
		}
		EndProfileTimer("Game Entities");

        l->Health = std::max(l->Health, 0);
        if (l->Health == 0 && l->GameplayState == GameplayState_Playing)
        {
            RemoveEntity(g->World, playerEntity);
            l->GameplayState = GameplayState_GameOver;
            PlaySequence(g->TweenState, l->GameOverMenuSequence, true);
        }
    }

    float menuMoveY = GetMenuAxisValue(g->Input, g->GUI.VerticalAxis, dt);
    if (l->GameplayState == GameplayState_Paused)
    {
        UpdateMenuSelection(g, pauseMenu, 0, 0, menuMoveY);
    }
    if (l->GameplayState == GameplayState_GameOver)
    {
        UpdateMenuSelection(g, gameOverMenu, 0, 0, menuMoveY);
    }

    AudioListenerUpdate(g->Camera);

	if (l->ScoreTextList.size() > 0)
	{
		auto scoreText = l->ScoreTextList.front();
		if (scoreText && scoreText->Sequence->Complete)
		{
			l->ScoreTextList.pop_front();
			FreeEntryFromData(l->ScoreTextPool, scoreText);
		}
	}

	if (l->IntroTextList.size() > 0)
	{
		auto introText = l->IntroTextList.front();
		if (introText && introText->Sequence->Complete)
		{
			l->IntroTextList.pop_front();
			FreeEntryFromData(l->IntroTextPool, introText);
		}
	}

	if (l->GameplayState == GameplayState_Playing)
	{
		l->CurrentCheckpointFrame++;
	}

#if 0
	BeginProfileTimer(g->Platform.GetMilliseconds(), "Update thread wait");
	WaitUpdate(g->World);
	EndProfileTimer(g->Platform.GetMilliseconds(), "Update thread wait");
#endif

    updateTime.End = g->Platform.GetMilliseconds();
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

#if 0
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
        DrawTextCentered(g->GUI, textFont, Format(&l->ScoreNumberFormat, text->Text, text->Score), rect{p.x, p.y, 0, 0}, text->Color);
    }
    End(g->GUI);
    PostProcessEnd(g->RenderState);

    Begin(g->GUI, g->GUICamera);

    switch (l->GameplayState)
    {
    case GameplayState_Playing:
        DrawRect(g->GUI, rect{20,20,200,20},
                 IntColor(FirstPalette.Colors[2]), GL_LINE_LOOP, false);

        DrawRect(g->GUI, rect{25,25,190 * l->DraftCharge,10},
                 IntColor(FirstPalette.Colors[3]), GL_TRIANGLES, false);
        break;

    case GameplayState_Paused:
        DrawMenu(g, pauseMenu, g->GUI.MenuChangeTimer, 1.0f, false);
        break;

    case GameplayState_GameOver:
        DrawMenu(g, gameOverMenu, g->GUI.MenuChangeTimer, l->GameOverAlpha, false);
        break;
    }

	if (l->DrawStats)
	{
		DrawHeader(g, "STATS", Color_black, l->StatsAlpha[0]);
		{
			rect container;
			DrawContainerPolygon(g, Color_black * 0.75f * l->StatsAlpha[1], container);

			l->ScoreTextGroup.Items[0].Text = Format(&l->ScorePercentFormat, "SCORE", int(l->Score / (float)1500 * 100));
			l->ScoreTextGroup.Items[1].Text = Format(&l->ScoreRatioFormat, l->Score, 1500);
			DrawTextGroupCentered(g->GUI, &l->ScoreTextGroup, rect{ g->Width*0.5f, g->Height*0.6f, 0, 0 }, color{ 1, 1, 1, l->StatsAlpha[1] });

			l->TargetTextGroup.Items[0].Text = Format(&l->ScorePercentFormat, "TARGET", int(1200 / (float)1500 * 100));
			l->TargetTextGroup.Items[1].Text = "(0)";
			DrawTextGroupCentered(g->GUI, &l->TargetTextGroup, rect{ g->Width*0.5f, g->Height*0.525f, 0, 0 }, color{ 1, 1, 1, l->StatsAlpha[1] });
		}
	}

    End(g->GUI);

    renderTime.End = g->Platform.GetMilliseconds();
}

MENU_FUNC(PauseMenuCallback)
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
		PlaySequence(g->TweenState, l->ExitSequence);
        break;
    }
    }
}

MENU_FUNC(GameOverMenuCallback)
{
    auto l = &g->LevelState;
    switch (itemIndex)
    {
    case 0:
    {
        // continue from last checkpoint
        l->CurrentCheckpointFrame = 0;
        l->GameplayState = GameplayState_Playing;
        l->Health = 50;
        RemoveGameplayEntities(g->World);
        AddEntity(g->World, g->World.PlayerEntity);
        break;
    }

    case 1:
    {
        // restart from the beginning
        RestartLevel(g);
        break;
    }

    case 2:
    {
		PlaySequence(g->TweenState, l->ExitSequence);
        break;
    }
    }
}
