#ifndef DRAFT_DRAFT_H
#define DRAFT_DRAFT_H

#include "config.h"
#include "common.h"
#include "memory.h"
#include "collision.h"
#include "render.h"
#include "asset.h"
#include "gui.h"

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

struct track_segment
{
    int NothingForNow;
};

#define EntityFlag_Kinematic 0x1
struct entity
{
    vec3 Position;
    vec3 Velocity;
    vec3 Size = vec3(1.0f);
    vec3 Rotation = vec3(0.0f);

    uint32 Flags = 0;
    int NumCollisions = 0;

    track_segment *TrackSegment = NULL;
    bounding_box *Bounds = NULL;
    model *Model = NULL;
};

#define TrackSegmentLength 20
#define TrackSegmentWidth  7
#define TrackLaneWidth     2.5f
#define TrackSegmentCount  20
#define TrackSegmentPadding 0.25f

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
