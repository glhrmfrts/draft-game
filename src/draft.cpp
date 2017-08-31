/*
  Current TODOs:
  - (Renderer) Framebuffers need to support multisampling
  - (Renderer) Fix order of rendering (by creating the renderer buffer & sorting renderables)
  - (Game)     Create the ship's "trail"
 */

#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <AL/alc.h>
#include "imgui.h"
#include "imgui_impl_sdl_gl3.h"
#include "draft.h"
#include "memory.cpp"
#include "collision.cpp"
#include "render.cpp"
#include "asset.cpp"
#include "gui.cpp"
#include "debug_ui.cpp"

#undef main

static void
RegisterInputActions(game_input &Input)
{
    Input.Actions[Action_camHorizontal] = {SDLK_d, SDLK_a, 0, 0};
    Input.Actions[Action_camVertical] = {SDLK_w, SDLK_s, 0, 0};
    Input.Actions[Action_debugFreeCam] = {SDLK_SPACE, 0, 0, 0};
    Input.Actions[Action_horizontal] = {SDLK_RIGHT, SDLK_LEFT, 0, 0};
    Input.Actions[Action_vertical] = {SDLK_UP, SDLK_DOWN, 0, 0};
}

#define CameraOffsetY 4.0f
#define CameraOffsetZ 5.0f

static void
AddLine(vertex_buffer &Buffer, vec3 p1, vec3 p2, color c = Color_white, vec3 n = vec3(0))
{
    PushVertex(Buffer, {p1.x, p1.y, p1.z, 0, 0,  c.r, c.g, c.b, c.a,  n.x, n.y, n.z});
    PushVertex(Buffer, {p2.x, p2.y, p2.z, 0, 0,  c.r, c.g, c.b, c.a,  n.x, n.y, n.z});
}

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
AddEntity(game_state &Game, entity *Entity)
{
    if (Entity->Model)
    {
        Game.ModelEntities.push_back(Entity);
    }
    if (Entity->Bounds)
    {
        Game.ShapedEntities.push_back(Entity);
    }
}

inline static void
AddFlags(entity *Entity, uint32 Flags)
{
    Entity->Flags |= Flags;
}

static material *
CreateMaterial(memory_arena &Arena, color Color, float Emission, float TexWeight, texture *Texture, uint32 Flags = 0)
{
    material *Result = PushStruct<material>(Arena);
    Result->DiffuseColor = Color;
    Result->Emission = Emission;
    Result->TexWeight = TexWeight;
    Result->Texture = Texture;
    Result->Flags = Flags;
    return Result;
}

static model *
CreateModel(memory_arena &Arena, mesh *Mesh)
{
    model *Result = PushStruct<model>(Arena);
    Result->Mesh = Mesh;
    return Result;
}

inline static void
AddPart(mesh &Mesh, const mesh_part &MeshPart)
{
    Mesh.Parts.push_back(MeshPart);
}

static void
AddSkyboxFace(mesh &Mesh, vec3 p1, vec3 p2, vec3 p3, vec3 p4, texture *Texture, size_t Index)
{
    AddQuad(Mesh.Buffer, p1, p2, p3, p4, Color_white, vec3(1.0f), true);
    AddPart(Mesh, mesh_part{material{Color_white, 0, 1, Texture}, Index*6, 6, GL_TRIANGLES});
}

static entity *
CreateShipEntity(game_state &Game, color Color, color OutlineColor)
{
    auto Entity = PushStruct<entity>(Game.Arena);
    Entity->Model = CreateModel(Game.Arena, &Game.ShipMesh);
    Entity->Model->Materials.push_back(CreateMaterial(Game.Arena, vec4(Color.r, Color.g, Color.b, 1), 1, 0, NULL));
    Entity->Model->Materials.push_back(CreateMaterial(Game.Arena, OutlineColor, 1, 0, NULL, Material_PolygonLines));
    Entity->Size.y = 3;
    Entity->Bounds = PushStruct<bounding_box>(Game.Arena);
    AddEntity(Game, Entity);
    return Entity;
}

#define SkyboxScale vec3(500.0f)
static void
StartLevel(game_state &Game)
{
    FreeArena(Game.Arena);

    {
        auto &FloorMesh = Game.FloorMesh;
        InitMeshBuffer(FloorMesh.Buffer);

        material FloorMaterial = {IntColor(FirstPalette.Colors[2], 0.25f), 0, 0, NULL};
        material LaneMaterial = {IntColor(FirstPalette.Colors[3]), 0.5f, 0, NULL};

        float w = TrackSegmentWidth * TrackLaneWidth;
        float l = -w/2;
        float r = w/2;
        AddQuad(FloorMesh.Buffer, vec3(l, 0, 0), vec3(r, 0, 0), vec3(r, 1, 0), vec3(l, 1, 0), Color_white, vec3(0, 0, 1));
        AddPart(FloorMesh, {FloorMaterial, 0, FloorMesh.Buffer.VertexCount, GL_TRIANGLES});

        size_t LineVertexCount = 0;
        for (int i = 1; i < TrackSegmentWidth; i++)
        {
            AddLine(FloorMesh.Buffer, vec3(l+i*TrackLaneWidth, 0, 0), vec3(l+i*TrackLaneWidth, 1, 0), Color_white, vec3(1.0f));
            LineVertexCount += 2;
        }
        AddPart(FloorMesh, {LaneMaterial, 6, LineVertexCount, GL_LINES});

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
        Entity->Size = SkyboxScale;
        AddEntity(Game, Entity);
        Game.SkyboxEntity = Entity;
    }

    MakeCameraPerspective(Game.Camera, (float)Game.Width, (float)Game.Height, 70.0f, 0.1f, 1000.0f);
    Game.Camera.Position = vec3(2, 0, 0);
    Game.Camera.LookAt = vec3(0, 0, 0);
    Game.Gravity = vec3(0, 0, 0);

    Game.EnemyEntity = CreateShipEntity(Game, IntColor(SecondPalette.Colors[2]), IntColor(SecondPalette.Colors[3]));
    Game.PlayerEntity = CreateShipEntity(Game, Color_blue, IntColor(FirstPalette.Colors[1]));
    Game.PlayerEntity->Rotation.y = 20.0f;

    Game.EnemyEntity->Position.x = 4;

    for (size_t i = 0; i < TrackSegmentCount; i++)
    {
        auto &TrackSegment = Game.Segments[i];
        TrackSegment.Position = vec3(0, i*TrackSegmentLength + TrackSegmentPadding*i, -0.25f);
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

#define ShipMaxVel  50.0f
#define ShipAcceleration 20.0f
#define ShipBreakAcceleration 30.0f
#define ShipSteerSpeed 10.0f
#define ShipSteerAcceleration 50.0f
#define ShipFriction 2.0f
#define CameraOffsetY 5.0f
#define CameraOffsetZ 2.0f
static void
MoveShipEntity(entity *Entity, float MoveH, float MoveV, float DeltaTime)
{
    if (MoveV > 0.0f && Entity->Velocity.y < ShipMaxVel)
    {
        Entity->Velocity.y += MoveV * ShipAcceleration * DeltaTime;
    }
    else if (MoveV < 0.0f && Entity->Velocity.y > 0)
    {
        Entity->Velocity.y = std::max(0.0f, Entity->Velocity.y + (MoveV * ShipBreakAcceleration * DeltaTime));
    }

    if (MoveV <= 0.0f)
    {
        Entity->Velocity.y -= ShipFriction * DeltaTime;
    }
    Entity->Velocity.y = std::max(0.0f, std::min(ShipMaxVel, Entity->Velocity.y));

    float SteerTarget = MoveH * ShipSteerSpeed;
    Entity->Velocity.x = Interp(Entity->Velocity.x,
                                SteerTarget,
                                ShipSteerAcceleration,
                                DeltaTime);

    Entity->Rotation.y = 20.0f * (Entity->Velocity.x / ShipSteerSpeed);
    Entity->Rotation.x = Interp(Entity->Rotation.x,
                                5.0f * MoveV,
                                20.0f,
                                DeltaTime);
}

static void
UpdateAndRenderLevel(game_state &Game, float DeltaTime)
{
    auto &Input = Game.Input;
    auto &Camera = Game.Camera;
    auto CamDir = CameraDir(Camera);

#ifdef DRAFT_DEBUG
    if (Global_Camera_FreeCam)
    {
        static float Pitch;
        static float Yaw;
        float Speed = 20.0f;
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
#endif

    float MoveH = GetAxisValue(Game.Input, Action_horizontal);
    float MoveV = GetAxisValue(Game.Input, Action_vertical);
    MoveShipEntity(Game.PlayerEntity, MoveH, MoveV, DeltaTime);
    MoveShipEntity(Game.EnemyEntity, 0, 0.4f, DeltaTime);

    size_t FrameCollisionCount = 0;
    DetectCollisions(Game.ShapedEntities, Game.CollisionCache, FrameCollisionCount);
    for (size_t i = 0; i < FrameCollisionCount; i++)
    {
        auto &Col = Game.CollisionCache[i];
        Col.First->NumCollisions++;
        Col.Second->NumCollisions++;

        if (HandleCollision(Col.First, Col.Second))
        {
            ResolveCollision(Col);
        }
    }
    Integrate(Game.ShapedEntities, Game.Gravity, DeltaTime);

    if (!Global_Camera_FreeCam)
    {
        auto PlayerPosition = Game.PlayerEntity->Position;
        Camera.Position = vec3(PlayerPosition.x,
                               PlayerPosition.y - CameraOffsetY,
                               PlayerPosition.z + CameraOffsetZ);
        Camera.LookAt = Camera.Position + vec3(0, 10, 0);
    }
    {
        Game.SkyboxEntity->Position.y = Game.PlayerEntity->Position.y;
        float dx = Game.PlayerEntity->Position.x - Game.SkyboxEntity->Position.x;

        Game.SkyboxEntity->Position.x += dx * 0.25f;
    }

    UpdateProjectionView(Game.Camera);
    RenderBegin(Game.RenderState);

    for (auto &Segment : Game.Segments)
    {
        size_t i = &Segment - &Game.Segments[0];
        if (Segment.Position.y + TrackSegmentLength+TrackSegmentPadding < Game.Camera.Position.y)
        {
            Segment.Position.y += TrackSegmentCount*TrackSegmentLength + (TrackSegmentPadding*TrackSegmentCount);
        }

        model TmpModel = {{}, &Game.FloorMesh};
        PushModel(Game.RenderState, TmpModel, Segment.Position, vec3(1, TrackSegmentLength, 0), vec3(0.0f));
    }

    for (auto Entity : Game.ModelEntities)
    {
        PushModel(Game.RenderState, *Entity->Model, Entity->Position, Entity->Size, Entity->Rotation);
    }

#ifdef DRAFT_DEBUG
    if (Global_Collision_DrawBounds)
    {
        Println(Global_Collision_DrawBounds);
        for (size_t i = 0; i < Game.ShapedEntities.size(); i++)
        {
            auto Entity = Game.ShapedEntities[i];
            PushDebugBounds(Game.RenderState, *Entity->Bounds, Entity->NumCollisions > 0);
        }
    }
#endif

    RenderEnd(Game.RenderState, Game.Camera);
}

int main(int argc, char **argv)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cout << "Failed to init display SDL" << std::endl;
    }

    int Width = 1280;
    int Height = 720;
    SDL_Window *Window = SDL_CreateWindow("RGB", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          Width, Height, SDL_WINDOW_OPENGL);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

    SDL_GLContext Context = SDL_GL_CreateContext(Window);

    SDL_GL_SetSwapInterval(0);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cout << "Error loading GL extensions" << std::endl;
        exit(EXIT_FAILURE);
    }

    glEnable(GL_MULTISAMPLE);
    glViewport(0, 0, Width, Height);

    ALCdevice *AudioDevice = alcOpenDevice(NULL);
    if (!AudioDevice)
        std::cerr << "Error opening audio device" << std::endl;

    ALCcontext *AudioContext = alcCreateContext(AudioDevice, NULL);
    if (!alcMakeContextCurrent(AudioContext))
        std::cerr << "Error setting audio context" << std::endl;

    ImGui_ImplSdlGL3_Init(Window);

    game_state Game;
    Game.Width = Width;
    Game.Height = Height;

    auto &Input = Game.Input;
    RegisterInputActions(Input);
    InitGUI(Game.GUI, Input);
    MakeCameraOrthographic(Game.GUICamera, 0, Width, 0, Height, -1, 1);
    InitRenderState(Game.RenderState, Width, Height);
    StartLevel(Game);

    ImVec4 clear_color = ImColor(114, 144, 154);
    bool show_test_window = true;
    bool show_another_window = false;

    clock_t PreviousTime = clock();
    float DeltaTime = 0.016f;
    float DeltaTimeMS = DeltaTime * 1000.0f;
    while (Game.Running)
    {
        clock_t CurrentTime = clock();
        float Elapsed = ((CurrentTime - PreviousTime) / (float)CLOCKS_PER_SEC);

        SDL_Event Event;
        while (SDL_PollEvent(&Event))
        {
            ImGui_ImplSdlGL3_ProcessEvent(&Event);
            switch (Event.type) {
            case SDL_QUIT:
                Game.Running = false;
                break;

            case SDL_KEYDOWN: {
                SDL_Keycode Key = Event.key.keysym.sym;
                for (int i = 0; i < Action_count; i++) {
                    auto &Action = Input.Actions[i];
                    if (Action.Positive == Key || Action.Negative == Key) {
                        Action.Pressed++;

                        if (Action.Positive == Key) {
                            Action.AxisValue = 1;
                        }
                        else if (Action.Negative == Key) {
                            Action.AxisValue = -1;
                        }
                    }
                }

                if (Key == SDLK_BACKQUOTE)
                {
                    Global_DebugUI = !Global_DebugUI;
                }
                break;
            }

            case SDL_KEYUP: {
                SDL_Keycode Key = Event.key.keysym.sym;
                for (int i = 0; i < Action_count; i++) {
                    auto &Action = Input.Actions[i];
                    if (Action.Positive == Key && Action.AxisValue == 1) {
                        Action.Pressed = 0;
                        Action.AxisValue = 0;
                        break;
                    }
                    else if (Action.Negative == Key && Action.AxisValue == -1) {
                        Action.Pressed = 0;
                        Action.AxisValue = 0;
                    }
                }
                break;
            }

            case SDL_MOUSEMOTION: {
                auto &Motion = Event.motion;
                Input.MouseState.X = Motion.x;
                Input.MouseState.Y = Motion.y;
                Input.MouseState.dX = Motion.xrel;
                Input.MouseState.dY = Motion.yrel;
                break;
            }

            case SDL_MOUSEWHEEL: {
                auto &Wheel = Event.wheel;
                Input.MouseState.ScrollY = Wheel.y;
                break;
            }

            case SDL_MOUSEBUTTONDOWN: {
                auto &Button = Event.button;
                switch (Button.button) {
                case SDL_BUTTON_LEFT:
                    Input.MouseState.Buttons |= MouseButton_left;
                    break;

                case SDL_BUTTON_MIDDLE:
                    Input.MouseState.Buttons |= MouseButton_middle;
                    break;

                case SDL_BUTTON_RIGHT:
                    Input.MouseState.Buttons |= MouseButton_right;
                    break;
                }
                break;
            }

            case SDL_MOUSEBUTTONUP: {
                auto &Button = Event.button;
                switch (Button.button) {
                case SDL_BUTTON_LEFT:
                    Input.MouseState.Buttons &= ~MouseButton_left;
                    break;

                case SDL_BUTTON_MIDDLE:
                    Input.MouseState.Buttons &= ~MouseButton_middle;
                    break;

                case SDL_BUTTON_RIGHT:
                    Input.MouseState.Buttons &= ~MouseButton_right;
                    break;
                }
                break;
            }
            }
        }

        ImGui_ImplSdlGL3_NewFrame(Window);
        UpdateAndRenderLevel(Game, DeltaTime);

        DrawDebugUI(Elapsed);

#if 0
        // 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
        if (show_test_window)
        {
            ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver);
            ImGui::ShowTestWindow(&show_test_window);
        }
#endif

        ImGui::Render();

        memcpy(&Game.PrevInput, &Game.Input, sizeof(game_input));
        Input.MouseState.dX = 0;
        Input.MouseState.dY = 0;

        SDL_GL_SwapWindow(Window);

        if (Elapsed*1000.0f < DeltaTimeMS)
        {
            SDL_Delay(DeltaTimeMS - Elapsed*1000.0f);
        }

        PreviousTime = CurrentTime;
    }

    ImGui_ImplSdlGL3_Shutdown();

    FreeArena(Game.Arena);
    FreeArena(Game.AssetCache.Arena);

    alcMakeContextCurrent(NULL);
    alcDestroyContext(AudioContext);
    alcCloseDevice(AudioDevice);

    SDL_GL_DeleteContext(Context);
    SDL_DestroyWindow(Window);
    SDL_Quit();

    return 0;
}
