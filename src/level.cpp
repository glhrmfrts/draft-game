// Copyright

static std::unordered_map<hash_string::result_type, color> LevelColors = {
	{ hash_string()("CRYSTAL_COLOR"), CRYSTAL_COLOR },
	{ hash_string()("SHIP_BLUE_COLOR"), IntColor(ShipPalette.Colors[SHIP_BLUE]) },
	{ hash_string()("SHIP_ORANGE_COLOR"), IntColor(ShipPalette.Colors[SHIP_ORANGE]) },
	{ hash_string()("RED"), Color_red }
};

static std::unordered_map<hash_string::result_type, gen_type> LevelGenTypes = {
	{ hash_string()("CRYSTALS"), GenType_Crystal },
	{ hash_string()("SHIPS"), GenType_Ship },
	{ hash_string()("RED_SHIPS"), GenType_RedShip },
	{ hash_string()("SKULLS"), GenType_EnemySkull },
};

static std::unordered_map<hash_string::result_type, uint32> LevelGenFlags = {
	{ hash_string()("RESERVE_LANE"), GenFlag_ReserveLane },
	{ hash_string()("ALTERNATE_LANE"), GenFlag_AlternateLane },
	{ hash_string()("RANDOMIZE"), GenFlag_Randomize },
	{ hash_string()("BASED_ON_VELOCITY"), GenFlag_BasedOnVelocity },
};

static std::unordered_map<hash_string::result_type, int> LevelShipColors = {
	{ hash_string()("SHIP_BLUE"), SHIP_BLUE },
	{ hash_string()("SHIP_ORANGE"), SHIP_ORANGE },
	{ hash_string()("ALL"), -1 },
};

static std::unordered_map<hash_string::result_type, road_change> LevelRoadChangeTypes = {
	{ hash_string()("NARROW_LEFT"), RoadChange_NarrowLeft },
	{ hash_string()("NARROW_RIGHT"), RoadChange_NarrowRight },
	{ hash_string()("NARROW_CENTER"), RoadChange_NarrowCenter },
	{ hash_string()("WIDENS_LEFT"), RoadChange_WidensLeft },
	{ hash_string()("WIDENS_RIGHT"), RoadChange_WidensRight },
	{ hash_string()("WIDENS_CENTER"), RoadChange_WidensCenter },
};

bool ParseGenericCommand(const std::string &cmd, const std::vector<std::string> &args, allocator *alloc, level_command *c)
{
    if (cmd == "add_intro_text")
    {
        c->Type = LevelCommand_AddIntroText;
        c->AddIntroText.ColorHash = hash_string()(args[1]);

        size_t size = args[0].size();
        char *buffer = (char *)PushSize(alloc, size + 1, "add_intro_text");
        memcpy(buffer, args[0].c_str(), size + 1);

        c->AddIntroText.Text = buffer;
        return true;
    }
    else if (cmd == "enable")
    {
        c->Type = LevelCommand_Enable;
        c->Hash = hash_string()(args[0]);
        return true;
    }
    else if (cmd == "disable")
    {
        c->Type = LevelCommand_Disable;
        c->Hash = hash_string()(args[0]);
        return true;
    }
	else if (cmd == "add_flags")
	{
		c->Type = LevelCommand_AddFlags;
		c->Flags.GenTypeHash = hash_string()(args[0]);
		c->Flags.FlagsHash = hash_string()(args[1]);
		return true;
	}
	else if (cmd == "remove_flags")
	{
		c->Type = LevelCommand_AddFlags;
		c->Flags.GenTypeHash = hash_string()(args[0]);
		c->Flags.FlagsHash = hash_string()(args[1]);
		return true;
	}
    else if (cmd == "ship_color")
    {
        c->Type = LevelCommand_ShipColor;
        c->Hash = hash_string()(args[0]);
        return true;
    }
    else if (cmd == "spawn_checkpoint")
    {
        c->Type = LevelCommand_SpawnCheckpoint;
        return true;
    }
    else if (cmd == "spawn_finish")
    {
        c->Type = LevelCommand_SpawnFinish;
        return true;
    }
    else if (cmd == "road_tangent")
    {
        c->Type = LevelCommand_RoadTangent;
        return true;
    }
	else if (cmd == "road_change")
	{
		c->Type = LevelCommand_RoadChange;
		c->Hash = hash_string()(args[0]);
		return true;
	}
    else if (cmd == "play_track")
    {
        c->Type = LevelCommand_PlayTrack;
        c->Track.TrackHash = hash_string()(args[0]);
		c->Track.BeatDivisor = 1;
		if (args.size() == 2)
		{
			c->Track.BeatDivisor = atoi(args[1].c_str());
		}
		return true;
    }
    else if (cmd == "stop_track")
    {
        c->Type = LevelCommand_StopTrack;
		c->Track.TrackHash = hash_string()(args[0]);
		c->Track.BeatDivisor = 1;
		if (args.size() == 2)
		{
			c->Track.BeatDivisor = atoi(args[1].c_str());
		}
		return true;
    }
	else if (cmd == "set_entity_clip")
	{
		c->Type = LevelCommand_SetEntityClip;
		c->SetEntityClip.GenTypeHash = hash_string()(args[0]);
		c->SetEntityClip.TrackHash = hash_string()(args[1]);
		return true;
	}

    return false;
}

void ParseLevel(std::istream &stream, allocator *alloc, level *result)
{
	enum
	{
		Parse_Level,
		Parse_Checkpoint,
		Parse_Frame,
        Parse_StatsScreen,
	} parseState = Parse_Level;

	level_checkpoint *currentCheckpoint = NULL;
	level_frame *currentFrame = NULL;
	std::string line;
	while (std::getline(stream, line))
	{
		trim(line);
		if (line == "}")
		{
			if (parseState == Parse_Frame)
			{
				currentFrame = NULL;
				parseState = Parse_Checkpoint;
			}
			else if (parseState == Parse_Checkpoint)
			{
				currentCheckpoint = NULL;
				parseState = Parse_Level;
			}
            else if (parseState == Parse_StatsScreen)
            {
                parseState = Parse_Level;
            }
			continue;
		}

		int i = line.find(":");
		std::string cmd;
		std::string argstr;
		if (i == std::string::npos)
		{
			cmd = line;
		}
		else
		{
			cmd = trim_copy(line.substr(0, i));
			argstr = line.substr(i + 1);
		}

		std::vector<std::string> args;
		if (argstr.size() > 0)
		{
			size_t pos;
			while ((pos = argstr.find("|")) != std::string::npos)
			{
				args.push_back(trim_copy(argstr.substr(0, pos)));
				argstr.erase(0, pos + 1);
			}
			if (argstr.size() > 0)
			{
				args.push_back(trim_copy(argstr));
			}
		}

        level_command genericCommand;
        bool generic = ParseGenericCommand(cmd, args, alloc, &genericCommand);

		switch (parseState)
		{
		case Parse_Level:
		{
			if (cmd == "checkpoint")
			{
				parseState = Parse_Checkpoint;
				result->Checkpoints.emplace_back();
				currentCheckpoint = &result->Checkpoints[result->Checkpoints.size() - 1];
			}
			else if (cmd == "name")
			{
				result->Name = args[0];
			}
			else if (cmd == "song")
			{
				result->SongName = args[0];
			}
			else if (cmd == "next")
			{
				result->Next = args[0];
			}
            else if (cmd == "stats_screen")
            {
                parseState = Parse_StatsScreen;
            }
			break;
		}

		case Parse_Checkpoint:
		{
			if (cmd == "frame_seconds")
			{
				parseState = Parse_Frame;
				currentCheckpoint->Frames.emplace_back();
				currentFrame = &currentCheckpoint->Frames[currentCheckpoint->Frames.size() - 1];
				currentFrame->Frame = (int)FRAME_SECONDS(atof(args[0].c_str()));
			}
			else if (cmd == "player_vel")
			{
				currentCheckpoint->PlayerMinVel = atof(args[0].c_str());
				currentCheckpoint->PlayerMaxVel = atof(args[1].c_str());
			}
			break;
		}

		case Parse_Frame:
		{
            if (generic)
            {
                currentFrame->Commands.push_back(genericCommand);
            }
			break;
		}

        case Parse_StatsScreen:
        {
            if (generic)
            {
                result->StatsScreenCommands.push_back(genericCommand);
            }
            break;
        }
		}
	}
}

void ResetLevel(level *l)
{
	l->HasRunStatsScreenCommands = false;
	for (auto &cp : l->Checkpoints)
	{
		cp.CurrentFrameIndex = 0;
	}
}

static void RunCommands(level *l, game_main *g, level_state *state, const std::vector<level_command> &commands)
{
    for (auto &cmd : commands)
    {
        switch (cmd.Type)
        {
        case LevelCommand_Enable:
            Enable(&g->World.GenState->GenParams[LevelGenTypes[cmd.Hash]]);
            break;

        case LevelCommand_Disable:
            Disable(&g->World.GenState->GenParams[LevelGenTypes[cmd.Hash]]);
            break;

		case LevelCommand_AddFlags:
			AddFlags(
				&g->World.GenState->GenParams[LevelGenTypes[cmd.Flags.GenTypeHash]],
				LevelGenFlags[cmd.Flags.FlagsHash]
			);
			break;

		case LevelCommand_RemoveFlags:
			RemoveFlags(
				&g->World.GenState->GenParams[LevelGenTypes[cmd.Flags.GenTypeHash]],
				LevelGenFlags[cmd.Flags.FlagsHash]
			);
			break;

        case LevelCommand_AddIntroText:
            AddIntroText(g, state, cmd.AddIntroText.Text, LevelColors[cmd.AddIntroText.ColorHash]);
            break;

        case LevelCommand_ShipColor:
            state->ForceShipColor = LevelShipColors[cmd.Hash];
            break;

        case LevelCommand_SpawnCheckpoint:
            SpawnCheckpoint(g, state);
            break;

        case LevelCommand_SpawnFinish:
            SpawnFinish(g, state);
            break;

        case LevelCommand_RoadTangent:
            RoadTangent(g, state);
            break;

		case LevelCommand_RoadChange:
			RoadChange(g->World, LevelRoadChangeTypes[cmd.Hash]);
			break;

		case LevelCommand_PlayTrack:
			PlayTrack(g, state, cmd.Track.TrackHash, cmd.Track.BeatDivisor);
			break;

		case LevelCommand_StopTrack:
			StopTrack(g, state, cmd.Track.TrackHash, cmd.Track.BeatDivisor);
			break;

		case LevelCommand_SetEntityClip:
			SetEntityClip(
				g->World,
				LevelGenTypes[cmd.SetEntityClip.GenTypeHash],
				l->Song->Clips[cmd.SetEntityClip.TrackHash]
			);
			break;
        }
    }
}

void LevelUpdate(level *l, game_main *g, level_state *state, float dt)
{
    if (state->GameplayState == GameplayState_Stats && !l->HasRunStatsScreenCommands)
    {
        l->HasRunStatsScreenCommands = true;
        RunCommands(l, g, state, l->StatsScreenCommands);
    }
    else
    {
        auto cp = &l->Checkpoints[state->CheckpointNum];
        state->PlayerMinVel = cp->PlayerMinVel;
        state->PlayerMaxVel = cp->PlayerMaxVel;

        if (cp->CurrentFrameIndex >= cp->Frames.size())
        {
            return;
        }

        if (cp->Frames[cp->CurrentFrameIndex].Frame == state->CurrentCheckpointFrame)
        {
            RunCommands(l, g, state, cp->Frames[cp->CurrentFrameIndex].Commands);
            cp->CurrentFrameIndex++;
        }
    }
}
