#ifndef DRAFT_LEVEL_STATE_H
#define DRAFT_LEVEL_STATE_H

struct audio_source;

#define PLAYER_INITIAL_MAX_VEL 70.0f

#define PLAYER_MAX_VEL_INCREASE_FACTOR 0.5f
#define PLAYER_MAX_VEL_LIMIT           150.0f

#define PLAYER_MIN_VEL          50.0f
#define PLAYER_MIN_VEL_BREAKING 20.0f

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

struct track_args
{
	game_main *Game;
	hash_string::result_type TrackHash;
};

struct level_state
{
    memory_arena Arena;
    random_series Entropy;
	asset_loader *AssetLoader;
	audio_clip *DraftBoostSound;
	level *Level;

    generic_pool<level_score_text> ScoreTextPool;
    generic_pool<level_intro_text> IntroTextPool;
	generic_pool<track_args> TrackArgsPool;

    std::vector<collision_result> CollisionCache;
    float PlayerMinVel = PLAYER_MIN_VEL;
    float PlayerMaxVel = PLAYER_INITIAL_MAX_VEL;
    float TimeElapsed = 0;
    float DamageTimer = 0;
    int Health = 100;
    int CurrentCheckpointFrame = 0;

	// The player's current checkpoint
    int CheckpointNum = 0;

    int ForceShipColor = -1;
    gameplay_state GameplayState = GameplayState_Playing;

    tween_sequence *GameOverMenuSequence;
    float GameOverAlpha;

	// stats screen stuff
    tween_sequence *StatsScreenSequence;
    tween_sequence *ChangeLevelSequence;
    float StatsAlpha[3];
	text_group ScoreTextGroup;
	text_group TargetTextGroup;
	string_format ScorePercentFormat;
	string_format ScoreRatioFormat;
	bool DrawStats = false;
	bool ChangeSong = false;

	tween_sequence *ExitSequence;

    entity *DraftTarget;
    float CurrentDraftTime = 0;
    float DraftCharge = 0;
    int NumTrailCollisions = 0;
    bool DraftActive = false;
    string_format HealthFormat;
    string_format ScoreNumberFormat;
    int Score = 0;
    std::list<level_score_text *> ScoreTextList;
    std::list<level_intro_text *> IntroTextList;
};

void ResetLevelState(game_main *g, level_state *l, const std::string &levelNumber);
void InitLevelState(game_main *g, level_state *l);
void AddIntroText(game_main *g, level_state *l, const char *text, color c);
void SpawnCheckpoint(game_main *g, level_state *l);
void SpawnFinish(game_main *g, level_state *l);
void RoadTangent(game_main *g, level_state *l);
void PlayTrack(game_main *g, level_state *l, hash_string::result_type trackHash, int divisor);
void StopTrack(game_main *g, level_state *l, hash_string::result_type trackHash, int divisor);

#endif
