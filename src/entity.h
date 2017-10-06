#ifndef DRAFT_ENTITY_H
#define DRAFT_ENTITY_H

/*
struct model
{
    vector<material *> Materials;
    mesh *Mesh;
};

struct collision_bounds
{
    bounding_box Box;
};

enum enemy_type
{
    EnemyType_Default,
    EnemyType_Explosive,
};
struct ship
{
    color Color;
    color OutlineColor;
    float CurrentDraftTime = 0;
    float DraftCharge = 0;
    int NumTrailCollisions = 0;
    enemy_type EnemyType;
};

struct player_state
{
    int Score = 0;
};

struct wall_state
{
    float Timer = 0;
    float BaseZ;
};

struct explosion
{
    mesh Mesh;
    color Color;
    float LifeTime;
    vector<vec3> Triangles;
    vector<vec3> Normals;
    vector<transform> Pieces;
};

struct trail;
struct trail_piece;

#define Entity_Kinematic 0x1
#define Entity_IsPlayer  0x2
enum entity_type
{
    EntityType_Ship,
    EntityType_TrailPiece,
    EntityType_Crystal,
    EntityType_TrackSegment,
    EntityType_Wall,
};
struct entity
{
    transform Transform;
    entity_type Type;
    uint32 Flags = 0;
    int NumCollisions = 0;

    wall_state *WallState = NULL;
    player_state *PlayerState = NULL;
    explosion *Explosion = NULL;
    ship *Ship = NULL;
    trail *Trail = NULL;
    trail_piece *TrailPiece = NULL;
    collision_bounds *Bounds = NULL;
    model *Model = NULL;
};

#define TrailCount 16
struct trail
{
    entity Entities[TrailCount];
    mesh Mesh;
    model Model;
    float Timer = 0;
    int PositionStackIndex = 0;
    bool FirstFrame = true;
};

struct trail_piece
{
    entity *Owner;
};
*/

#endif
