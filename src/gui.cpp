// Copyright

// gui shaders are embedded because we need them to draw the loading
// screen :)
static string GUIVertexShader = R"FOO(
#version 330

layout (location = 0) in vec2  a_Position;
layout (location = 1) in vec2  a_Uv;
layout (location = 2) in vec4  a_Color;

uniform mat4 u_ProjectionView;

smooth out vec2 v_Uv;
smooth out vec4 v_Color;

void main() {
    gl_Position = u_ProjectionView * vec4(a_Position, 0.0, 1.0);
    v_Uv = a_Uv;
    v_Color = a_Color;
}
)FOO";

static string GUIFragmentShader = R"FOO(
#version 330

uniform sampler2D u_Sampler;
uniform float u_TexWeight;
uniform vec4 u_DiffuseColor;

smooth in vec2 v_Uv;
smooth in vec4 v_Color;

out vec4 BlendUnitColor;

void main() {
    vec4 TexColor = texture(u_Sampler, v_Uv);
    BlendUnitColor = mix(v_Color, TexColor, u_TexWeight) * u_DiffuseColor;
}
)FOO";

static uint32 CheckElementState(game_input &Input, rect Rect)
{
    uint32 Result = GUIElementState_none;
    auto &MouseState = Input.MouseState;
    if (MouseState.X > Rect.X &&
        MouseState.Y > Rect.Y &&
        MouseState.X < Rect.X + Rect.Width &&
        MouseState.Y < Rect.Y + Rect.Height)
    {
        Result |= GUIElementState_hovered;
        if (MouseState.Buttons & MouseButton_Left)
        {
            Result |= GUIElementState_left_pressed;
        }
    }

    return Result;
}

static void CompileGUIShader(gui &g)
{
    g.Program.VertexShader = glCreateShader(GL_VERTEX_SHADER);
    g.Program.FragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    CompileShader(g.Program.VertexShader, GUIVertexShader.c_str());
    CompileShader(g.Program.FragmentShader, GUIFragmentShader.c_str());
    LinkShaderProgram(g.Program);

    g.ProjectionView = glGetUniformLocation(g.Program.ID, "u_ProjectionView");
    g.TexWeight = glGetUniformLocation(g.Program.ID, "u_TexWeight");
    g.Sampler = glGetUniformLocation(g.Program.ID, "u_Sampler");
    g.DiffuseColor = glGetUniformLocation(g.Program.ID, "u_DiffuseColor");

    Bind(g.Program);
    SetUniform(g.Sampler, 0);
    UnbindShaderProgram();
}

#define GUIVertexSize 8

void InitGUI(gui &g, game_input &Input)
{
    g.Input = &Input;
    CompileGUIShader(g);
    InitBuffer(g.Buffer, GUIVertexSize, 3,
               vertex_attribute{0, 2, GL_FLOAT, 8 * sizeof(float), 0},
               vertex_attribute{1, 2, GL_FLOAT, 8 * sizeof(float), 2 * sizeof(float)},
               vertex_attribute{2, 4, GL_FLOAT, 8 * sizeof(float), 4 * sizeof(float)});
}

void Begin(gui &g, camera &Camera)
{
    assert(g.Program.ID);
    assert(Camera.Updated);

    Bind(g.Program);
    SetUniform(g.ProjectionView, Camera.ProjectionView);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    g.CurrentDrawCommand.Texture = NULL;
    g.DrawCommandList.clear();

    ResetBuffer(g.Buffer);
}

static gui_draw_command NextDrawCommand(gui &g, GLuint PrimType, float TexWeight, color DiffuseColor, texture *Texture)
{
    gui_draw_command Command{DiffuseColor, PrimType, g.Buffer.VertexCount, 0, TexWeight, Texture};
    return Command;
}

static void PushDrawCommand(gui &g)
{
    if (!g.Buffer.VertexCount) return;

    auto &Curr = g.CurrentDrawCommand;
    Curr.Count = g.Buffer.VertexCount - Curr.Offset;
    g.DrawCommandList.push_back(Curr);
}

uint32 DrawRect(gui &g, rect Rect, color vColor, color DiffuseColor,
                texture *Texture, texture_rect TexRect, float TexWeight = 0.0f,
                bool FlipV = false, GLuint PrimType = GL_TRIANGLES, bool CheckState = true)
{
    auto &Curr = g.CurrentDrawCommand;
    if (Curr.Texture != Texture ||
        Curr.TexWeight != TexWeight ||
        Curr.PrimitiveType != PrimType ||
        Curr.DiffuseColor != DiffuseColor ||
        g.Buffer.VertexCount == 0)
    {
        PushDrawCommand(g);
        g.CurrentDrawCommand = NextDrawCommand(g, PrimType, TexWeight, DiffuseColor, Texture);
    }

    float x = Rect.X;
    float y = Rect.Y;
    float w = Rect.Width;
    float h = Rect.Height;
    float u = TexRect.u;
    float v = TexRect.v;
    float u2 = TexRect.u2;
    float v2 = TexRect.v2;
    if (FlipV)
    {
        v2 = v;
        v = TexRect.v2;
    }

    color C = vColor;
    switch (PrimType)
    {
    case GL_LINE_LOOP:
        PushVertex(g.Buffer, {x,   y,   u, v,   C.r, C.g, C.b, C.a});
        PushVertex(g.Buffer, {x+w, y,   u2, v,  C.r, C.g, C.b, C.a});
        PushVertex(g.Buffer, {x+w, y+h, u2, v2, C.r, C.g, C.b, C.a});
        PushVertex(g.Buffer, {x,   y+h, u,  v2, C.r, C.g, C.b, C.a});
        break;

    case GL_TRIANGLES:
        PushVertex(g.Buffer, {x,   y,   u, v,   C.r, C.g, C.b, C.a});
        PushVertex(g.Buffer, {x,   y+h, u,  v2, C.r, C.g, C.b, C.a});
        PushVertex(g.Buffer, {x+w, y,   u2, v,  C.r, C.g, C.b, C.a});
        PushVertex(g.Buffer, {x,   y+h, u,  v2, C.r, C.g, C.b, C.a});
        PushVertex(g.Buffer, {x+w, y+h, u2, v2, C.r, C.g, C.b, C.a});
        PushVertex(g.Buffer, {x+w, y,   u2, v,  C.r, C.g, C.b, C.a});
        break;

    default:
        assert(!"Invalid primitive type");
        break;
    }

    if (CheckState)
    {
        return CheckElementState(*g.Input, Rect);
    }
    return GUIElementState_none;
}

uint32 DrawRect(gui &g, rect R, color C, GLuint PrimType = GL_TRIANGLES, bool CheckState = true)
{
    return DrawRect(g, R, C, Color_white, NULL, {}, 0, false, PrimType, CheckState);
}

uint32 DrawTexture(gui &g, rect R, texture *T, bool FlipV, GLuint PrimType, bool CheckState = true)
{
    return DrawRect(g, R, Color_white, Color_white, T, {0, 0, 1, 1}, 1, FlipV, PrimType, CheckState);
}

vec2 MeasureText(bitmap_font *Font, const char *text)
{
    vec2 Result = {};
    int CurX = 0;
    int CurY = 0;
    for (size_t i = 0; text[i]; i++)
    {
        if (text[i] == '\n')
        {
            CurX = 0;
            CurY -= Font->NewLine;
            continue;
        }

        int Index = int(text[i]);
        CurX += Font->BearingX[Index];
        CurX += Font->AdvX[Index] - Font->BearingX[Index];
        Result.x = std::max((int)Result.x, CurX);
    }

    Result.y = CurY;
    return Result;
}

uint32 DrawText(gui &g, bitmap_font *Font, const char *text, rect r, color c, bool CheckState = true)
{
    int CurX = r.X;
    int CurY = r.Y;
    for (size_t i = 0; i < text[i]; i++)
    {
        if (text[i] == '\n')
        {
            CurX = r.X;
            CurY -= Font->NewLine;
            continue;
        }

        int Index = int(text[i]);
        CurX += Font->BearingX[Index];
        if (text[i] != ' ')
        {
            int Diff = Font->CharHeight[Index] - Font->BearingY[Index];
            int Width = Font->CharWidth[Index];
            int Height = Font->CharHeight[Index];
            DrawRect(g,
                     rect{ (float)CurX, (float)CurY - Diff, (float)Width, (float)Height },
                     Color_white,
                     c,
                     Font->Texture,
                     Font->TexRect[Index],
                     1.0f,
                     false,
                     GL_TRIANGLES,
                     false);
        }

        CurX += Font->AdvX[Index] - Font->BearingX[Index];
        r.Width = std::max((int)r.Width, CurX);
    }

    r.Width = r.Width - r.X;
    r.Height = (CurY + Font->NewLine) - r.Y;
    if (CheckState)
    {
        return CheckElementState(*g.Input, r);
    }
    return GUIElementState_none;
}

void End(gui &g)
{
    PushDrawCommand(g);
    UploadVertices(g.Buffer, GL_DYNAMIC_DRAW);

    glBindVertexArray(g.Buffer.VAO);

    for (const auto &Cmd : g.DrawCommandList)
    {
        if (Cmd.Texture)
        {
            Bind(*Cmd.Texture, 0);
        }
        SetUniform(g.TexWeight, Cmd.TexWeight);
        SetUniform(g.DiffuseColor, Cmd.DiffuseColor);
        glDrawArrays(Cmd.PrimitiveType, Cmd.Offset, Cmd.Count);
        if (Cmd.Texture)
        {
            Unbind(*Cmd.Texture, 0);
        }
    }

    glBindVertexArray(0);
    UnbindShaderProgram();
    glDisable(GL_BLEND);
}
