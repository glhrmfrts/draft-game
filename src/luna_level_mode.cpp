#include "luna.h"
#include "luna_level_mode.h"

#define FloorTile 1
#define WallTile 2
#define WallHeight 2.5f
#define TileHeight 0.25f
#define CameraOffsetY 4
#define CameraOffsetZ 5

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
    Result->Sprite = new animated_sprite;
    Result->Sprite->Texture = LoadTextureFile(GameState.AssetCache, "data/textures/player.png");
    Result->Sprite->Frames  = SplitTexture(*Result->Sprite->Texture, 16, 16);
    SetAnimationFrames(*Result->Sprite, {0, 16}, 0.8f);

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

    Mode.Gravity = vec3(0, -0.87f, 0);
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
                Entity->Scale.y = WallHeight;
                Entity->Shape = CreateShape(Shape_aabb);
                Entity->Flags |= EntityFlag_kinematic;
                AddEntity(Mode, Entity);
            }
        }
    }
}

#ifdef LUNA_DEBUG
static bool DebugFreeCamEnabled = true;
static float Pitch;
static float Yaw;
#endif

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

#define PlayerSpeed 5.0f
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
    }

    CollisionDetectionStep(Mode.ShapedEntities, Mode.Gravity, DeltaTime);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    UpdateProjectionView(Mode.Camera);

    for (size_t i = 0; i < Mode.MeshEntities.size(); i++)
    {
        auto Entity = Mode.MeshEntities[i];
        mat4 TransformMatrix = glm::translate(mat4(1.0f), Entity->Position);
        TransformMatrix = glm::scale(TransformMatrix, Entity->Scale);
        RenderMesh(GameState.RenderState, Mode.Camera, *Entity->Mesh, TransformMatrix);
    }

    for (size_t i = 0; i < Mode.SpriteEntities.size(); i++)
    {
        auto Entity = Mode.SpriteEntities[i];
        UpdateAnimation(*Entity->Sprite, DeltaTime);
        RenderSprite(GameState.RenderState, Mode.Camera, *Entity->Sprite, Entity->Position);
    }
}
