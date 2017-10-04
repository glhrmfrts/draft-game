// Copyright

void InitEditor(game_state &Game)
{
    Game.Mode = GameMode_Editor;
    auto &Editor = Game.EditorState;

    Editor.Name = (char *)PushSize(Editor.Arena, 128, "editor name");
    Editor.Filename = (char *)PushSize(Editor.Arena, 128, "editor filename");

    *Editor.Name = 0;
    *Editor.Filename = 0;
    Editor.Mode = EditorMode_None;

    Game.Camera.Position = vec3{0, 0, 10};
    Game.Camera.LookAt = vec3{0, 10, 0};

    {
        auto &CursorMesh = Editor.CursorMesh;
        InitMeshBuffer(CursorMesh.Buffer);

        float w = 1.0f;
        float l = -w/2;
        float r = w/2;
        AddQuad(CursorMesh.Buffer, vec3(l, l, 0), vec3(r, l, 0), vec3(r, r, 0), vec3(l, r, 0), Color_white, vec3(1, 1, 1));
        AddPart(&CursorMesh, {BlankMaterial, 0, CursorMesh.Buffer.VertexCount, GL_TRIANGLES});
        EndMesh(&CursorMesh, GL_STATIC_DRAW);
    }
    Editor.CursorTransform.Scale *= 0.5f;
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

static void Commit(editor_state &Editor)
{
}

void RenderEditor(game_state &Game, float DeltaTime)
{
    auto &Editor = Game.EditorState;
    auto *FloorMesh = GetFloorMesh(Game);

    if (Editor.Mode != EditorMode_None)
    {
        float MouseX = float(Game.Input.MouseState.X);
        float MouseY = float(Game.Height - Game.Input.MouseState.Y);
        ray Ray = PickRay(Game.Camera, MouseX, MouseY, 0, 0, Game.Width, Game.Height);
        vec3 Pos = GetCursorPositionOnFloor(Ray);
        Pos.z = 0.25f;

        Editor.CursorTransform.Position = Pos;
    }

    UpdateFreeCam(Game.Camera, Game.Input, DeltaTime);
    UpdateProjectionView(Game.Camera);

    RenderBegin(Game.RenderState, DeltaTime);

    {
        transform Transform;
        Transform.Scale = vec3{LEVEL_PLANE_SIZE, LEVEL_PLANE_SIZE, 1};
        PushMeshPart(Game.RenderState, *FloorMesh, FloorMesh->Parts[0], Transform);
    }
    {
        PushMeshPart(Game.RenderState, Editor.CursorMesh, Editor.CursorMesh.Parts[0], Editor.CursorTransform);
    }

    RenderEnd(Game.RenderState, Game.Camera);

    {
        ImGui::SetNextWindowSize(ImVec2(Game.RealWidth*0.25f, Game.RealHeight*1.0f), ImGuiSetCond_Always);
        ImGui::Begin("Editor Main");
        ImGui::InputText("name", Editor.Name, 128);
        ImGui::InputText("filename", Editor.Filename, 128);
        ImGui::Spacing();
        if (ImGui::Button("Save"))
        {
            Println("Save");
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Text("Current Mode: %s", EditorModeStrings[Editor.Mode]);
        if (Editor.Mode != EditorMode_None)
        {
            if (ImGui::Button("Commit"))
            {
                Commit(Editor);
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
            {
                Editor.Mode = EditorMode_None;
            }
        }

        if (ImGui::CollapsingHeader("Add"))
        {
            if (ImGui::Button("Wall"))
            {
                Editor.Mode = EditorMode_Wall;
            }
            if (ImGui::Button("Collision"))
            {
                Editor.Mode = EditorMode_Collision;
            }

            ImGui::Spacing();
        }

        ImGui::End();
    }
}
