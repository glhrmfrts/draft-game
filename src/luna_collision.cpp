#include <cmath>
#include "luna.h"
#include "luna_collision.h"

static bool
IsColliding(shape_aabb &First, shape_aabb &Second, collision &Result)
{
    vec3 Dif = First.Position - Second.Position;

    float dx = First.Half.x + Second.Half.x - std::abs(Dif.x);
    if (dx <= 0)
    {
        return false;
    }

    float dy = First.Half.y + Second.Half.y - std::abs(Dif.y);
    if (dy <= 0)
    {
        return false;
    }

    float dz = First.Half.z + Second.Half.z - std::abs(Dif.z);
    if (dz <= 0)
    {
        return false;
    }

    Result.Normal = vec3(0.0f);
    if (dx < dy && dy < 0.1f)
    {
        Result.Normal.x = std::copysign(Dif.x, 1);
        Result.Depth = dx;
    }
    else
    {
        Result.Normal.y = std::copysign(Dif.y, 1);
        Result.Depth = dy;
    }
    return true;
}

static void
UpdateEntityAABB(entity *Entity)
{
    // TODO: this is not good
    Entity->Shape->AABB.Half = Entity->Scale*0.5f;
    Entity->Shape->AABB.Position = Entity->Position;
}

static bool
ShouldResolveCollision(collision &Col)
{
    return true;
}

void CollisionDetectionStep(const vector<entity *> &Entities, const vec3 &Gravity, float DeltaTime)
{
    static vector<collision> Collisions;

    size_t FrameCollisionCount = 0;
    size_t EntityCount = Entities.size();
    for (size_t i = 0; i < EntityCount; i++)
    {
        auto EntityA = Entities[i];
        EntityA->NumCollisions = 0;
        if (!EntityA->Shape)
        {
            continue;
        }
        UpdateEntityAABB(EntityA);

        for (size_t j = i+1; j < EntityCount; j++)
        {
            auto EntityB = Entities[j];
            if (!EntityB->Shape)
            {
                continue;
            }
            UpdateEntityAABB(EntityB);

            collision Col;
            if (IsColliding(EntityA->Shape->AABB, EntityB->Shape->AABB, Col))
            {
                size_t Size = Collisions.size();
                if (FrameCollisionCount + 1 > Size)
                {
                    if (Size == 0)
                    {
                        Size = 4;
                    }

                    Collisions.resize(Size * 2);
                }

                Col.First = EntityA;
                Col.Second = EntityB;
                Collisions[FrameCollisionCount++] = Col;
            }
        }
    }

    for (size_t i = 0; i < FrameCollisionCount; i++)
    {
        auto &Col = Collisions[i];
        Col.First->NumCollisions++;
        Col.Second->NumCollisions++;

        if (ShouldResolveCollision(Col))
        {
            Println("collision");
        }
    }

    for (size_t i = 0; i < EntityCount; i++)
    {
        auto Entity = Entities[i];
        if (!(Entity->Flags & EntityFlag_kinematic))
        {
            Entity->Velocity += Gravity * DeltaTime;
        }
        Entity->Position += Entity->Velocity * DeltaTime;
    }
}
