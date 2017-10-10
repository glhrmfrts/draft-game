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
#include "init.cpp"

#define IterEntities(First, Func, Arg) \
    {\
        auto *It = First;\
        while (It)\
        {\
            It = Func(It, Arg);\
        }\
    }

entity *CreateEntity(level *Level, entity_type Type)
{
    auto *Result = PushStruct<entity>(Level->Arena);
    Result->Type = Type;
    Result->ID = Level->NextEntityID++;
    return Result;
}

entity *CreateModelEntity(level *Level, mesh *Mesh)
{
    auto *Result = CreateEntity(Level, EntityType_Model);
    Result->Model = PushStruct<model>(Level->Arena);
    Result->Model->Mesh = Mesh;
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

static void WriteCollisionShape(collision_shape *Shape, FILE *Handle)
{
    uint8 Type = uint8(Shape->Type);
    fwrite(&Type, sizeof(uint8), 1, Handle);
    switch (Shape->Type)
    {
    case CollisionShapeType_Polygon:
    {
        uint32 NumVertices = uint32(Shape->Polygon.Vertices.size());
        fwrite(&NumVertices, sizeof(uint32), 1, Handle);
        fwrite(&Shape->Polygon.Vertices[0], sizeof(vec2), NumVertices, Handle);
        break;
    }

    case CollisionShapeType_Circle:
    {
        uint32 Radius = uint32(Shape->Circle.Radius);
        fwrite(&Radius, sizeof(uint32), 1, Handle);
        fwrite(&Shape->Circle.Center, sizeof(vec2), 1, Handle);
        break;
    }
    }
}

static void WriteWall(level_wall *Wall, FILE *Handle)
{
    uint32 NumPoints = uint32(Wall->Points.size());
    fwrite(&NumPoints, sizeof(uint32), 1, Handle);
    fwrite(&Wall->Points[0], sizeof(vec2), NumPoints, Handle);
}

static entity *WriteEntity(entity *Entity, FILE *Handle)
{
    uint8 Type = uint8(Entity->Type);
    uint32 Id = uint32(Entity->ID);
    uint32 NumChildren = uint32(Entity->NumChildren);
    fwrite(&Type, sizeof(uint8), 1, Handle);
    fwrite(&Id, sizeof(uint32), 1, Handle);
    fwrite(&NumChildren, sizeof(uint32), 1, Handle);
    fwrite(&Entity->Transform.Position, sizeof(float), 3, Handle);
    fwrite(&Entity->Transform.Scale, sizeof(float), 3, Handle);
    fwrite(&Entity->Transform.Rotation, sizeof(float), 3, Handle);
    switch (Entity->Type)
    {
    case EntityType_Collision:
        WriteCollisionShape(Entity->Shape, Handle);
        break;

    case EntityType_Wall:
        WriteWall(Entity->Wall, Handle);
        break;
    }
    IterEntities(Entity->FirstChild, WriteEntity, Handle);
    return Entity->NextSibling;
}

static void WriteLevel(void *Arg)
{
    auto *Level = static_cast<level *>(Arg);
    FILE *Handle = fopen(Level->Filename, "wb");
    fseek(Handle, SEEK_SET, 0);

    uint16 NameLen = uint16(strlen(Level->Name));
    fwrite(&NameLen, sizeof(uint16), 1, Handle);
    fwrite(Level->Name, sizeof(char), NameLen, Handle);
    IterEntities(Level->RootEntity, WriteEntity, Handle);
    fclose(Handle);
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

    Editor->Level->Name = (char *)PushSize(Editor->Level->Arena, 128, "editor name");
    Editor->Level->Filename = (char *)PushSize(Editor->Level->Arena, 128, "editor filename");
    *Editor->Level->Name = 0;
    *Editor->Level->Filename = 0;
    Editor->Mode = EditorMode_None;
    CreateThreadPool(Editor->Pool, 1, 8);

    Editor->LevelTextSequence = PushStruct<tween_sequence>(Editor->Arena);
    AddSequences(Game.TweenState, Editor->LevelTextSequence, 1);

    float *Target = &Editor->LevelTextAlpha;
    AddTween(Editor->LevelTextSequence, CreateTween(float_tween{0.0f,1.0f,Target}, 0.5f));
    AddTween(Editor->LevelTextSequence, CreateTween(float_tween{1.0f,1.0f,Target}, 1.0f));
    AddTween(Editor->LevelTextSequence, CreateTween(float_tween{1.0f,0.0f,Target}, 0.5f));

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

    case EditorMode_Wall:
    {
        auto *Entity = CreateEntity(Editor->Level, EntityType_Wall);
        Entity->Wall = PushStruct<level_wall>(Editor->Arena);
        for (auto &Vert : Editor->LinePoints)
        {
            Entity->Wall->Points.push_back(vec2{Vert.x, Vert.y});
        }
        Editor->LinePoints.clear();

        auto *Mesh = GenerateWallMesh(Editor->Arena, Entity->Wall->Points);
        auto *ModelEntity = CreateModelEntity(Editor->Level, Mesh);
        AddChild(Entity, ModelEntity);
        AddChild(Editor->SelectedEntity, Entity);
        break;
    }
    }
}

static entity *DrawEntityTree(entity *Entity, editor_state *Editor)
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
        IterEntities(Entity->FirstChild, DrawEntityTree, Editor);
        ImGui::TreePop();
    }
    return Entity->NextSibling;
}

static entity *RenderEntity(entity *Entity, render_state &RenderState)
{
    switch (Entity->Type)
    {
    case EntityType_Collision:
        DrawDebugShape(RenderState, Entity->Shape);
        break;

    case EntityType_Model:
        DrawModel(RenderState, *Entity->Model, Entity->Transform);
        break;
    }
    IterEntities(Entity->FirstChild, RenderEntity, RenderState);
    return Entity->NextSibling;
}

void RenderEditor(game_state &Game, float DeltaTime)
{
    auto *Editor = Game.EditorState;
    auto *FloorMesh = GetFloorMesh(Game);

    UpdateFreeCam(Game.Camera, Game.Input, DeltaTime);
    UpdateProjectionView(Game.Camera);

    RenderBegin(Game.RenderState, DeltaTime);

    {
        transform Transform;
        Transform.Position.z = -0.1f;
        Transform.Scale = vec3{LEVEL_PLANE_SIZE, LEVEL_PLANE_SIZE, 1};
        DrawMeshPart(Game.RenderState, *FloorMesh, FloorMesh->Parts[0], Transform);
    }
    if (Editor->Mode != EditorMode_None)
    {
        float MouseX = float(Game.Input.MouseState.X);
        float MouseY = float(Game.RealHeight - Game.Input.MouseState.Y);
        ray Ray = PickRay(Game.Camera, MouseX, MouseY, 0, 0, Game.RealWidth, Game.RealHeight);
        vec3 CursorPos = GetCursorPositionOnFloor(Ray);
        CursorPos.z = 0.0f;
        if (Editor->SnapToInteger)
        {
            CursorPos.x = std::floor(CursorPos.x);
            CursorPos.y = std::floor(CursorPos.y);
        }

        transform Transform;
        Transform.Position = CursorPos;
        Transform.Scale *= 0.5f;
        DrawMeshPart(Game.RenderState, Editor->CursorMesh, Editor->CursorMesh.Parts[0], Transform);

        if (IsJustReleased(Game, MouseButton_Left) && Editor->EditingLines)
        {
            Editor->LinePoints.push_back(CursorPos);
            if (Editor->Mode == EditorMode_Collision && Editor->LinePoints.size() == 3)
            {
                EditorCommit(Editor);
            }
        }
        else if (IsJustReleased(Game, MouseButton_Right))
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

    IterEntities(Editor->Level->RootEntity, RenderEntity, Game.RenderState);
    RenderEnd(Game.RenderState, Game.Camera);

    if (Editor->LevelTextAlpha > 0.0f)
    {
        auto &g = Game.GUI;
        auto Font = FindBitmapFont(Game.AssetLoader, "vcr_16");
        auto Text = "Level Saved";
        vec2 Size = MeasureText(Font, Text);
        float x = Game.Width/2 - Size.x/2;
        float y = Game.Height/2 - Size.y/2;
        color Color = Color_white;
        Color.a = Editor->LevelTextAlpha;

        UpdateProjectionView(Game.GUICamera);
        Begin(g, Game.GUICamera);
        DrawText(g, Font, Text, rect{x, y}, Color, false);
        End(g);
    }

    ImGui::SetNextWindowSize(ImVec2(Game.RealWidth*0.25f, Game.RealHeight*1.0f), ImGuiSetCond_Always);
    ImGui::Begin("Editor Main");
    ImGui::InputText("name", Editor->Level->Name, 128);
    ImGui::InputText("filename", Editor->Level->Filename, 128);
    ImGui::Spacing();
    if (ImGui::Button("Save"))
    {
        AddJob(Editor->Pool, WriteLevel, Editor->Level);
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

    IterEntities(Editor->Level->RootEntity, DrawEntityTree, Editor);
    ImGui::End();

    if (int(Editor->Pool.NumJobs) == 0 && Editor->NumCurrentJobs > 0)
    {
        PlaySequence(Game.TweenState, Editor->LevelTextSequence);
    }
    Editor->NumCurrentJobs = int(Editor->Pool.NumJobs);

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
        InitTweenState(Game->TweenState);

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

        Update(Game->TweenState, DeltaTime);
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
