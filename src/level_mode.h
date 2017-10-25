#ifndef DRAFT_LEVEL_MODE_H
#define DRAFT_LEVEL_MODE_H

#define LevelFlag_Crystals (1 << 0)
#define LevelFlag_Ships    (1 << 1)
#define LevelFlag_RedShips (1 << 2)

struct audio_source;

#define PLAYER_INITIAL_MAX_VEL 70.0f

#define BASE_CRYSTAL_INTERVAL 4.0f

#define INITIAL_SHIP_INTERVAL 4.0f
#define CHANGE_SHIP_TIMER     8.0f

#define NO_RESERVED_LANE 500

/*
struct level_gen_params
{
    uint32 Flags;
    float NextInterval;
    float NextTimer;
    float ChangeTimer;
};
*/

struct level_score_text
{
    color Color;
    vec2 Pos;
    vec2 TargetPos;
    tween_sequence *Sequence;
    float TweenValue = 0;
    int Score = 0;
};

struct level_mode
{
    memory_arena Arena;
    random_series Entropy;
    audio_source *DraftBoostAudio;
    uint64 GenFlags = 0; // entity generation flags
    uint64 IncFlags = 0; // timer increment flags
    uint64 RandFlags = 0; // random timer flags
    int LaneSlots[5];

    generic_pool<tween_sequence> SequencePool;
    generic_pool<level_score_text> ScoreTextPool;

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
    int PlayerLaneIndex = 0;

    entity *DraftTarget;
    float CurrentDraftTime = 0;
    float DraftCharge = 0;
    int NumTrailCollisions = 0;
    bool DraftActive = false;
    string_format ScoreFormat;
    string_format ScoreNumberFormat;
    int Score = 0;
    std::list<level_score_text *> ScoreTextList;
};

#endif
