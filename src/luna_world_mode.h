#ifndef LUNA_WORLD_MODE_H
#define LUNA_WORLD_MODE_H

struct world_mode
{
    int FPS;
};

struct game_state;

void StartWorld(game_state *GameState);
void UpdateWorld(game_state *GameState, float DeltaTime);
void RenderWorld(game_state *GameState);

#endif
