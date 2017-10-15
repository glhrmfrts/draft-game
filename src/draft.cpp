// Copyright

#define DRAFT_DEBUG 1

#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include "imgui.h"
#include "imgui_impl_sdl_gl3.h"
#include "draft.h"
#include "memory.cpp"
#include "thread_pool.cpp"
#include "tween.cpp"
#include "collision.cpp"
#include "render.cpp"
#include "asset.cpp"
#include "gui.cpp"
#include "debug_ui.cpp"
#include "meshes.cpp"
#include "entity.cpp"
#include "init.cpp"
 
struct audio_source
{
    ALuint Source;
    float Gain = 1;
};
static audio_source *
CreateAudioSource(memory_arena &Arena, ALuint Buffer)
{
    auto *Result = PushStruct<audio_source>(Arena);
    alGenSources(1, &Result->Source);
    alSourcef(Result->Source, AL_PITCH, 1);
    alSourcef(Result->Source, AL_GAIN, 1);
    alSource3f(Result->Source, AL_POSITION, 0,0,0);
    alSource3f(Result->Source, AL_VELOCITY, 0,0,0);
    alSourcei(Result->Source, AL_LOOPING, AL_FALSE);
    alSourcei(Result->Source, AL_BUFFER, Buffer);
    return Result;
}

static void
UpdateAudioParams(audio_source *Audio)
{
    alSourcef(Audio->Source, AL_GAIN, Audio->Gain);
}

// Finds the first free slot on the list and insert the entity
inline static void
AddEntityToList(vector<entity *> &List, entity *Entity)
{
    for (auto it = List.begin(), end = List.end(); it != end; it++)
    {        if (!(*it))
        {
            *it = Entity;
            return;
        }
    }

    // no free slot, insert at the end
    List.push_back(Entity);
}

inline static void
ApplyExplosionLight(render_state &RenderState, color Color)
{
    RenderState.ExplosionLightColor = Color;
    RenderState.ExplosionLightTimer = Global_Game_ExplosionLightTime;
}

void AddEntity(game_state &Game, entity *Entity)
{
    if (Entity->Model)
    {
        AddEntityToList(Game.ModelEntities, Entity);
    }
    if (Entity->Collider)
    {
        AddEntityToList(Game.CollisionEntities, Entity);
    }
    if (Entity->Trail)
    {
        AddEntityToList(Game.TrailEntities, Entity);
        for (int i = 0; i < TrailCount; i++)
        {
            AddEntity(Game, Entity->Trail->Entities + i);
        }
    }
    if (Entity->Explosion)
    {
        ApplyExplosionLight(Game.RenderState, Entity->Explosion->Color);
        AddEntityToList(Game.ExplosionEntities, Entity);
    }
    if (Entity->Ship && !(Entity->Flags & EntityFlag_IsPlayer))
    {
        AddEntityToList(Game.ShipEntities, Entity);
    }
    if (Entity->Repeat)
    {
        AddEntityToList(Game.RepeatingEntities, Entity);
    }
    if (Entity->Flags & EntityFlag_RemoveOffscreen)
    {
        AddEntityToList(Game.RemoveOffscreenEntities, Entity);
    }
    Game.NumEntities++;
}

inline static void
RemoveEntityFromList(vector<entity *> &List, entity *Entity)
{
    for (auto it = List.begin(), end = List.end(); it != end; it++)
    {
        if (*it == Entity)
        {
            *it = NULL;
            return;
        }
    }
}

static void
RemoveEntity(game_state &Game, entity *Entity)
{
    if (Entity->Model)
    {
        RemoveEntityFromList(Game.ModelEntities, Entity);
    }
    if (Entity->Collider)
    {
        RemoveEntityFromList(Game.CollisionEntities, Entity);
    }
    if (Entity->Trail)
    {
        RemoveEntityFromList(Game.TrailEntities, Entity);
        for (int i = 0; i < TrailCount; i++)
        {
            RemoveEntity(Game, Entity->Trail->Entities + i);
        }
    }
    if (Entity->Explosion)
    {
        RemoveEntityFromList(Game.ExplosionEntities, Entity);
    }
    if (Entity->Ship)
    {
        RemoveEntityFromList(Game.ShipEntities, Entity);
    }
    if (Entity->Repeat)
    {
        RemoveEntityFromList(Game.RepeatingEntities, Entity);
    }
    if (Entity->Flags & EntityFlag_RemoveOffscreen)
    {
        RemoveEntityFromList(Game.RemoveOffscreenEntities, Entity);
    }
    Game.NumEntities = std::max(0, Game.NumEntities - 1);
}

#define ROAD_LANE_WIDTH   2
#define LEVEL_PLANE_COUNT 5
#define SHIP_Z            0.2f
static void
InitLevel(game_state &Game)
{
    Game.Mode = GameMode_Level;
    FreeArena(Game.Arena);

    Game.LevelEntropy = RandomSeed(Game.Platform.GetMilliseconds());

    Game.RenderState.FogColor = IntColor(FirstPalette.Colors[3]) * 0.5f;
    Game.RenderState.FogColor.a = 1.0f;
    ApplyExplosionLight(Game.RenderState, IntColor(FirstPalette.Colors[3]));

    Game.Gravity = vec3(0, 0, 0);

    Game.PlayerEntity = CreateShipEntity(Game, Color_blue, IntColor(FirstPalette.Colors[1]), true);
    Game.PlayerEntity->Transform.Position.z = SHIP_Z;
    Game.PlayerEntity->Transform.Velocity.y = PlayerMinVel;
    AddEntity(Game, Game.PlayerEntity);

    for (int i = 0; i < LEVEL_PLANE_COUNT; i++)
    {
        auto *Entity = PushStruct<entity>(Game.Arena);
        Entity->Transform.Position.y = LEVEL_PLANE_SIZE * i;
        Entity->Transform.Position.z = -0.25f;
        Entity->Transform.Scale = vec3{LEVEL_PLANE_SIZE, LEVEL_PLANE_SIZE, 0};
        Entity->Model = CreateModel(Game.Arena, GetFloorMesh(Game));
        Entity->Repeat = PushStruct<entity_repeat>(Game.Arena);
        Entity->Repeat->Count = LEVEL_PLANE_COUNT;
        Entity->Repeat->Size = LEVEL_PLANE_SIZE;
        Entity->Repeat->DistanceFromCamera = LEVEL_PLANE_SIZE/2;
        AddEntity(Game, Entity);
    }
    for (int i = 0; i < LEVEL_PLANE_COUNT; i++)
    {
        auto *Entity = PushStruct<entity>(Game.Arena);
        Entity->Transform.Position.y = LEVEL_PLANE_SIZE * i;
        Entity->Transform.Position.z = 0.0f;
        Entity->Transform.Scale = vec3{ 2, LEVEL_PLANE_SIZE, 1 };
        Entity->Model = CreateModel(Game.Arena, GetRoadMesh(Game));
        Entity->Repeat = PushStruct<entity_repeat>(Game.Arena);
        Entity->Repeat->Count = LEVEL_PLANE_COUNT;
        Entity->Repeat->Size = LEVEL_PLANE_SIZE;
        Entity->Repeat->DistanceFromCamera = LEVEL_PLANE_SIZE / 2;
        AddEntity(Game, Entity);
    }

    Game.TestFont = FindBitmapFont(Game.AssetLoader, "vcr_16");
    Game.DraftBoostAudio = CreateAudioSource(Game.Arena, FindSound(Game.AssetLoader, "boost")->Buffer);
}

static void
DebugReset(game_state &Game)
{
}

struct find_entity_result
{
    entity *Found = NULL;
    entity *Other = NULL;
};
static find_entity_result
FindEntityOfType(entity *First, entity *Second, collider_type Type)
{
    find_entity_result Result;
    if (First->Collider->Type == Type)
    {
        Result.Found = First;
        Result.Other = Second;
    }
    else if (Second->Collider->Type == Type)
    {
        Result.Found = Second;
        Result.Other = First;
    }
    return Result;
}

#define CrystalBoost      10.0f
#define DraftBoost        40.0f
#define CrystalBoostScore 10
#define DraftBoostScore   100
inline static void
ApplyBoostToShip(entity *Entity, float Boost, float Max)
{
    Entity->Transform.Velocity.y += Boost;
}

static bool
HandleCollision(game_state &Game, entity *First, entity *Second, float DeltaTime)
{
    auto ShipEntity = FindEntityOfType(First, Second, ColliderType_Ship).Found;
    auto CrystalEntity = FindEntityOfType(First, Second, ColliderType_Crystal).Found;
    if (First->Collider->Type == ColliderType_TrailPiece || Second->Collider->Type == ColliderType_TrailPiece)
    {
        auto TrailPieceEntity = FindEntityOfType(First, Second, ColliderType_TrailPiece).Found;
        if (ShipEntity && ShipEntity != TrailPieceEntity->TrailPiece->Owner)
        {
            auto Ship = ShipEntity->Ship;
            if (Ship && Ship->NumTrailCollisions == 0 && !Ship->DraftActive)
            {
                Ship->CurrentDraftTime += DeltaTime;
                Ship->NumTrailCollisions++;
                Ship->DraftTarget = TrailPieceEntity->TrailPiece->Owner;
            }
        }
        return false;
    }
    if (First->Collider->Type == ColliderType_Ship && Second->Collider->Type == ColliderType_Ship)
    {
        vec3 rv = First->Transform.Velocity - Second->Transform.Velocity;
        entity *EntityToExplode = NULL;
        entity *OtherEntity = NULL;
        if (rv.y < 0.0f)
        {
            EntityToExplode = First;
            OtherEntity = Second;
        }
        else
        {
            EntityToExplode = Second;
            OtherEntity = First;
        }
        if (OtherEntity->Ship->DraftActive && OtherEntity->Ship->DraftTarget == EntityToExplode)
        {
            OtherEntity->Ship->DraftActive = false;
            if (EntityToExplode->Ship->EnemyType == EnemyType_Explosive)
            {
                EntityToExplode = OtherEntity;
            }

            auto Explosion = CreateExplosionEntity(
                Game,
                *EntityToExplode->Model->Mesh,
                EntityToExplode->Model->Mesh->Parts[0],
                EntityToExplode->Transform.Position,
                OtherEntity->Transform.Velocity,
                EntityToExplode->Transform.Scale,
                EntityToExplode->Ship->Color,
                EntityToExplode->Ship->OutlineColor,
                vec3{ 0, 1, 1 }
            );
            AddEntity(Game, Explosion);
            RemoveEntity(Game, EntityToExplode);
            return false;
        }
    }
    if (ShipEntity != NULL && CrystalEntity != NULL)
    {
        if (ShipEntity->Flags & EntityFlag_IsPlayer)
        {
            auto Explosion = CreateExplosionEntity(Game,
                *CrystalEntity->Model->Mesh,
                CrystalEntity->Model->Mesh->Parts[0],
                CrystalEntity->Pos(),
                ShipEntity->Vel(),
                CrystalEntity->Scl(),
                CRYSTAL_COLOR,
                CRYSTAL_COLOR,
                vec3{ 0, 1, 1 }
            );
            AddEntity(Game, Explosion);
            RemoveEntity(Game, CrystalEntity);
        }
        return false;
    }
    return true;
}

static void UpdateListener(camera &Camera, vec3 PlayerPosition)
{
    static float *Orient = new float[6];
    Orient[0] = Camera.View[2][0];
    Orient[1] = Camera.View[2][1];
    Orient[2] = Camera.View[2][2];
    Orient[3] = Camera.View[0][0];
    Orient[4] = Camera.View[0][1];
    Orient[5] = Camera.View[0][2];
    alListenerfv(AL_ORIENTATION, Orient);
    alListenerfv(AL_POSITION, &Camera.Position[0]);
    alListener3f(AL_VELOCITY, 0, 0, 0);
}

static void UpdateFreeCam(camera &Camera, game_input &Input, float DeltaTime)
{
    static float Pitch;
    static float Yaw;
    float Speed = 50.0f;
    float AxisValue = GetAxisValue(Input, Action_camVertical);
    vec3 CamDir = glm::normalize(Camera.LookAt - Camera.Position);

    Camera.Position += CamDir * AxisValue * Speed * DeltaTime;

    if (Input.MouseState.Buttons & MouseButton_Middle)
    {
        Yaw += Input.MouseState.dX * DeltaTime;
        Pitch -= Input.MouseState.dY * DeltaTime;
        Pitch = glm::clamp(Pitch, -1.5f, 1.5f);
    }

    CamDir.x = sin(Yaw);
    CamDir.y = cos(Yaw);
    CamDir.z = Pitch;
    Camera.LookAt = Camera.Position + CamDir * 50.0f;

    float StrafeYaw = Yaw + (M_PI / 2);
    float hAxisValue = GetAxisValue(Input, Action_camHorizontal);
    Camera.Position += vec3(sin(StrafeYaw), cos(StrafeYaw), 0) * hAxisValue * Speed * DeltaTime;
}

static int GetNextSpawnLane(game_state &Game, bool IsShip = false)
{
    static int LastLane = 0;
    static int LastShipLane = 0;
    int Lane = LastLane;
    while (Lane == LastLane || (IsShip && Lane == LastShipLane))
    {
        Lane = RandomBetween(Game.LevelEntropy, -2, 2);
    }
    LastLane = Lane;
    if (IsShip)
    {
        LastShipLane = Lane;
    }
    return Lane;
}

static int GetNextShipColor(game_state &Game)
{
    static int LastColor = 0;
    int Color = LastColor;
    while (Color == LastColor)
    {
        Color = RandomBetween(Game.LevelEntropy, 0, 1);
    }
    LastColor = Color;
    return Color;
}

#define INITIAL_SHIP_INTERVAL 4.0f
#define CHANGE_SHIP_TIMER     8.0f
static void GenerateEnemyShips(game_state &Game, float DeltaTime)
{
    static float NextShipInterval = INITIAL_SHIP_INTERVAL;
    static float NextShipTimer = INITIAL_SHIP_INTERVAL;
    static float ChangeShipTimer = CHANGE_SHIP_TIMER;

    // @TODO: remove debug key
    if (NextShipTimer <= 0 || Game.Input.Keys[SDL_SCANCODE_S])
    {
        if (ChangeShipTimer <= 0)
        {
            ChangeShipTimer = CHANGE_SHIP_TIMER;
            NextShipInterval -= 0.2f;
            NextShipInterval = std::max(1.0f, NextShipInterval);
            NextShipInterval -= 0.0015f * Game.PlayerEntity->Vel().y;
            NextShipInterval = std::max(0.05f, NextShipInterval);
        }
        NextShipTimer = NextShipInterval;

        int ColorIndex = GetNextShipColor(Game);
        color Color = IntColor(ShipPalette.Colors[ColorIndex]);
        auto Entity = CreateShipEntity(Game, Color, Color, false);
        int Lane = GetNextSpawnLane(Game, true);
        Entity->Pos().x = Lane * ROAD_LANE_WIDTH;
        Entity->Pos().y = Game.PlayerEntity->Pos().y + 200;
        Entity->Pos().z = SHIP_Z;
        Entity->Ship->ColorIndex = ColorIndex;
        AddEntity(Game, Entity);
    }

    NextShipTimer -= DeltaTime;
    ChangeShipTimer -= DeltaTime;
}

static void GenerateCrystals(game_state &g, float dt)
{
    const float baseCrystalInterval = 2.0f;
    static float nextCrystalTimer = baseCrystalInterval;
    if (nextCrystalTimer <= 0)
    {
        nextCrystalTimer = baseCrystalInterval + RandomBetween(g.LevelEntropy, -1.5f, 1.5f);

        auto ent = CreateCrystalEntity(g);
        ent->Pos().x = GetNextSpawnLane(g) * ROAD_LANE_WIDTH;
        ent->Pos().y = g.PlayerEntity->Pos().y + 200;
        ent->Pos().z = SHIP_Z + 0.4f;
        ent->Scl().x = 0.3f;
        ent->Scl().y = 0.3f;
        ent->Scl().z = 0.5f;
        ent->Vel().y = g.PlayerEntity->Vel().y * 0.5f;
        AddEntity(g, ent);
    }

    nextCrystalTimer -= dt;
}

static void UpdateShipDraftCharge(ship *s, float dt)
{
    if (s->NumTrailCollisions == 0)
    {
        s->CurrentDraftTime -= dt;
    }
    s->CurrentDraftTime = std::max(0.0f, std::min(s->CurrentDraftTime, Global_Game_DraftChargeTime));
    s->DraftCharge = s->CurrentDraftTime / Global_Game_DraftChargeTime;
    s->NumTrailCollisions = 0;
}

static void ShipEntityPerformDraft(entity *ent)
{
    ent->Ship->CurrentDraftTime = 0.0f;
    ent->Ship->DraftActive = true;
    ApplyBoostToShip(ent, DraftBoost, 0);
}

static void KeepEntityInsideOfRoad(entity *ent)
{
    const float limit = ROAD_LANE_COUNT * ROAD_LANE_WIDTH / 2;
    ent->Pos().x = glm::clamp(ent->Pos().x, -limit + 0.5f, limit - 0.5f);
}

#define PLAYER_MAX_VEL_INCREASE_FACTOR 0.5f
static void RenderLevel(game_state &Game, float DeltaTime)
{
    auto &UpdateTime = Game.UpdateTime;
    auto &RenderTime = Game.RenderTime;
    UpdateTime.Begin = Game.Platform.GetMilliseconds();

    auto &Input = Game.Input;
    auto &Camera = Game.Camera;
    auto *PlayerEntity = Game.PlayerEntity;
    auto *PlayerShip = PlayerEntity->Ship;
    DeltaTime *= Global_Game_TimeSpeed;

    Game.PlayerMaxVel += PLAYER_MAX_VEL_INCREASE_FACTOR * DeltaTime;

    static bool Paused = false;
    if (IsJustPressed(Game, Action_debugPause))
    {
        Paused = !Paused;
    }
    if (Paused) return;

    if (Game.Input.Keys[SDL_SCANCODE_R])
    {
        DebugReset(Game);
    }
    if (Global_Camera_FreeCam)
    {
        UpdateFreeCam(Camera, Input, DeltaTime);
    }

    if (Game.Input.Keys[SDL_SCANCODE_E])
    {
        auto Explosion = CreateExplosionEntity(Game,
                                                *Game.PlayerEntity->Model->Mesh,
                                                Game.PlayerEntity->Model->Mesh->Parts[0],
                                                Game.PlayerEntity->Transform.Position,
                                                Game.PlayerEntity->Transform.Velocity,
                                                Game.PlayerEntity->Transform.Scale,
                                                Game.PlayerEntity->Ship->Color,
                                                Game.PlayerEntity->Ship->OutlineColor,
                                                vec3{ 0, 1, 1 });
        AddEntity(Game, Explosion);
    }

    GenerateEnemyShips(Game, DeltaTime);
    GenerateCrystals(Game, DeltaTime);

    bool DraftThisFrame = PlayerShip->DraftCharge == 1.0f && IsJustPressed(Game, Action_boost);
    {
        float MoveX = GetAxisValue(Input, Action_horizontal);
        MoveShipEntity(PlayerEntity, MoveX, 0, Game.PlayerMaxVel, DeltaTime);
        KeepEntityInsideOfRoad(PlayerEntity);

        float &PlayerX = PlayerEntity->Pos().x;
        float NearestLane = std::floor(std::ceil(PlayerX)/ROAD_LANE_WIDTH);
        float TargetX = NearestLane * ROAD_LANE_WIDTH;
        if (MoveX == 0.0f)
        {
            float Dif = TargetX - PlayerX;
            if (std::abs(Dif) > 0.05f)
            {
                PlayerEntity->Transform.Velocity.x = Dif*4.0f;
            }
        }

        if (DraftThisFrame)
        {
            ShipEntityPerformDraft(PlayerEntity);
        }
    }

    size_t FrameCollisionCount = 0;
    DetectCollisions(Game.CollisionEntities, Game.CollisionCache, FrameCollisionCount);
    for (size_t i = 0; i < FrameCollisionCount; i++)
    {
        auto &Col = Game.CollisionCache[i];
        Col.First->NumCollisions++;
        Col.Second->NumCollisions++;

        if (HandleCollision(Game, Col.First, Col.Second, DeltaTime))
        {
            ResolveCollision(Col);
        }
    }
    Integrate(Game.CollisionEntities, Game.Gravity, DeltaTime);
    UpdateShipDraftCharge(PlayerShip, DeltaTime);

    auto PlayerPosition = Game.PlayerEntity->Transform.Position;
    if (!Global_Camera_FreeCam)
    {
        const float CAMERA_LERP = 10.0f;
        vec3 d = PlayerPosition - Camera.LookAt;
        Camera.Position += d * CAMERA_LERP * DeltaTime;
        Camera.LookAt = Camera.Position + vec3{0, -Global_Camera_OffsetY, -Global_Camera_OffsetZ};
    }

    for (auto Entity : Game.RemoveOffscreenEntities)
    {
        if (!Entity) continue;

        if (Entity->Pos().y < Camera.Position.y)
        {
            RemoveEntity(Game, Entity);
        }
    }

    for (auto Entity : Game.ShipEntities)
    {
        if (!Entity) continue;

        UpdateShipDraftCharge(Entity->Ship, DeltaTime);
        if (Entity->Pos().y < PlayerPosition.y)
        {
            float MoveX = PlayerPosition.x - Entity->Pos().x;
            MoveX = std::min(MoveX, 1.0f);

            float MoveY = 0.1f + (0.0055f * PlayerEntity->Vel().y);
            if (Entity->Vel().y > PlayerEntity->Vel().y)
            {
                MoveY = 0.0f;
            }
            MoveShipEntity(Entity, MoveX, MoveY, Game.PlayerMaxVel, DeltaTime);

            if (Entity->Ship->DraftCharge == 1.0f)
            {
                ShipEntityPerformDraft(Entity);
            }
        }
        else if (PlayerShip->DraftTarget != Entity)
        {
            Entity->Vel().y = PlayerEntity->Vel().y * 0.8f;
        }

        KeepEntityInsideOfRoad(Entity);
    }

    for (auto Entity : Game.RepeatingEntities)
    {
        auto Repeat = Entity->Repeat;
        if (Camera.Position.y - Entity->Transform.Position.y > Repeat->DistanceFromCamera)
        {
            Entity->Transform.Position.y += Repeat->Size * Repeat->Count;
        }
    }

    alSourcefv(Game.DraftBoostAudio->Source, AL_POSITION, &PlayerPosition[0]);
    UpdateListener(Game.Camera, PlayerPosition);
    UpdateProjectionView(Game.Camera);

    UpdateTime.End = Game.Platform.GetMilliseconds();
    RenderTime.Begin = Game.Platform.GetMilliseconds();

    RenderBegin(Game.RenderState, DeltaTime);

    for (auto Entity : Game.TrailEntities)
    {
        if (!Entity) continue;

        auto Trail = Entity->Trail;
        if (Trail->FirstFrame)
        {
            Trail->FirstFrame = false;
            for (int i = 0; i < TrailCount; i++)
            {
                Trail->Entities[i].Transform.Position = Entity->Transform.Position;
            }
        }

        Trail->Timer -= DeltaTime;
        if (Trail->Timer <= 0)
        {
            Trail->Timer = Global_Game_TrailRecordTimer;
            PushPosition(Trail, Entity->Transform.Position);
        }

        mesh &Mesh = Trail->Mesh;
        ResetBuffer(Mesh.Buffer);

        // First store the quads, then the lines
        vec3 PointCache[TrailCount*4];
        for (int i = 0; i < TrailCount; i++)
        {
            auto *PieceEntity = Trail->Entities + i;
            vec3 c1 = PieceEntity->Transform.Position;
            vec3 c2;
            if (i == TrailCount - 1)
            {
                c2 = Entity->Transform.Position;
            }
            else
            {
                c2 = Trail->Entities[i + 1].Transform.Position;
            }

            float CurrentTrailTime = Trail->Timer/Global_Game_TrailRecordTimer;
            if (i == 0)
            {
                c1 -= (c2 - c1) * CurrentTrailTime;
            }

            const float r = 0.5f;
            float min2 =  0.4f * ((TrailCount - i) / (float)TrailCount);
            float min1 =  0.4f * ((TrailCount - i+1) / (float)TrailCount);
            vec3 p1 = c2 - vec3(r - min2, 0, 0);
            vec3 p2 = c2 + vec3(r - min2, 0, 0);
            vec3 p3 = c1 - vec3(r - min1, 0, 0);
            vec3 p4 = c1 + vec3(r - min1, 0, 0);
            color cl2 = color{ 1, 1, 1, 1.0f - min2 * 2 };
            color cl1 = color{ 1, 1, 1, 1.0f - min1 * 2 };
            AddQuad(Mesh.Buffer, p2, p1, p3, p4, cl2, cl2, cl1, cl1);

            const float lo = 0.05f;
            PointCache[i*4] = p1 - vec3(lo, 0, 0);
            PointCache[i*4 + 1] = p3 - vec3(lo, 0, 0);
            PointCache[i*4 + 2] = p2 + vec3(lo, 0, 0);
            PointCache[i*4 + 3] = p4 + vec3(lo, 0, 0);

            auto &Box = PieceEntity->Collider->Box;
            Box.Half = vec3(r, (c2.y-c1.y) * 0.5f, 0.5f);
            Box.Center = vec3(c1.x, c1.y + Box.Half.y, c1.z + Box.Half.z);
        }
        for (int i = 0; i < TrailCount; i++)
        {
            vec3 *p = PointCache + i*4;
            AddLine(Mesh.Buffer, *p++, *p++);
        }
        for (int i = 0; i < TrailCount; i++)
        {
            vec3 *p = PointCache + i*4 + 2;
            AddLine(Mesh.Buffer, *p++, *p++);
        }
        UploadVertices(Mesh.Buffer, 0, Mesh.Buffer.VertexCount);

        DrawModel(Game.RenderState, Trail->Model, transform{});
    }

    for (auto Entity : Game.ExplosionEntities)
    {
        if (!Entity) continue;

        auto Explosion = Entity->Explosion;
        Explosion->LifeTime -= DeltaTime;
        if (Explosion->LifeTime <= 0)
        {
            RemoveEntity(Game, Entity);
            continue;
        }

        float Alpha = Explosion->LifeTime / Global_Game_ExplosionLifeTime;
        ResetBuffer(Explosion->Mesh.Buffer);
        for (size_t i = 0; i < Explosion->Pieces.size(); i++)
        {
            auto &Piece = Explosion->Pieces[i];
            Piece.Position += Piece.Velocity * DeltaTime;
            mat4 Transform = GetTransformMatrix(Piece);

            size_t TriangleIndex = i * 3;
            vec3 p1 = vec3(Transform * vec4{ Explosion->Triangles[TriangleIndex], 1.0f });
            vec3 p2 = vec3(Transform * vec4{ Explosion->Triangles[TriangleIndex + 1], 1.0f });
            vec3 p3 = vec3(Transform * vec4{ Explosion->Triangles[TriangleIndex + 2], 1.0f });
            AddTriangle(Explosion->Mesh.Buffer, p1, p2, p3, vec3{ 1, 1, 1 }, color{1, 1, 1, Alpha});
        }
        UploadVertices(Explosion->Mesh.Buffer, GL_DYNAMIC_DRAW);

        DrawMeshPart(Game.RenderState, Explosion->Mesh, Explosion->Mesh.Parts[0], transform{});
        DrawMeshPart(Game.RenderState, Explosion->Mesh, Explosion->Mesh.Parts[1], transform{});
    }

    for (auto Entity : Game.ModelEntities)
    {
        if (!Entity) continue;

        DrawModel(Game.RenderState, *Entity->Model, Entity->Transform);
    }

#ifdef DRAFT_DEBUG
    if (Global_Collision_DrawCollider)
    {
        for (auto *Entity : Game.CollisionEntities)
        {
            if (!Entity) continue;

            DrawDebugCollider(Game.RenderState, Entity->Collider->Box, Entity->NumCollisions > 0);
        }
    }
#endif

    RenderEnd(Game.RenderState, Game.Camera);
    RenderTime.End = Game.Platform.GetMilliseconds();

    {
        UpdateProjectionView(Game.GUICamera);
        Begin(Game.GUI, Game.GUICamera);
        DrawRect(Game.GUI, rect{20,20,200,20},
                 IntColor(FirstPalette.Colors[2]), GL_LINE_LOOP, false);

        DrawRect(Game.GUI, rect{25,25,190 * PlayerShip->DraftCharge,10},
                 IntColor(FirstPalette.Colors[3]), GL_TRIANGLES, false);

        //DrawText(Game.GUI, Game.TestFont, "Score gui", rect{50, 20, 0, 0}, Color_white);

        End(Game.GUI);
    }
}

#ifdef _WIN32
#define export_func __declspec(dllexport)
#else
#define export_func
#endif

extern "C"
{
    export_func GAME_INIT(GameInit)
    {
        int Width = Game->Width;
        int Height = Game->Height;

        ImGui_ImplSdlGL3_Init(Game->Window);

        RegisterInputActions(Game->Input);
        InitGUI(Game->GUI, Game->Input);
        MakeCameraOrthographic(Game->GUICamera, 0, Width, 0, Height, -1, 1);
        MakeCameraPerspective(Game->Camera, (float)Game->Width, (float)Game->Height, 70.0f, 0.1f, 1000.0f);
        InitRenderState(Game->RenderState, Width, Height);
        InitTweenState(Game->TweenState);

        Game->Assets.push_back(
            CreateAssetEntry(
                AssetType_Texture,
                "data/textures/grid1.png",
                "grid",
                (void *)(TextureFlag_Mipmap | TextureFlag_Anisotropic | TextureFlag_WrapRepeat)
            )
        );
        Game->Assets.push_back(
            CreateAssetEntry(
                AssetType_Font,
                "data/fonts/vcr.ttf",
                "vcr_16",
                (void *)16
            )
        );
        Game->Assets.push_back(
            CreateAssetEntry(
                AssetType_Sound,
                "data/audio/boost.wav",
                "boost",
                NULL
            )
        );

        InitLoadingScreen(*Game);
    }

    // @TODO: this exists only for imgui, remove in the future
    export_func GAME_PROCESS_EVENT(GameProcessEvent)
    {
        ImGui_ImplSdlGL3_ProcessEvent(Event);
    }

    export_func GAME_RENDER(GameRender)
    {
        ImGui_ImplSdlGL3_NewFrame(Game->Window);
        if (IsJustPressed(*Game, Action_debugUI))
        {
            Global_DebugUI = !Global_DebugUI;
        }

        switch (Game->Mode)
        {
        case GameMode_LoadingScreen:
            RenderLoadingScreen(*Game, DeltaTime, InitLevel);
            break;

        case GameMode_Level:
            RenderLevel(*Game, DeltaTime);
            break;
        }

#ifdef DRAFT_DEBUG
        if (Game->Mode != GameMode_LoadingScreen)
        {
            static float ReloadTimer = 0.0f;
            ReloadTimer += DeltaTime;
            if (ReloadTimer >= 1.0f)
            {
                ReloadTimer = 0.0f;
                CheckAssetsChange(Game->AssetLoader);
            }
        }
#endif

        DrawDebugUI(*Game, DeltaTime);
        ImGui::Render();
    }

    export_func GAME_DESTROY(GameDestroy)
    {
        ImGui_ImplSdlGL3_Shutdown();
        DestroyAssetLoader(Game->AssetLoader);
        FreeArena(Game->Arena);
        FreeArena(Game->AssetLoader.Arena);
    }
}
