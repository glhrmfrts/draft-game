#ifndef DRAFT_ENTITY_H
#define DRAFT_ENTITY_H

struct entity;
struct entity_world;

struct model
{
    std::vector<material *> Materials;
    mesh *Mesh;
    int SortNumber = -1;
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
	typedef void(callback)(entity_world *, entity *);

    int Count = 0;
    float Size = 0;
    float DistanceFromCamera = 0;
	callback *Callback = NULL;
};

struct powerup
{
    color Color;
    float TimeSpawn;
};

struct lane_slot
{
    int Index;
	int Lane;
	bool Occupy = true;
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

#define CHECKPOINT_FADE_OUT_DURATION 4.0f
struct checkpoint
{
    float Timer = 0;
    checkpoint_state State = CheckpointState_Initial;
};

struct finish
{
	bool Finished = false;
};

struct explosion;
struct trail;
struct trail_group;
struct trail_piece;

#define EntityFlag_Kinematic        0x1
#define EntityFlag_IsPlayer         0x2
#define EntityFlag_RemoveOffscreen  0x4
#define EntityFlag_UpdateMovement   0x8
enum collider_type
{
    ColliderType_Crystal,
    ColliderType_Ship,
    ColliderType_TrailPiece,
    ColliderType_Asteroid,
	ColliderType_EnemySkull,
};
struct collider
{
    bounding_box Box;
    vec3 Scale = vec3(1.0f);
	collider_type Type;
	bool Active = false; // Passive if false
};

struct audio_source
{
	ALuint ID;
	float Gain = 1;
	audio_clip *Clip = NULL;
	bool Loop = false;
};

struct road_piece
{
	float BackLeft;
	float BackRight;
	float FrontLeft;
	float FrontRight;
};

enum entity_type
{
	EntityType_Invalid,
	EntityType_Crystal,
	EntityType_Ship,
	EntityType_Explosion,
	EntityType_Powerup,
	EntityType_Asteroid,
	EntityType_Checkpoint,
	EntityType_Finish,
	EntityType_EnemySkull,
	EntityType_MAX,
};

struct entity
{
    transform Transform;
    uint32 Flags = 0;
    int NumCollisions = 0;
    memory_pool_entry *PoolEntry = NULL;
	entity_type Type = EntityType_Invalid;

    // entity components
    checkpoint *Checkpoint = NULL;
	finish *Finish = NULL;
    asteroid *Asteroid = NULL;
    powerup *Powerup = NULL;
    explosion *Explosion = NULL;
    ship *Ship = NULL;
	trail *Trail = NULL;
	trail_group *TrailGroup = NULL;
    trail_piece *TrailPiece = NULL;
    entity_repeat *Repeat = NULL;
    collider *Collider = NULL;
    model *Model = NULL;
    lane_slot *LaneSlot = NULL;
    frame_rotation *FrameRotation = NULL;
	audio_source *AudioSource = NULL;
	road_piece *RoadPiece = NULL;

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
};

#define TRAIL_COUNT 24
struct trail
{
    entity Entities[TRAIL_COUNT];
    float Timer = 0;
    float Radius = 0;
    int PositionStackIndex = 0;
    bool FirstFrame = true;
    bool RenderOnly = false;
};

struct trail_group
{
	mesh Mesh;
	model Model;
	std::vector<entity *> Entities;
	std::vector<vec3 *> PointCache;
	size_t Count;
	float Radius;
	float Timer = 0;
	bool FirstFrame = true;
	bool RenderOnly;
};

struct trail_piece
{
    entity *Owner;
};

struct background_instance
{
	color Color;
	vec2 Offset;
};

struct background_state
{
    std::vector<background_instance> Instances;
	vec2 Offset = vec2(0.0f);
    texture *RandomTexture;
	float Time = 0;
};

struct gen_state;
struct entity_world;

struct entity_update_args
{
	entity_world *World;
	entity *Entity;
	memory_pool_entry *Entry;
	float DeltaTime;
};

struct road_mesh_manager
{
	memory_arena Arena;
	string_format KeyFormat;
	std::string Key;
	std::unordered_map<std::string, mesh *> Meshes;
};

enum road_change
{
	RoadChange_NarrowRight,
	RoadChange_NarrowLeft,
	RoadChange_NarrowCenter,
	RoadChange_WidensLeft,
	RoadChange_WidensRight,
	RoadChange_WidensCenter,
};

struct road_state
{
	road_piece NextPiece;
	road_change Change;
	bool ShouldChange = false;
	mesh *RoadMesh = NULL;
	float Left = 0;
	float Right = 0;
	int PlayerActiveEntityIndex = 0;
	int MinLaneIndex = 0;
	int MaxLaneIndex = 4;
};

struct entity_world
{
	thread_pool UpdateThreadPool;

    memory_arena PersistentArena;
    memory_arena Arena;

    memory_pool ShipPool;
    memory_pool CrystalPool;
    memory_pool ExplosionPool;
    memory_pool PowerupPool;
    memory_pool AsteroidPool;
    memory_pool CheckpointPool;
	memory_pool FinishPool;
	memory_pool EnemySkullPool;
	generic_pool<entity_update_args> UpdateArgsPool;

    mesh *FloorMesh = NULL;
    mesh *ShipMesh = NULL;
    mesh *CrystalMesh = NULL;
    mesh *AsteroidMesh = NULL;
    mesh *CheckpointMesh = NULL;
    mesh *BackgroundMesh = NULL;
	mesh *FinishMesh = NULL;

	road_state RoadState;
	road_mesh_manager RoadMeshManager;

    background_state BackgroundState;
    gen_state *GenState;

	std::vector<entity *> AudioEntities;
    std::vector<entity *> CrystalEntities;
    std::vector<entity *> CheckpointEntities;
    std::vector<entity *> AsteroidEntities;
    std::vector<entity *> PowerupEntities;
    std::vector<entity *> ModelEntities;
    std::vector<entity *> ActiveCollisionEntities;
	std::vector<entity *> PassiveCollisionEntities;
    std::vector<entity *> TrailGroupEntities;
    std::vector<entity *> ShipEntities;
    std::vector<entity *> ExplosionEntities;
    std::vector<entity *> RepeatingEntities;
    std::vector<entity *> RemoveOffscreenEntities;
    std::vector<entity *> LaneSlotEntities;
    std::vector<entity *> RotatingEntities;
    std::vector<entity *> MovementEntities;
	std::vector<entity *> RoadPieceEntities;
	std::vector<entity *> FinishEntities;
	std::vector<entity *> EnemySkullEntities;
    asset_loader *AssetLoader = NULL;
    camera *Camera = NULL;
    explosion *LastExplosion = NULL;
    entity *PlayerEntity;
    entity *RoadEntity;
    int NumEntities = 0;

	std::function<void()> OnSpawnFinish;
	bool ShouldSpawnFinish = false;
	bool DisableRoadRepeat = false;

    float RoadTangentPoint = std::numeric_limits<float>::infinity();
    bool ShouldRoadTangent = false;
};

enum gen_type;

void RoadChange(entity_world &w, road_change change);
void SetEntityClip(entity_world &world, gen_type genType, audio_clip *track);

#endif
