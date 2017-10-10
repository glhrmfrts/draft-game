#ifndef DRAFT_EDITOR_H
#define DRAFT_EDITOR_H

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

enum editor_mode
{
    EditorMode_None,
    EditorMode_Wall,
    EditorMode_Collision,
};

static const char *EditorModeStrings[] = {
    "none", "wall", "collision"
};

struct editor_state
{
    editor_mode Mode;
    memory_arena Arena;
    thread_pool Pool;
    int NumCurrentJobs = 0;

    mesh CursorMesh;
    mesh LineMesh;
    std::vector<vec3> LinePoints;
    bool EditingLines;
    bool SnapToInteger = true;

    level *Level;
    entity *SelectedEntity = NULL;

    tween_sequence *LevelTextSequence = NULL;
    float LevelTextAlpha = 0;
};

#endif
