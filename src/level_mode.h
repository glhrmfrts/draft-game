#ifndef DRAFT_LEVEL_MODE_H
#define DRAFT_LEVEL_MODE_H

struct audio_source;

#define PLAYER_INITIAL_MAX_VEL 70.0f
struct level_mode
{
    memory_arena Arena;
    random_series Entropy;
    audio_source *DraftBoostAudio;

    std::vector<collision_result> CollisionCache;
    float PlayerMaxVel = PLAYER_INITIAL_MAX_VEL;
};

#endif
