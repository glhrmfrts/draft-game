#ifndef DRAFT_ENTITY_H
#define DRAFT_ENTITY_H

struct track_segment
{
    int NothingForNow;
};

struct trail;

#define Entity_Kinematic 0x1

enum entity_type
{
    EntityType_Ship,
    EntityType_TrailPiece,
};

struct entity
{
    vec3 Position;
    vec3 Velocity;
    vec3 Size = vec3(1.0f);
    vec3 Rotation = vec3(0.0f);

    entity_type Type;
    uint32 Flags = 0;
    int NumCollisions = 0;

    trail *Trail = NULL;
    track_segment *TrackSegment = NULL;
    bounding_box *Bounds = NULL;
    model *Model = NULL;
};

#define TrailCount 6
struct trail
{
    entity Entities[TrailCount];
    mesh Mesh;
    model Model;
    float Timer = 0;
    int PositionStackIndex = 0;
    bool FirstFrame = true;
};

#endif
