#include "luna.h"
#include "luna_gui.h"

extern char *GUIVertexShader;
extern char *GUIFragmentShader;

static uint32 CheckElementState(game_input *Input, rect Rect)
{
    uint32 Result = GUIElementState_none;
    mouse_state *MouseState = &Input->MouseState;
    if (MouseState->X > Rect.X &&
        MouseState->Y > Rect.Y &&
        MouseState->X < Rect.X + Rect.Width &&
        MouseState->Y < Rect.Y + Rect.Height) {

        Result |= GUIElementState_hovered;
        if (MouseState->Buttons & MouseButton_left) {
            Result |= GUIElementState_left_pressed;
        }
    }

    return Result;
}

static void CompileGUIShader(gui *GUI)
{
    CompileShaderProgram(&GUI->Program, GUIVertexShader, GUIFragmentShader);
    GUI->ProjectionView = glGetUniformLocation(GUI->Program.ID, "u_ProjectionView");
    GUI->TexWeight = glGetUniformLocation(GUI->Program.ID, "u_TexWeight");
    GUI->Sampler = glGetUniformLocation(GUI->Program.ID, "u_Sampler");
    GUI->DiffuseColor = glGetUniformLocation(GUI->Program.ID, "u_DiffuseColor");

    Bind(&GUI->Program);
    SetUniform(GUI->Sampler, 0);
    UnbindShaderProgram();
}

#define GUIVertexSize 8

void InitGUI(gui *GUI, game_input *Input)
{
    GUI->Input = Input;
    CompileGUIShader(GUI);
    InitBuffer(&GUI->Buffer, GUIVertexSize, 3,
               (vertex_attribute){0, 2, GL_FLOAT, 8 * sizeof(float), 0},
               (vertex_attribute){1, 2, GL_FLOAT, 8 * sizeof(float), 2 * sizeof(float)},
               (vertex_attribute){2, 4, GL_FLOAT, 8 * sizeof(float), 4 * sizeof(float)});
}

void Begin(gui *GUI, camera *Camera)
{
    assert(GUI->Program.ID);

    Bind(&GUI->Program);
    SetUniform(GUI->ProjectionView, Camera->ProjectionView);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GUI->CurrentDrawCommand.Texture = NULL;
    GUI->DrawCommandList.clear();

    ResetBuffer(&GUI->Buffer);
}

static gui_draw_command NextDrawCommand(gui *GUI, GLuint PrimType, float TexWeight, color DiffuseColor, texture *Texture)
{
    gui_draw_command Command = {DiffuseColor, PrimType, GUI->Buffer.VertexCount, 0, TexWeight, Texture};
    return Command;
}

static void PushDrawCommand(gui *GUI)
{
    if (!GUI->Buffer.VertexCount) return;

    auto *Curr = &GUI->CurrentDrawCommand;
    Curr->Count = GUI->Buffer.VertexCount - Curr->Offset;
    GUI->DrawCommandList.push_back(*Curr);
}

uint32 PushRect(gui *GUI, rect Rect, color vColor, color DiffuseColor, texture *Texture, texture_rect TexRect, float TexWeight, bool FlipV, GLuint PrimType, bool CheckState)
{
    auto *Curr = &GUI->CurrentDrawCommand;
    if (Curr->Texture != Texture ||
        Curr->TexWeight != TexWeight ||
        Curr->PrimitiveType != PrimType ||
        Curr->DiffuseColor != DiffuseColor ||
        GUI->Buffer.VertexCount == 0) {

        PushDrawCommand(GUI);
        GUI->CurrentDrawCommand = NextDrawCommand(GUI, PrimType, TexWeight, DiffuseColor, Texture);
    }

    float X = Rect.X;
    float Y = Rect.Y;
    float W = Rect.Width;
    float H = Rect.Height;
    float U = TexRect.U;
    float V = TexRect.V;
    float U2 = TexRect.U2;
    float V2 = TexRect.V2;
    if (FlipV) {
        V2 = V;
        V = TexRect.V2;
    }

    color C = vColor;
    switch (PrimType) {
    case GL_LINE_LOOP:
        PushVertex(&GUI->Buffer, X,   Y,   U, V,   C.r, C.g, C.b, C.a);
        PushVertex(&GUI->Buffer, X+W, Y,   U2, V,  C.r, C.g, C.b, C.a);
        PushVertex(&GUI->Buffer, X+W, Y+H, U2, V2, C.r, C.g, C.b, C.a);
        PushVertex(&GUI->Buffer, X,   Y+H, U,  V2, C.r, C.g, C.b, C.a);
        break;

    case GL_TRIANGLES:
        PushVertex(&GUI->Buffer, X,   Y,   U, V,   C.r, C.g, C.b, C.a);
        PushVertex(&GUI->Buffer, X,   Y+H, U,  V2, C.r, C.g, C.b, C.a);
        PushVertex(&GUI->Buffer, X+W, Y,   U2, V,  C.r, C.g, C.b, C.a);
        PushVertex(&GUI->Buffer, X,   Y+H, U,  V2, C.r, C.g, C.b, C.a);
        PushVertex(&GUI->Buffer, X+W, Y+H, U2, V2, C.r, C.g, C.b, C.a);
        PushVertex(&GUI->Buffer, X+W, Y,   U2, V,  C.r, C.g, C.b, C.a);
        break;
    }

    if (CheckState) {
        return CheckElementState(GUI->Input, Rect);
    }
    return GUIElementState_none;
}

uint32 PushRect(gui *GUI, rect R, color C, GLuint PrimType, bool CheckState)
{
    return PushRect(GUI, R, C, Colors_white, NULL, {0}, 0, false, PrimType, CheckState);
}

uint32 PushTexture(gui *GUI, rect R, texture *T, bool FlipV, GLuint PrimType, bool CheckState)
{
    return PushRect(GUI, R, Colors_white, Colors_white, T, {0, 0, 1, 1}, 1, FlipV, PrimType, CheckState);
}

#if 0
uint32 PushText(gui *GUI, font *Font, const string &Text, rect R, color C, bool CheckState)
{
    vec2 Size(0, 0);
    RenderText(GUI, Font, Text, R, C, &Size);

    R.Width = Size.x;
    R.Height = Size.y;
    if (CheckState) {
        return CheckElementState(GUI->Input, R);
    }
    return GUIElementState_none;
}
#endif

void End(gui *GUI)
{
    PushDrawCommand(GUI);
    UploadVertices(&GUI->Buffer, GL_DYNAMIC_DRAW);

    glBindVertexArray(GUI->Buffer.VAO);

    for (const auto &Cmd : GUI->DrawCommandList) {
        if (Cmd.Texture) {
            Bind(Cmd.Texture, 0);
        }

        SetUniform(GUI->TexWeight, Cmd.TexWeight);
        SetUniform(GUI->DiffuseColor, Cmd.DiffuseColor);
        glDrawArrays(Cmd.PrimitiveType, Cmd.Offset, Cmd.Count);
    }

    glBindVertexArray(0);
    UnbindShaderProgram();
    glDisable(GL_BLEND);
}

char *GUIVertexShader = R"FOO(
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

char *GUIFragmentShader = R"FOO(
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
