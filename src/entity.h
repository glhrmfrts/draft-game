#ifndef DRAFT_ENTITY_H
#define DRAFT_ENTITY_H

struct model
{
    std::vector<material *> Materials;
    mesh *Mesh;
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

struct explosion
{
    mesh Mesh;
    color Color;
    float LifeTime;
    std::vector<vec3> Triangles;
    std::vector<vec3> Normals;
    std::vector<transform> Pieces;
};

struct trail;
struct trail_piece;

#define EntityFlag_Kinematic 0x1
#define EntityFlag_IsPlayer  0x2
enum collider_type
{
    ColliderType_Ship,
    ColliderType_TrailPiece,
};
struct collider
{
    collider_type Type;
    bounding_box Box;
};

struct entity
{
    transform Transform;
    uint32 Flags = 0;
    int NumCollisions = 0;

    player_state *PlayerState = NULL;
    explosion *Explosion = NULL;
    ship *Ship = NULL;
    trail *Trail = NULL;
    trail_piece *TrailPiece = NULL;
    collider *Collider = NULL;
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

#endif
