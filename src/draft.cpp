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
static audio_source *CreateAudioSource(memory_arena &arena, ALuint buffer)
{
    auto result = PushStruct<audio_source>(arena);
    alGenSources(1, &result->Source);
    alSourcef(result->Source, AL_PITCH, 1);
    alSourcef(result->Source, AL_GAIN, 1);
    alSource3f(result->Source, AL_POSITION, 0,0,0);
    alSource3f(result->Source, AL_VELOCITY, 0,0,0);
    alSourcei(result->Source, AL_LOOPING, AL_FALSE);
    alSourcei(result->Source, AL_BUFFER, buffer);
    return result;
}

static void UpdateAudioParams(audio_source *audio)
{
    alSourcef(audio->Source, AL_GAIN, audio->Gain);
}

static void ApplyExplosionLight(render_state &rs, color c)
{
    rs.ExplosionLightColor = c;
    rs.ExplosionLightTimer = Global_Game_ExplosionLightTime;
}

#define ROAD_LANE_WIDTH   2
#define LEVEL_PLANE_COUNT 5
#define SHIP_Z            0.2f
static void InitLevel(game_state &g)
{
    g.Mode = GameMode_Level;
    FreeArena(g.Arena);

    Println(g.Platform.GetMilliseconds());
    g.LevelEntropy = RandomSeed(g.Platform.GetMilliseconds());

    g.RenderState.FogColor = IntColor(FirstPalette.Colors[3]) * 0.5f;
    g.RenderState.FogColor.a = 1.0f;
    ApplyExplosionLight(g.RenderState, IntColor(FirstPalette.Colors[3]));

    g.Gravity = vec3(0, 0, 0);

    g.PlayerEntity = CreateShipEntity(g, Color_blue, IntColor(FirstPalette.Colors[1]), true);
    g.PlayerEntity->Transform.Position.z = SHIP_Z;
    g.PlayerEntity->Transform.Velocity.y = PlayerMinVel;
    AddEntity(g, g.PlayerEntity);

    for (int i = 0; i < LEVEL_PLANE_COUNT; i++)
    {
        auto *ent = PushStruct<entity>(g.Arena);
        ent->Transform.Position.y = LEVEL_PLANE_SIZE * i;
        ent->Transform.Position.z = -0.25f;
        ent->Transform.Scale = vec3{LEVEL_PLANE_SIZE, LEVEL_PLANE_SIZE, 0};
        ent->Model = CreateModel(g.Arena, GetFloorMesh(g));
        ent->Repeat = PushStruct<entity_repeat>(g.Arena);
        ent->Repeat->Count = LEVEL_PLANE_COUNT;
        ent->Repeat->Size = LEVEL_PLANE_SIZE;
        ent->Repeat->DistanceFromCamera = LEVEL_PLANE_SIZE/2;
        AddEntity(g, ent);
    }
    for (int i = 0; i < LEVEL_PLANE_COUNT; i++)
    {
        auto *ent = PushStruct<entity>(g.Arena);
        ent->Transform.Position.y = LEVEL_PLANE_SIZE * i;
        ent->Transform.Position.z = 0.0f;
        ent->Transform.Scale = vec3{ 2, LEVEL_PLANE_SIZE, 1 };
        ent->Model = CreateModel(g.Arena, GetRoadMesh(g));
        ent->Repeat = PushStruct<entity_repeat>(g.Arena);
        ent->Repeat->Count = LEVEL_PLANE_COUNT;
        ent->Repeat->Size = LEVEL_PLANE_SIZE;
        ent->Repeat->DistanceFromCamera = LEVEL_PLANE_SIZE / 2;
        AddEntity(g, ent);
    }

    g.TestFont = FindBitmapFont(g.AssetLoader, "vcr_16");
    g.DraftBoostAudio = CreateAudioSource(g.Arena, FindSound(g.AssetLoader, "boost")->Buffer);
}

static void
DebugReset(game_state &g)
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
ApplyBoostToShip(entity *ent, float Boost, float Max)
{
    ent->Transform.Velocity.y += Boost;
}

static bool
HandleCollision(game_state &g, entity *first, entity *second, float dt)
{
    auto shipEntity = FindEntityOfType(first, second, ColliderType_Ship).Found;
    auto crystalEntity = FindEntityOfType(first, second, ColliderType_Crystal).Found;
    if (first->Collider->Type == ColliderType_TrailPiece || second->Collider->Type == ColliderType_TrailPiece)
    {
        auto trailPieceEntity = FindEntityOfType(first, second, ColliderType_TrailPiece).Found;
        if (shipEntity && shipEntity != trailPieceEntity->TrailPiece->Owner)
        {
            auto s = shipEntity->Ship;
            if (s && s->NumTrailCollisions == 0 && !s->DraftActive)
            {
                s->CurrentDraftTime += dt;
                s->NumTrailCollisions++;
                s->DraftTarget = trailPieceEntity->TrailPiece->Owner;
            }
        }
        return false;
    }
    if (first->Collider->Type == ColliderType_Ship && second->Collider->Type == ColliderType_Ship)
    {
        vec3 rv = first->Transform.Velocity - second->Transform.Velocity;
        entity *entityToExplode = NULL;
        entity *otherEntity = NULL;
        if (rv.y < 0.0f)
        {
            entityToExplode = first;
            otherEntity = second;
        }
        else
        {
            entityToExplode = second;
            otherEntity = first;
        }
        if (otherEntity->Ship->DraftActive && otherEntity->Ship->DraftTarget == entityToExplode)
        {
            otherEntity->Ship->DraftActive = false;
            if (entityToExplode->Ship->EnemyType == EnemyType_Explosive)
            {
                entityToExplode = otherEntity;
            }

            auto exp = CreateExplosionEntity(
                g,
                *entityToExplode->Model->Mesh,
                entityToExplode->Model->Mesh->Parts[0],
                entityToExplode->Transform.Position,
                otherEntity->Transform.Velocity,
                entityToExplode->Transform.Scale,
                entityToExplode->Ship->Color,
                entityToExplode->Ship->OutlineColor,
                vec3{ 0, 1, 1 }
            );
            AddEntity(g, exp);
            RemoveEntity(g, entityToExplode);
            return false;
        }
    }
    if (shipEntity != NULL && crystalEntity != NULL)
    {
        if (shipEntity->Flags & EntityFlag_IsPlayer)
        {
            auto exp = CreateExplosionEntity(g,
                *crystalEntity->Model->Mesh,
                crystalEntity->Model->Mesh->Parts[0],
                crystalEntity->Pos(),
                shipEntity->Vel(),
                crystalEntity->Scl(),
                CRYSTAL_COLOR,
                CRYSTAL_COLOR,
                vec3{ 0, 1, 1 }
            );
            AddEntity(g, exp);
            RemoveEntity(g, crystalEntity);
        }
        return false;
    }
    return true;
}

static void UpdateListener(camera &cam, vec3 PlayerPosition)
{
    static float *orient = new float[6];
    orient[0] = cam.View[2][0];
    orient[1] = cam.View[2][1];
    orient[2] = cam.View[2][2];
    orient[3] = cam.View[0][0];
    orient[4] = cam.View[0][1];
    orient[5] = cam.View[0][2];
    alListenerfv(AL_ORIENTATION, orient);
    alListenerfv(AL_POSITION, &cam.Position[0]);
    alListener3f(AL_VELOCITY, 0, 0, 0);
}

static void UpdateFreeCam(camera &cam, game_input &input, float dt)
{
    static float Pitch;
    static float Yaw;
    const float speed = 50.0f;
    float axisValue = GetAxisValue(input, Action_camVertical);
    vec3 camDir = glm::normalize(cam.LookAt - cam.Position);

    cam.Position += camDir * axisValue * speed * dt;

    if (input.MouseState.Buttons & MouseButton_Middle)
    {
        Yaw += input.MouseState.dX * dt;
        Pitch -= input.MouseState.dY * dt;
        Pitch = glm::clamp(Pitch, -1.5f, 1.5f);
    }

    camDir.x = sin(Yaw);
    camDir.y = cos(Yaw);
    camDir.z = Pitch;
    cam.LookAt = cam.Position + camDir * 50.0f;

    float strafeYaw = Yaw + (M_PI / 2);
    float hAxisValue = GetAxisValue(input, Action_camHorizontal);
    cam.Position += vec3(sin(strafeYaw), cos(strafeYaw), 0) * hAxisValue * speed * dt;
}

static int GetNextSpawnLane(game_state &g, bool IsShip = false)
{
    static int LastLane = 0;
    static int LastShipLane = 0;
    int Lane = LastLane;
    while (Lane == LastLane || (IsShip && Lane == LastShipLane))
    {
        Lane = RandomBetween(g.LevelEntropy, -2, 2);
    }
    LastLane = Lane;
    if (IsShip)
    {
        LastShipLane = Lane;
    }
    return Lane;
}

static int GetNextShipColor(game_state &g)
{
    static int LastColor = 0;
    int Color = LastColor;
    while (Color == LastColor)
    {
        Color = RandomBetween(g.LevelEntropy, 0, 1);
    }
    LastColor = Color;
    return Color;
}

#define INITIAL_SHIP_INTERVAL 4.0f
#define CHANGE_SHIP_TIMER     8.0f
static void GenerateEnemyShips(game_state &g, float dt)
{
    static float NextShipInterval = INITIAL_SHIP_INTERVAL;
    static float NextShipTimer = INITIAL_SHIP_INTERVAL;
    static float ChangeShipTimer = CHANGE_SHIP_TIMER;

    // @TODO: remove debug key
    if (NextShipTimer <= 0 || g.Input.Keys[SDL_SCANCODE_S])
    {
        if (ChangeShipTimer <= 0)
        {
            ChangeShipTimer = CHANGE_SHIP_TIMER;
            NextShipInterval -= 0.2f;
            NextShipInterval = std::max(1.0f, NextShipInterval);
            NextShipInterval -= 0.0015f * g.PlayerEntity->Vel().y;
            NextShipInterval = std::max(0.05f, NextShipInterval);
        }
        NextShipTimer = NextShipInterval;

        int ColorIndex = GetNextShipColor(g);
        color Color = IntColor(ShipPalette.Colors[ColorIndex]);
        auto ent = CreateShipEntity(g, Color, Color, false);
        int Lane = GetNextSpawnLane(g, true);
        ent->Pos().x = Lane * ROAD_LANE_WIDTH;
        ent->Pos().y = g.PlayerEntity->Pos().y + 200;
        ent->Pos().z = SHIP_Z;
        ent->Ship->ColorIndex = ColorIndex;
        AddEntity(g, ent);
    }

    NextShipTimer -= dt;
    ChangeShipTimer -= dt;
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
static void RenderLevel(game_state &g, float dt)
{
    auto &updateTime = g.UpdateTime;
    auto &renderTime = g.RenderTime;
    updateTime.Begin = g.Platform.GetMilliseconds();

    auto &input = g.Input;
    auto &cam = g.Camera;
    auto *playerEntity = g.PlayerEntity;
    auto *playerShip = playerEntity->Ship;
    dt *= Global_Game_TimeSpeed;

    g.PlayerMaxVel += PLAYER_MAX_VEL_INCREASE_FACTOR * dt;

    static bool Paused = false;
    if (IsJustPressed(g, Action_debugPause))
    {
        Paused = !Paused;
    }
    if (Paused) return;

    if (g.Input.Keys[SDL_SCANCODE_R])
    {
        DebugReset(g);
    }
    if (Global_Camera_FreeCam)
    {
        UpdateFreeCam(cam, input, dt);
    }

    if (g.Input.Keys[SDL_SCANCODE_E])
    {
        auto exp = CreateExplosionEntity(g,
                                         *g.PlayerEntity->Model->Mesh,
                                         g.PlayerEntity->Model->Mesh->Parts[0],
                                         g.PlayerEntity->Transform.Position,
                                         g.PlayerEntity->Transform.Velocity,
                                         g.PlayerEntity->Transform.Scale,
                                         g.PlayerEntity->Ship->Color,
                                         g.PlayerEntity->Ship->OutlineColor,
                                         vec3{ 0, 1, 1 });
        AddEntity(g, exp);
    }

    GenerateEnemyShips(g, dt);
    GenerateCrystals(g, dt);

    {
        float moveX = GetAxisValue(input, Action_horizontal);
        MoveShipEntity(playerEntity, moveX, 0, g.PlayerMaxVel, dt);
        KeepEntityInsideOfRoad(playerEntity);

        float &playerX = playerEntity->Pos().x;
        float nearestLane = std::floor(std::ceil(playerX)/ROAD_LANE_WIDTH);
        float targetX = nearestLane * ROAD_LANE_WIDTH;
        if (moveX == 0.0f)
        {
            float dif = targetX - playerX;
            if (std::abs(dif) > 0.05f)
            {
                playerEntity->Transform.Velocity.x = dif*4.0f;
            }
        }

        if (playerShip->DraftCharge == 1.0f && IsJustPressed(g, Action_boost))
        {
            ShipEntityPerformDraft(playerEntity);
        }
    }

    size_t FrameCollisionCount = 0;
    DetectCollisions(g.CollisionEntities, g.CollisionCache, FrameCollisionCount);
    for (size_t i = 0; i < FrameCollisionCount; i++)
    {
        auto &Col = g.CollisionCache[i];
        Col.First->NumCollisions++;
        Col.Second->NumCollisions++;

        if (HandleCollision(g, Col.First, Col.Second, dt))
        {
            ResolveCollision(Col);
        }
    }
    Integrate(g.CollisionEntities, g.Gravity, dt);
    UpdateShipDraftCharge(playerShip, dt);

    auto PlayerPosition = g.PlayerEntity->Transform.Position;
    if (!Global_Camera_FreeCam)
    {
        const float CAMERA_LERP = 10.0f;
        vec3 d = PlayerPosition - cam.LookAt;
        cam.Position += d * CAMERA_LERP * dt;
        cam.LookAt = cam.Position + vec3{0, -Global_Camera_OffsetY, -Global_Camera_OffsetZ};
    }

    for (auto ent : g.RemoveOffscreenEntities)
    {
        if (!ent) continue;

        if (ent->Pos().y < cam.Position.y)
        {
            RemoveEntity(g, ent);
        }
    }

    for (auto ent : g.ShipEntities)
    {
        if (!ent) continue;

        UpdateShipDraftCharge(ent->Ship, dt);
        if (ent->Pos().y < PlayerPosition.y)
        {
            float MoveX = PlayerPosition.x - ent->Pos().x;
            MoveX = std::min(MoveX, 1.0f);

            float MoveY = 0.1f + (0.0055f * playerEntity->Vel().y);
            if (ent->Vel().y > playerEntity->Vel().y)
            {
                MoveY = 0.0f;
            }
            MoveShipEntity(ent, MoveX, MoveY, g.PlayerMaxVel, dt);

            if (ent->Ship->DraftCharge == 1.0f)
            {
                ShipEntityPerformDraft(ent);
            }
        }
        else if (playerShip->DraftTarget != ent)
        {
            ent->Vel().y = playerEntity->Vel().y * 0.8f;
        }

        KeepEntityInsideOfRoad(ent);
    }

    for (auto ent : g.RepeatingEntities)
    {
        auto repeat = ent->Repeat;
        if (cam.Position.y - ent->Transform.Position.y > repeat->DistanceFromCamera)
        {
            ent->Transform.Position.y += repeat->Size * repeat->Count;
        }
    }

    alSourcefv(g.DraftBoostAudio->Source, AL_POSITION, &PlayerPosition[0]);
    UpdateListener(g.Camera, PlayerPosition);
    UpdateProjectionView(g.Camera);

    updateTime.End = g.Platform.GetMilliseconds();
    renderTime.Begin = g.Platform.GetMilliseconds();

    RenderBegin(g.RenderState, dt);

    for (auto ent : g.TrailEntities)
    {
        if (!ent) continue;

        auto tr = ent->Trail;
        if (tr->FirstFrame)
        {
            tr->FirstFrame = false;
            for (int i = 0; i < TrailCount; i++)
            {
                tr->Entities[i].Transform.Position = ent->Transform.Position;
            }
        }

        tr->Timer -= dt;
        if (tr->Timer <= 0)
        {
            tr->Timer = Global_Game_TrailRecordTimer;
            PushPosition(tr, ent->Transform.Position);
        }

        auto &m = tr->Mesh;
        ResetBuffer(m.Buffer);

        // First store the quads, then the lines
        vec3 PointCache[TrailCount*4];
        for (int i = 0; i < TrailCount; i++)
        {
            auto *pieceEntity = tr->Entities + i;
            vec3 c1 = pieceEntity->Transform.Position;
            vec3 c2;
            if (i == TrailCount - 1)
            {
                c2 = ent->Transform.Position;
            }
            else
            {
                c2 = tr->Entities[i + 1].Transform.Position;
            }

            float CurrentTrailTime = tr->Timer/Global_Game_TrailRecordTimer;
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
            AddQuad(m.Buffer, p2, p1, p3, p4, cl2, cl2, cl1, cl1);

            const float lo = 0.05f;
            PointCache[i*4] = p1 - vec3(lo, 0, 0);
            PointCache[i*4 + 1] = p3 - vec3(lo, 0, 0);
            PointCache[i*4 + 2] = p2 + vec3(lo, 0, 0);
            PointCache[i*4 + 3] = p4 + vec3(lo, 0, 0);

            auto &Box = pieceEntity->Collider->Box;
            Box.Half = vec3(r, (c2.y-c1.y) * 0.5f, 0.5f);
            Box.Center = vec3(c1.x, c1.y + Box.Half.y, c1.z + Box.Half.z);
        }
        for (int i = 0; i < TrailCount; i++)
        {
            vec3 *p = PointCache + i*4;
            AddLine(m.Buffer, *p++, *p++);
        }
        for (int i = 0; i < TrailCount; i++)
        {
            vec3 *p = PointCache + i*4 + 2;
            AddLine(m.Buffer, *p++, *p++);
        }
        UploadVertices(m.Buffer, 0, m.Buffer.VertexCount);

        DrawModel(g.RenderState, tr->Model, transform{});
    }

    for (auto ent : g.ExplosionEntities)
    {
        if (!ent) continue;

        auto exp = ent->Explosion;
        exp->LifeTime -= dt;
        if (exp->LifeTime <= 0)
        {
            RemoveEntity(g, ent);
            continue;
        }

        float alpha = exp->LifeTime / Global_Game_ExplosionLifeTime;
        ResetBuffer(exp->Mesh.Buffer);
        for (size_t i = 0; i < exp->Pieces.size(); i++)
        {
            auto &Piece = exp->Pieces[i];
            Piece.Position += Piece.Velocity * dt;
            mat4 Transform = GetTransformMatrix(Piece);

            size_t TriangleIndex = i * 3;
            vec3 p1 = vec3(Transform * vec4{ exp->Triangles[TriangleIndex], 1.0f });
            vec3 p2 = vec3(Transform * vec4{ exp->Triangles[TriangleIndex + 1], 1.0f });
            vec3 p3 = vec3(Transform * vec4{ exp->Triangles[TriangleIndex + 2], 1.0f });
            AddTriangle(exp->Mesh.Buffer, p1, p2, p3, vec3{ 1, 1, 1 }, color{1, 1, 1, alpha});
        }
        UploadVertices(exp->Mesh.Buffer, GL_DYNAMIC_DRAW);

        DrawMeshPart(g.RenderState, exp->Mesh, exp->Mesh.Parts[0], transform{});
        DrawMeshPart(g.RenderState, exp->Mesh, exp->Mesh.Parts[1], transform{});
    }

    for (auto ent : g.ModelEntities)
    {
        if (!ent) continue;

        DrawModel(g.RenderState, *ent->Model, ent->Transform);
    }

#ifdef DRAFT_DEBUG
    if (Global_Collision_DrawCollider)
    {
        for (auto *ent : g.CollisionEntities)
        {
            if (!ent) continue;

            DrawDebugCollider(g.RenderState, ent->Collider->Box, ent->NumCollisions > 0);
        }
    }
#endif

    RenderEnd(g.RenderState, g.Camera);
    renderTime.End = g.Platform.GetMilliseconds();

    {
        UpdateProjectionView(g.GUICamera);
        Begin(g.GUI, g.GUICamera);
        DrawRect(g.GUI, rect{20,20,200,20},
                 IntColor(FirstPalette.Colors[2]), GL_LINE_LOOP, false);

        DrawRect(g.GUI, rect{25,25,190 * playerShip->DraftCharge,10},
                 IntColor(FirstPalette.Colors[3]), GL_TRIANGLES, false);

        //DrawText(g.GUI, g.TestFont, "Score gui", rect{50, 20, 0, 0}, Color_white);

        End(g.GUI);
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
        auto g = Game;
        int Width = g->Width;
        int Height = g->Height;

        ImGui_ImplSdlGL3_Init(g->Window);

        RegisterInputActions(g->Input);
        InitGUI(g->GUI, g->Input);
        MakeCameraOrthographic(g->GUICamera, 0, Width, 0, Height, -1, 1);
        MakeCameraPerspective(g->Camera, (float)g->Width, (float)g->Height, 70.0f, 0.1f, 1000.0f);
        InitRenderState(g->RenderState, Width, Height);
        InitTweenState(g->TweenState);

        g->Assets.push_back(
            CreateAssetEntry(
                AssetType_Texture,
                "data/textures/grid1.png",
                "grid",
                (void *)(TextureFlag_Mipmap | TextureFlag_Anisotropic | TextureFlag_WrapRepeat)
            )
        );
        g->Assets.push_back(
            CreateAssetEntry(
                AssetType_Font,
                "data/fonts/vcr.ttf",
                "vcr_16",
                (void *)16
            )
        );
        g->Assets.push_back(
            CreateAssetEntry(
                AssetType_Sound,
                "data/audio/boost.wav",
                "boost",
                NULL
            )
        );

        InitLoadingScreen(*g);
    }

    // @TODO: this exists only for imgui, remove in the future
    export_func GAME_PROCESS_EVENT(GameProcessEvent)
    {
        ImGui_ImplSdlGL3_ProcessEvent(Event);
    }

    export_func GAME_RENDER(GameRender)
    {
        auto g = Game;
        float dt = DeltaTime;

        ImGui_ImplSdlGL3_NewFrame(g->Window);
        if (IsJustPressed(*g, Action_debugUI))
        {
            Global_DebugUI = !Global_DebugUI;
        }

        switch (g->Mode)
        {
        case GameMode_LoadingScreen:
            RenderLoadingScreen(*g, dt, InitLevel);
            break;

        case GameMode_Level:
            RenderLevel(*g, dt);
            break;
        }

#ifdef DRAFT_DEBUG
        if (g->Mode != GameMode_LoadingScreen)
        {
            static float ReloadTimer = 0.0f;
            ReloadTimer += dt;
            if (ReloadTimer >= 1.0f)
            {
                ReloadTimer = 0.0f;
                CheckAssetsChange(g->AssetLoader);
            }
        }
#endif

        DrawDebugUI(*g, dt);
        ImGui::Render();
    }

    export_func GAME_DESTROY(GameDestroy)
    {
        auto g = Game;
        ImGui_ImplSdlGL3_Shutdown();
        DestroyAssetLoader(g->AssetLoader);
        FreeArena(g->Arena);
        FreeArena(g->AssetLoader.Arena);
    }
}
