#ifndef LUNA_LUNA_H
#define LUNA_LUNA_H

#include "luna_asset.h"
#include "luna_gui.h"
#include "luna_world_mode.h"

enum action_type
{
    Action_horizontal,
    Action_vertical,
    Action_count,
};

enum game_mode
{
    GameMode_world,
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

    game_mode GameMode;
    bool Running = true;
};

#endif
