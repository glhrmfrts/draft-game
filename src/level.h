#ifndef DRAFT_LEVEL_H
#define DRAFT_LEVEL_H

#include <list>

struct crystal_event
{
    float x, y, z;
};

struct ship_event
{
    enemy_type EnemyType;
    float x, y, z;
    float vy;
};

struct wall_event
{
    float x, y, z;
    float Width;
};

enum level_event_type
{
    LevelEventType_Crystal,
    LevelEventType_Ship,
    LevelEventType_Wall,
};
struct level_event
{
    level_event_type Type;
    float Delay = 0;
    float Timer = 0;
    float ResetTimer = 0; // Set the timer to this value if the
                          // MinScore condition does not evaluate
    int MinScore = 0;

    union
    {
        crystal_event Crystal;
        ship_event Ship;
        wall_event Wall;
    };
};

struct level_tick
{
    vector<level_event> Events;
    float TriggerDistance = 0;
};

struct level
{
    vector<entity *> EntitiesToRemove;
    vector<level_tick> Ticks;
    std::list<level_event> DelayedEvents;
    int CurrentTick = 0;
};

#endif
