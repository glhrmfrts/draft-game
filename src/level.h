#ifndef DRAFT_LEVEL_H
#define DRAFT_LEVEL_H

struct crystal_event
{
	float Position[3];
};

enum level_event_type
{
	LevelEventType_Crystal,
};
struct level_event
{
	level_event_type Type;
	union
	{
		crystal_event Crystal;
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