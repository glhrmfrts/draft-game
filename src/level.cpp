// Copyright

void AddEntity(game_state &Game, entity *Entity);

inline static void
AddEvent(level_tick &Tick, crystal_event Event)
{
	level_event LevelEvent;
	LevelEvent.Type = LevelEventType_Crystal;
	LevelEvent.Crystal = Event;
	Tick.Events.push_back(LevelEvent);
}

inline static void
AddEvent(level_tick &Tick, ship_event Event)
{
	level_event LevelEvent;
	LevelEvent.Type = LevelEventType_Ship;
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

		Result->Ticks.push_back(Tick);
	}
	{
		level_tick Tick;
		Tick.TriggerDistance = 800;
		AddEvent(Tick, ship_event{ EnemyType_Default, -8, -10, 0, 145 });

		Result->Ticks.push_back(Tick);
	}
    return Result;
}

template<typename T>
static inline vec3 EventPosition(const T &e)
{
	return vec3{ e.x, e.y, e.z };
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
                    vec3 Pos = EventPosition(Event.Crystal);
                    AddEntity(Game, CreateCrystalEntity(Game, PlayerPosition + Pos));
                    break;
                }

				case LevelEventType_Ship:
				{
					vec3 Pos = EventPosition(Event.Ship);
					AddEntity(Game, CreateEnemyShipEntity(Game, PlayerPosition + Pos, vec3{0,Event.Ship.vy,0}, Event.Ship.EnemyType));
					break;
				}
                }
            }
        }
    }
}