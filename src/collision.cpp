#include "collision.h"
#include "level.h"

inline static void
UpdateEntityBounds(entity *Entity)
{
    // TODO: this is not good
    if (Entity->Model)
    {
        Entity->Shape->BoundingBox = BoundsFromMinMax(Entity->Model->Mesh->Min*Entity->Size,
                                                      Entity->Model->Mesh->Max*Entity->Size);
        Entity->Shape->BoundingBox.Center += Entity->Position;
    }
    else
    {
        Entity->Shape->BoundingBox.Center = Entity->Position;
        Entity->Shape->BoundingBox.Half = Entity->Size*0.5f;
    }
}

inline static void
ApplyVelocity(entity *Entity, vec3 Velocity)
{
    if (Entity->Flags & EntityFlag_Kinematic)
    {
        return;
    }
    Entity->Velocity += Velocity;
}

inline static void
ApplyCorrection(entity *Entity, vec3 Correction)
{
    if (Entity->Flags & EntityFlag_Kinematic)
    {
        return;
    }
    Entity->Position += Correction;
}

#define ClimbHeight 0.26f
void DetectCollisions(const vector<entity *> Entities, vector<collision> &Collisions, size_t &NumCollisions)
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
    size_t EntityCount = Entities.size();
    for (size_t i = 0; i < EntityCount; i++)
    {
        auto EntityA = Entities[i];
        EntityA->NumCollisions = 0;

        for (size_t j = i+1; j < EntityCount; j++)
        {
            auto EntityB = Entities[j];
            auto &First = EntityA->Shape->BoundingBox;
            auto &Second = EntityB->Shape->BoundingBox;
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
            collision Col;
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
    size_t EntityCount = Entities.size();
    for (size_t i = 0; i < EntityCount; i++)
    {
        auto Entity = Entities[i];
        if (!(Entity->Flags & EntityFlag_Kinematic))
        {
            Entity->Velocity += Gravity * DeltaTime;
        }
        Entity->Position += Entity->Velocity * DeltaTime;
        UpdateEntityBounds(Entity);
    }
}

void ResolveCollision(collision &Col)
{
    vec3 rv = Col.Second->Velocity - Col.First->Velocity;
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
