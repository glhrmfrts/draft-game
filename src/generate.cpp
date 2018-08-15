// Copyright

int GetNextSpawnLane(gen_state *state, bool isShip = false)
{
    static int lastLane = 0;
    static int lastShipLane = 0;
    int lane = lastLane;
    while (lane == lastLane || (isShip && lane == lastShipLane) || (state->ReservedLanes[lane+2] == 1))
    {
        lane = RandomBetween(state->Entropy, -2, 2);
    }
    lastLane = lane;
    if (isShip)
    {
        lastShipLane = lane;
    }
    return lane;
}

int GetNextShipColor(gen_state *state, level_state *l)
{
    static int lastColor = 0;
    int color = lastColor;
    if (l->ForceShipColor > -1)
    {
        return lastColor = l->ForceShipColor;
    }
    while (color == lastColor)
    {
        color = RandomBetween(state->Entropy, 0, 1);
    }
    lastColor = color;
    return color;
}

GEN_FUNC(GenerateCrystal)
{
    auto l = static_cast<level_state *>(data);
    int lane = GetNextSpawnLane(state);
    auto ent = CreateCrystalEntity(GetEntry(g->World.CrystalPool), g->AssetLoader, GetCrystalMesh(g->World), lane);
    ent->Pos().x = lane * ROAD_LANE_WIDTH;
    ent->Pos().y = g->World.PlayerEntity->Pos().y + GEN_PLAYER_OFFSET;
    ent->Pos().z = SHIP_Z + 0.4f;
    ent->Scl().x = 0.3f;
    ent->Scl().y = 0.3f;
    ent->Scl().z = 0.5f;
    ent->Vel().y = PLAYER_MIN_VEL * 0.2f;
    AddEntity(g->World, ent);
}

GEN_FUNC(GenerateShip)
{
    auto l = static_cast<level_state *>(data);
    int colorIndex = GetNextShipColor(state, l);
    color c = IntColor(ShipPalette.Colors[colorIndex]);
    int lane = GetNextSpawnLane(state, true);
    auto ent = CreateShipEntity(GetEntry(g->World.ShipPool), GetShipMesh(g->World), c, c, p->Clip, false, colorIndex, lane);
    ent->Pos().x = lane * ROAD_LANE_WIDTH;
    ent->Pos().y = g->World.PlayerEntity->Pos().y + GEN_PLAYER_OFFSET;
    ent->Pos().z = SHIP_Z;
    ent->Ship->ColorIndex = colorIndex;
    AddFlags(ent, EntityFlag_RemoveOffscreen);
    AddEntity(g->World, ent);
}

GEN_FUNC(GenerateRedShip)
{
  color c = IntColor(ShipPalette.Colors[SHIP_RED]);
	int lane = glm::clamp(state->PlayerLaneIndex, g->World.RoadState.MinLaneIndex, g->World.RoadState.MaxLaneIndex) - 2;
	if ((p->Flags & GenFlag_ReserveLane) || (p->Flags & GenFlag_AlternateLane))
	{
		lane = p->ReservedLane;
	}
    auto ent = CreateShipEntity(GetEntry(g->World.ShipPool), GetShipMesh(g->World), c, c, p->Clip, false, SHIP_RED, lane);
    ent->Pos().x = lane * ROAD_LANE_WIDTH;
    ent->Pos().y = g->World.PlayerEntity->Pos().y + GEN_PLAYER_OFFSET;
    ent->Pos().z = SHIP_Z;
    ent->Ship->ColorIndex = SHIP_RED;
    AddFlags(ent, EntityFlag_RemoveOffscreen);
    AddEntity(g->World, ent);

	if (p->Clip)
	{
		AudioSourceSetPitch(ent->AudioSource, 0.5f + (float(lane+2) / 5.0f)*0.5f);
	}
}

#define ASTEROID_Z (SHIP_Z + 80)
GEN_FUNC(GenerateAsteroid)
{
    //Println("I should be generating an asteroid");
    auto ent = CreateAsteroidEntity(GetEntry(g->World.AsteroidPool), GetAsteroidMesh(g->World));
    int lane = state->PlayerLaneIndex - 2;
    ent->Pos().x = lane * ROAD_LANE_WIDTH;
    ent->Pos().y = g->World.PlayerEntity->Pos().y + (GEN_PLAYER_OFFSET/4) + (100.0f * (g->World.PlayerEntity->Vel().y/PLAYER_MAX_VEL_LIMIT));
    ent->Pos().z = ASTEROID_Z;
    //ent->Vel().y = g->World.PlayerEntity->Vel().y * 1.5f;
    AddFlags(ent, EntityFlag_RemoveOffscreen);
    AddEntity(g->World, ent);
}

#define SKULL_VEL_X 7
GEN_FUNC(GenerateEnemySkull)
{
	static bool longInterval = false;

	auto ent = CreateEnemySkullEntity(GetEntry(g->World.EnemySkullPool), FindMesh(g->AssetLoader, "skull", "main_assets"));
	ent->Pos().y = g->World.PlayerEntity->Pos().y + GEN_PLAYER_OFFSET;
	ent->Pos().z = SHIP_Z;

	if (longInterval)
	{
		ent->Vel().x = SKULL_VEL_X;
	}
	else
	{
		ent->Vel().x = -SKULL_VEL_X;
	}

	AddFlags(ent, EntityFlag_RemoveOffscreen);
	AddEntity(g->World, ent);

	if (longInterval)
	{
		p->Interval = 0.5f;
	}
	else
	{
		p->Interval = INITIAL_SHIP_INTERVAL;
	}
	longInterval = !longInterval;
}

GEN_FUNC(GenerateSideTrail)
{
    // TODO: create the side trail pool
    auto ent = CreateEntity(&g->World.Arena);
    ent->Pos().y = g->World.PlayerEntity->Pos().y + GEN_PLAYER_OFFSET;
    ent->TrailGroup = CreateTrailGroup(&g->World.Arena, ent, Color_white, 0.5f, true);
    AddFlags(ent, EntityFlag_RemoveOffscreen | EntityFlag_UpdateMovement);
    AddEntity(g->World, ent);

    ent->Vel().y = RandomBetween(state->Entropy, -25.0f, -75.0f);
    ent->Pos().x = RandomBetween(state->Entropy, 10.0f, 40.0f);
    if (RandomBetween(state->Entropy, 0, 1) == 0)
    {
        ent->Pos().x = -ent->Pos().x;
    }
}

GEN_FUNC(GenerateRandomGeometry)
{
    // TODO: create a pool for this
    mesh *m = NULL;
    vec3 scale = vec3(1.0f);
    color c = Color_white;
    if (RandomBetween(state->Entropy, 0, 1) == 0)
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
    ent->FrameRotation->Rotation.x = RandomBetween(state->Entropy, -45.0f, 45.0f);
    ent->FrameRotation->Rotation.y = RandomBetween(state->Entropy, -45.0f, 45.0f);
    ent->FrameRotation->Rotation.z = RandomBetween(state->Entropy, -45.0f, 45.0f);
    //ent->Trail = CreateTrail(&g->World.Arena, ent, c, 0.5f, true);

    ent->Pos().x = RandomBetween(state->Entropy, -1000.0f, 1000.0f);
    ent->Pos().y = g->World.PlayerEntity->Pos().y + GEN_PLAYER_OFFSET*2.0f;
    ent->Pos().z = RandomBetween(state->Entropy, -100.0f, -600.0f);
    ent->Vel().x = RandomBetween(state->Entropy, -20.0f, 20.0f);
    ent->Vel().y = RandomBetween(state->Entropy, -1.0f, -50.0f);
    ent->SetScl(scale);

    AddFlags(ent, EntityFlag_RemoveOffscreen | EntityFlag_UpdateMovement);
    AddEntity(g->World, ent);
}

void InitGenState(gen_state *state)
{
    auto gen = state->GenParams + GenType_Crystal;
	gen->MaxTimerDecrease = 0.92f;
    gen->Flags = GenFlag_Randomize;
    gen->Interval = BASE_CRYSTAL_INTERVAL;
    gen->Func = GenerateCrystal;

    gen = state->GenParams + GenType_Ship;
	gen->MaxTimerDecrease = 0.91f;
    gen->Flags = GenFlag_BasedOnVelocity;
    gen->Interval = INITIAL_SHIP_INTERVAL;
    gen->Func = GenerateShip;

    gen = state->GenParams + GenType_RedShip;
	gen->MaxTimerDecrease = 0.9f;
	gen->Flags = GenFlag_BasedOnVelocity;
    gen->Interval = INITIAL_SHIP_INTERVAL;
    gen->Func = GenerateRedShip;

    gen = state->GenParams + GenType_Asteroid;
	gen->MaxTimerDecrease = 0.92f;
    gen->Flags = GenFlag_BasedOnVelocity;// | GenFlag_ReserveLane;
    gen->Interval = INITIAL_SHIP_INTERVAL;
    gen->Func = GenerateAsteroid;

    gen = state->GenParams + GenType_SideTrail;
	gen->MaxTimerDecrease = 0.99f;
    gen->Flags = GenFlag_Enabled | GenFlag_Randomize;
    gen->Interval = 0.5f;
    gen->RandomOffset = 0.4f;
    gen->Func = GenerateSideTrail;

    gen = state->GenParams + GenType_RandomGeometry;
	gen->MaxTimerDecrease = 0.99f;
    gen->Flags = /*GenFlag_Enabled |*/ GenFlag_Randomize;
    gen->Interval = 0.5f;
    gen->RandomOffset = 0.4f;
    gen->Func = GenerateRandomGeometry;

	gen = state->GenParams + GenType_EnemySkull;
	gen->MaxTimerDecrease = 0.99f;
	gen->Flags = GenFlag_BasedOnVelocity;
	gen->Interval = INITIAL_SHIP_INTERVAL;
	gen->Func = GenerateEnemySkull;

    for (int i = 0; i < ROAD_LANE_COUNT; i++)
    {
        state->ReservedLanes[i] = 0;
    }
}

float GetNextTimer(gen_params *p, random_series random)
{
    if (p->Flags & GenFlag_Randomize)
    {
        const float offset = p->RandomOffset;
        return p->Interval + RandomBetween(random, -offset, offset);
    }
    return p->Interval;
}

void ResetGen(gen_params *p)
{
    p->Interval = INITIAL_SHIP_INTERVAL;
    p->Timer = p->Interval;
}

void UpdateGen(game_main *g, gen_state *state, gen_params *p, void *data, float dt)
{
  if (!(p->Flags & GenFlag_Enabled))
  {
      return;
  }

	if (p->Flags & GenFlag_ReserveLane)
	{
		if (p->Timer <= p->Interval*0.5f && p->ReservedLane == NO_RESERVED_LANE)
		{
			const int maxTries = 5;
			int i = 0;
			int laneIndex = state->PlayerLaneIndex;

			// get the first non-occupied lane, or give up for this frame
			while (state->LaneSlots[laneIndex] > 0 && i < maxTries)
			{
				laneIndex = RandomBetween(state->Entropy, g->World.RoadState.MinLaneIndex, g->World.RoadState.MaxLaneIndex);
				i++;
			}
			if (i >= maxTries)
			{
				p->Timer = GetNextTimer(p, state->Entropy);
			}
			else
			{
				p->ReservedLane = laneIndex - 2;
				state->ReservedLanes[laneIndex] = 1;
			}
		}
	}

	if (p->Flags & GenFlag_AlternateLane)
	{
		if (p->Timer <= p->Interval*0.5f && p->ReservedLane == NO_RESERVED_LANE)
		{
			int lastLaneIndex = p->ReservedLane + 2;
			int laneIndex = lastLaneIndex;
			while (laneIndex == lastLaneIndex)
			{
				laneIndex = RandomBetween(state->Entropy, g->World.RoadState.MinLaneIndex, g->World.RoadState.MaxLaneIndex);
			}
			p->ReservedLane = laneIndex - 2;
		}
	}

    if (p->Timer <= 0)
    {
        p->Func(p, g, state, data);
        p->Timer = GetNextTimer(p, state->Entropy);

        if (p->Flags & GenFlag_BasedOnVelocity)
        {
            p->Timer -= (p->Timer * p->MaxTimerDecrease) * (g->World.PlayerEntity->Vel().y / PLAYER_MAX_VEL_LIMIT);
        }

        if (p->ReservedLane != NO_RESERVED_LANE)
        {
            state->ReservedLanes[p->ReservedLane + 2] = 0;
        }

        p->ReservedLane = NO_RESERVED_LANE;
    }

    p->Timer -= dt;
}

void UpdateGenState(game_main *g, gen_state *state, void *data, float dt)
{
    for (int i = 0; i < GenType_MAX; i++)
    {
        UpdateGen(g, state, state->GenParams+i, data, dt);
    }
}
