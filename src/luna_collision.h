#ifndef LUNA_COLLISION_H
#define LUNA_COLLISION_H

enum shape_type
{
    Shape_aabb,
};

struct shape_aabb
{
    vec3 Half;
    vec3 Position;
};

struct shape
{
    shape_type Type;

    shape() {}

    union
    {
        shape_aabb AABB;
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

void CollisionDetectionStep(const vector<entity *> &Entities, const vec3 &Gravity, float DeltaTime);

#endif
