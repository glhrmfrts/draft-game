// Copyright

using hash_string = std::hash<std::string>;

static std::unordered_map<size_t, color> LevelColors = {
	{ hash_string()("CRYSTAL_COLOR"), CRYSTAL_COLOR },
	{ hash_string()("SHIP_BLUE_COLOR"), IntColor(ShipPalette.Colors[SHIP_BLUE]) },
	{ hash_string()("SHIP_ORANGE_COLOR"), IntColor(ShipPalette.Colors[SHIP_ORANGE]) },
};

static std::unordered_map<size_t, gen_type> LevelGenTypes = {
	{ hash_string()("CRYSTALS"), GenType_Crystal },
	{ hash_string()("SHIPS"), GenType_Ship },
	{ hash_string()("RED_SHIPS"), GenType_RedShip },
};

static std::unordered_map<size_t, int> LevelShipColors = {
	{ hash_string()("SHIP_BLUE"), SHIP_BLUE },
	{ hash_string()("SHIP_ORANGE"), SHIP_ORANGE },
	{ hash_string()("ALL"), -1 },
};

void ParseLevel(std::istream &stream, allocator *alloc, level *result)
{
	enum
	{
		Parse_Level,
		Parse_Checkpoint,
		Parse_Frame,
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
			currentFrame->Commands.emplace_back();
			auto c = &currentFrame->Commands[currentFrame->Commands.size() - 1];

			if (cmd == "add_intro_text")
			{
				c->Type = LevelCommand_AddIntroText;
				c->AddIntroText.ColorHash = hash_string()(args[1]);

				size_t size = args[0].size();
				char *buffer = (char *)PushSize(alloc, size + 1, "add_intro_text");
				memcpy(buffer, args[0].c_str(), size + 1);

				c->AddIntroText.Text = buffer;
			}
			else if (cmd == "enable")
			{
				c->Type = LevelCommand_Enable;
				c->Hash = hash_string()(args[0]);
			}
			else if (cmd == "disable")
			{
				c->Type = LevelCommand_Disable;
				c->Hash = hash_string()(args[0]);
			}
			else if (cmd == "ship_color")
			{
				c->Type = LevelCommand_ShipColor;
				c->Hash = hash_string()(args[0]);
			}
			else if (cmd == "spawn_checkpoint")
			{
				c->Type = LevelCommand_SpawnCheckpoint;
			}
			else if (cmd == "spawn_finish")
			{
				c->Type = LevelCommand_SpawnFinish;
			}
            else if (cmd == "road_tangent")
            {
                c->Type = LevelCommand_RoadTangent;
            }
			break;
		}
		}
	}
}

void LevelUpdate(level *l, game_main *g, level_state *state, float dt)
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
		for (auto &cmd : cp->Frames[cp->CurrentFrameIndex].Commands)
		{
			switch (cmd.Type)
			{
			case LevelCommand_Enable:
				Enable(&g->World.GenState->GenParams[LevelGenTypes[cmd.Hash]]);
				break;

			case LevelCommand_Disable:
				Disable(&g->World.GenState->GenParams[LevelGenTypes[cmd.Hash]]);
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
			}
		}
		cp->CurrentFrameIndex++;
	}
}