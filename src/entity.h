#ifndef DRAFT_ENTITY_H
#define DRAFT_ENTITY_H

struct track_segment
{
    int NothingForNow;
};

#define TrailCount 6
struct trail
{
    mesh Mesh;
    model Model;
    vec3 Pos[TrailCount];
    float Timer = 0;
    int PositionStackIndex = 0;
    bool FirstFrame = true;
};

#define Entity_Kinematic 0x1
struct entity
{
    vec3 Position;
    vec3 Velocity;
    vec3 Size = vec3(1.0f);
    vec3 Rotation = vec3(0.0f);

    uint32 Flags = 0;
    int NumCollisions = 0;

    trail *Trail = NULL;
    track_segment *TrackSegment = NULL;
    bounding_box *Bounds = NULL;
    model *Model = NULL;
};

#endif
