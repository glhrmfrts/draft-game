// Copyright

#include "collision.h"

/*
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
*/

inline static void
ApplyVelocity(entity *Entity, vec2 Velocity)
{

}

inline static void
ApplyCorrection(entity *Entity, vec2 Correction)
{

}

static vec2
Support(collision_shape *Shape, vec2 Direction)
{
    Direction = glm::normalize(Direction);
    switch (Shape->Type)
    {
    case CollisionShapeType_Circle:
        return Shape->Circle.Center + Shape->Circle.Radius * Direction;

    case CollisionShapeType_Polygon:
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

    default:
        assert(!"invalid shape type");
        break;
    }
}

static vec2
AverageCenter(collision_shape *Shape)
{
    switch (Shape->Type)
    {
    case CollisionShapeType_Circle:
        return Shape->Circle.Center;

    case CollisionShapeType_Polygon:
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

    default:
        assert(!"invalid shape type");
        break;
    }
}

static vec2
TripleProduct(vec2 a, vec2 b, vec2 c)
{
    float ac = glm::dot(a, c);
    float bc = glm::dot(b, c);
    vec2 Result = (b * ac) - (a * bc);
    return Result;
}

static vec2
CalculateSupport(collision_shape *Shape1, collision_shape *Shape2, vec2 Direction)
{
    return Support(Shape1, Direction) - Support(Shape2, Direction * -1.0f);
}

static bool
CalculateBaseSimplex(physics_state &State, collision_shape *Shape1, collision_shape *Shape2)
{
    auto &Simplex = State.Simplex;
    Simplex.clear();

    vec2 Direction;
    for (;;)
    {
        switch (Simplex.size())
        {
        case 0:
            Direction = AverageCenter(Shape2) - AverageCenter(Shape1);
            break;

        case 1:
            Direction *= -1.0f;
            break;

        case 2:
        {
            vec2 b = Simplex[1];
            vec2 c = Simplex[0];

            // cb is the line formed by the two vertices
            vec2 cb = b - c;

            // c0 is the line from the first vertex to the origin
            vec2 c0 = c * -1.0f;
            Direction = TripleProduct(cb, c0, cb);
        }

        case 3:
        {
            vec2 a = Simplex[2];
            vec2 b = Simplex[1];
            vec2 c = Simplex[0];

            vec2 a0 = a * -1.0f;
            vec2 ab = b - a;
            vec2 ac = c - a;
            vec2 abPerp = TripleProduct(ac, ab, ab);
            vec2 acPerp = TripleProduct(ab, ac, ac);

            if (glm::dot(abPerp, a0) > 0.0f)
            {
                // get rid of c
                auto End = std::remove(Simplex.begin(), Simplex.end(), c);
                Simplex.erase(End, Simplex.end());

                Direction = abPerp;
            }
            else if (glm::dot(acPerp, a0) > 0.0f)
            {
                // get rid of b
                auto End = std::remove(Simplex.begin(), Simplex.end(), b);
                Simplex.erase(End, Simplex.end());

                Direction = acPerp;
            }
            else
            {
                return true;
            }
        }
        }

        vec2 NewVertex = CalculateSupport(Shape1, Shape2, Direction);
        if (glm::dot(Direction, NewVertex) > 0.0f)
        {
            Simplex.push_back(NewVertex);
        }
        else
        {
            return false;
        }
    }
}

#define PHYSICS_EPA_ITERATIONS 16
#define PHYSICS_MIN_DISTANCE   0.000001f

enum polygon_winding
{
    PolygonWinding_Clockwise,
    PolygonWinding_CounterClockwise,
};
struct polygon_edge
{
    vec2  Normal;
    float Distance;
    int   Index;
};
static void
FindClosestEdge(const std::vector<vec2> &Simplex, polygon_winding Winding, polygon_edge *Edge)
{
    float ClosestDistance = std::numeric_limits<float>::max();
    vec2  ClosestNormal;
    int   ClosestIndex = 0;
    vec2  Line;

    size_t Count = Simplex.size();
    for (size_t i = 0; i < Count; i++)
    {
        size_t j = i + 1;
        if (j >= Count)
        {
            j = 0;
        }

        vec2 Normal;
        vec2 Line = Simplex[j];
        Line -= Simplex[i];
        switch (Winding)
        {
        case PolygonWinding_Clockwise:
            Normal = vec2{Line.y, -Line.x};
            break;

        case PolygonWinding_CounterClockwise:
            Normal = vec2{-Line.y, Line.x};
            break;
        }
        Normal = glm::normalize(Normal);

        float Dist = glm::dot(Normal, Simplex[i]);
        if (Dist < ClosestDistance)
        {
            ClosestDistance = Dist;
            ClosestNormal = Normal;
            ClosestIndex = j;
        }
    }

    Edge->Distance = ClosestDistance;
    Edge->Normal = ClosestNormal;
    Edge->Index = ClosestIndex;
}

static bool
GJKCollision(physics_state &State, collision_shape *Shape1, collision_shape *Shape2, collision_result *Col)
{
    if (!CalculateBaseSimplex(State, Shape1, Shape2))
    {
        return false;
    }

    auto &Simplex = State.Simplex;
    float e0 = (Simplex[1].x - Simplex[0].x) * (Simplex[1].y + Simplex[0].y);
    float e1 = (Simplex[2].x - Simplex[1].x) * (Simplex[2].y + Simplex[1].y);
    float e2 = (Simplex[0].x - Simplex[2].x) * (Simplex[0].y + Simplex[2].y);
    polygon_winding Winding;
    if (e0 + e1 + e2 >= 0.0f)
    {
        Winding = PolygonWinding_Clockwise;
    }
    else
    {
        Winding = PolygonWinding_CounterClockwise;
    }

    polygon_edge Edge;
    for (int i = 0; i < PHYSICS_EPA_ITERATIONS; i++)
    {
        FindClosestEdge(Simplex, Winding, &Edge);
        vec2 SupportVert = CalculateSupport(Shape1, Shape2, Edge.Normal);
        float Distance = glm::dot(SupportVert, Edge.Normal);
        Col->Normal = Edge.Normal;
        Col->Depth = Distance;

        if (std::abs(Distance - Edge.Distance) <= PHYSICS_MIN_DISTANCE)
        {
            break;
        }
        else
        {
            Simplex.insert(Simplex.begin() + Edge.Index, SupportVert);
        }
    }
    return true;
}

/*
#define ClimbHeight 0.26f
void DetectCollisions(const vector<entity *> Entities, vector<collision_result> &Collisions, size_t &NumCollisions)
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
*/

void Integrate(const vector<entity *> Entities, vec3 Gravity, float DeltaTime)
{
    for (auto *Entity : Entities)
    {
        if (!Entity) continue;

        if (false)
        {
            Entity->Transform.Velocity += Gravity * DeltaTime;
        }
        Entity->Transform.Position += Entity->Transform.Velocity * DeltaTime;
        //UpdateEntityBounds(Entity);
    }
}

void ResolveCollision(collision_result &Col)
{
    vec3 rv3 = Col.Second->Transform.Velocity - Col.First->Transform.Velocity;
    vec2 rv = vec2{rv3.x, rv3.y};
    float VelNormal = glm::dot(rv, Col.Normal);
    if (VelNormal > 0.0f)
    {
        return;
    }

    float j = -1 * VelNormal;
    vec2 Impulse = Col.Normal * j;
    ApplyVelocity(Col.First, -Impulse);
    ApplyVelocity(Col.Second, Impulse);

    float s = std::max(Col.Depth - 0.01f, 0.0f);
    vec2 Correction = Col.Normal * s;
    ApplyCorrection(Col.First, -Correction);
    ApplyCorrection(Col.Second, Correction);
}
