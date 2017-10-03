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
#include "collision.cpp"
#include "render.cpp"
#include "asset.cpp"
#include "gui.cpp"
#include "debug_ui.cpp"
#include "entity.cpp"
#include "level.cpp"

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
    {
        if (!(*it))
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
    if (Entity->Bounds)
    {
        AddEntityToList(Game.ShapedEntities, Entity);
    }
    if (Entity->Type == EntityType_TrackSegment)
    {
        AddEntityToList(Game.TrackEntities, Entity);
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
    if (Entity->Ship && !(Entity->Flags & Entity_IsPlayer))
    {
        AddEntityToList(Game.ShipEntities, Entity);
    }
    if (Entity->Type == EntityType_Wall)
    {
        AddEntityToList(Game.WallEntities, Entity);
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
    if (Entity->Bounds)
    {
        RemoveEntityFromList(Game.ShapedEntities, Entity);
    }
    if (Entity->Type == EntityType_TrackSegment)
    {
        RemoveEntityFromList(Game.TrackEntities, Entity);
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
    if (Entity->Type == EntityType_Wall)
    {
        RemoveEntityFromList(Game.WallEntities, Entity);
    }
    Game.NumEntities = std::max(0, Game.NumEntities - 1);
}

#define MapPlaneSize  512
#define CrystalColor  IntColor(FirstPalette.Colors[1])
static void
StartLevel(game_state &Game)
{
    Game.Mode = GameMode_Level;
    FreeArena(Game.Arena);
    Game.RenderState.FogColor = IntColor(FirstPalette.Colors[3]) * 0.5f;
    Game.RenderState.FogColor.a = 1.0f;
    ApplyExplosionLight(Game.RenderState, IntColor(FirstPalette.Colors[3]));

    {
        auto &FloorMesh = Game.FloorMesh;
        InitMeshBuffer(FloorMesh.Buffer);

        material FloorMaterial = {
            Game.RenderState.FogColor,
            1.0f,
            1,
            FindTexture(Game.AssetLoader, "grid"),
            0,
            vec2{MapPlaneSize/16,MapPlaneSize/16}
        };

        float w = 1.0f;
        float l = -w/2;
        float r = w/2;
        AddQuad(FloorMesh.Buffer, vec3(l, l, 0), vec3(r, l, 0), vec3(r, r, 0), vec3(l, r, 0), Color_white, vec3(1, 1, 1));
        AddPart(FloorMesh, {FloorMaterial, 0, FloorMesh.Buffer.VertexCount, GL_TRIANGLES});
        EndMesh(FloorMesh, GL_STATIC_DRAW);
    }

    {
        auto &ShipMesh = Game.ShipMesh;
        auto &ShipCollision = Game.ShipCollision;
        float h = 0.5f;

        InitMeshBuffer(ShipMesh.Buffer);
        AddTriangle(ShipMesh.Buffer, vec3(-1, 0, 0), vec3(0, 0.1f, h), vec3(0, 1, 0.1f));
        AddTriangle(ShipMesh.Buffer, vec3(1, 0, 0),  vec3(0, 1, 0.1f), vec3(0, 0.1f, h));
        AddTriangle(ShipMesh.Buffer, vec3(-1, 0, 0), vec3(0, 0.1f, 0), vec3(0, 0.1f, h));
        AddTriangle(ShipMesh.Buffer, vec3(1, 0, 0), vec3(0, 0.1f, h), vec3(0, 0.1f, 0));

        material ShipMaterial = {Color_white, 0, 0, NULL};
        material ShipOutlineMaterial = {Color_white, 1, 0, NULL, Material_PolygonLines};
        AddPart(ShipMesh, {ShipMaterial, 0, ShipMesh.Buffer.VertexCount, GL_TRIANGLES});
        AddPart(ShipMesh, {ShipOutlineMaterial, 0, ShipMesh.Buffer.VertexCount, GL_TRIANGLES});

        EndMesh(ShipMesh, GL_STATIC_DRAW);

        ShipCollision.push_back(vec2{-1, 0});
        ShipCollision.push_back(vec2{1, 0});
        ShipCollision.push_back(vec2{0, 1});
    }

    {
        auto &CrystalMesh = Game.CrystalMesh;
        InitMeshBuffer(CrystalMesh.Buffer);

        AddTriangle(CrystalMesh.Buffer, vec3{ -1, -1, 0 }, vec3{ 1, -1, 0 }, vec3{ 0, 0, 1 });
        AddTriangle(CrystalMesh.Buffer, vec3{ 1, -1, 0 }, vec3{ 1, 1, 0 }, vec3{ 0, 0, 1 });
        AddTriangle(CrystalMesh.Buffer, vec3{ 1, 1, 0 }, vec3{ -1, 1, 0 }, vec3{ 0, 0, 1 });
        AddTriangle(CrystalMesh.Buffer, vec3{ -1, 1, 0 }, vec3{ -1, -1, 0 }, vec3{ 0, 0, 1 });

        AddTriangle(CrystalMesh.Buffer, vec3{ 0, 0, -1 }, vec3{ 1, -1, 0 }, vec3{ -1, -1, 0 });
        AddTriangle(CrystalMesh.Buffer, vec3{ 0, 0, -1 }, vec3{ 1, 1, 0 }, vec3{ 1, -1, 0 });
        AddTriangle(CrystalMesh.Buffer, vec3{ 0, 0, -1 }, vec3{ -1, 1, 0 }, vec3{ 1, 1, 0 });
        AddTriangle(CrystalMesh.Buffer, vec3{ 0, 0, -1 }, vec3{ -1, -1, 0 }, vec3{ -1, 1, 0 });

        AddPart(CrystalMesh, mesh_part{ material{ CrystalColor, 1.0f, 0, NULL }, 0, CrystalMesh.Buffer.VertexCount, GL_TRIANGLES });
        EndMesh(CrystalMesh, GL_STATIC_DRAW);
    }

    {
        auto &WallMesh = Game.WallMesh;
        InitMeshBuffer(WallMesh.Buffer);
        AddCube(WallMesh.Buffer);
        AddPart(WallMesh, mesh_part{ material{ IntColor(0xffffff, 0.75f), 0.0f, 0.0f, NULL }, 0, WallMesh.Buffer.VertexCount, GL_TRIANGLES });
        EndMesh(WallMesh, GL_STATIC_DRAW);
    }

    MakeCameraPerspective(Game.Camera, (float)Game.Width, (float)Game.Height, 70.0f, 0.1f, 1000.0f);
    Game.Gravity = vec3(0, 0, 0);

    Game.PlayerEntity = CreateShipEntity(Game, Color_blue, IntColor(FirstPalette.Colors[1]), true);
    AddEntity(Game, Game.PlayerEntity);

    {
        auto *Entity = PushStruct<entity>(Game.Arena);
        Entity->Type = EntityType_TrackSegment;
        Entity->Transform.Position.z = -0.25f;
        Entity->Transform.Scale = vec3(MapPlaneSize, MapPlaneSize, 0);
        Entity->Model = CreateModel(Game.Arena, &Game.FloorMesh);
        AddEntity(Game, Entity);
    }

    Game.CurrentLevel = GenerateTestLevel(Game.Arena);
    Game.TestFont = FindBitmapFont(Game.AssetLoader, "vcr_16");
    Game.DraftBoostAudio = CreateAudioSource(Game.Arena, FindSound(Game.AssetLoader, "boost")->Buffer);
}

static void
DebugReset(game_state &Game)
{
}

inline static float
GetAxisValue(game_input &Input, action_type Type)
{
    return Input.Actions[Type].AxisValue;
}

inline static bool
IsJustPressed(game_state &Game, action_type Type)
{
    return Game.Input.Actions[Type].Pressed > 0 &&
        Game.PrevInput.Actions[Type].Pressed == 0;
}

inline static vec3
CameraDir(camera &Camera)
{
    return glm::normalize(Camera.LookAt - Camera.Position);
}

struct find_entity_result
{
    entity *Found = NULL;
    entity *Other = NULL;
};
static find_entity_result
FindEntityOfType(entity *First, entity *Second, entity_type Type)
{
    find_entity_result Result;
    if (First->Type == Type)
    {
        Result.Found = First;
        Result.Other = Second;
    }
    else if (Second->Type == Type)
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
    //Entity->Transform.Velocity.y = std::min(Entity->Transform.Velocity.y, Max);
}

static bool
HandleCollision(game_state &Game, entity *First, entity *Second, float DeltaTime)
{
    auto *ShipEntity = FindEntityOfType(First, Second, EntityType_Ship).Found;
    if (First->Type == EntityType_TrailPiece|| Second->Type == EntityType_TrailPiece)
    {
        auto *TrailPieceEntity = FindEntityOfType(First, Second, EntityType_TrailPiece).Found;
        if (ShipEntity && ShipEntity != TrailPieceEntity->TrailPiece->Owner)
        {
            auto *Ship = ShipEntity->Ship;
            if (Ship && Ship->NumTrailCollisions == 0)
            {
                Ship->CurrentDraftTime += DeltaTime;
                Ship->NumTrailCollisions++;
            }
        }
        return false;
    }
    if (First->Type == EntityType_Crystal || Second->Type == EntityType_Crystal)
    {
        auto Result = FindEntityOfType(First, Second, EntityType_Crystal);
        auto *CrystalEntity = Result.Found;
        auto *OtherEntity = Result.Other;
        auto *Explosion = CreateExplosionEntity(
            Game,
            *CrystalEntity->Model->Mesh,
            CrystalEntity->Model->Mesh->Parts[0],
            CrystalEntity->Transform.Position,
            OtherEntity->Transform.Velocity,
            CrystalEntity->Transform.Scale,
            CrystalColor,
            CrystalColor,
            vec3{ 0, 1, 1 }
        );
        AddEntity(Game, Explosion);
        RemoveEntity(Game, CrystalEntity);

        if (OtherEntity->Type == EntityType_Ship)
        {
            ApplyBoostToShip(OtherEntity, CrystalBoost, ShipMaxVel);
            if (OtherEntity->PlayerState)
            {
                OtherEntity->PlayerState->Score += CrystalBoostScore;
            }
        }
        return false;
    }
    if (First->Type == EntityType_Ship && Second->Type == EntityType_Ship)
    {
        vec3 rv = First->Transform.Velocity - Second->Transform.Velocity;
        if (std::abs(rv.y) > 20.0f)
        {
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

            if (EntityToExplode->Ship->EnemyType == EnemyType_Explosive)
            {
                EntityToExplode = OtherEntity;
            }

            auto *Explosion = CreateExplosionEntity(Game,
                                                    *EntityToExplode->Model->Mesh,
                                                    EntityToExplode->Model->Mesh->Parts[0],
                                                    EntityToExplode->Transform.Position,
                                                    OtherEntity->Transform.Velocity,
                                                    EntityToExplode->Transform.Scale,
                                                    EntityToExplode->Ship->Color,
                                                    EntityToExplode->Ship->OutlineColor,
                                                    vec3{0,1,1});
            AddEntity(Game, Explosion);
            RemoveEntity(Game, EntityToExplode);
            return false;
        }
    }
    if (First->Type == EntityType_Wall || Second->Type == EntityType_Wall)
    {
        auto Result = FindEntityOfType(First, Second, EntityType_Wall);
        auto *EntityToExplode = Result.Other;
        auto *Explosion = CreateExplosionEntity(Game,
                                                *EntityToExplode->Model->Mesh,
                                                EntityToExplode->Model->Mesh->Parts[0],
                                                EntityToExplode->Transform.Position,
                                                vec3{ 0.0f },
                                                EntityToExplode->Transform.Scale,
                                                EntityToExplode->Ship->Color,
                                                EntityToExplode->Ship->OutlineColor,
                                                vec3{ 0,1,1 });
        AddEntity(Game, Explosion);
        RemoveEntity(Game, EntityToExplode);
        return false;
    }
    return true;
}

static void
UpdateFreeCam(camera &Camera, game_input &Input, float DeltaTime)
{
    static float Pitch;
    static float Yaw;
    float Speed = 50.0f;
    float AxisValue = GetAxisValue(Input, Action_camVertical);
    vec3 CamDir = CameraDir(Camera);

    Camera.Position += CamDir * AxisValue * Speed * DeltaTime;

    if (Input.MouseState.Buttons & MouseButton_middle)
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

static void
UpdateListener(camera &Camera, vec3 PlayerPosition)
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

static void
UpdateAndRenderLevel(game_state &Game, float DeltaTime)
{
    auto &Input = Game.Input;
    auto &Camera = Game.Camera;
    auto *PlayerEntity = Game.PlayerEntity;
    auto *PlayerShip = PlayerEntity->Ship;
    DeltaTime *= Global_Game_TimeSpeed;

    static bool Paused = false;
    if (IsJustPressed(Game, Action_debugPause))
    {
        Paused = !Paused;
    }
    if (Paused) return;

    if (Global_Camera_FreeCam)
    {
        UpdateFreeCam(Camera, Input, DeltaTime);
    }
    if (IsJustPressed(Game, Action_debugUI))
    {
        Global_DebugUI = !Global_DebugUI;
    }
    if (Game.Input.Keys[SDL_SCANCODE_R])
    {
        DebugReset(Game);
    }

    if (Game.Input.Keys[SDL_SCANCODE_E])
    {
        auto *Explosion = CreateExplosionEntity(Game,
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

    {
        const float LOOK_SPEED = 5.0f;
        const float ACCELERATION = 10.0f;
        const float DECELERATION = 20.0f;

        static float Yaw;
        static float MoveYaw;
        static float Speed;
        static vec3 MoveDir;
        static vec3 LookDir;
        static vec3 LookAt = vec3(0, 1, 0);
        LookDir = glm::normalize(LookAt - PlayerEntity->Transform.Position);

        float InputLook = GetAxisValue(Game.Input, Action_horizontal);
        float InputMove = GetAxisValue(Game.Input, Action_vertical);
        Yaw += InputLook * LOOK_SPEED * DeltaTime;

        if (InputMove > 0 && Speed < 20.0f)
        {
            Speed += InputMove * ACCELERATION * DeltaTime;
        }
        else if (InputMove < 0 && Speed > 0.0f)
        {
            Speed += InputMove * DECELERATION * DeltaTime;
        }

        LookDir.x = sin(Yaw);
        LookDir.y = cos(Yaw);
        LookAt = PlayerEntity->Transform.Position + LookDir * 20.0f;

        if (InputMove > 0.0f)
        {
            MoveDir += (LookDir - MoveDir) * 2.0f * DeltaTime;
        }
        //MoveDir.x = sin(Yaw);
        //MoveDir.y = cos(Yaw);
        PlayerEntity->Transform.Velocity = MoveDir * Speed;
        PlayerEntity->Transform.Position += PlayerEntity->Transform.Velocity * DeltaTime;

        PlayerEntity->Transform.Rotation.z = glm::degrees(-Yaw);
    }

    size_t FrameCollisionCount = 0;
    PlayerShip->NumTrailCollisions = 0;
    DetectCollisions(Game.ShapedEntities, Game.CollisionCache, FrameCollisionCount);
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
    Integrate(Game.ShapedEntities, Game.Gravity, DeltaTime);
    if (PlayerShip->NumTrailCollisions == 0)
    {
        PlayerShip->CurrentDraftTime -= DeltaTime;
    }
    PlayerShip->CurrentDraftTime = std::max(0.0f, std::min(PlayerShip->CurrentDraftTime, Global_Game_DraftChargeTime));
    PlayerShip->DraftCharge = PlayerShip->CurrentDraftTime / Global_Game_DraftChargeTime;

    auto PlayerPosition = Game.PlayerEntity->Transform.Position;
    if (!Global_Camera_FreeCam)
    {
        vec3 d = PlayerPosition - Camera.LookAt;
        Camera.Position += d * 5.0f * DeltaTime;
        Camera.LookAt = Camera.Position + vec3{0, -Global_Camera_OffsetY, -Global_Camera_OffsetZ};
    }

    alSourcefv(Game.DraftBoostAudio->Source, AL_POSITION, &PlayerPosition[0]);
    UpdateListener(Game.Camera, PlayerPosition);
    UpdateProjectionView(Game.Camera);
    RenderBegin(Game.RenderState, DeltaTime);

    static float EnemyMoveH = 0.0f;
    //EnemyMoveH += DeltaTime;
    for (auto *Entity : Game.ShipEntities)
    {
        if (!Entity) continue;

        float MoveY = 0.0f;
        if (Entity->Transform.Position.y < PlayerPosition.y)
        {
            MoveY = 0.1f;
        }
        MoveShipEntity(Entity, sin(EnemyMoveH), MoveY, DeltaTime);
    }

    for (auto *Entity : Game.TrailEntities)
    {
        if (!Entity) continue;

        auto *Trail = Entity->Trail;
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

            auto &Box = PieceEntity->Bounds->Box;
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
        UploadVertices(Mesh.Buffer, GL_DYNAMIC_DRAW);

        PushModel(Game.RenderState, Trail->Model, transform{});
    }

    for (auto *Entity : Game.ExplosionEntities)
    {
        if (!Entity) continue;

        auto *Explosion = Entity->Explosion;
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

        PushMeshPart(Game.RenderState, Explosion->Mesh, Explosion->Mesh.Parts[0], transform{});
        PushMeshPart(Game.RenderState, Explosion->Mesh, Explosion->Mesh.Parts[1], transform{});
    }

    for (auto *Entity : Game.ModelEntities)
    {
        if (!Entity) continue;

        PushModel(Game.RenderState, *Entity->Model, Entity->Transform);
    }

#ifdef DRAFT_DEBUG
    if (Global_Collision_DrawBounds)
    {
        for (auto *Entity : Game.ShapedEntities)
        {
            if (!Entity) continue;

            PushDebugBounds(Game.RenderState, Entity->Bounds->Box, Entity->NumCollisions > 0);
        }
    }
#endif

    RenderEnd(Game.RenderState, Game.Camera);

    {
        UpdateProjectionView(Game.GUICamera);
        Begin(Game.GUI, Game.GUICamera);
        PushRect(Game.GUI, rect{20,20,200,20},
                 IntColor(FirstPalette.Colors[2]), GL_LINE_LOOP, false);

        PushRect(Game.GUI, rect{25,25,190 * PlayerShip->DraftCharge,10},
                 IntColor(FirstPalette.Colors[3]), GL_TRIANGLES, false);

        PushText(Game.GUI, Game.TestFont, "Score gui", rect{50, 20, 0, 0}, Color_white);

        End(Game.GUI);
    }
}

static void
RegisterInputActions(game_input &Input)
{
    Input.Actions[Action_camHorizontal] = action_state{ SDL_SCANCODE_D, SDL_SCANCODE_A, 0, 0, Axis_Invalid, Button_Invalid };
    Input.Actions[Action_camVertical] = action_state{ SDL_SCANCODE_W, SDL_SCANCODE_S, 0, 0, Axis_Invalid, Button_Invalid};
    Input.Actions[Action_horizontal] = action_state{ SDL_SCANCODE_RIGHT, SDL_SCANCODE_LEFT, 0, 0, Axis_LeftX, Button_Invalid };
    Input.Actions[Action_vertical] = action_state{ SDL_SCANCODE_UP, SDL_SCANCODE_DOWN, 0, 0, Axis_RightTrigger, Button_Invalid };
    Input.Actions[Action_boost] = action_state{ SDL_SCANCODE_SPACE, 0, 0, 0, Axis_Invalid, XboxButton_X };
    Input.Actions[Action_debugUI] = action_state{ SDL_SCANCODE_GRAVE, 0, 0, 0, Axis_Invalid, Button_Invalid };
    Input.Actions[Action_debugPause] = action_state{ SDL_SCANCODE_P, 0, 0, 0, Axis_Invalid, Button_Invalid };
}

static void
StartLoadingScreen(game_state &Game)
{
    Game.Mode = GameMode_LoadingScreen;
    InitAssetLoader(Game.AssetLoader, Game.Platform);

    AddAssetEntry(
        Game.AssetLoader,
        AssetType_Texture,
        "data/textures/grid1.png",
        "grid",
        (void *)(Texture_Mipmap | Texture_Anisotropic | Texture_WrapRepeat)
    );
    AddAssetEntry(
        Game.AssetLoader,
        AssetType_Font,
        "data/fonts/vcr.ttf",
        "vcr_16",
        (void *)16
    );
    AddAssetEntry(
        Game.AssetLoader,
        AssetType_Sound,
        "data/audio/boost.wav",
        "boost",
        NULL
    );
    AddShaderProgramEntries(Game.AssetLoader, Game.RenderState.ModelProgram);
    AddShaderProgramEntries(Game.AssetLoader, Game.RenderState.BlurHorizontalProgram);
    AddShaderProgramEntries(Game.AssetLoader, Game.RenderState.BlurVerticalProgram);
    AddShaderProgramEntries(Game.AssetLoader, Game.RenderState.BlendProgram);
    AddShaderProgramEntries(Game.AssetLoader, Game.RenderState.BlitProgram);
    AddShaderProgramEntries(Game.AssetLoader, Game.RenderState.ResolveMultisampleProgram);

    StartLoading(Game.AssetLoader);
}

static void
RenderLoadingScreen(game_state &Game, float DeltaTime)
{
    glClear(GL_COLOR_BUFFER_BIT);

    auto &g = Game.GUI;
    float Width = Game.Width * 0.5f;
    float Height = Game.Height * 0.1f;
    float x = (float)Game.Width/2 - Width/2;
    float y = (float)Game.Height/2 - Height/2;
    float LoadingPercentage = (float)(int)Game.AssetLoader.NumLoadedEntries / (float)Game.AssetLoader.Entries.size();
    float ProgressBarWidth = Width*LoadingPercentage;

    UpdateProjectionView(Game.GUICamera);
    Begin(g, Game.GUICamera);
    PushRect(g, rect{ x, y, Width, Height }, Color_white, GL_LINE_LOOP);
    PushRect(g, rect{ x + 5, y + 5, ProgressBarWidth - 10, Height - 10 }, Color_white);
    End(g);

    if (Update(Game.AssetLoader))
    {
        StartLevel(Game);
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

        Game->ExplosionEntropy = RandomSeed(1234);

        RegisterInputActions(Game->Input);
        InitGUI(Game->GUI, Game->Input);
        MakeCameraOrthographic(Game->GUICamera, 0, Width, 0, Height, -1, 1);
        InitRenderState(Game->RenderState, Width, Height);
        StartLoadingScreen(*Game);
    }

    // @TODO: this exists only for imgui, remove in the future
    export_func GAME_PROCESS_EVENT(GameProcessEvent)
    {
        ImGui_ImplSdlGL3_ProcessEvent(Event);
    }

    export_func GAME_RENDER(GameRender)
    {
        ImGui_ImplSdlGL3_NewFrame(Game->Window);
        switch (Game->Mode)
        {
        case GameMode_LoadingScreen:
            RenderLoadingScreen(*Game, DeltaTime);
            break;
        case GameMode_Level:
            UpdateAndRenderLevel(*Game, DeltaTime);
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
