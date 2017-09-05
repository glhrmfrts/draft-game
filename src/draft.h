#ifndef DRAFT_DRAFT_H
#define DRAFT_DRAFT_H

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <AL/alc.h>
#include "config.h"
#include "common.h"
#include "memory.h"
#include "collision.h"
#include "render.h"
#include "asset.h"
#include "gui.h"
#include "entity.h"

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

// @TODO: this will probably only work for linux
enum game_controller_axis_id
{
    Axis_Invalid = -1,
    Axis_LeftX = 0,
    Axis_LeftY = 1,
    Axis_LeftTrigger = 2,
    Axis_RightTrigger = 5,
    Axis_RightX = 3,
    Axis_RightY = 4,
};
struct game_controller
{
    SDL_Joystick *Joystick;
};

struct action_state
{
    int Positive;
    int Negative;
    int Pressed = 0;
    float AxisValue = 0;
    game_controller_axis_id AxisID = Axis_Invalid;
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
    game_controller Controller;
};

struct game_state
{
    game_input Input;
    game_input PrevInput;

    asset_cache AssetCache;
    gui GUI;
    camera GUICamera;
    render_state RenderState;

    memory_arena Arena;
    camera Camera;
    mesh FloorMesh;
    mesh ShipMesh;
    mesh SkyboxMesh;
    vec3 Gravity;
    vector<entity *> ModelEntities;
    vector<entity *> ShapedEntities;
    vector<entity *> TrackEntities;
    vector<entity *> TrailEntities;
    vector<collision> CollisionCache;
    entity *SkyboxEntity;
    entity *PlayerEntity;
    entity *EnemyEntity;
    int CurrentLevel = 0;

    int Width;
    int Height;
    bool Running = true;

    game_state() {}
};

#endif
