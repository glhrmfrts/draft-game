#ifndef DRAFT_DRAFT_H
#define DRAFT_DRAFT_H

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <AL/alc.h>
#include "config.h"
#include "common.h"
#include "collision.h"
#include "memory.h"
#include "render.h"
#include "asset.h"
#include "gui.h"
#include "entity.h"
#include "random.h"

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
enum game_controller_button_id
{
    Button_Invalid = -1,

#ifdef _WIN32
	XboxButton_A = 10,
	XboxButton_B = 11,
	XboxButton_X = 12,
	XboxButton_Y = 13,
	XboxButton_Left = 8,
	XboxButton_Right = 9,
#else
    XboxButton_A = 0,
    XboxButton_B = 1,
    XboxButton_X = 2,
    XboxButton_Y = 3,
    XboxButton_Left = 4,
    XboxButton_Right = 5,
#endif
};
struct game_controller
{
    SDL_Joystick *Joystick;
};

enum action_type
{
    Action_camHorizontal,
    Action_camVertical,
    Action_horizontal,
    Action_vertical,
    Action_boost,
	Action_debugUI,
    Action_count,
};
struct action_state
{
    int Positive;
    int Negative;
    int Pressed = 0;
    float AxisValue = 0;
    game_controller_axis_id AxisID = Axis_Invalid;
    game_controller_button_id ButtonID = Button_Invalid;

	action_state() {}
	action_state(int p, int n, int pr, float av,
				 game_controller_axis_id ai = Axis_Invalid,
				 game_controller_button_id bi = Button_Invalid)
		: Positive(p), Negative(n), Pressed(pr), AxisValue(av),
		  AxisID(ai), ButtonID(bi) {}
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

	game_input() {}
};

enum game_mode
{
    GameMode_level,
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

    std::list<entity *> ModelEntities;
    std::list<entity *> ShapedEntities;
    std::list<entity *> TrackEntities;
    std::list<entity *> TrailEntities;
    std::list<entity *> ExplosionEntities;
    std::list<entity *> ShipEntities;
    std::vector<collision> CollisionCache;
    entity *SkyboxEntity;
    entity *PlayerEntity;
    int CurrentLevel = 0;

    random_series ExplosionEntropy;

    int Width;
    int Height;
    bool Running = true;

    game_state() {}
};

#endif
