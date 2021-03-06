#ifndef DRAFT_DRAFT_H
#define DRAFT_DRAFT_H

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <AL/al.h>
#include <AL/alc.h>
#include <sndfile.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "enums.h"
#include "types.h"
#include "thread_pool.h"
#include "config.h"
#include "common.h"
#include "collision.h"
#include "memory.h"
#include "tween.h"
#include "options.h"
#include "render.h"
#include "audio.h"
#include "level.h"
#include "asset.h"
#include "gui.h"
#include "random.h"
#include "entity.h"
#include "generate.h"
#include "menu_state.h"
#include "level_state.h"

#define GAME_BASE_WIDTH  1280
#define GAME_BASE_HEIGHT 720

#define PLATFORM_GET_FILE_LAST_WRITE_TIME(name) uint64 name(const char *Filename)
#define PLATFORM_COMPARE_FILE_TIME(name)        int32 name(uint64 t1, uint64 t2)
#define PLATFORM_GET_MILLISECONDS(name)         uint64 name()
#define PLATFORM_CREATE_THREAD(f)               platform_thread *f(game_main *g, const char *name, void *arg)

typedef PLATFORM_GET_FILE_LAST_WRITE_TIME(platform_get_file_last_write_time_func);
typedef PLATFORM_COMPARE_FILE_TIME(platform_compare_file_time_func);
typedef PLATFORM_GET_MILLISECONDS(platform_get_milliseconds_func);
typedef PLATFORM_CREATE_THREAD(platform_create_thread_func);

struct platform_thread
{
	typedef void(func)(void *);

	std::thread Thread;
	func *Func;
	const char *Name;
	void *Arg;
};

struct platform_api
{
	ALCdevice *AudioDevice;
    platform_get_file_last_write_time_func *GetFileLastWriteTime;
    platform_compare_file_time_func *CompareFileTime;
    platform_get_milliseconds_func *GetMilliseconds;
	platform_create_thread_func *CreateThread;
};

static platform_api *Global_Platform;

#define GAME_INIT(name) void name(game_main *game)
#define GAME_RELOAD(name) void name(game_main *game)
#define GAME_UNLOAD(name) void name(game_main *game)
#define GAME_UPDATE(name) void name(game_main *game, float dt)
#define GAME_RENDER(name) void name(game_main *game, float dt)
#define GAME_DESTROY(name) void name(game_main *game)
#define GAME_PROCESS_EVENT(name) void name(game_main *game, SDL_Event *event)

struct profile_time
{
	uint64 Begin;
	uint64 End;
	const char *Name;

	uint64 Delta() const { return End - Begin; }
};

static profile_time Global_ProfileTimers[64];
static int Global_ProfileTimersCount;

inline static void ResetProfileTimers()
{
	Global_ProfileTimersCount = 0;
}

inline static void BeginProfileTimer(const char *name)
{
	Global_ProfileTimers[Global_ProfileTimersCount++] = profile_time{ Global_Platform->GetMilliseconds(), 0, name };
}

inline static void EndProfileTimer(const char *name)
{
	Global_ProfileTimers[Global_ProfileTimersCount - 1].End = Global_Platform->GetMilliseconds();
}

extern "C"
{
	typedef GAME_INIT(game_init_func);
	typedef GAME_RELOAD(game_reload_func);
	typedef GAME_UNLOAD(game_unload_func);
    typedef GAME_UPDATE(game_update_func);
	typedef GAME_RENDER(game_render_func);
	typedef GAME_DESTROY(game_destroy_func);
	typedef GAME_PROCESS_EVENT(game_process_event_func);
}

struct game_controller
{
    SDL_Joystick *Joystick = NULL;
};

struct action_state
{
    int Positive = 0;
    int Negative = 0;
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

#define MouseButton_Left 0x1
#define MouseButton_Middle 0x2
#define MouseButton_Right 0x4
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

struct game_menu_context
{
    tween_sequence ChangeSequence;
    float ChangeTimer = 0;
};

struct game_main
{
	game_state State;
    game_input Input;
    game_input PrevInput;
    game_menu_context MenuContext;
	options *Options;

    profile_time UpdateTime;
    profile_time RenderTime;

	music_master MusicMaster;
	asset_loader AssetLoader;
    gui GUI;
    camera GUICamera;
    render_state RenderState;
    std::vector<asset_entry> Assets;
    tween_state TweenState;

    memory_arena Arena;
    camera Camera;
    camera FinalCamera;
    vec3 Gravity;
    entity_world World;
    level_state LevelState;
    menu_state MenuState;

    random_series ExplosionEntropy;
	float ScreenRectAlpha;

	void *GameLibrary;
    platform_api Platform;
	SDL_Window *Window;
    int Width;
    int Height;
    int ViewportWidth;
    int ViewportHeight;
    bool Running = true;

    game_main() {}
};

inline static float GetAxisValue(game_input &Input, action_type Type)
{
    return Input.Actions[Type].AxisValue;
}

inline static bool IsPressed(game_main *game, action_type type)
{
    return game->Input.Actions[type].Pressed > 0;
}

inline static bool IsJustPressed(game_main *Game, action_type Type)
{
    return Game->Input.Actions[Type].Pressed > 0 &&
        Game->PrevInput.Actions[Type].Pressed == 0;
}

inline static bool IsJustPressed(game_main *Game, uint8 Button)
{
    return (Game->Input.MouseState.Buttons & Button) &&
        !(Game->PrevInput.MouseState.Buttons & Button);
}

inline static bool IsJustReleased(game_main *Game, uint8 Button)
{
    return !(Game->Input.MouseState.Buttons & Button) &&
        (Game->PrevInput.MouseState.Buttons & Button);
}

inline static float GetRealPixels(game_main *game, float p)
{
    return p * (float(game->Width)/float(GAME_BASE_WIDTH));
}

inline static rect GetRealPixels(game_main *game, rect r)
{
    return r * (float(game->Width)/float(GAME_BASE_WIDTH));
}

#ifdef DRAFT_DEBUG
static int Global_DebugLogIdent = 0;

#define PrintSpaces(i) { \
		int myx = i; \
		while (myx > 0) \
		{ \
			std::cerr << "  "; \
			myx--; \
		} \
	}

#define DebugLog(msg) { \
		int i = Global_DebugLogIdent; \
		PrintSpaces(i) \
		std::cerr << "[DEBUG] " << __FUNCTION__ << ": " << msg << std::endl; \
	}

#define DebugLogCall() { \
		int i = Global_DebugLogIdent++; \
		PrintSpaces(i) \
		std::cerr << "[DEBUG] " << __FUNCTION__ << std::endl; \
		PrintSpaces(i) \
		std::cerr << "{" << std::endl; \
	}

#define DebugLogCall_(msg) { \
		int i = Global_DebugLogIdent++; \
		PrintSpaces(i) \
		std::cerr << "[DEBUG] " << __FUNCTION__ << " " << msg << std::endl; \
		PrintSpaces(i) \
		std::cerr << "{" << std::endl; \
	}

#define DebugLogCallEnd() { \
		int i = --Global_DebugLogIdent; \
		PrintSpaces(i) \
		std::cerr << "}" << std::endl; \
	}
#else
#define DebugLog(msg)
#define DebugLogCall()
#define DebugLogCallEnd()
#endif

#endif
