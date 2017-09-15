#ifndef DRAFT_ENTITY_H
#define DRAFT_ENTITY_H

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

/*
Playing with level format

tick 5
{
  ship(lane=2)
  ship(lane=4)

  tick 3
  {
    block(lane=3)
    block(lane=2, distance=40)
    block(lane=4, distance=40)
  }
}

struct level
{
  u16 NumOfRootTicks;
  level_tick Ticks[NumOfRootTicks];
};

struct level_tick
{
  int Milliseconds;

  u16 NumOfEvents;
  level_event Events[NumOfEvents];

  u16 NumOfChildTicks;
  level_tick Ticks[NumOfChildTicks];
};

struct level_event
{
  level_event_type Type;
  union
  {
    level_ship_event  ShipEvent;
    level_block_event BlockEvent;
    level_track_event TrackEvent;
  };
};

 */

#endif
