#ifndef DRAFT_LEVEL_H
#define DRAFT_LEVEL_H

struct crystal_event
{
	float x, y, z;
};

enum enemy_type
{
	EnemyType_Default,
	EnemyType_Explosive,
};
struct ship_event
{
	enemy_type EnemyType;
	float x, y, z;
	float vy;
};

enum level_event_type
{
    LevelEventType_Crystal,
	LevelEventType_Ship,
};
struct level_event
{
    level_event_type Type;
    union
    {
        crystal_event Crystal;
		ship_event Ship;
    };
};

struct level_tick
{
    float TriggerDistance = 0;
    vector<level_event> Events;
};

struct level
{
    int CurrentTick = 0;
    vector<level_tick> Ticks;
};

#endif