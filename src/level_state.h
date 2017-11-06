#ifndef DRAFT_LEVEL_STATE_H
#define DRAFT_LEVEL_STATE_H

struct audio_source;

#define PLAYER_INITIAL_MAX_VEL 70.0f

#define BASE_CRYSTAL_INTERVAL 2.0f

#define INITIAL_SHIP_INTERVAL 4.0f
#define CHANGE_SHIP_TIMER 8.0f

#define NO_RESERVED_LANE 500

// entity generation flags
#define LevelGenFlag_Enabled         (1 << 0)
#define LevelGenFlag_Randomize       (1 << 1)
#define LevelGenFlag_BasedOnVelocity (1 << 2)
#define LevelGenFlag_ReserveLane     (1 << 3)

enum level_gen_type
{
    LevelGenType_Crystal,
    LevelGenType_Ship,
    LevelGenType_RedShip,
    LevelGenType_Asteroid,
    LevelGenType_MAX,
};

struct game_main;
struct level_state;
struct level_gen_params;

#define LEVEL_GEN_FUNC(name) void name(level_gen_params *p, game_main *g, level_state *l)
typedef LEVEL_GEN_FUNC(level_gen_func);

struct level_gen_params
{
    uint32 Flags;
    level_gen_func *Func;
    float Interval;
    float Timer = 0;
    int ReservedLane = NO_RESERVED_LANE;
};

inline static void Enable(level_gen_params *p)
{
    p->Flags |= LevelGenFlag_Enabled;
}

inline static void Disable(level_gen_params *p)
{
    p->Flags &= ~LevelGenFlag_Enabled;
}

inline static void Randomize(level_gen_params *p)
{
    p->Flags |= LevelGenFlag_Randomize;
}

struct level_score_text
{
    color Color;
    vec2 Pos;
    vec2 TargetPos;
    tween_sequence *Sequence;
    float TweenPosValue = 0;
    float TweenAlphaValue = 0;
    const char *Text;
    int Score = 0;
};

struct level_state
{
    memory_arena Arena;
    random_series Entropy;
    audio_source *DraftBoostAudio;
    int LaneSlots[5];
    int ReservedLanes[5];

    generic_pool<tween_sequence> SequencePool;
    generic_pool<level_score_text> ScoreTextPool;

    level_gen_params GenParams[LevelGenType_MAX];

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

static void InitLevel(game_main *g);

#endif
