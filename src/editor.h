#ifndef DRAFT_EDITOR_H
#define DRAFT_EDITOR_H

enum entity_type
{
    EntityType_Empty,
    EntityType_Collision,
};

static const char *EntityTypeStrings[] = {
    "empty", "collision"
};

struct entity
{
    entity_type Type;
    int         ID;
    transform   Transform;

    union
    {
        collision_shape Shape;
    };

    int    NameIndex;
    entity *FirstChild = NULL;
    entity *NextSibling = NULL;

    entity() {}
};

struct level
{
    memory_arena Arena;
    //std::vector<sl_string> Strings;

    entity *RootEntity;
    int    NextEntityID = 0;
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

    mesh CursorMesh;
    mesh LineMesh;
    std::vector<vec3> LinePoints;
    bool EditingLines;

    level *Level;
    entity *SelectedEntity;

    char *Name;
    char *Filename;
};

#endif
