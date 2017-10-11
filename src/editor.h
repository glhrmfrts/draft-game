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
