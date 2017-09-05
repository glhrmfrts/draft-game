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

struct player_ship
{
    float CurrentDraftTime = 0;
    float DraftCharge = 0;
    int NumTrailCollisions = 0;
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

    player_ship *PlayerShip;
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
