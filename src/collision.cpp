// Copyright

#include "collision.h"

static void UpdateEntityBounds(entity *ent)
{
    // TODO: this is not good
    if (ent->Model)
    {
        ent->Collider->Box = BoundsFromMinMax(ent->Model->Mesh->Min*ent->Scl()*ent->Collider->Scale,
                                              ent->Model->Mesh->Max*ent->Scl()*ent->Collider->Scale);
        ent->Collider->Box.Center += ent->Transform.Position;
    }
    else
    {
        //ent->Bounds->Center = ent->Position;
        //ent->Bounds->Half = ent->Size*0.5f;
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
void DetectCollisions(const std::vector<entity *> &activeEntities, const std::vector<entity *> &passiveEntities, std::vector<collision_result> &collisions, size_t *numCollisions)
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

	*numCollisions = 0;
	size_t activeEntitiesSize = activeEntities.size();
	size_t passiveEntitiesSize = passiveEntities.size();
    for (int i = 0; i < activeEntitiesSize; i++)
    {
		auto *EntityA = activeEntities[i];
        if (!EntityA) continue;

        EntityA->NumCollisions = 0;

        for (int j = 0; j < passiveEntitiesSize; j++)
        {
			auto *EntityB = passiveEntities[j];
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
            collision_result col;
			col.Normal = vec3(0.0f);
            if (dx < dy && dy > ClimbHeight)
            {
				col.Normal.x = std::copysign(1, Dif.x);
				col.Depth = dx;
            }
            else
            {
				col.Normal.y = std::copysign(1, Dif.y);
				col.Depth = dy;
            }

            size_t size = collisions.size();
            if (*numCollisions + 1 > size)
            {
                if (size == 0)
                {
                    size = 4;
                }

                collisions.resize(size * 2);
            }

			col.First = EntityA;
			col.Second = EntityB;
            collisions[*numCollisions] = col;
			*numCollisions += 1;
        }
    }
}

void Integrate(const std::vector<entity *> &entities, const vec3 &Gravity, float DeltaTime)
{
    for (auto *Entity : entities)
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
