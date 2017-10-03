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

struct collision_shape
{
    shape_type Type;
    union
    {
        bounding_box Box;
        circle Circle;
        polygon Polygon;
    };
};

struct entity;
struct collision_result
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

#endif
