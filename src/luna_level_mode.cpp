#include "luna.h"
#include "luna_level_mode.h"

#define FloorTile 1
#define WallTile 2
#define WallHeight 1.75f

#define TileAt(Data, Width, Length, x, z) (Data[(Length - z - 1) * Width + (x)])

static string LevelTable[] = {
    "data/levels/test.ll", // first map
};

static void AddQuad(vertex_buffer &Buffer, vec3 p1, vec3 p2, vec3 p3, vec3 p4,
                    color c1 = Color_white)
{
    color c2 = c1;
    color c3 = c1;
    color c4 = c1;

    texture_rect Uv = {0, 0, 1, 1};
    PushVertex(Buffer, {p1.x, p1.y, p1.z, Uv.U,  Uv.V,   c1.r, c1.g, c1.b, c1.a,  0, 0, 0});
    PushVertex(Buffer, {p2.x, p2.y, p2.z, Uv.U2, Uv.V,   c2.r, c2.g, c2.b, c2.a,  0, 0, 0});
    PushVertex(Buffer, {p4.x, p4.y, p4.z, Uv.U,  Uv.V2,  c4.r, c4.g, c4.b, c4.a,  0, 0, 0});

    PushVertex(Buffer, {p2.x, p2.y, p2.z, Uv.U2, Uv.V,   c2.r, c2.g, c2.b, c2.a,  0, 0, 0});
    PushVertex(Buffer, {p3.x, p3.y, p3.z, Uv.U2, Uv.V2,  c3.r, c3.g, c3.b, c3.a,  0, 0, 0});
    PushVertex(Buffer, {p4.x, p4.y, p4.z, Uv.U,  Uv.V2,  c4.r, c4.g, c4.b, c4.a,  0, 0, 0});
}

static void AddWall(vertex_buffer &Buffer, uint8 *TileData, uint32 Width, uint32 Length, uint32 x, uint32 z)
{
    uint8 TileLeft = TileAt(TileData, Width, Length, x-1, z);
    uint8 TileRight = TileAt(TileData, Width, Length, x+1, z);
    uint8 TileDown = TileAt(TileData, Width, Length, x, z+1);
    float h = WallHeight;

    float fx = (float)x;
    float fz = (float)z;
    if (TileLeft != WallTile) { // Draw left
        printf("Left\n");
        AddQuad(Buffer,
                vec3(fx, 0, -fz - 1),
                vec3(fx, 0, -fz),
                vec3(fx, h, -fz),
                vec3(fx, h, -fz - 1),
                Color_blue);
    }
    if (TileRight != WallTile) { // Draw right
        AddQuad(Buffer,
                vec3(fx+1, 0, -fz),
                vec3(fx+1, 0, -fz - 1),
                vec3(fx+1, h, -fz - 1),
                vec3(fx+1, h, -fz),
                Color_blue);
    }
    if (TileDown != WallTile) {
        AddQuad(Buffer,
                vec3(fx,   0, -fz),
                vec3(fx+1, 0, -fz),
                vec3(fx+1, h, -fz),
                vec3(fx,   h, -fz),
                Color_blue);
    }

    AddQuad(Buffer,
            vec3(fx,   h, -fz),
            vec3(fx+1, h, -fz),
            vec3(fx+1, h, -fz - 1),
            vec3(fx,   h, -fz - 1 ),
            Color_gray);
}

void StartLevel(game_state &GameState, level_mode &LevelMode)
{
    GameState.GameMode = GameMode_level;

    FILE *FileHandle = fopen(LevelTable[LevelMode.CurrentLevel].c_str(), "rb");
    if (!FileHandle) {
        std::cout << "Error loading map file: " << LevelTable[LevelMode.CurrentLevel] << std::endl;
    }
    fseek(FileHandle, 0, SEEK_SET);

    uint32 Header[2];
    uint32 Width, Length;
    uint8 *TileData;
    fread(Header, sizeof(uint32), 2, FileHandle);

    Width = Header[0];
    Length = Header[1];
    TileData = new uint8[Width * Length];
    fread(TileData, sizeof(uint8), Width * Length, FileHandle);

    auto &FloorMesh = LevelMode.FloorMesh;
    auto &WallMesh = LevelMode.WallMesh;
    InitMeshBuffer(FloorMesh.Buffer);
    InitMeshBuffer(WallMesh.Buffer);

    uint8 PreviousTile = 255;
    for (uint32 x = 0; x < Width; x++) {
        for (uint32 z = 0; z < Length; z++) {
            uint8 Tile = TileAt(TileData, Width, Length, x, z);
            if (Tile == FloorTile) {
                float fx = (float)x;
                float fz = (float)z;
                AddQuad(FloorMesh.Buffer,
                        vec3(fx, 0, -fz),
                        vec3(fx+1, 0, -fz),
                        vec3(fx+1, 0, -fz - 1),
                        vec3(fx, 0, -fz - 1));
            }
            else if (Tile == WallTile) {
                AddWall(WallMesh.Buffer, TileData, Width, Length, x, z);
            }

            PreviousTile = Tile;
        }
    }

    material FloorMaterial = {Color_white, 0, 1, LoadTextureFile(GameState.AssetCache, "data/textures/floor.png")};
    FloorMesh.Parts.resize(1);
    FloorMesh.Parts[0] = {FloorMaterial, 0, FloorMesh.Buffer.VertexCount, GL_TRIANGLES};
    UploadVertices(FloorMesh.Buffer, GL_STATIC_DRAW);

    material WallMaterial = {Color_white, 0, 0, NULL};
    WallMesh.Parts.resize(1);
    WallMesh.Parts[0] = {WallMaterial, 0, WallMesh.Buffer.VertexCount, GL_TRIANGLES};
    UploadVertices(WallMesh.Buffer, GL_STATIC_DRAW);

    fclose(FileHandle);
    free(TileData);

    MakeCameraPerspective(LevelMode.Camera, (float)GameState.Width, (float)GameState.Height, 70.0f, 0.1f, 1000.0f);
    LevelMode.Camera.Position = vec3(5, 5, 0);
    LevelMode.Camera.LookAt = vec3(5, 0, -10);
}

#ifdef LUNA_DEBUG
static bool DebugFreeCamEnabled = true;
static float Pitch;
static float Yaw;
#endif

inline static float GetAxisValue(game_input &Input, action_type Type)
{
    return Input.Actions[Type].AxisValue;
}

inline static vec3 CameraDir(camera &Camera)
{
    return glm::normalize(Camera.LookAt - Camera.Position);
}

void UpdateLevel(game_state &GameState, level_mode &LevelMode, float DeltaTime)
{
#ifdef LUNA_DEBUG
    if (DebugFreeCamEnabled) {
        auto &Input = GameState.Input;
        auto &Camera = LevelMode.Camera;
        auto CamDir = CameraDir(Camera);
        float Speed = 20.0f;
        float AxisValue = GetAxisValue(Input, Action_camVertical);

        Camera.Position += CameraDir(LevelMode.Camera) * AxisValue * Speed * DeltaTime;

        // TODO: use middle mouse button to look around
        Yaw += Input.MouseState.dX * DeltaTime;
        Pitch -= Input.MouseState.dY * DeltaTime;
        Pitch = glm::clamp(Pitch, -1.5f, 1.5f);

        CamDir.x = sin(-Yaw);
        CamDir.y = Pitch;
        CamDir.z = cos(Yaw);
        Camera.LookAt = Camera.Position + CamDir * 50.0f;

        float StrafeYaw = -Yaw - (M_PI / 2);
        float hAxisValue = GetAxisValue(Input, Action_camHorizontal);
        Camera.Position += vec3(sin(StrafeYaw), 0, cos(StrafeYaw)) * hAxisValue * Speed * DeltaTime;
    }
#endif
}

void RenderLevel(game_state &GameState, level_mode &LevelMode)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    UpdateProjectionView(LevelMode.Camera);
    RenderMesh(GameState.RenderState, LevelMode.Camera, LevelMode.FloorMesh);
    RenderMesh(GameState.RenderState, LevelMode.Camera, LevelMode.WallMesh);

    //auto &g = GameState.GUI;
    //UpdateProjectionView(GameState.GUICamera);
    //Begin(g, GameState.GUICamera);
    //PushRect(g, {0, 0, 50, 50}, color(1, 0, 0, 1));
    //End(g);
}
