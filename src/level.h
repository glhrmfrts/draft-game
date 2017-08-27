#ifndef DRAFT_LEVEL_H
#define DRAFT_LEVEL_H

#include "collision.h"
#include "memory.h"
#include "render.h"

#define EntityFlag_Kinematic 0x1

struct entity
{
    vec3 Position;
    vec3 Velocity;
    vec3 Size = vec3(1.0f);

    uint32 Flags = 0;
    int NumCollisions = 0;

    shape *Shape = NULL;
    model *Model = NULL;
};

#define TrackSegmentLength 10
#define TrackSegmentWidth  7
#define TrackSegmentCount  10
struct track_segment
{
    vec3 Position;
};

struct level_mode
{
    memory_arena Arena;
    camera Camera;
    mesh FloorMesh;
    mesh ShipMesh;
    mesh SkyboxMesh;
    vec3 Gravity;
    vector<entity *> ModelEntities;
    vector<entity *> ShapedEntities;
    vector<collision> CollisionCache;
    track_segment Segments[TrackSegmentCount];
    entity *SkyboxEntity;
    entity *PlayerEntity;
    int CurrentLevel = 0;
};

struct game_state;

void StartLevel(game_state &GameState, level_mode &LevelMode);
void UpdateAndRenderLevel(game_state &GameState, level_mode &LevelMode, float DeltaTime);

#endif
