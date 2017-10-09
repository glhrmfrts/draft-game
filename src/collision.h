#ifndef DRAFT_COLLISION_H
#define DRAFT_COLLISION_H

struct bounding_box
{
    vec3 Center = vec3(0.0f);
    vec3 Half = vec3(0.0f);
};

struct circle
{
    vec2 Center;
    float Radius;
};

struct polygon
{
    std::vector<vec2> Vertices;
};

enum collision_shape_type
{
    CollisionShapeType_Circle,
    CollisionShapeType_Polygon,
};

struct collision_shape
{
    collision_shape_type Type;
    union
    {
        struct { bounding_box Box; };
        struct { circle Circle; };
        struct { polygon Polygon; };
    };

    collision_shape() {}
};

struct entity;
struct collision_result
{
    vec2  Normal;
    float Depth;
    entity *First;
    entity *Second;
};

struct physics_state
{
    std::vector<vec2> Simplex;
};

inline static bounding_box
BoundsFromMinMax(vec3 Min, vec3 Max)
{
    bounding_box Result = {};
    Result.Half = (Max-Min) * 0.5f;
    Result.Center = Min + Result.Half;
    return Result;
}

#endif
