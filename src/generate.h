#ifndef DRAFT_GENERATE_H
#define DRAFT_GENERATE_H

#define BASE_CRYSTAL_INTERVAL 2.0f

#define INITIAL_SHIP_INTERVAL 4.0f
#define CHANGE_SHIP_TIMER 8.0f

#define NO_RESERVED_LANE 500

#define GEN_PLAYER_OFFSET 250

// generation flags
#define GenFlag_Enabled         (1 << 0)
#define GenFlag_Randomize       (1 << 1)
#define GenFlag_BasedOnVelocity (1 << 2)
#define GenFlag_ReserveLane     (1 << 3)
#define GenFlag_AlternateLane   (1 << 4)

struct game_main;
struct level_state;
struct gen_params;

#define GEN_FUNC(name) void name(gen_params *p, game_main *g, gen_state *state, void *data)
typedef GEN_FUNC(gen_func);

struct gen_params
{
    uint32 Flags;
    gen_func *Func;
	audio_clip *Clip = NULL;
    float Interval;
	float MaxTimerDecrease;
    float Timer = 0;
    float RandomOffset = 1.5f;
    int ReservedLane = NO_RESERVED_LANE;
};

struct gen_state
{
    gen_params GenParams[GenType_MAX];
    int LaneSlots[5];
    int ReservedLanes[5];
    int PlayerLaneIndex = 0;
    random_series Entropy;
};

inline void AddFlags(gen_params *p, uint32 flags)
{
    p->Flags |= flags;
}

inline void RemoveFlags(gen_params *p, uint32 flags)
{
    p->Flags &= ~flags;
}

inline void Enable(gen_params *p)
{
    p->Flags |= GenFlag_Enabled;
}

inline void Disable(gen_params *p)
{
    p->Flags &= ~GenFlag_Enabled;
}

inline void Randomize(gen_params *p)
{
    p->Flags |= GenFlag_Randomize;
}

void InitGenState(gen_state *);

#endif
