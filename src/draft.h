#ifndef DRAFT_DRAFT_H
#define DRAFT_DRAFT_H

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <AL/alc.h>
#include "thread_pool.h"
#include "config.h"
#include "common.h"
#include "collision.h"
#include "memory.h"
#include "render.h"
#include "asset.h"
#include "gui.h"
#include "entity.h"
#include "random.h"
#include "level.h"

#define PLATFORM_GET_FILE_LAST_WRITE_TIME(name) uint64 name(const char *Filename)
#define PLATFORM_COMPARE_FILE_TIME(name)        int32 name(uint64 t1, uint64 t2)

typedef PLATFORM_GET_FILE_LAST_WRITE_TIME(platform_get_file_last_write_time_func);
typedef PLATFORM_COMPARE_FILE_TIME(platform_compare_file_time_func);

struct platform_api
{
    platform_get_file_last_write_time_func *GetFileLastWriteTime;
    platform_compare_file_time_func *CompareFileTime;    
};

#define GAME_INIT(name) void name(game_state *Game)
#define GAME_RENDER(name) void name(game_state *Game, float DeltaTime)
#define GAME_DESTROY(name) void name(game_state *Game)
#define GAME_PROCESS_EVENT(name) void name(game_state *Game, SDL_Event *Event)

struct game_state;

extern "C"
{
	typedef GAME_INIT(game_init_func);
	typedef GAME_RENDER(game_render_func);
	typedef GAME_DESTROY(game_destroy_func);
	typedef GAME_PROCESS_EVENT(game_process_event_func);
}

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
    SDL_Joystick *Joystick = NULL;
};

enum action_type
{
    Action_camHorizontal,
    Action_camVertical,
    Action_horizontal,
    Action_vertical,
    Action_boost,
	Action_debugUI,
    Action_debugPause,
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

	const uint8 *Keys;
	int KeyCount;

	game_input() {}
};

enum game_mode
{
	GameMode_LoadingScreen,
    GameMode_Level,
};
struct game_state
{
	game_mode Mode;
    game_input Input;
    game_input PrevInput;

	asset_loader AssetLoader;
    gui GUI;
    camera GUICamera;
    render_state RenderState;

    memory_arena Arena;
    camera Camera;
    mesh FloorMesh;
    mesh ShipMesh;
    mesh SkyboxMesh;
	mesh CrystalMesh;
    mesh WallMesh;
    vec3 Gravity;

    vector<entity *> ModelEntities;
	vector<entity *> ShapedEntities;
	vector<entity *> TrackEntities;
	vector<entity *> TrailEntities;
	vector<entity *> ExplosionEntities;
	vector<entity *> ShipEntities;
    vector<entity *> WallEntities;
    vector<collision> CollisionCache;
    entity *SkyboxEntity;
    entity *PlayerEntity;
    level *CurrentLevel = 0;
	int NumEntities = 0;

    random_series ExplosionEntropy;
    bitmap_font *TestFont;

    platform_api Platform;
	SDL_Window *Window;
    int Width;
    int Height;
    bool Running = true;

    game_state() {}
};

#endif
