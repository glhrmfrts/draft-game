// Copyright

#include "collision.h"

inline static void
UpdateEntityBounds(entity *Entity)
{
    // TODO: this is not good
    if (Entity->Model)
    {
        Entity->Collider->Box = BoundsFromMinMax(Entity->Model->Mesh->Min*Entity->Transform.Scale,
                                               Entity->Model->Mesh->Max*Entity->Transform.Scale);
        Entity->Collider->Box.Center += Entity->Transform.Position;
    }
    else
    {
        //Entity->Bounds->Center = Entity->Position;
        //Entity->Bounds->Half = Entity->Size*0.5f;
    }
}

inline static void
ApplyVelocity(entity *Entity, vec3 Velocity)
{
    if (Entity->Flags & EntityFlag_Kinematic)
    {
        return;
    }
    Entity->Transform.Velocity += Velocity;
}

inline static void
ApplyCorrection(entity *Entity, vec3 Correction)
{
    if (Entity->Flags & EntityFlag_Kinematic)
    {
        return;
    }
    Entity->Transform.Position += Correction;
}

#define ClimbHeight 0.26f
void DetectCollisions(const std::vector<entity *> Entities, std::vector<collision_result> &Collisions, size_t &NumCollisions)
{
    // The entities' bounding boxes only get updated in the integration
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

            auto &First = EntityA->Collider->Box;
            auto &Second = EntityB->Collider->Box;
            vec3 Dif = Second.Center - First.Center;
            float dx = First.Half.x + Second.Half.x - std::abs(Dif.x);
            if (dx <= 0)
            {
                continue;
            }

            float dy = First.Half.y + Second.Half.y - std::abs(Dif.y);
            if (dy <= 0)
            {
                continue;
            }

            float dz = First.Half.z + Second.Half.z - std::abs(Dif.z);
            if (dz <= 0)
            {
                continue;
            }

            // At this point we have a collision
            collision_result Col;
            Col.Normal = vec3(0.0f);
            if (dx < dy && dy > ClimbHeight)
            {
                Col.Normal.x = std::copysign(1, Dif.x);
                Col.Depth = dx;
            }
            else if (dz < dy && dy > ClimbHeight)
            {
                Col.Normal.z = std::copysign(1, Dif.z);
                Col.Depth = dz;
            }
            else
            {
                Col.Normal.y = std::copysign(1, Dif.y);
                Col.Depth = dy;
            }

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

void Integrate(const vector<entity *> Entities, vec3 Gravity, float DeltaTime)
{
    for (auto *Entity : Entities)
    {
        if (!Entity) continue;

        if (!(Entity->Flags & EntityFlag_Kinematic))
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
