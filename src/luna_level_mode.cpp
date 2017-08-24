#include "luna.h"
#include "luna_level_mode.h"

#define FloorTile 1
#define WallTile 2
#define WallHeight 2.5f
#define TileHeight 0.25f
#define CameraOffsetY 4.0f
#define CameraOffsetZ 5.0f

#define TileAt(Data, Width, Length, x, z) (Data[(Length - z - 1) * Width + (x)])
#define HeightAt(Data, Width, Length, x, z) (TileAt(Data,Width,Length,x,z) - 13*16)

static string LevelTable[] = {
    "data/levels/test.ll", // first map
};

static void
AddQuad(vertex_buffer &Buffer, vec3 p1, vec3 p2, vec3 p3, vec3 p4,
        color c1 = Color_white, vec3 n = vec3(0))
{
    color c2 = c1;
    color c3 = c1;
    color c4 = c1;

    texture_rect Uv = {0, 0, 1, 1};
    PushVertex(Buffer, {p1.x, p1.y, p1.z, Uv.u,  Uv.v,   c1.r, c1.g, c1.b, c1.a,  n.x, n.y, n.z});
    PushVertex(Buffer, {p2.x, p2.y, p2.z, Uv.u2, Uv.v,   c2.r, c2.g, c2.b, c2.a,  n.x, n.y, n.z});
    PushVertex(Buffer, {p4.x, p4.y, p4.z, Uv.u,  Uv.v2,  c4.r, c4.g, c4.b, c4.a,  n.x, n.y, n.z});

    PushVertex(Buffer, {p2.x, p2.y, p2.z, Uv.u2, Uv.v,   c2.r, c2.g, c2.b, c2.a,  n.x, n.y, n.z});
    PushVertex(Buffer, {p3.x, p3.y, p3.z, Uv.u2, Uv.v2,  c3.r, c3.g, c3.b, c3.a,  n.x, n.y, n.z});
    PushVertex(Buffer, {p4.x, p4.y, p4.z, Uv.u,  Uv.v2,  c4.r, c4.g, c4.b, c4.a,  n.x, n.y, n.z});
}

static void
AddCube(vertex_buffer &Buffer, float height)
{
    float y = -0.5;
    float h = y + height;
    float x = -0.5f;
    float w = x+1;
    float z = 0.5f;
    float d = z-1;
    AddQuad(Buffer, vec3(x, y, z), vec3(w, y, z), vec3(w, h, z), vec3(x, h, z), Color_white, vec3(0, 0, 1));
    AddQuad(Buffer, vec3(w, y, z), vec3(w, y, d), vec3(w, h, d), vec3(w, h, z), Color_white, vec3(1, 0, 0));
    AddQuad(Buffer, vec3(w, y, d), vec3(x, y, d), vec3(x, h, d), vec3(w, h, d), Color_white, vec3(0, 0, -1));
    AddQuad(Buffer, vec3(x, y, d), vec3(x, y, z), vec3(x, h, z), vec3(x, h, d), Color_white, vec3(-1, 0, 0));
    AddQuad(Buffer, vec3(x, h, z), vec3(w, h, z), vec3(w, h, d), vec3(x, h, d), Color_white, vec3(0, 1, 0));
}

static void
AddEntity(level_mode &Mode, entity *Entity)
{
    if (Entity->Mesh)
    {
        Mode.MeshEntities.push_back(Entity);
    }
    if (Entity->Sprite)
    {
        Mode.SpriteEntities.push_back(Entity);
    }
    if (Entity->Shape)
    {
        Mode.ShapedEntities.push_back(Entity);
    }
}

static shape *
CreateShape(shape_type Type)
{
    shape *Result = new shape;
    Result->Type = Type;
    return Result;
}

static entity *
CreatePlayerEntity(game_state &GameState)
{
    entity *Result = new entity;
    Result->Size.x = 0.5f;
    Result->Size.z = 0.5f;
    Result->MovementSpeed = 3;
    Result->IdleAnimation = {{0, 1}, 0.8f};
    Result->WalkAnimation = {{16, 17, 18, 19, 20, 21, 22, 23}, 0.1f};

    Result->Sprite = new animated_sprite;
    Result->Sprite->Scale = vec3(0.75f);
    Result->Sprite->Offset.y = 0.25f;
    Result->Sprite->Texture = LoadTextureFile(GameState.AssetCache, "data/textures/player.png");
    Result->Sprite->Frames  = SplitTexture(*Result->Sprite->Texture, 16, 16);
    SetAnimation(*Result->Sprite, Result->IdleAnimation);

    Result->Shape = CreateShape(Shape_aabb);
    return Result;
}

void StartLevel(game_state &GameState, level_mode &Mode)
{
    GameState.GameMode = GameMode_level;

    FILE *FileHandle = fopen(LevelTable[Mode.CurrentLevel].c_str(), "rb");
    if (!FileHandle)
    {
        std::cout << "Error loading map file: " << LevelTable[Mode.CurrentLevel] << std::endl;
    }
    fseek(FileHandle, 0, SEEK_SET);

    uint32 Header[2];
    uint32 Width, Length;
    uint8 *TileData;
    uint8 *HeightMap;
    fread(Header, sizeof(uint32), 2, FileHandle);

    Width = Header[0];
    Length = Header[1];
    TileData = new uint8[Width * Length];
    HeightMap = new uint8[Width * Length];
    fread(TileData, sizeof(uint8), Width * Length, FileHandle);
    fread(HeightMap, sizeof(uint8), Width * Length, FileHandle);
    fclose(FileHandle);

    {
        auto &FloorMesh = Mode.FloorMesh;
        InitMeshBuffer(FloorMesh.Buffer);
        AddCube(FloorMesh.Buffer, 1);

        material FloorMaterial = {Color_white, 0, 1, LoadTextureFile(GameState.AssetCache, "data/textures/floor2.png")};
        FloorMesh.Parts.resize(1);
        FloorMesh.Parts[0] = {FloorMaterial, 0, FloorMesh.Buffer.VertexCount, GL_TRIANGLES};
        UploadVertices(FloorMesh.Buffer, GL_STATIC_DRAW);
    }

    {
        auto &WallMesh = Mode.WallMesh;
        InitMeshBuffer(WallMesh.Buffer);
        AddCube(WallMesh.Buffer, 1);

        material WallMaterial = {Color_blue, 0, 0, NULL};
        WallMesh.Parts.resize(1);
        WallMesh.Parts[0] = {WallMaterial, 0, WallMesh.Buffer.VertexCount, GL_TRIANGLES};
        UploadVertices(WallMesh.Buffer, GL_STATIC_DRAW);
    }

    MakeCameraPerspective(Mode.Camera, (float)GameState.Width, (float)GameState.Height, 70.0f, 0.1f, 1000.0f);
    Mode.Camera.Position = vec3(5, 5, 0);
    Mode.Camera.LookAt = vec3(5, 0, -10);

    Mode.Width = Width;
    Mode.Length = Length;
    Mode.TileData = TileData;
    Mode.HeightMap = HeightMap;

    Mode.PlayerEntity = CreatePlayerEntity(GameState);
    Mode.PlayerEntity->Position = vec3(31, 5, -5);

    Mode.Gravity = vec3(0, -9.87f, 0);
    AddEntity(Mode, Mode.PlayerEntity);

    for (uint32 x = 0; x < Mode.Width; x++)
    {
        for (uint32 z = 0; z < Mode.Length; z++)
        {
            uint8 Tile = TileAt(Mode.TileData, Mode.Width, Mode.Length, x, z);
            uint8 Height = HeightAt(Mode.HeightMap, Mode.Width, Mode.Length, x, z);

            float fx = (float)x;
            float fy = (float)Height;
            float fz = (float)z;

            if (Tile == FloorTile)
            {
                auto Entity = new entity;
                Entity->Position = vec3(fx, fy*TileHeight, -fz);
                Entity->Mesh = &Mode.FloorMesh;
                Entity->Shape = CreateShape(Shape_aabb);
                Entity->Flags |= EntityFlag_kinematic;
                AddEntity(Mode, Entity);
            }
            else if (Tile == WallTile)
            {
                auto Entity = new entity;
                Entity->Position = vec3(fx, fy*TileHeight + 0.75f, -fz);
                Entity->Mesh = &Mode.WallMesh;
                Entity->Size.y = WallHeight;
                Entity->Shape = CreateShape(Shape_aabb);
                Entity->Flags |= EntityFlag_kinematic;
                AddEntity(Mode, Entity);
            }
        }
    }
}

inline static float
GetAxisValue(game_input &Input, action_type Type)
{
    return Input.Actions[Type].AxisValue;
}

inline static bool
IsJustPressed(game_state &GameState, action_type Type)
{
    return GameState.Input.Actions[Type].Pressed > 0 &&
        GameState.PrevInput.Actions[Type].Pressed == 0;
}

inline static vec3
CameraDir(camera &Camera)
{
    return glm::normalize(Camera.LookAt - Camera.Position);
}

inline static void
UpdateEntityAABB(entity *Entity)
{
    // TODO: this is not good
    Entity->Shape->AABB.Half = Entity->Size*0.5f;
    Entity->Shape->AABB.Position = Entity->Position;
}

static bool
HandleCollision(entity *First, entity *Second)
{
    return true;
}

inline static void
ApplyVelocity(entity *Entity, vec3 Velocity)
{
    if (Entity->Flags & EntityFlag_kinematic)
    {
        return;
    }
    Entity->Velocity += Velocity;
}

inline static void
ApplyCorrection(entity *Entity, vec3 Correction)
{
    if (Entity->Flags & EntityFlag_kinematic)
    {
        return;
    }
    Entity->Position += Correction;
}

inline static float
Length2DSq(vec3 v)
{
    return std::abs(v.x + v.z);
}

inline static float
Square(float v)
{
    return v * v;
}

inline static void
StopMovement(entity *Entity)
{
    Entity->Position.x = Entity->MovementDest.x;
    Entity->Position.z = Entity->MovementDest.z;
    Entity->IsMoving = false;
    Entity->Velocity = vec3(0.0f);
    SetAnimation(*Entity->Sprite, Entity->IdleAnimation, true);
}

static void
SetMovementDest(level_mode &Mode, entity *Entity, vec3 BasePosition, float MoveH, float MoveV)
{
    direction Dir = Entity->Sprite->Direction;
    if (MoveH < 0)
    {
        Dir = Direction_left;
    }
    else if (MoveH > 0)
    {
        Dir = Direction_right;
    }
    Entity->Sprite->Direction = Dir;

    bool MoveAny = (std::abs(MoveH) + std::abs(MoveV)) == 1;
    if (MoveAny)
    {
        int DestTileX   = (int)(BasePosition.x+0.5f + MoveH);
        int DestTileZ   = (int)(BasePosition.z-0.5f - MoveV);
        DestTileZ = -DestTileZ;

        uint8 Tile = TileAt(Mode.TileData, Mode.Width, Mode.Length, DestTileX, DestTileZ);
        if (Tile != WallTile)
        {
            Entity->IsMoving = true;
            Entity->MovementVelocity = vec3(MoveH * Entity->MovementSpeed, Entity->Velocity.y, -MoveV * Entity->MovementSpeed);
            Entity->MovementDest = vec3(int(BasePosition.x+0.5f + MoveH), 0,
                                        int(BasePosition.z-0.5f - MoveV));
            SetAnimation(*Entity->Sprite, Entity->WalkAnimation, false);
        }
        else
        {
            StopMovement(Entity);
        }
    }
}

#ifdef LUNA_DEBUG
static bool DebugFreeCamEnabled = true;
static float Pitch;
static float Yaw;
#endif

#define PlayerSpeed 5.0f
#define ClimbHeight 0.02f
void UpdateAndRenderLevel(game_state &GameState, level_mode &Mode, float DeltaTime)
{
    auto &Input = GameState.Input;
    auto &Camera = Mode.Camera;
    auto CamDir = CameraDir(Camera);

#ifdef LUNA_DEBUG
    if (IsJustPressed(GameState, Action_debugFreeCam))
    {
        DebugFreeCamEnabled = !DebugFreeCamEnabled;
    }

    if (DebugFreeCamEnabled)
    {
        float Speed = 20.0f;
        float AxisValue = GetAxisValue(Input, Action_camVertical);

        Camera.Position += CameraDir(Mode.Camera) * AxisValue * Speed * DeltaTime;

        if (Input.MouseState.Buttons & MouseButton_middle)
        {
            Yaw += Input.MouseState.dX * DeltaTime;
            Pitch -= Input.MouseState.dY * DeltaTime;
            Pitch = glm::clamp(Pitch, -1.5f, 1.5f);
        }

        CamDir.x = sin(-Yaw);
        CamDir.y = Pitch;
        CamDir.z = cos(Yaw);
        Camera.LookAt = Camera.Position + CamDir * 50.0f;

        float StrafeYaw = -Yaw - (M_PI / 2);
        float hAxisValue = GetAxisValue(Input, Action_camHorizontal);
        Camera.Position += vec3(sin(StrafeYaw), 0, cos(StrafeYaw)) * hAxisValue * Speed * DeltaTime;
    }
    else
#endif
    {
        Mode.Camera.Position = vec3(Mode.PlayerEntity->Position.x, Mode.PlayerEntity->Position.y + CameraOffsetY, Mode.PlayerEntity->Position.z + CameraOffsetZ);
        Mode.Camera.LookAt = Mode.PlayerEntity->Position;
    }

    {
        float MoveH = GetAxisValue(GameState.Input, Action_horizontal);
        float MoveV = GetAxisValue(GameState.Input, Action_vertical);
        bool MoveAny = (std::abs(MoveH) + std::abs(MoveV)) == 1;
        entity *Entity = Mode.PlayerEntity;

        if (Entity->IsMoving)
        {
            if (Length2DSq(Entity->Position - Entity->MovementDest) < Square(0.2f))
            {
                if (!MoveAny)
                {
                    StopMovement(Entity);
                }
                else
                {
                    SetMovementDest(Mode, Entity, Entity->MovementDest, MoveH, MoveV);
                }
            }
            else
            {
                Entity->Velocity.x = Entity->MovementVelocity.x;
                Entity->Velocity.z = Entity->MovementVelocity.z;
            }
        }
        else
        {
            SetMovementDest(Mode, Entity, Entity->Position, MoveH, MoveV);
        }
    }

    size_t FrameCollisionCount = 0;
    size_t EntityCount = Mode.ShapedEntities.size();
    for (size_t i = 0; i < EntityCount; i++)
    {
        auto EntityA = Mode.ShapedEntities[i];
        EntityA->NumCollisions = 0;
        UpdateEntityAABB(EntityA);

        for (size_t j = i+1; j < EntityCount; j++)
        {
            auto EntityB = Mode.ShapedEntities[j];
            UpdateEntityAABB(EntityB);

            shape_aabb &First = EntityA->Shape->AABB;
            shape_aabb &Second = EntityB->Shape->AABB;
            vec3 Dif = Second.Position - First.Position;
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
            if (dx < dy && dy > TileHeight+ClimbHeight)
            {
                Col.Normal.x = std::copysign(1, Dif.x);
                Col.Depth = dx;
            }
            else if (dz < dy && dy > TileHeight+ClimbHeight)
            {
                Col.Normal.z = std::copysign(1, Dif.z);
                Col.Depth = dz;
            }
            else
            {
                Col.Normal.y = std::copysign(1, Dif.y);
                Col.Depth = dy;
            }

            size_t Size = Mode.Collisions.size();
            if (FrameCollisionCount + 1 > Size)
            {
                if (Size == 0)
                {
                    Size = 4;
                }

                Mode.Collisions.resize(Size * 2);
            }

            Col.First = EntityA;
            Col.Second = EntityB;
            Mode.Collisions[FrameCollisionCount++] = Col;
        }
    }

    for (size_t i = 0; i < FrameCollisionCount; i++)
    {
        auto &Col = Mode.Collisions[i];
        Col.First->NumCollisions++;
        Col.Second->NumCollisions++;

        if (HandleCollision(Col.First, Col.Second))
        {
            vec3 rv = Col.Second->Velocity - Col.First->Velocity;
            float VelNormal = glm::dot(rv, Col.Normal);
            if (VelNormal > 0.0f)
            {
                continue;
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
    }

    for (size_t i = 0; i < EntityCount; i++)
    {
        auto Entity = Mode.ShapedEntities[i];
        if (!(Entity->Flags & EntityFlag_kinematic))
        {
            Entity->Velocity += Mode.Gravity * DeltaTime;
        }
        Entity->Position += Entity->Velocity * DeltaTime;
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    UpdateProjectionView(Mode.Camera);

    for (size_t i = 0; i < Mode.MeshEntities.size(); i++)
    {
        auto Entity = Mode.MeshEntities[i];
        mat4 TransformMatrix = glm::translate(mat4(1.0f), Entity->Position);
        TransformMatrix = glm::scale(TransformMatrix, Entity->Size);
        RenderMesh(GameState.RenderState, Mode.Camera, *Entity->Mesh, TransformMatrix);
    }

    for (size_t i = 0; i < Mode.SpriteEntities.size(); i++)
    {
        auto Entity = Mode.SpriteEntities[i];
        UpdateAnimation(*Entity->Sprite, DeltaTime);
        RenderSprite(GameState.RenderState, Mode.Camera, *Entity->Sprite, Entity->Position);
    }

#ifdef LUNA_DEBUG
    for (size_t i = 0; i < Mode.ShapedEntities.size(); i++)
    {
        auto Entity = Mode.ShapedEntities[i];
        DebugRenderAABB(GameState.RenderState, Mode.Camera, &Entity->Shape->AABB, Entity->NumCollisions > 0);
    }
#endif
}
