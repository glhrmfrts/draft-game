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
#include "meshes.cpp"
#include "init.cpp"

entity *CreateEntity(level *Level, entity_type Type)
{
    auto *Result = PushStruct<entity>(Level->Arena);
    Result->Type = Type;
    Result->ID = Level->NextEntityID++;
    return Result;
}

void AddChild(entity *Entity, entity *Child)
{
    if (Entity->FirstChild)
    {
        auto *Prev = Entity->FirstChild;
        Entity->FirstChild = Child;
        Child->NextSibling = Prev;
    }
    else
    {
        Entity->FirstChild = Child;
    }
}

#define EDITOR_MAX_LINES 16

void InitEditor(game_state &Game)
{
    Game.Mode = GameMode_Editor;
    Game.EditorState = PushStruct<editor_state>(Game.Arena);
    auto Editor = Game.EditorState;
    Editor->Level = PushStruct<level>(Editor->Arena);
    Editor->Level->RootEntity = CreateEntity(Editor->Level, EntityType_Empty);
    Editor->SelectedEntity = Editor->Level->RootEntity;

    Editor->Name = (char *)PushSize(Editor->Arena, 128, "editor name");
    Editor->Filename = (char *)PushSize(Editor->Arena, 128, "editor filename");

    *Editor->Name = 0;
    *Editor->Filename = 0;
    Editor->Mode = EditorMode_None;

    Game.Camera.Position = vec3{0, 0, 10};
    Game.Camera.LookAt = vec3{0, 10, 0};

    {
        auto &CursorMesh = Editor->CursorMesh;
        InitMeshBuffer(CursorMesh.Buffer);

        float w = 1.0f;
        float l = -w/2;
        float r = w/2;
        AddQuad(CursorMesh.Buffer, vec3(l, l, 0), vec3(r, l, 0), vec3(r, r, 0), vec3(l, r, 0), Color_white, vec3(1, 1, 1));
        AddPart(&CursorMesh, {BlankMaterial, 0, CursorMesh.Buffer.VertexCount, GL_TRIANGLES});
        EndMesh(&CursorMesh, GL_STATIC_DRAW);
    }
    {
        auto &LineMesh = Editor->LineMesh;
        InitMeshBuffer(LineMesh.Buffer);
        AddPart(&LineMesh, {BlankMaterial, 0, 0, GL_LINES});
        ReserveVertices(LineMesh.Buffer, EDITOR_MAX_LINES*2, GL_DYNAMIC_DRAW);
    }
}

inline static vec3
CameraDir(camera &Camera)
{
    return glm::normalize(Camera.LookAt - Camera.Position);
}

static void UpdateFreeCam(camera &Camera, game_input &Input, float DeltaTime)
{
    static float Pitch;
    static float Yaw;
    float Speed = 50.0f;
    float AxisValue = GetAxisValue(Input, Action_camVertical);
    vec3 CamDir = CameraDir(Camera);

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

static vec3 Unproject(camera &Camera, vec3 ScreenCoords, float vx, float vy, float vw, float vh)
{
    float x = ScreenCoords.x;
    float y = ScreenCoords.y;
    x -= vx;
    y -= vy;
    ScreenCoords.x = x/vw * 2 - 1;
    ScreenCoords.y = y/vh * 2 - 1;
    ScreenCoords.z = 2 * ScreenCoords.z - 1;

    vec4 v4 = vec4{ScreenCoords.x, ScreenCoords.y, ScreenCoords.z, 1.0f};
    v4 = glm::inverse(Camera.ProjectionView) * v4;
    v4.w = 1.0f / v4.w;
    return vec3{v4.x*v4.w, v4.y*v4.w, v4.z*v4.w};
}

static ray PickRay(camera &Camera, float sx, float sy, float vx, float vy, float vw, float vh)
{
    ray Ray = ray{vec3{sx, sy, 0}, vec3{sx, sy, 1}};
    Ray.Origin = Unproject(Camera, Ray.Origin, vx, vy, vw, vh);
    Ray.Direction = Unproject(Camera, Ray.Direction, vx, vy, vw, vh);
    Ray.Direction = glm::normalize(Ray.Direction - Ray.Origin);
    return Ray;
}

static vec3 GetCursorPositionOnFloor(ray Ray)
{
    if (Ray.Direction.z > 0.0f)
    {
        return vec3(0.0f);
    }

    vec3 Pos = Ray.Origin;
    for (;;)
    {
        Pos += Ray.Direction;
        if (Pos.z < 0.0f)
        {
            float RemZ = -Pos.z;
            float Rel = RemZ / Ray.Direction.z;
            Pos += Ray.Direction * Rel;
            return Pos;
        }
    }
}

static void EditorCommit(editor_state *Editor)
{
    switch (Editor->Mode)
    {
    case EditorMode_Collision:
    {
        auto *Entity = CreateEntity(Editor->Level, EntityType_Collision);
        Entity->Shape = CreateCollisionShape(Editor->Arena, CollisionShapeType_Polygon);
        for (auto &Vert : Editor->LinePoints)
        {
            Entity->Shape->Polygon.Vertices.push_back(vec2{Vert.x, Vert.y});
        }
        Editor->LinePoints.clear();
        AddChild(Editor->SelectedEntity, Entity);
        break;
    }
    }
}

static entity *DrawEntityTree(editor_state *Editor, entity *Entity)
{
    ImGuiTreeNodeFlags Flags = ImGuiTreeNodeFlags_OpenOnArrow;
    if (Editor->SelectedEntity == Entity)
    {
        Flags |= ImGuiTreeNodeFlags_Selected;
    }

    bool NodeOpen = ImGui::TreeNodeEx((void*)(intptr_t)Entity->ID, Flags, "%s %d", EntityTypeStrings[Entity->Type], Entity->ID);
    if (ImGui::IsItemClicked())
    {
        Editor->SelectedEntity = Entity;
    }
    if (NodeOpen)
    {
        auto *Child = Entity->FirstChild;
        while (Child)
        {
            Child = DrawEntityTree(Editor, Child);
        }
        ImGui::TreePop();
    }
    return Entity->NextSibling;
}

static entity *RenderEntity(render_state &RenderState, entity *Entity)
{
    switch (Entity->Type)
    {
    case EntityType_Collision:
        DrawDebugShape(RenderState, Entity->Shape);
        break;
    }
    auto *Child = Entity->FirstChild;
    while (Child)
    {
        Child = RenderEntity(RenderState, Child);
    }
    return Entity->NextSibling;
}

void RenderEditor(game_state &Game, float DeltaTime)
{
    auto *Editor = Game.EditorState;
    auto *FloorMesh = GetFloorMesh(Game);

    UpdateFreeCam(Game.Camera, Game.Input, DeltaTime);
    UpdateProjectionView(Game.Camera);

    RenderBegin(Game.RenderState, DeltaTime);

    transform Transform;
    Transform.Scale = vec3{LEVEL_PLANE_SIZE, LEVEL_PLANE_SIZE, 1};
    DrawMeshPart(Game.RenderState, *FloorMesh, FloorMesh->Parts[0], Transform);
    if (Editor->Mode != EditorMode_None)
    {
        float MouseX = float(Game.Input.MouseState.X);
        float MouseY = float(Game.Height - Game.Input.MouseState.Y);
        ray Ray = PickRay(Game.Camera, MouseX, MouseY, 0, 0, Game.Width, Game.Height);
        vec3 CursorPos = GetCursorPositionOnFloor(Ray);
        CursorPos.z = 0.25f;
        if (Editor->SnapToInteger)
        {
            CursorPos.x = std::floor(CursorPos.x);
            CursorPos.y = std::floor(CursorPos.y);
        }

        transform Transform;
        Transform.Position = CursorPos;
        Transform.Scale *= 0.5f;
        DrawMeshPart(Game.RenderState, Editor->CursorMesh, Editor->CursorMesh.Parts[0], Transform);

        if (IsJustPressed(Game, MouseButton_Left) && Editor->EditingLines)
        {
            Editor->LinePoints.push_back(CursorPos);
            if (Editor->Mode == EditorMode_Collision && Editor->LinePoints.size() == 3)
            {
                EditorCommit(Editor);
            }
        }
        else if (IsJustPressed(Game, MouseButton_Right))
        {
            Editor->EditingLines = false;
        }

        auto &LineBuffer = Editor->LineMesh.Buffer;
        ResetBuffer(LineBuffer, 0);

        size_t PointsCount = Editor->LinePoints.size();
        for (size_t i = 0; i < PointsCount; i++)
        {
            vec3 Pos = Editor->LinePoints[i];
            vec3 NextPos;
            if (i == PointsCount - 1)
            {
                if (Editor->EditingLines)
                {
                    NextPos = CursorPos;
                }
                else
                {
                    break;
                }
            }
            else
            {
                NextPos = Editor->LinePoints[i+1];
            }
            PushVertex(LineBuffer, mesh_vertex{Pos, vec2{0,0}, Color_white, vec3{1,1,1}});
            PushVertex(LineBuffer, mesh_vertex{NextPos, vec2{0,0}, Color_white, vec3{1,1,1}});
        }
        UploadVertices(LineBuffer, 0, LineBuffer.VertexCount);

        auto &Part = Editor->LineMesh.Parts[0];
        Part.Count = LineBuffer.VertexCount;
        DrawMeshPart(Game.RenderState, Editor->LineMesh, Part, transform{});
    }

    {
        auto *Entity = Editor->Level->RootEntity;
        while (Entity)
        {
            Entity = RenderEntity(Game.RenderState, Entity);
        }
    }

    RenderEnd(Game.RenderState, Game.Camera);

    ImGui::SetNextWindowSize(ImVec2(Game.RealWidth*0.25f, Game.RealHeight*1.0f), ImGuiSetCond_Always);
    ImGui::Begin("Editor Main");
    ImGui::InputText("name", Editor->Name, 128);
    ImGui::InputText("filename", Editor->Filename, 128);
    ImGui::Spacing();
    if (ImGui::Button("Save"))
    {
        Println("Save");
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Current Mode: %s", EditorModeStrings[Editor->Mode]);
    ImGui::Checkbox("Snap to integer", &Editor->SnapToInteger);
    if (Editor->Mode != EditorMode_None)
    {
        if (ImGui::Button("Commit"))
        {
            EditorCommit(Editor);
            Editor->Mode = EditorMode_None;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            Editor->Mode = EditorMode_None;
            Editor->LinePoints.clear();
            Editor->EditingLines = false;
        }
    }

    if (ImGui::CollapsingHeader("Add"))
    {
        if (ImGui::Button("Wall"))
        {
            Editor->Mode = EditorMode_Wall;
            Editor->EditingLines = true;
        }
        if (ImGui::Button("Collision"))
        {
            Editor->Mode = EditorMode_Collision;
            Editor->EditingLines = true;
        }

        ImGui::Spacing();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    {
        auto *Entity = Editor->Level->RootEntity;
        while (Entity)
        {
            Entity = DrawEntityTree(Editor, Entity);
        }
    }

    ImGui::End();

    //bool Open = true;
    //ImGui::ShowTestWindow(&Open);
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
        MakeCameraPerspective(Game->Camera, (float)Game->Width, (float)Game->Height, 70.0f, 0.1f, 1000.0f);
        InitRenderState(Game->RenderState, Width, Height);

        Game->Assets.push_back(
            CreateAssetEntry(
                AssetType_Texture,
                "data/textures/grid1.png",
                "grid",
                (void *)(Texture_Mipmap | Texture_Anisotropic | Texture_WrapRepeat)
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
            RenderLoadingScreen(*Game, DeltaTime, InitEditor);
            break;

        case GameMode_Editor:
            RenderEditor(*Game, DeltaTime);
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
                //CheckAssetsChange(Game->AssetLoader);
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

        if (Game->EditorState)
        {
            FreeArena(Game->EditorState->Arena);
        }
    }
}
