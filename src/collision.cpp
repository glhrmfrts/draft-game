// Copyright

#include "collision.h"

inline static void
UpdateEntityCollisionVertices(entity *Entity)
{
    auto &Verts = Entity->Bounds->Vertices;
    size_t Size = Verts.size();
    for (size_t i = 0; i < Size; i++)
    {
        vec4 Vert = vec4{Verts[i].x, Verts[i].y, 0.0f, 1.0f};
        Vert *= GetTransformMatrix(Entity->Transform);
        Verts[i] = vec2{Vert.x, Vert.y};
    }
}

inline static void
ApplyVelocity(entity *Entity, vec3 Velocity)
{
    if (Entity->Flags & Entity_Kinematic)
    {
        return;
    }
    Entity->Transform.Velocity += Velocity;
}

inline static void
ApplyCorrection(entity *Entity, vec3 Correction)
{
    if (Entity->Flags & Entity_Kinematic)
    {
        return;
    }
    Entity->Transform.Position += Correction;
}

static vec2
Support(collision_shape *Shape, vec2 Direction)
{
    switch (Shape->Type)
    {
    case ShapeType_Circle:
        return Shape->Circle.Center * Shape->Circle.Radius * Direction;

    case ShapeType_Polygon:
    {
        float MaxDistance = std::numeric_limits<float>::lowest();
        vec2 MaxVertex;
        for (auto &Vert : Shape->Polygon.Vertices)
        {
            float Distance = glm::dot(Vert, Direction);
            if (Distance > MaxDistance)
            {
                MaxDistance = Distance;
                MaxVertex = Vert;
            }
        }
        return MaxVertex;
    }
    }
}

static vec2
AverageCenter(collision_shape *Shape)
{
    switch (Shape->Type)
    {
    case ShapeType_Circle:
        return Shape->Circle.Center;

    case ShapeType_Polygon:
    {
        vec2 Average(0.0f);
        size_t Count = Shape->Polygon.Vertices.size();
        for (auto &Vert : Shape->Polygon.Vertices)
        {
            Average += Vert;
        }
        Average /= Count;
        return Average;
    }
    }
}

static float
TripleProduct(vec2 a, vec2 b, vec2 c)
{
    float ac = glm::dot(a, c);
    float bc = glm::dot(b, c);
    vec2 Result = (b * ac) - (a * bc);
    return Result;
}

static bool
GJKCollision(collision_shape *Shape1, collision_shape *Shape2, collision *Col)
{
    vec2 Simplex[3];
    int  SimplexCount = 0;
    int  SimplexIndex = 0;
    vec2 Direction;

    for (;;)
    {
        switch (SimplexCount)
        {
        case 0:
            Direction = AverageCenter(Shape2) - AverageCenter(Shape1);
            SimplexIndex = SimplexCount++;
            break;

        case 1:
            Direction *= -1;
            SimplexIndex = SimplexCount++;
            break;

        case 2:
        {
            vec2 b = Simplex[1];
            vec2 c = Simplex[0];

            // cb is the line formed by the two vertices
            vec2 cb = b - c;

            // c0 is the line from the first vertex to the origin
            vec2 c0 = c * -1;
            Direction = TripleProduct(cb, c0, cb);
            SimplexIndex = SimplexCount++;
        }

        case 3:
        {
            vec2 a = Simplex[2];
            vec2 b = Simplex[1];
            vec2 c = Simplex[0];

            vec2 a0 = a * -1;
            vec2 ab = b - a;
            vec2 ac = c - a;
            vec2 abPerp = TripleProduct(ac, ab, ab);
            vec2 acPerp = TripleProduct(ab, ac, ac);

            if (glm::dot(abPerp, a0) > 0.0f)
            {
                // get rid of c
                SimplexIndex = 0;
                Direction = abPerp;
            }
            else if (glm::dot(acPerp, a0) > 0.0f)
            {
                // get rid of b
                SimplexIndex = 1;
                Direction = acPerp;
            }
            else
            {
                return true;
            }
        }

        vec2 NewVertex = Support(Shape1, Direction) - Support(Shape2, Direction);
        if (glm::dot(Direction, NewVertex) > 0.0f)
        {
            Simplex[SimplexIndex] = NewVertex;
        }
        else
        {
            return false;
        }
    }
}

#define ClimbHeight 0.26f
void DetectCollisions(const vector<entity *> Entities, vector<collision> &Collisions, size_t &NumCollisions)
{
    // The entities' vertices only get updated in the integration
    // phase, which happens after the collision detection so we need
    // to skip the first frame
    static bool FirstFrame = true;
    if (FirstFrame)
    {
        FirstFrame = false;
        return;
    }

    NumCollisions = 0;
    for (auto it = Entities.begin(); it != Entities.end(); it++)
    {
        auto *EntityA = *it;
        if (!EntityA) continue;

        EntityA->NumCollisions = 0;

        auto it2 = it;
        it2++;
        for (; it2 != Entities.end(); it2++)
        {
            auto *EntityB = *it2;
            if (!EntityB) continue;

            collision_result Col;
            if (GJKCollision(EntityA->Shape, EntityB->Shape, &Col))
            {
                size_t Size = Collisions.size();
                if (NumCollisions + 1 > Size)
                {
                    if (Size == 0)
                    {
                        Size = 4;
                    }

                    Collisions.resize(Size * 2);
                }
                Col.First = EntityA;
                Col.Second = EntityB;
                Collisions[NumCollisions++] = Col;
            }
        }
    }
}

void Integrate(const vector<entity *> Entities, vec3 Gravity, float DeltaTime)
{
    for (auto *Entity : Entities)
    {
        if (!Entity) continue;

        if (!(Entity->Flags & Entity_Kinematic))
        {
            Entity->Transform.Velocity += Gravity * DeltaTime;
        }
        Entity->Transform.Position += Entity->Transform.Velocity * DeltaTime;
        UpdateEntityBounds(Entity);
    }
}

void ResolveCollision(collision_result &Col)
{
    vec3 rv = Col.Second->Transform.Velocity - Col.First->Transform.Velocity;
    float VelNormal = glm::dot(rv, Col.Normal);
    if (VelNormal > 0.0f)
    {
        return;
    }

    float j = -1 * VelNormal;
    vec3 Impulse = Col.Normal * j;
    ApplyVelocity(Col.First, -Impulse);
    ApplyVelocity(Col.Second, Impulse);

    float s = std::max(Col.Depth - 0.01f, 0.0f);
    vec3 Correction = Col.Normal * s;
    ApplyCorrection(Col.First, -Correction);
    ApplyCorrection(Col.Second, Correction);
}
