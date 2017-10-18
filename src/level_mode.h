#ifndef DRAFT_LEVEL_MODE_H
#define DRAFT_LEVEL_MODE_H

#define LevelGenFlag_Crystals (1 << 0)
#define LevelGenFlag_Ships    (1 << 1)
#define LevelGenFlag_RedShips (1 << 2)

struct audio_source;

#define PLAYER_INITIAL_MAX_VEL 70.0f

#define BASE_CRYSTAL_INTERVAL 4.0f

#define INITIAL_SHIP_INTERVAL 4.0f
#define CHANGE_SHIP_TIMER     8.0f

#define NO_RESERVED_LANE 500
struct level_mode
{
    memory_arena Arena;
    random_series Entropy;
    audio_source *DraftBoostAudio;
    uint64 GenFlags = 0;
    int LaneSlots[5];

    float NextCrystalTimer = BASE_CRYSTAL_INTERVAL;

    float NextShipInterval = INITIAL_SHIP_INTERVAL;
    float NextShipTimer = INITIAL_SHIP_INTERVAL;
    float ChangeShipTimer = CHANGE_SHIP_TIMER;

    int RedShipReservedLane = NO_RESERVED_LANE;
    float NextRedShipTimer = INITIAL_SHIP_INTERVAL;
    float NextRedShipInterval = INITIAL_SHIP_INTERVAL;
    float ChangeRedShipTimer = CHANGE_SHIP_TIMER;

    std::vector<collision_result> CollisionCache;
    float PlayerMaxVel = PLAYER_INITIAL_MAX_VEL;
    float TimeElapsed = 0.0f;
};

#endif
