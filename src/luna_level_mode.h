#ifndef LUNA_LEVEL_MODE_H
#define LUNA_LEVEL_MODE_H

#include "luna_render.h"

#define EntityFlag_kinematic 0x1

struct entity
{
    vec3 Position;
    vec3 Velocity;
    vec3 Size = vec3(1.0f);

    sprite_animation IdleAnimation;
    sprite_animation WalkAnimation;

    vec3 MovementVelocity;
    vec3 MovementDest;
    float MovementSpeed;
    bool IsMoving = false;

    uint32 Flags = 0;
    int NumCollisions = 0;

    shape *Shape = NULL;
    animated_sprite *Sprite = NULL;
    mesh *Mesh = NULL;
};

struct collision
{
    vec3 Normal;
    float Depth;
    entity *First;
    entity *Second;
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
    vector<collision> Collisions;
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
