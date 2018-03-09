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
        menu_item{"CONTINUE", MenuItemType_Text},
        menu_item{"EXIT", MenuItemType_Text},
    }
};

static menu_data gameOverMenu = {
    "GAME OVER", 3, GameOverMenuCallback,
    {
        menu_item{"CONTINUE", MenuItemType_Text},
        menu_item{"RESTART", MenuItemType_Text},
        menu_item{"EXIT", MenuItemType_Text},
    }
};

static song *firstSong;

void InitLevel(game_main *g)
{
    g->State = GameState_Level;

    auto l = &g->LevelState;
    FreeArena(l->Arena);
    ResetPool(l->IntroTextPool);
    ResetPool(l->ScoreTextPool);
    ResetPool(l->SequencePool);
    
	l->AssetLoader = &g->AssetLoader;
    l->Entropy = RandomSeed(g->Platform.GetMilliseconds());
    l->IntroTextPool.Arena = &l->Arena;
    l->ScoreTextPool.Arena = &l->Arena;
    l->SequencePool.Arena = &l->Arena;
    l->GameOverMenuSequence = PushStruct<tween_sequence>(GetEntry(l->SequencePool));
    l->GameOverMenuSequence->Tweens.push_back(
        tween(&l->GameOverAlpha)
            .SetFrom(0.0f)
            .SetTo(1.0f)
            .SetDuration(1.0f)
            .SetEasing(TweenEasing_Linear)
    );
    AddSequences(g->TweenState, l->GameOverMenuSequence, 1);

    g->Gravity = vec3(0, 0, 0);
    g->World.Camera = &g->Camera;
	l->DraftBoostSound = FindSound(g->AssetLoader, "boost");

    InitFormat(&l->HealthFormat, "Health: %d\n", 24, &l->Arena);
    InitFormat(&l->ScoreFormat, "Score: %d\n", 24, &l->Arena);
    InitFormat(&l->ScoreNumberFormat, "%s +%d", 16, &l->Arena);
    
    l->Health = 50;
    l->CheckpointNum = 0;
    l->CurrentCheckpointFrame = 0;
    l->GameplayState = GameplayState_Playing;

	firstSong = FindSong(g->AssetLoader, "first_song");
	g->MusicMaster.StepBeat = true;
	MusicMasterPlayTrack(g->MusicMaster, firstSong->Names["hats"]);
	MusicMasterPlayTrack(g->MusicMaster, firstSong->Names["pad_chords"]);
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
}

void RestartLevel(game_main *g)
{
    g->LevelState.Health = 50;
    RemoveGameplayEntities(g->World);
    AddEntity(g->World, g->World.PlayerEntity);
    InitLevel(g);
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

#define FrameSeconds(s) (s * 60)

void PlayKick(void *arg)
{
	auto m = (music_master *)arg;
	MusicMasterPlayTrack(*m, firstSong->Names["kick"]);
}

void PlaySnare(void *arg)
{
	auto m = (music_master *)arg;
	MusicMasterPlayTrack(*m, firstSong->Names["snare"]);
}

void UpdateClassicMode(game_main *g, level_state *l)
{
    auto crystals = g->World.GenState->GenParams + GenType_Crystal;
    auto ships = g->World.GenState->GenParams + GenType_Ship;
    auto redShips = g->World.GenState->GenParams + GenType_RedShip;
    auto asteroids = g->World.GenState->GenParams + GenType_Asteroid;
    int frame = l->CurrentCheckpointFrame;
    switch (l->CheckpointNum)
    {
    case 0:
    {
        //l->PlayerMinVel = PLAYER_MIN_VEL;
        //l->PlayerMaxVel = PLAYER_MIN_VEL + 30.0f;
		l->PlayerMinVel = PLAYER_MIN_VEL + 50.0f;
		l->PlayerMaxVel = PLAYER_MIN_VEL + 100.0f;
        
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
		l->PlayerMinVel = PLAYER_MIN_VEL + 50.0f;
		l->PlayerMaxVel = PLAYER_MIN_VEL + 100.0f;

        if (frame == 0)
        {
            AddIntroText(g, l, "DRAFT & BLAST", IntColor(ShipPalette.Colors[SHIP_BLUE]));
			MusicMasterOnNextBeat(g->MusicMaster, PlayKick, &g->MusicMaster, 4);
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
			MusicMasterOnNextBeat(g->MusicMaster, PlaySnare, &g->MusicMaster, 2);
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
        l->PlayerMinVel = PLAYER_MIN_VEL + 15.0f;
        l->PlayerMaxVel = PLAYER_MIN_VEL + 50.0f;
        
        if (frame == 0)
        {
            AddIntroText(g, l, "DODGE", IntColor(ShipPalette.Colors[SHIP_RED]));
        }
        if (frame == FrameSeconds(5))
        {
            RemoveFlags(redShips, GenFlag_ReserveLane);
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
        l->PlayerMinVel = PLAYER_MIN_VEL;
        l->PlayerMaxVel = PLAYER_MIN_VEL + 30.0f;
        
        if (frame == 0)
        {
            AddIntroText(g, l, "ASTEROIDS", ASTEROID_COLOR);
        }
        if (frame == FrameSeconds(5))
        {
            Enable(asteroids);
        }
        if (frame == FrameSeconds(25))
        {
            Disable(asteroids);
            SpawnCheckpoint(g, l);
        }
        break;
    }
    
    case 5:
    {
        l->PlayerMinVel = PLAYER_MIN_VEL + 50.0f;
        l->PlayerMaxVel = PLAYER_MIN_VEL + 100.0f;
        
        if (frame == 0)
        {
            AddIntroText(g, l, "DRAFT & BLAST", IntColor(ShipPalette.Colors[SHIP_BLUE]));
        }
        if (frame == FrameSeconds(1))
        {
            AddIntroText(g, l, "DRAFT & MISS", IntColor(ShipPalette.Colors[SHIP_ORANGE]));
        }
        if (frame == FrameSeconds(5))
        {
            l->ForceShipColor = -1;
            Enable(ships);
        }
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
        else
        {
            l->GameplayState = GameplayState_Playing;
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

        if (l->GameplayState == GameplayState_Playing)
        {
            UpdateClassicMode(g, l);
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
			if ((std::abs(playerX) - ROAD_LANE_WIDTH / 2) > 2.0f * ROAD_LANE_WIDTH)
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
                        meshPart.Material.DiffuseColor.a = alpha;
                    }
                }

                cp->Timer += dt;
                break;
            }
            }

            ent->Pos().y += ent->Vel().y * dt;
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
        UpdateMenuSelection(g, pauseMenu, menuMoveY);
    }
    if (l->GameplayState == GameplayState_GameOver)
    {
        UpdateMenuSelection(g, gameOverMenu, menuMoveY);
    }

    AudioListenerUpdate(g->Camera);

	if (l->ScoreTextList.size() > 0)
	{
		auto scoreText = l->ScoreTextList.front();
		if (scoreText && scoreText->Sequence->Complete)
		{
			l->ScoreTextList.pop_front();
			DestroySequences(g->TweenState, scoreText->Sequence, 1);
			FreeEntryFromData(l->SequencePool, scoreText->Sequence);
			FreeEntryFromData(l->ScoreTextPool, scoreText);
		}
	}

	if (l->IntroTextList.size() > 0)
	{
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
    DrawRect(g->GUI, rect{20,20,200,20},
             IntColor(FirstPalette.Colors[2]), GL_LINE_LOOP, false);

    DrawRect(g->GUI, rect{25,25,190 * l->DraftCharge,10},
             IntColor(FirstPalette.Colors[3]), GL_TRIANGLES, false);

    float left = GetRealPixels(g, 10.0f);
    float top = g->Height - GetRealPixels(g, 10.0f);
    DrawText(g->GUI, hudFont, Format(&l->ScoreFormat, l->Score), rect{left, top - fontSize, 0, 0}, Color_white);
    DrawText(g->GUI, hudFont, Format(&l->HealthFormat, l->Health), rect{left + 400, top - fontSize, 0, 0}, Color_white);
    
    if (l->GameplayState == GameplayState_Paused)
    {
        DrawMenu(g, pauseMenu, g->GUI.MenuChangeTimer, 1.0f, false);
    }
    else if (l->GameplayState == GameplayState_GameOver)
    {
        DrawMenu(g, gameOverMenu, g->GUI.MenuChangeTimer, l->GameOverAlpha, false);
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
        InitMenu(g);
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
        DestroySequences(g->TweenState, l->GameOverMenuSequence, 1);
        InitMenu(g);
        break;
    }
    }
}