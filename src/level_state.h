#ifndef DRAFT_LEVEL_STATE_H
#define DRAFT_LEVEL_STATE_H

struct audio_source;

#define PLAYER_INITIAL_MAX_VEL 70.0f

#define PLAYER_MAX_VEL_INCREASE_FACTOR 0.5f
#define PLAYER_MAX_VEL_LIMIT           170.0f

#define MAX_INTRO_TEXTS  8

struct game_main;
struct level_state;

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

struct level_intro_text
{
    color Color;
    vec2 Pos;
    const char *Text;
    tween_sequence *Sequence = NULL;
    tween_sequence *PosSequence = NULL;
};

enum gameplay_state
{
    GameplayState_Playing,
    GameplayState_Paused,
    GameplayState_GameOver,
};

struct level_state
{
    memory_arena Arena;
    random_series Entropy;
    audio_source *DraftBoostAudio;

    generic_pool<tween_sequence> SequencePool;
    generic_pool<level_score_text> ScoreTextPool;
    generic_pool<level_intro_text> IntroTextPool;

    std::vector<collision_result> CollisionCache;
    float PlayerMaxVel = PLAYER_INITIAL_MAX_VEL;
    float TimeElapsed = 0;
    float DamageTimer = 0;
    int Health = 100;
    int CurrentCheckpointFrame = 0;
    int CheckpointNum = 0;
    int ForceShipColor = -1;
    gameplay_state GameplayState = GameplayState_Playing;
    
    tween_sequence *GameOverMenuSequence;
    float GameOverAlpha;
    
    entity *DraftTarget;
    float CurrentDraftTime = 0;
    float DraftCharge = 0;
    int NumTrailCollisions = 0;
    bool DraftActive = false;
    string_format HealthFormat;
    string_format ScoreFormat;
    string_format ScoreNumberFormat;
    int Score = 0;
    std::list<level_score_text *> ScoreTextList;
    std::list<level_intro_text *> IntroTextList;
};

static void InitLevel(game_main *g);

#endif
