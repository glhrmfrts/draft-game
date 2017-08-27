#include "draft.h"
#include "level.h"

#define CameraOffsetY 4.0f
#define CameraOffsetZ 5.0f

static void
AddQuad(vertex_buffer &Buffer, vec3 p1, vec3 p2, vec3 p3, vec3 p4,
        color c1 = Color_white, vec3 n = vec3(0), bool FlipV = false)
{
    color c2 = c1;
    color c3 = c1;
    color c4 = c1;

    texture_rect Uv = {0, 0, 1, 1};
    if (FlipV)
    {
        Uv.v = 1;
        Uv.v2 = 0;
    }
    PushVertex(Buffer, {p1.x, p1.y, p1.z, Uv.u,  Uv.v,   c1.r, c1.g, c1.b, c1.a,  n.x, n.y, n.z});
    PushVertex(Buffer, {p2.x, p2.y, p2.z, Uv.u2, Uv.v,   c2.r, c2.g, c2.b, c2.a,  n.x, n.y, n.z});
    PushVertex(Buffer, {p4.x, p4.y, p4.z, Uv.u,  Uv.v2,  c4.r, c4.g, c4.b, c4.a,  n.x, n.y, n.z});

    PushVertex(Buffer, {p2.x, p2.y, p2.z, Uv.u2, Uv.v,   c2.r, c2.g, c2.b, c2.a,  n.x, n.y, n.z});
    PushVertex(Buffer, {p3.x, p3.y, p3.z, Uv.u2, Uv.v2,  c3.r, c3.g, c3.b, c3.a,  n.x, n.y, n.z});
    PushVertex(Buffer, {p4.x, p4.y, p4.z, Uv.u,  Uv.v2,  c4.r, c4.g, c4.b, c4.a,  n.x, n.y, n.z});
}

inline static vec3
GenerateNormal(vec3 p1, vec3 p2, vec3 p3)
{
    vec3 v1 = p2 - p1;
    vec3 v2 = p3 - p1;
    return glm::normalize(glm::cross(v1, v2));
}

static void
AddTriangle(vertex_buffer &Buffer, vec3 p1, vec3 p2, vec3 p3, color c1 = Color_white)
{
    color c2 = c1;
    color c3 = c1;

    vec3 n = GenerateNormal(p1, p2, p3);
    PushVertex(Buffer, {p1.x, p1.y, p1.z, 0, 0,   c1.r, c1.g, c1.b, c1.a,  n.x, n.y, n.z});
    PushVertex(Buffer, {p2.x, p2.y, p2.z, 0, 0,   c2.r, c2.g, c2.b, c2.a,  n.x, n.y, n.z});
    PushVertex(Buffer, {p3.x, p3.y, p3.z, 0, 0,   c3.r, c3.g, c3.b, c3.a,  n.x, n.y, n.z});
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
    if (Entity->Model)
    {
        Mode.ModelEntities.push_back(Entity);
    }
    if (Entity->Shape)
    {
        Mode.ShapedEntities.push_back(Entity);
    }
}

inline static void
AddFlags(entity *Entity, uint32 Flags)
{
    Entity->Flags |= Flags;
}

static shape *
CreateShape(shape_type Type)
{
    shape *Result = new shape;
    Result->Type = Type;
    return Result;
}

static material *
CreateMaterial(memory_arena &Arena, color Color, float Emission, float TexWeight, texture *Texture)
{
    material *Result = PushStruct<material>(Arena);
    Result->DiffuseColor = Color;
    Result->Emission = Emission;
    Result->TexWeight = TexWeight;
    Result->Texture = Texture;
    return Result;
}

static model *
CreateModel(memory_arena &Arena, mesh *Mesh, material *Material)
{
    model *Result = PushStruct<model>(Arena);
    Result->Mesh = Mesh;
    Result->Material = Material;
    return Result;
}

static void
AddPart(mesh &Mesh, const material &Material, size_t Offset, size_t Count, GLuint PrimitiveType)
{
    Mesh.Parts.push_back({Material, Offset, Count, PrimitiveType});
}

static void
AddSkyboxFace(mesh &Mesh, vec3 p1, vec3 p2, vec3 p3, vec3 p4, texture *Texture, size_t Index)
{
    AddQuad(Mesh.Buffer, p1, p2, p3, p4, Color_white, vec3(0.0f), true);
    AddPart(Mesh, {Color_white, 0, 1, Texture}, Index*6, 6, GL_TRIANGLES);
}

#define SkyboxScale vec3(200.0f)
void StartLevel(game_state &GameState, level_mode &Mode)
{
    GameState.GameMode = GameMode_level;
    FreeArena(Mode.Arena);

    {
        auto &FloorMesh = Mode.FloorMesh;
        InitMeshBuffer(FloorMesh.Buffer);
        AddQuad(FloorMesh.Buffer, vec3(0, 0, 0), vec3(1, 0, 0), vec3(1, 1, 0), vec3(0, 1, 0), Color_white, vec3(0, 0, 1));

        material FloorMaterial = {Color_white, 0, 1, LoadTextureFile(GameState.AssetCache, "data/textures/checker.png")};
        FloorMesh.Parts.resize(1);
        FloorMesh.Parts[0] = {FloorMaterial, 0, FloorMesh.Buffer.VertexCount, GL_TRIANGLES};
        EndMesh(FloorMesh, GL_STATIC_DRAW);
    }

    {
        auto &ShipMesh = Mode.ShipMesh;
        float h = 0.5f;

        InitMeshBuffer(ShipMesh.Buffer);
        AddTriangle(ShipMesh.Buffer, vec3(-1, 0, 0), vec3(0, 0.1f, h), vec3(0, 1, 0.1f));
        AddTriangle(ShipMesh.Buffer, vec3(1, 0, 0),  vec3(0, 1, 0.1f), vec3(0, 0.1f, h));
        AddTriangle(ShipMesh.Buffer, vec3(-1, 0, 0), vec3(0, 0.1f, 0), vec3(0, 0.1f, h));
        AddTriangle(ShipMesh.Buffer, vec3(1, 0, 0), vec3(0, 0.1f, h), vec3(0, 0.1f, 0));

        material ShipMaterial = {Color_white, 0, 0, NULL};
        ShipMesh.Parts.resize(1);
        ShipMesh.Parts[0] = {ShipMaterial, 0, ShipMesh.Buffer.VertexCount, GL_TRIANGLES};
        EndMesh(ShipMesh, GL_STATIC_DRAW);
    }

    {
        auto &SkyboxMesh = Mode.SkyboxMesh;
        InitMeshBuffer(SkyboxMesh.Buffer);

        texture *FrontTexture = LoadTextureFile(GameState.AssetCache, "data/skyboxes/kurt/space_ft.png");
        texture *RightTexture = LoadTextureFile(GameState.AssetCache, "data/skyboxes/kurt/space_rt.png");
        texture *BackTexture = LoadTextureFile(GameState.AssetCache, "data/skyboxes/kurt/space_bk.png");
        texture *LeftTexture = LoadTextureFile(GameState.AssetCache, "data/skyboxes/kurt/space_lf.png");
        texture *TopTexture = LoadTextureFile(GameState.AssetCache, "data/skyboxes/kurt/space_up.png");
        texture *BottomTexture = LoadTextureFile(GameState.AssetCache, "data/skyboxes/kurt/space_dn.png");
        AddSkyboxFace(SkyboxMesh, vec3(-1, 1, -1), vec3(1, 1, -1), vec3(1, 1, 1), vec3(-1, 1, 1), FrontTexture, 0);
        AddSkyboxFace(SkyboxMesh, vec3(1, 1, -1), vec3(1, -1, -1), vec3(1, -1, 1), vec3(1, 1, 1), RightTexture, 1);
        AddSkyboxFace(SkyboxMesh, vec3(1, -1, -1), vec3(-1, -1, -1), vec3(-1, -1, 1), vec3(1, -1, 1), BackTexture, 2);
        AddSkyboxFace(SkyboxMesh, vec3(-1, -1, -1), vec3(-1, 1, -1), vec3(-1, 1, 1), vec3(-1, -1, 1), LeftTexture, 3);
        AddSkyboxFace(SkyboxMesh, vec3(-1, 1, 1), vec3(1, 1, 1), vec3(1, -1, 1), vec3(-1, -1, 1), TopTexture, 4);
        AddSkyboxFace(SkyboxMesh, vec3(-1, -1, -1), vec3(1, -1, -1), vec3(1, 1, -1), vec3(-1, 1, -1), BottomTexture, 5);
        EndMesh(SkyboxMesh, GL_STATIC_DRAW);

        auto Entity = PushStruct<entity>(Mode.Arena);
        Entity->Model = CreateModel(Mode.Arena, &SkyboxMesh, NULL);
        Entity->Size = SkyboxScale;
        AddEntity(Mode, Entity);
        Mode.SkyboxEntity = Entity;
    }

    MakeCameraPerspective(Mode.Camera, (float)GameState.Width, (float)GameState.Height, 70.0f, 0.1f, 1000.0f);
    Mode.Camera.Position = vec3(2, 0, 0);
    Mode.Camera.LookAt = vec3(0, 0, 0);
    Mode.Gravity = vec3(0, 0, 0);

    auto Entity = PushStruct<entity>(Mode.Arena);
    Entity->Model = CreateModel(Mode.Arena, &Mode.ShipMesh, CreateMaterial(Mode.Arena, vec4(0.0f, 0.0f, 1.0f, 0.75f), 0, 0, NULL));
    Entity->Size.y = 3;
    Entity->Shape = CreateShape(Shape_BoundingBox);
    AddEntity(Mode, Entity);

    Mode.PlayerEntity = Entity;

    for (size_t i = 0; i < 10; i++)
    {
        auto &TrackSegment = Mode.Segments[i];
        TrackSegment.Position = vec3(0, i*TrackSegmentLength, 0);
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

inline static bool
HandleCollision(entity *First, entity *Second)
{
    return true;
}

static float
Interp(float c, float t, float a, float dt)
{
    if (c == t)
    {
        return t;
    }
    float dir = std::copysign(1, t - c);
    c += a * dir * dt;
    return (dir == std::copysign(1, t - c)) ? c : t;
}

#ifdef DRAFT_DEBUG
static bool DebugFreeCamEnabled = true;
static float Pitch;
static float Yaw;
#endif

#define PlayerMaxVel  50.0f
#define PlayerAcceleration 20.0f
#define PlayerBreakAcceleration 30.0f
#define PlayerSteerSpeed 10.0f
#define PlayerSteerAcceleration 30.0f
#define CameraOffsetY 5.0f
#define CameraOffsetZ 2.0f
void UpdateAndRenderLevel(game_state &GameState, level_mode &Mode, float DeltaTime)
{
    auto &Input = GameState.Input;
    auto &Camera = Mode.Camera;
    auto CamDir = CameraDir(Camera);

#ifdef DRAFT_DEBUG
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

        CamDir.x = sin(Yaw);
        CamDir.y = cos(Yaw);
        CamDir.z = Pitch;
        Camera.LookAt = Camera.Position + CamDir * 50.0f;

        float StrafeYaw = Yaw + (M_PI / 2);
        float hAxisValue = GetAxisValue(Input, Action_camHorizontal);
        Camera.Position += vec3(sin(StrafeYaw), cos(StrafeYaw), 0) * hAxisValue * Speed * DeltaTime;
    }
    else
#endif
    {
    }

    float MoveH = GetAxisValue(GameState.Input, Action_horizontal);
    float MoveV = GetAxisValue(GameState.Input, Action_vertical);

    if (MoveV > 0.0f && Mode.PlayerEntity->Velocity.y < PlayerMaxVel)
    {
        Mode.PlayerEntity->Velocity.y += MoveV * PlayerAcceleration * DeltaTime;
    }
    else if (MoveV < 0.0f && Mode.PlayerEntity->Velocity.y > 0)
    {
        Mode.PlayerEntity->Velocity.y = std::max(0.0f, Mode.PlayerEntity->Velocity.y + (MoveV * PlayerBreakAcceleration * DeltaTime));
    }

    float SteerTarget = MoveH * PlayerSteerSpeed;
    Mode.PlayerEntity->Velocity.x = Interp(Mode.PlayerEntity->Velocity.x,
                                           SteerTarget,
                                           PlayerSteerAcceleration,
                                           DeltaTime);

    size_t FrameCollisionCount = 0;
    DetectCollisions(Mode.ShapedEntities, Mode.CollisionCache, FrameCollisionCount);
    for (size_t i = 0; i < FrameCollisionCount; i++)
    {
        auto &Col = Mode.CollisionCache[i];
        Col.First->NumCollisions++;
        Col.Second->NumCollisions++;

        if (HandleCollision(Col.First, Col.Second))
        {
            ResolveCollision(Col);
        }
    }
    Integrate(Mode.ShapedEntities, Mode.Gravity, DeltaTime);

    if (!DebugFreeCamEnabled)
    {
        auto PlayerPosition = Mode.PlayerEntity->Position;
        Camera.Position = vec3(PlayerPosition.x,
                               PlayerPosition.y - CameraOffsetY,
                               PlayerPosition.z + CameraOffsetZ);
        Camera.LookAt = Camera.Position + vec3(0, 10, 0);
    }
    Mode.SkyboxEntity->Position.y = Mode.PlayerEntity->Position.y;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    UpdateProjectionView(Mode.Camera);

    for (auto &Segment : Mode.Segments)
    {
        if (Segment.Position.y + TrackSegmentLength < Mode.Camera.Position.y)
        {
            Segment.Position.y += TrackSegmentCount*TrackSegmentLength;
        }

        mat4 TransformMatrix = glm::translate(mat4(1.0f), Segment.Position);
        TransformMatrix = glm::scale(TransformMatrix, vec3(TrackSegmentWidth, TrackSegmentLength, 0));

        model TmpModel = {&Mode.FloorMesh, NULL};
        RenderModel(GameState.RenderState, Mode.Camera, TmpModel, TransformMatrix);
    }

    for (auto Entity : Mode.ModelEntities)
    {
        mat4 TransformMatrix = glm::translate(mat4(1.0f), Entity->Position);
        TransformMatrix = glm::scale(TransformMatrix, Entity->Size);
        RenderModel(GameState.RenderState, Mode.Camera, *Entity->Model, TransformMatrix);
    }

#ifdef DRAFT_DEBUG
    for (size_t i = 0; i < Mode.ShapedEntities.size(); i++)
    {
        auto Entity = Mode.ShapedEntities[i];
        DebugRenderBounds(GameState.RenderState, Mode.Camera, Entity->Shape->BoundingBox, Entity->NumCollisions > 0);
    }
#endif
}
