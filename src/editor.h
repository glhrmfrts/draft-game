#ifndef DRAFT_EDITOR_H
#define DRAFT_EDITOR_H

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
    transform CursorTransform;

    char *Name;
    char *Filename;
};

#endif
