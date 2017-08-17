#include "luna.h"
#include "luna_world_mode.h"

void StartWorld(game_state *GameState)
{
    GameState->GameMode = GameMode_world;
}

void UpdateWorld(game_state *GameState, float DeltaTime)
{
}

void RenderWorld(game_state *GameState)
{
    glClear(GL_COLOR_BUFFER_BIT);

    gui *g = &GameState->GUI;

    UpdateProjectionView(&GameState->GUICamera);
    Begin(g, &GameState->GUICamera);
    PushRect(g, {0, 0, 50, 50}, color(1, 0, 0, 1));
    End(g);
}
