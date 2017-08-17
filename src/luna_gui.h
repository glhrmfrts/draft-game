#ifndef LUNA_GUI_H
#define LUNA_GUI_H

#include "luna_common.h"
#include "luna_render.h"

#define GUIElementState_none 0x0
#define GUIElementState_hovered 0x1
#define GUIElementState_left_pressed 0x2

struct gui_draw_command
{
    color DiffuseColor;
    GLuint PrimitiveType;
    size_t Offset;
    size_t Count;
    float TexWeight = 0;
    texture *Texture;
};

struct game_input;

struct gui
{
    gui_draw_command CurrentDrawCommand;
    vertex_buffer Buffer;
    vector<gui_draw_command> DrawCommandList;
    shader_program Program;
    game_input *Input;

    // uniforms
    int ProjectionView;
    int DiffuseColor;
    int Sampler;
    int TexWeight;
};

// Prototypes.
void InitGUI(gui *GUI, game_input *Input);
void Begin(gui *GUI, camera *Camera);

uint32 PushRect(gui *GUI, rect Rect, color vColor, color DiffuseColor, texture *Texture, texture_rect TexRect, float TexWeight = 0.0f, bool FlipV = false, GLuint PrimType = GL_TRIANGLES, bool CheckState = true);

uint32 PushRect(gui *GUI, rect R, color C, GLuint PrimType = GL_TRIANGLES, bool CheckState = true);

uint32 PushTexture(gui *GUI, rect R, texture *T, bool FlipV = false, GLuint PrimType = GL_TRIANGLES, bool CheckState = true);

#if 0
uint32 PushText(gui *GUI, font *Font, const string &Text, rect R, color C, bool CheckState = true);
#endif

void End(gui *GUI);

#endif
