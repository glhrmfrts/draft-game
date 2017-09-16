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

#define TrackSegmentLength  512
#define TrackSegmentWidth   1024
#define TrackSegmentCount   8
#define TrackSegmentPadding 0
#define SkyboxScale         vec3(500.0f)
#define CrystalColor        IntColor(FirstPalette.Colors[1])
static void
StartLevel(game_state &Game)
{
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
			LoadTextureFile(Game.AssetCache, "data/textures/grid.png", Texture_Mipmap | Texture_Anisotropic | Texture_WrapRepeat),
			0,
			vec2{TrackSegmentWidth/16,TrackSegmentWidth/16}
		};

		float w = TrackSegmentWidth;
        float l = -w/2;
        float r = w/2;
        AddQuad(FloorMesh.Buffer, vec3(l, 0, 0), vec3(r, 0, 0), vec3(r, 1, 0), vec3(l, 1, 0), Color_white, vec3(1, 1, 1));
        AddPart(FloorMesh, {FloorMaterial, 0, FloorMesh.Buffer.VertexCount, GL_TRIANGLES});
        EndMesh(FloorMesh, GL_STATIC_DRAW);
    }

    {
        auto &ShipMesh = Game.ShipMesh;
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
    }

    {
        auto &SkyboxMesh = Game.SkyboxMesh;
        InitMeshBuffer(SkyboxMesh.Buffer);

        texture *FrontTexture = LoadTextureFile(Game.AssetCache, "data/skyboxes/kurt/space_bk.png");
        texture *RightTexture = LoadTextureFile(Game.AssetCache, "data/skyboxes/kurt/space_rt.png");
        texture *BackTexture = LoadTextureFile(Game.AssetCache, "data/skyboxes/kurt/space_ft.png");
        texture *LeftTexture = LoadTextureFile(Game.AssetCache, "data/skyboxes/kurt/space_lf.png");
        texture *TopTexture = LoadTextureFile(Game.AssetCache, "data/skyboxes/kurt/space_up.png");
        texture *BottomTexture = LoadTextureFile(Game.AssetCache, "data/skyboxes/kurt/space_dn.png");
        AddSkyboxFace(SkyboxMesh, vec3(-1, 1, -1), vec3(1, 1, -1), vec3(1, 1, 1), vec3(-1, 1, 1), FrontTexture, 0);
        AddSkyboxFace(SkyboxMesh, vec3(1, 1, -1), vec3(1, -1, -1), vec3(1, -1, 1), vec3(1, 1, 1), RightTexture, 1);
        AddSkyboxFace(SkyboxMesh, vec3(1, -1, -1), vec3(-1, -1, -1), vec3(-1, -1, 1), vec3(1, -1, 1), BackTexture, 2);
        AddSkyboxFace(SkyboxMesh, vec3(-1, -1, -1), vec3(-1, 1, -1), vec3(-1, 1, 1), vec3(-1, -1, 1), LeftTexture, 3);
        AddSkyboxFace(SkyboxMesh, vec3(-1, 1, 1), vec3(1, 1, 1), vec3(1, -1, 1), vec3(-1, -1, 1), TopTexture, 4);
        AddSkyboxFace(SkyboxMesh, vec3(-1, -1, -1), vec3(1, -1, -1), vec3(1, 1, -1), vec3(-1, 1, -1), BottomTexture, 5);
        EndMesh(SkyboxMesh, GL_STATIC_DRAW);

        auto Entity = PushStruct<entity>(Game.Arena);
        Entity->Model = CreateModel(Game.Arena, &SkyboxMesh);
        Entity->Transform.Scale = SkyboxScale;
        AddEntity(Game, Entity);
        Game.SkyboxEntity = Entity;
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
    Game.Camera.Position = vec3(2, 0, 0);
    Game.Camera.LookAt = vec3(0, 0, 0);
    Game.Gravity = vec3(0, 0, 0);

    Game.PlayerEntity = CreateShipEntity(Game, Color_blue, IntColor(FirstPalette.Colors[1]), true);
    Game.PlayerEntity->Transform.Rotation.y = 20.0f;
	Game.PlayerEntity->Transform.Velocity.y = ShipMaxVel;
    AddEntity(Game, Game.PlayerEntity);

    for (int i = 0; i < TrackSegmentCount * 2; i++)
    {
        auto *Entity = PushStruct<entity>(Game.Arena);
		Entity->Type = EntityType_TrackSegment;
        Entity->Transform.Scale = vec3(1, TrackSegmentLength, 0);
        Entity->Model = CreateModel(Game.Arena, &Game.FloorMesh);
        AddEntity(Game, Entity);

		float z = 0;
		if (i < TrackSegmentCount)
		{
			z = -0.25f;
		}
		else
		{
			z = 10.0f;
		}
		Entity->Transform.Position = vec3(0, (i%TrackSegmentCount)*TrackSegmentLength + TrackSegmentPadding*(i%TrackSegmentCount), z);
    }

	Game.CurrentLevel = GenerateTestLevel(Game.Arena);
}

static void
DebugReset(game_state &Game)
{
	Game.PlayerEntity->Transform.Position = vec3{ 0.0f };
	Game.PlayerEntity->Transform.Velocity = vec3{ 0.0f };
	Game.PlayerEntity->Transform.Velocity.y = ShipMaxVel;
    Game.PlayerEntity->PlayerState->Score = 0;
    Game.CurrentLevel->CurrentTick = 0;
	Game.CurrentLevel->DelayedEvents.clear();

	for (auto *Entity : Game.CurrentLevel->EntitiesToRemove)
	{
		RemoveEntity(Game, Entity);
	}

	for (int i = 0; i < TrackSegmentCount * 2; i++)
	{
		auto *Entity = Game.TrackEntities[i];
		Entity->Transform.Position.y = (i%TrackSegmentCount)*TrackSegmentLength + TrackSegmentPadding*(i%TrackSegmentCount);
	}
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
UpdateAndRenderLevel(game_state &Game, float DeltaTime)
{
    auto &Input = Game.Input;
    auto &Camera = Game.Camera;
	auto *PlayerEntity = Game.PlayerEntity;
    auto *PlayerShip = PlayerEntity->Ship;
    vec3 CamDir = CameraDir(Camera);

    static bool Paused = false;
    if (IsJustPressed(Game, Action_debugPause))
    {
        Paused = !Paused;
    }
    if (Paused) return;

	UpdateLevel(Game, DeltaTime);

#ifdef DRAFT_DEBUG
    if (Global_Camera_FreeCam)
    {
        static float Pitch;
        static float Yaw;
        float Speed = 50.0f;
        float AxisValue = GetAxisValue(Input, Action_camVertical);

        Camera.Position += CameraDir(Game.Camera) * AxisValue * Speed * DeltaTime;

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
#endif

	{
		float MoveX = GetAxisValue(Game.Input, Action_horizontal);
		float MoveY = 0;
		MoveShipEntity(PlayerEntity, MoveX, MoveY, DeltaTime);
	}

    if (PlayerShip->DraftCharge == 1.0f)
    {
        if (IsJustPressed(Game, Action_boost))
        {
			ApplyBoostToShip(Game.PlayerEntity, DraftBoost, ShipMaxVel + DraftBoost*1.5f);
			PlayerShip->CurrentDraftTime = 0.0f;
            Game.PlayerEntity->PlayerState->Score += DraftBoostScore;
        }
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
        Camera.Position = vec3(PlayerPosition.x,
                               PlayerPosition.y + Global_Camera_OffsetY,
                               PlayerPosition.z + Global_Camera_OffsetZ);
        Camera.LookAt = Camera.Position + vec3(0, 10, 0);
    }

    {
        auto *Entity = Game.PlayerEntity;
        Game.SkyboxEntity->Transform.Position.y = Entity->Transform.Position.y;
        float dx = Entity->Transform.Position.x - Game.SkyboxEntity->Transform.Position.x;
        Game.SkyboxEntity->Transform.Position.x += dx * 0.25f;
    }

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
            MoveY = 4.0f;
        }
        MoveShipEntity(Entity, sin(EnemyMoveH), MoveY, DeltaTime);
    }

    for (auto *Entity : Game.WallEntities)
    {
        if (!Entity) continue;

        auto *WallState = Entity->WallState;
        if (WallState->Timer >= Global_Game_WallRaiseTime)
        {
            WallState->Timer = Global_Game_WallRaiseTime;
        }
        else
        {
            WallState->Timer += DeltaTime;
        }

        float Amount = 1.0f - (WallState->Timer / Global_Game_WallRaiseTime);
        Println(Amount);
        Entity->Transform.Position.z = WallState->BaseZ + (Global_Game_WallStartOffset * Amount);
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
                c1 += (c2 - c1) * CurrentTrailTime;
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
        for (int i = 0; i < Explosion->Pieces.size(); i++)
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

    for (auto *Entity : Game.TrackEntities)
    {
        if (!Entity) continue;

        if (Entity->Transform.Position.y + TrackSegmentLength+TrackSegmentPadding < Game.Camera.Position.y)
        {
            Entity->Transform.Position.y += TrackSegmentCount*TrackSegmentLength + (TrackSegmentPadding*TrackSegmentCount);
        }
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
        InitFreeType(Game->AssetCache);
		InitGUI(Game->GUI, Game->Input);
		MakeCameraOrthographic(Game->GUICamera, 0, Width, 0, Height, -1, 1);
		InitRenderState(Game->RenderState, Width, Height);
		StartLevel(*Game);
	}

	// @TODO: this exists only for imgui, remove in the future
	export_func GAME_PROCESS_EVENT(GameProcessEvent)
	{
		ImGui_ImplSdlGL3_ProcessEvent(Event);
	}

	export_func GAME_RENDER(GameRender)
	{
		ImGui_ImplSdlGL3_NewFrame(Game->Window);
		UpdateAndRenderLevel(*Game, DeltaTime);
		DrawDebugUI(*Game, DeltaTime);
		ImGui::Render();
	}

	export_func GAME_DESTROY(GameDestroy)
	{
		ImGui_ImplSdlGL3_Shutdown();
		FreeArena(Game->Arena);
		FreeArena(Game->AssetCache.Arena);
	}
}
