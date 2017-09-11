// Copyright

void AddEntity(game_state &Game, entity *Entity);

inline static void
AddLevelEntity(game_state &Game, level *Level, entity *Entity)
{
	Level->EntitiesToRemove.push_back(Entity);
	AddEntity(Game, Entity);
}

inline static void
AddEvent(level_tick &Tick, crystal_event Event, float Delay = 0)
{
	level_event LevelEvent;
	LevelEvent.Type = LevelEventType_Crystal;
	LevelEvent.Delay = Delay;
	LevelEvent.Crystal = Event;
	Tick.Events.push_back(LevelEvent);
}

inline static void
AddEvent(level_tick &Tick, ship_event Event, float Delay = 0)
{
	level_event LevelEvent;
	LevelEvent.Type = LevelEventType_Ship;
	LevelEvent.Delay = Delay;
	LevelEvent.Ship = Event;
	Tick.Events.push_back(LevelEvent);
}

level *GenerateTestLevel(memory_arena &Arena)
{
    level *Result = PushStruct<level>(Arena);
    {
        level_tick Tick;
        Tick.TriggerDistance = 0;
        AddEvent(Tick, crystal_event{ 0, 400, 0 });
        AddEvent(Tick, crystal_event{ 0, 500, 0 });
        AddEvent(Tick, crystal_event{ 0, 600, 0 });
        
        Result->Ticks.push_back(Tick);
    }

	{
		level_tick Tick;
		Tick.TriggerDistance = 500;
		AddEvent(Tick, ship_event{ EnemyType_Default, 4, -10, 0, 108 });
		AddEvent(Tick, ship_event{ EnemyType_Default, -8, -10, 0, 145 }, 2.0f);
		AddEvent(Tick, ship_event{ EnemyType_Explosive, -8, -10, 0, 145 }, 4.0f);

		Result->Ticks.push_back(Tick);
	}
    return Result;
}

template<typename T>
static inline vec3 EventPosition(const T &e)
{
	return vec3{ e.x, e.y, e.z };
}

static void ProcessEvent(game_state &Game, level *Level, vec3 PlayerPosition, const level_event &Event)
{
	switch (Event.Type)
	{
	case LevelEventType_Crystal:
	{
		vec3 Pos = EventPosition(Event.Crystal);
		AddLevelEntity(Game, Level, CreateCrystalEntity(Game, PlayerPosition + Pos));
		break;
	}

	case LevelEventType_Ship:
	{
		vec3 Pos = EventPosition(Event.Ship);
		AddLevelEntity(Game, Level, CreateEnemyShipEntity(Game, PlayerPosition + Pos, vec3{ 0, Event.Ship.vy, 0 }, Event.Ship.EnemyType));
		break;
	}
	}
}

void UpdateLevel(game_state &Game, float DeltaTime)
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
            for (auto &Event : Tick.Events)
            {
				if (Event.Delay > 0.0f)
				{
					Level->DelayedEvents.push_back(Event);
				}
				else
				{
					ProcessEvent(Game, Level, PlayerPosition, Event);
				}
            }
        }
    }

	auto it = Level->DelayedEvents.begin();
	while (it != Level->DelayedEvents.end())
	{
		auto &Event = *it;
		if (Event.Timer >= Event.Delay)
		{
			ProcessEvent(Game, Level, PlayerPosition, Event);
			it = Level->DelayedEvents.erase(it);
		}
		else
		{
			Event.Timer += DeltaTime;
			it++;
		}
	}
}