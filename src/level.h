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

    union
    {
        collision_shape *Shape;
        level_wall *Wall;
        model *Model;
    };

    entity *FirstChild = NULL;
    entity *NextSibling = NULL;
    int NumChildren = 0;

    entity() {}
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

    entity_data *FirstChild;
    entity_data *NextSibling;
};

struct level_data
{
    memory_arena TempArena;
    uint32 NameLen;
    char *Name;

    uint32 WallCount;
    level_wall_data *Walls;

    uint32 ColliderCount;
    collider_data *Colliders;

    entity_data *RootEntity;
};

enum level_action_type
{
    LevelAction_TranslateEntity,
    LevelAction_AddEntity,
    LevelAction_RemoveEntity,
    LevelAction_WakeEntity,
    LevelAction_SleepEntity,
    LevelAction_Finish,
};

#endif
