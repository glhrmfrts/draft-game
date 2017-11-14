#ifndef DRAFT_ENTITY_H
#define DRAFT_ENTITY_H

struct entity;

struct model
{
    std::vector<material *> Materials;
    mesh *Mesh;
    bool Visible = true;
};

struct ship
{
    color Color;
    color OutlineColor;
    float PassedVelocity = 0;
    int ColorIndex = 0;
    bool Scored = false;
    bool HasBeenDrafted = false;
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

struct asteroid
{
    bool Exploded = false;
};

enum checkpoint_state
{
    CheckpointState_Initial,
    CheckpointState_Drafted,
    CheckpointState_Active,
};

#define CHECKPOINT_FADE_OUT_DURATION 1.0f
struct checkpoint
{
    float Timer = 0;
    checkpoint_state State = CheckpointState_Initial;
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
    ColliderType_Asteroid,
};
struct collider
{
    collider_type Type;
    bounding_box Box;
    vec3 Scale = vec3(1.0f);
};

struct entity
{
    transform Transform;
    uint32 Flags = 0;
    int NumCollisions = 0;
    memory_pool_entry *PoolEntry = NULL;

    // entity components
    checkpoint *Checkpoint = NULL;
    asteroid *Asteroid = NULL;
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

#define TRAIL_COUNT 24
struct trail
{
    entity Entities[TRAIL_COUNT];
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

    memory_pool ShipPool;
    memory_pool CrystalPool;
    memory_pool ExplosionPool;
    memory_pool PowerupPool;
    memory_pool AsteroidPool;
    memory_pool CheckpointPool;

    mesh *FloorMesh = NULL;
    mesh *ShipMesh = NULL;
    mesh *CrystalMesh = NULL;
    mesh *RoadMesh = NULL;
    mesh *AsteroidMesh = NULL;
    mesh *CheckpointMesh = NULL;
    mesh *BackgroundMesh = NULL;

    std::vector<entity *> CheckpointEntities;
    std::vector<entity *> AsteroidEntities;
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
    asset_loader *AssetLoader = NULL;
    camera *Camera = NULL;
    explosion *LastExplosion = NULL;
    entity *PlayerEntity;
    int NumEntities = 0;
};

#endif
