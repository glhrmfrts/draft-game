#ifndef DRAFT_LEVEL_H
#define DRAFT_LEVEL_H

enum entity_type
{
    EntityType_Empty,
    EntityType_Collision,
    EntityType_Wall,
    EntityType_Model,
};

static const char *EntityTypeStrings[] = {
    "empty", "collision", "wall", "model"
};

struct level_wall
{
    std::vector<vec2> Points;
};

struct model
{
    std::vector<material *> Materials;
    mesh *Mesh;
};

struct entity
{
    entity_type Type;
    int         ID;
    transform   Transform;

    collider *Collider = NULL;
    level_wall *Wall = NULL;
    model *Model = NULL;
};

struct level
{
    memory_arena Arena;
    char *Name;
    char *Filename;

    int NextEntityID = 0;
    entity *RootEntity;
};

// structs representing the level binary data in file
struct collision_shape_data
{
    uint8 Type;
    float CircleRadius;
    vec2 *Vertices;
};

struct collider_data
{
    uint8 Type;
    collision_shape_data Shape;
};

struct level_wall_data
{
    uint32 NumPoints;
    vec2 *Points;
};

struct entity_data
{
    uint8 Type;
    uint32 Id;
    uint32 NumChildren;
    vec3 Position;
    vec3 Scale;
    vec3 Rotation;
    uint32 WallIndex;
    uint32 ColliderIndex;
};

struct level_data
{
    memory_arena TempArena;
    uint32 NameLen;
    char *Name;3

    entity_data *RootEntity;
};

#endif
