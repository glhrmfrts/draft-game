#ifndef DRAFT_DRAFT_H
#define DRAFT_DRAFT_H

#include "asset.h"
#include "gui.h"
#include "level.h"

enum action_type
{
    Action_camHorizontal,
    Action_camVertical,
    Action_debugFreeCam,
    Action_horizontal,
    Action_vertical,
    Action_count,
};

enum game_mode
{
    GameMode_level,
};

struct action_state
{
    int Positive;
    int Negative;
    int Pressed = 0;
    float AxisValue;
};

#define MouseButton_left 0x1
#define MouseButton_middle 0x2
#define MouseButton_right 0x4

struct mouse_state
{
    int X, Y;
    int dX = 0;
    int dY = 0;
    int ScrollY = 0;
    uint8 Buttons = 0;
};

struct game_input
{
    mouse_state MouseState;
    action_state Actions[Action_count];
};

struct game_state
{
    game_input Input;
    game_input PrevInput;

    asset_cache AssetCache;
    gui GUI;
    camera GUICamera;
    render_state RenderState;

    level_mode LevelMode;
    game_mode GameMode;

    int Width;
    int Height;
    bool Running = true;
};

#endif
