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
uniform float u_Emission;

smooth in vec2 v_Uv;
smooth in vec4 v_Color;

out vec4 BlendUnitColor[2];

void main() {
    vec4 TexColor = texture(u_Sampler, v_Uv);
    vec4 Color = mix(v_Color, TexColor, u_TexWeight) * u_DiffuseColor;
    BlendUnitColor[0] = Color;
    BlendUnitColor[1] = Color * u_Emission;
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
    g.Emission = glGetUniformLocation(g.Program.ID, "u_Emission");

    Bind(g.Program);
    SetUniform(g.Sampler, 0);
    UnbindShaderProgram();
}

#define GUIVertexSize 8

static void InitGUI(gui &g, game_input &Input)
{
    g.Input = &Input;
    CompileGUIShader(g);
    InitBuffer(g.Buffer, GUIVertexSize, 3,
               vertex_attribute{0, 2, GL_FLOAT, 8 * sizeof(float), 0},
               vertex_attribute{1, 2, GL_FLOAT, 8 * sizeof(float), 2 * sizeof(float)},
               vertex_attribute{2, 4, GL_FLOAT, 8 * sizeof(float), 4 * sizeof(float)});
}

static void Begin(gui &g, camera &Camera, float emission = 0.0f)
{
    assert(g.Program.ID);
    assert(Camera.Updated);

    Bind(g.Program);
    SetUniform(g.ProjectionView, Camera.ProjectionView);
    g.CurrentDrawCommand.Texture = NULL;
    g.DrawCommandList.clear();
    ResetBuffer(g.Buffer);

    g.EmissionValue = emission;
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

static void DrawLine(gui &g, vec2 p1, vec2 p2, color c)
{
    auto &curr = g.CurrentDrawCommand;
    if (curr.PrimitiveType != GL_LINES)
    {
        PushDrawCommand(g);
        g.CurrentDrawCommand = NextDrawCommand(g, GL_LINES, 0.0f, Color_white, NULL);
    }

    PushVertex(g.Buffer, {p1.x, p1.y, 0,0, c.r, c.g, c.b, c.a});
    PushVertex(g.Buffer, {p2.x, p2.y, 0,0, c.r, c.g, c.b, c.a});
}

static uint32 DrawRect(gui &g, rect Rect, color vColor, color DiffuseColor,
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
    float e = g.Emission;
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

static uint32 DrawRect(gui &g, rect R, color C, GLuint PrimType = GL_TRIANGLES, bool CheckState = true)
{
    return DrawRect(g, R, C, Color_white, NULL, {}, 0, false, PrimType, CheckState);
}

static uint32 DrawTexture(gui &g, rect R, texture *T, bool FlipV, GLuint PrimType, bool CheckState = true)
{
    return DrawRect(g, R, Color_white, Color_white, T, {0, 0, 1, 1}, 1, FlipV, PrimType, CheckState);
}

static vec2 MeasureText(bitmap_font *font, const char *text)
{
    vec2 result = {};
    int curX = 0;
    int curY = 0;
    for (size_t i = 0; text[i]; i++)
    {
        if (text[i] == '\n')
        {
            curX = 0;
            curY -= font->NewLine;
            continue;
        }

        int Index = int(text[i]);
        curX += font->BearingX[Index];
        curX += font->AdvX[Index] - font->BearingX[Index];
        result.x = std::max((int)result.x, curX);
    }

    result.y = curY;
    return result;
}

static uint32 DrawText(gui &g, bitmap_font *Font, const char *text, rect r, color c, bool CheckState = true)
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

static uint32 DrawTextCentered(gui &g, bitmap_font *font, const char *text, rect r, color c, bool checkState = true)
{
    vec2 size = MeasureText(font, text);
    r.X -= size.x/2;
    r.Y -= size.y/2;
    return DrawText(g, font, text, r, c, checkState);
}

static void End(gui &g)
{
    PushDrawCommand(g);

    // TODO: reserve vertices before uploading
    UploadVertices(g.Buffer, GL_DYNAMIC_DRAW);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindVertexArray(g.Buffer.VAO);
    SetUniform(g.Emission, g.EmissionValue);

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
