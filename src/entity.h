#ifndef DRAFT_ENTITY_H
#define DRAFT_ENTITY_H

struct entity;

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
    int ColorIndex = 0;
    enemy_type EnemyType;
    entity *DraftTarget;
    bool DraftActive = false;
};

struct entity_repeat
{
    int Count = 0;
    float Size = 0;
    float DistanceFromCamera = 0;
};

struct powerup
{
    color Color;
    float TimeSpawn;
};

struct lane_slot
{
    int Index;
};

struct frame_rotation
{
    vec3 Rotation;
};

struct explosion;
struct trail;
struct trail_piece;

#define EntityFlag_Kinematic        0x1
#define EntityFlag_IsPlayer         0x2
#define EntityFlag_RemoveOffscreen  0x4
enum collider_type
{
    ColliderType_Crystal,
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

    powerup *Powerup = NULL;
    explosion *Explosion = NULL;
    ship *Ship = NULL;
    trail *Trail = NULL;
    trail_piece *TrailPiece = NULL;
    entity_repeat *Repeat = NULL;
    collider *Collider = NULL;
    model *Model = NULL;
    lane_slot *LaneSlot = NULL;
    frame_rotation *FrameRotation = NULL;

    inline vec3 &Pos() { return Transform.Position; }
    inline vec3 &Vel() { return Transform.Velocity; }
    inline vec3 &Scl() { return Transform.Scale; }
    inline vec3 &Rot() { return Transform.Rotation; }

    inline void SetPos(vec3 p) { Transform.Position = p; }
    inline void SetVel(vec3 v) { Transform.Velocity = v; }
    inline void SetScl(vec3 s) { Transform.Scale = s; }
    inline void SetRot(vec3 r) { Transform.Rotation = r; }
};

#define EXPLOSION_PARTS_COUNT 12
struct explosion
{
    mesh Mesh;
    color Color;
    float LifeTime;
    entity Entities[EXPLOSION_PARTS_COUNT];
};

#define TrailCount 16
struct trail
{
    entity Entities[TrailCount];
    mesh Mesh;
    model Model;
    float Timer = 0;
    float Radius = 0;
    int PositionStackIndex = 0;
    bool FirstFrame = true;
    bool RenderOnly = false;
};

struct trail_piece
{
    entity *Owner;
};

struct entity_world
{
    memory_arena Arena;

    std::vector<entity *> PowerupEntities;
    std::vector<entity *> ModelEntities;
    std::vector<entity *> CollisionEntities;
    std::vector<entity *> TrailEntities;
    std::vector<entity *> ShipEntities;
    std::vector<entity *> ExplosionEntities;
    std::vector<entity *> RepeatingEntities;
    std::vector<entity *> RemoveOffscreenEntities;
    std::vector<entity *> LaneSlotEntities;
    std::vector<entity *> RotatingEntities;
    camera *Camera = NULL;
    explosion *LastExplosion = NULL;
    int NumEntities = 0;
};

#endif
