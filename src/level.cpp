// Copyright

void AddEntity(game_state &Game, entity *Entity);

inline static void
AddEvent(level_tick &Tick, crystal_event Event)
{
	Tick.Events.push_back(level_event{ LevelEventType_Crystal, { Event } });
}

level *GenerateTestLevel(memory_arena &Arena)
{
	level *Result = PushStruct<level>(Arena);
	{
		level_tick Tick;
		Tick.TriggerDistance = 0;
		AddEvent(Tick, crystal_event{ { 0, 400, 0} });
		AddEvent(Tick, crystal_event{ { 0, 500, 0 } });
		AddEvent(Tick, crystal_event{ { 0, 600, 0 } });
		
		Result->Ticks.push_back(Tick);
	}
	return Result;
}

void UpdateLevel(game_state &Game)
{
	auto *Level = Game.CurrentLevel;
	auto *PlayerEntity = Game.PlayerEntity;
	auto &PlayerPosition = PlayerEntity->Transform.Position;
	int NumTicks = (int)Level->Ticks.size();
	for (int i = Level->CurrentTick; i < NumTicks; i++)
	{
		auto &Tick = Level->Ticks[i];
		if (PlayerPosition.y > Tick.TriggerDistance)
		{
			Level->CurrentTick++;
			for (const auto &Event : Tick.Events)
			{
				switch (Event.Type)
				{
				case LevelEventType_Crystal:
				{
					const float *p = Event.Crystal.Position;
					AddEntity(Game, CreateCrystalEntity(Game, PlayerPosition + vec3{ p[0], p[1], p[2] }));
					break;
				}
				}
			}
		}
	}
}