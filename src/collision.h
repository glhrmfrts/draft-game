#ifndef DRAFT_COLLISION_H
#define DRAFT_COLLISION_H

#include "common.h"

enum shape_type
{
    Shape_BoundingBox,
};

struct bounding_box
{
    vec3 Center = vec3(0.0f);
    vec3 Half = vec3(0.0f);
};

struct shape
{
    shape_type Type;

    shape() {}

    union
    {
        bounding_box BoundingBox;
    };
};

struct entity;
struct collision
{
    vec3 Normal;
    float Depth;
    entity *First;
    entity *Second;
};

inline static bounding_box
BoundsFromMinMax(vec3 Min, vec3 Max)
{
    bounding_box Result = {};
    Result.Half = (Max-Min) * 0.5f;
    Result.Center = Min + Result.Half;
    return Result;
}

void DetectCollisions(const vector<entity *> Entities, vector<collision> &Collisions, size_t &NumCollisions);
void Integrate(const vector<entity *> Entities, vec3 Gravity, float DeltaTime);
void ResolveCollision(collision &Col);

#endif
