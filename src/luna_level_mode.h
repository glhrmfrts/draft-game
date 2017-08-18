#ifndef LUNA_LEVEL_MODE_H
#define LUNA_LEVEL_MODE_H

#include "luna_render.h"

struct level_mode
{
    camera Camera;
    mesh FloorMesh;
    mesh WallMesh;
    int CurrentLevel = 0;
};

struct game_state;

void StartLevel(game_state &GameState, level_mode &LevelMode);
void UpdateLevel(game_state &GameState, level_mode &LevelMode, float DeltaTime);
void RenderLevel(game_state &GameState, level_mode &LevelMode);

#endif
