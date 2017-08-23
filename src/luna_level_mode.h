#ifndef LUNA_LEVEL_MODE_H
#define LUNA_LEVEL_MODE_H

#include "luna_collision.h"
#include "luna_render.h"

#define EntityFlag_kinematic 0x1

struct entity
{
    vec3 Position;
    vec3 Velocity;
    vec3 Scale = vec3(1.0f);
    uint32 Flags = 0;
    int NumCollisions = 0;

    shape *Shape = NULL;
    animated_sprite *Sprite = NULL;
    mesh *Mesh = NULL;
};

struct level_mode
{
    camera Camera;
    animated_sprite PlayerSprite;
    mesh FloorMesh;
    mesh WallMesh;
    vec3 Gravity;
    vector<entity *> MeshEntities;
    vector<entity *> SpriteEntities;
    vector<entity *> ShapedEntities;
    entity *PlayerEntity;
    int CurrentLevel = 0;
    uint32 Width;
    uint32 Length;
    uint8 *TileData;
    uint8 *HeightMap;
};

struct game_state;

void StartLevel(game_state &GameState, level_mode &LevelMode);
void UpdateAndRenderLevel(game_state &GameState, level_mode &LevelMode, float DeltaTime);

#endif
