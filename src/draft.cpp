// Copyright

#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include "imgui.h"
#include "imgui_impl_sdl_gl3.h"
#include "draft.h"
#include "memory.cpp"
#include "collision.cpp"
#include "render.cpp"
#include "asset.cpp"
#include "gui.cpp"
#include "debug_ui.cpp"
#include "entity.cpp"

#undef main

// Finds the first free slot on the list and insert the entity
inline static void
AddEntityToList(std::list<entity *> &List, entity *Entity)
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

static void
AddEntity(game_state &Game, entity *Entity)
{
    if (Entity->Model)
    {
        AddEntityToList(Game.ModelEntities, Entity);
    }
    if (Entity->Bounds)
    {
        AddEntityToList(Game.ShapedEntities, Entity);
    }
    if (Entity->TrackSegment)
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
        AddEntityToList(Game.ExplosionEntities, Entity);
    }
    if (Entity->Ship && !(Entity->Flags & Entity_IsPlayer))
    {
        AddEntityToList(Game.ShipEntities, Entity);
    }
}

inline static void
RemoveEntityFromList(std::list<entity *> &List, entity *Entity)
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
    if (Entity->TrackSegment)
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
}

#define TrackSegmentLength 20
#define TrackSegmentWidth  5
#define TrackLaneWidth     2.5f
#define TrackSegmentCount  20
#define TrackSegmentPadding 0.5f
#define SkyboxScale vec3(500.0f)
static void
StartLevel(game_state &Game)
{
    FreeArena(Game.Arena);

    {
        auto &FloorMesh = Game.FloorMesh;
        InitMeshBuffer(FloorMesh.Buffer);

        material FloorMaterial = {IntColor(FirstPalette.Colors[0], 1), 0, 1, LoadTextureFile(Game.AssetCache, "data/textures/checker.png")};
        material LaneMaterial = {Color_white, 0.1f, 0, NULL};

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
        //AddPart(FloorMesh, {LaneMaterial, 6, LineVertexCount, GL_LINES});

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

    MakeCameraPerspective(Game.Camera, (float)Game.Width, (float)Game.Height, 70.0f, 0.1f, 1000.0f);
    Game.Camera.Position = vec3(2, 0, 0);
    Game.Camera.LookAt = vec3(0, 0, 0);
    Game.Gravity = vec3(0, 0, 0);

    for (int i = 0; i < 3; i++)
    {
        auto *EnemyEntity = CreateShipEntity(Game, IntColor(SecondPalette.Colors[0]), IntColor(SecondPalette.Colors[3]));
        EnemyEntity->Transform.Position.x = i + 1;
        AddEntity(Game, EnemyEntity);
    }

    Game.PlayerEntity = CreateShipEntity(Game, Color_blue, IntColor(FirstPalette.Colors[1]), true);
    Game.PlayerEntity->Transform.Rotation.y = 20.0f;
    AddEntity(Game, Game.PlayerEntity);

    for (int i = 0; i < TrackSegmentCount; i++)
    {
        auto *Entity = PushStruct<entity>(Game.Arena);
        Entity->TrackSegment = PushStruct<track_segment>(Game.Arena);
        Entity->Transform.Position = vec3(0, i*TrackSegmentLength + TrackSegmentPadding*i, -0.25f);
        Entity->Transform.Scale = vec3(1, TrackSegmentLength, 0);
        Entity->Model = CreateModel(Game.Arena, &Game.FloorMesh);
        AddEntity(Game, Entity);
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

static bool
HandleCollision(game_state &Game, entity *First, entity *Second, float DeltaTime)
{
    if (First->Type == EntityType_TrailPiece || Second->Type == EntityType_TrailPiece)
    {
        ship *Ship = NULL;
        if (First->Ship)
        {
            Ship = First->Ship;
        }
        else if (Second->Ship)
        {
            Ship = Second->Ship;
        }

        if (Ship && Ship->NumTrailCollisions == 0)
        {
            Ship->CurrentDraftTime += DeltaTime;
            Ship->NumTrailCollisions++;
        }
        return false;
    }
    if (First->Type == EntityType_Ship && Second->Type == EntityType_Ship)
    {
        vec3 rv = First->Transform.Velocity - Second->Transform.Velocity;
        if (std::abs(rv.y) > 20.0f)
        {
            entity *EntityToExplode = NULL;
            if (rv.y < 0.0f)
            {
                EntityToExplode = First;
            }
            if (rv.y > 0.0f)
            {
                EntityToExplode = Second;
            }

            auto *Explosion = CreateExplosionEntity(Game,
                                                    EntityToExplode->Transform,
                                                    EntityToExplode->Ship->Color,
                                                    EntityToExplode->Ship->OutlineColor,
                                                    vec3{0,1,0});
            AddEntity(Game, Explosion);
            //RemoveEntity(Game, EntityToExplode);
            return false;
        }
    }
    return true;
}

static void
UpdateAndRenderLevel(game_state &Game, float DeltaTime)
{
    auto &Input = Game.Input;
    auto &Camera = Game.Camera;
    auto *PlayerShip = Game.PlayerEntity->Ship;
    vec3 CamDir = CameraDir(Camera);

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
#endif

    float MoveH = GetAxisValue(Game.Input, Action_horizontal);
    float MoveV = GetAxisValue(Game.Input, Action_vertical);
    //Println(MoveV);
    //MoveV = 0.3f;
    MoveShipEntity(Game.PlayerEntity, MoveH, MoveV, DeltaTime);

    if (PlayerShip->DraftCharge == 1.0f)
    {
        if (IsJustPressed(Game, Action_boost))
        {
            Game.PlayerEntity->Transform.Velocity.y = Global_Game_BoostVelocity;
            PlayerShip->DraftCharge = 0.0f;
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

    if (!Global_Camera_FreeCam)
    {
        auto PlayerPosition = Game.PlayerEntity->Transform.Position;
        Camera.Position = vec3(PlayerPosition.x,
                               PlayerPosition.y + Global_Camera_OffsetY,
                               PlayerPosition.z + Global_Camera_OffsetZ);
        Camera.LookAt = Camera.Position + vec3(0, 10, 0);
    }

    {
        auto Entity = Game.PlayerEntity;
        Game.SkyboxEntity->Transform.Position.y = Entity->Transform.Position.y;
        float dx = Entity->Transform.Position.x - Game.SkyboxEntity->Transform.Position.x;
        Game.SkyboxEntity->Transform.Position.x += dx * 0.25f;
    }

    UpdateProjectionView(Game.Camera);
    RenderBegin(Game.RenderState);

    static float EnemyMoveH = 0.0f;
    for (auto *Entity : Game.ShipEntities)
    {
        if (!Entity) continue;

        MoveShipEntity(Entity, sin(EnemyMoveH), 0.4f, DeltaTime);
    }

    for (auto *Entity : Game.TrailEntities)
    {
        if (!Entity) continue;

        auto *Trail = Entity->Trail;
        Trail->Timer += DeltaTime;

        if (Trail->FirstFrame)
        {
            Trail->FirstFrame = false;
            for (int i = 0; i < TrailCount; i++)
            {
                Trail->Entities[i].Transform.Position = Entity->Transform.Position;
            }
        }

        if (Trail->Timer > Global_Game_TrailRecordTimer)
        {
            Trail->Timer = 0;
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
            const float min2 =  0.4f * ((TrailCount - i) / (float)TrailCount);
            const float min1 =  0.4f * ((TrailCount - i+1) / (float)TrailCount);
            vec3 p1 = c2 - vec3(r - min2, 0, 0);
            vec3 p2 = c2 + vec3(r - min2, 0, 0);
            vec3 p3 = c1 - vec3(r - min1, 0, 0);
            vec3 p4 = c1 + vec3(r - min1, 0, 0);
            AddQuad(Mesh.Buffer, p2, p1, p3, p4);

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
        ResetBuffer(Explosion->Mesh.Buffer);
        for (int i = 0; i < ExplosionPieceCount; i++)
        {
            auto &Piece = Explosion->Pieces[i];
            Piece.Position += Piece.Velocity * DeltaTime;
            mat4 Transform = GetTransformMatrix(Piece);
            vec3 p1 = vec3(Transform * vec4(-1.0f, 0.0f, -1.0f, 1.0f));
            vec3 p2 = vec3(Transform * vec4(1.0f, 0.0f, -1.0f, 1.0f));
            vec3 p3 = vec3(Transform * vec4(0.0f, 0.0f, 1.0f, 1.0f));
            AddTriangle(Explosion->Mesh.Buffer, p1, p2, p3, vec3{1,1,1});
        }
        UploadVertices(Explosion->Mesh.Buffer, GL_DYNAMIC_DRAW);
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
    Input.Actions[Action_camHorizontal] = {SDLK_d, SDLK_a, 0, 0};
    Input.Actions[Action_camVertical] = {SDLK_w, SDLK_s, 0, 0};
    Input.Actions[Action_horizontal] = {SDLK_RIGHT, SDLK_LEFT, 0, 0, Axis_LeftX};
    Input.Actions[Action_vertical] = {SDLK_UP, SDLK_DOWN, 0, 0, Axis_RightTrigger};
    Input.Actions[Action_boost] = {SDLK_SPACE, 0, 0, 0, Axis_Invalid, XboxButton_X};
}

static void
OpenGameController(game_input &Input)
{
    Input.Controller.Joystick = SDL_JoystickOpen(0);
    if (Input.Controller.Joystick)
    {
        printf("Joystick name: %s\n", SDL_JoystickNameForIndex(0));
    }
    else
    {
        fprintf(stderr, "Could not open joystick %i: %s\n", 0, SDL_GetError());
    }
}

#define GameControllerAxisDeadzone 5000
static void
ProcessAxisEvent(game_input &Input, SDL_JoyAxisEvent &Event)
{
    int Value = Event.value;
    if (Event.axis == Axis_RightTrigger || Event.axis == Axis_LeftTrigger)
    {
        float fv = (Value + 32768) / float(65535);
        Value = short(fv * 32767);
    }
    if (std::abs(Value) < GameControllerAxisDeadzone)
    {
        Value = 0;
    }
    for (int i = 0; i < Action_count; i++)
    {
        auto &Action = Input.Actions[i];
        if (Action.AxisID == Event.axis)
        {
            Action.AxisValue = Value / float(32767);
            break;
        }
    }
}

static void
ProcessButtonEvent(game_input &Input, SDL_JoyButtonEvent &Event, bool Down)
{
    for (int i = 0; i < Action_count; i++)
    {
        auto &Action = Input.Actions[i];
        if (Action.ButtonID == Event.button)
        {
            if (Down)
            {
                Action.Pressed++;
            }
            else
            {
                Action.Pressed = 0;
            }
            break;
        }
    }
}

int main(int argc, char **argv)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0)
    {
        std::cout << "Failed to init display SDL" << std::endl;
    }

    int Width = 1280;
    int Height = 720;
    SDL_Window *Window = SDL_CreateWindow("Draft", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
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

    printf("%s\n", glGetString(GL_VERSION));
    printf("%s\n", glGetString(GL_VENDOR));
    printf("%s\n", glGetString(GL_RENDERER));

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
    Game.ExplosionEntropy = RandomSeed(1234);

    auto &Input = Game.Input;
    RegisterInputActions(Input);
    if (SDL_NumJoysticks() > 0)
    {
        OpenGameController(Input);
    }

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
                for (int i = 0; i < Action_count; i++)
                {
                    auto &Action = Input.Actions[i];
                    if (Action.Positive == Key || Action.Negative == Key)
                    {
                        Action.Pressed++;

                        if (Action.Positive == Key)
                        {
                            Action.AxisValue = 1;
                        }
                        else if (Action.Negative == Key)
                        {
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
                for (int i = 0; i < Action_count; i++)
                {
                    auto &Action = Input.Actions[i];
                    if (Action.Positive == Key && Action.AxisValue == 1)
                    {
                        Action.Pressed = 0;
                        Action.AxisValue = 0;
                        break;
                    }
                    else if (Action.Negative == Key && Action.AxisValue == -1)
                    {
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

            case SDL_JOYDEVICEADDED: {
                OpenGameController(Input);
                break;
            }

            case SDL_JOYDEVICEREMOVED: {
                SDL_JoystickClose(Input.Controller.Joystick);
                Input.Controller.Joystick = NULL;
                printf("Controller removed\n");
                break;
            }

            case SDL_JOYAXISMOTION: {
                ProcessAxisEvent(Input, Event.jaxis);
                break;
            }

            case SDL_JOYBUTTONDOWN: {
                ProcessButtonEvent(Input, Event.jbutton, true);
                break;
            }

            case SDL_JOYBUTTONUP: {
                ProcessButtonEvent(Input, Event.jbutton, false);
                break;
            }
            }
        }

        ImGui_ImplSdlGL3_NewFrame(Window);
        UpdateAndRenderLevel(Game, DeltaTime);

        DrawDebugUI(Game.PlayerEntity, Elapsed);

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
