#ifndef DRAFT_GUI_H
#define DRAFT_GUI_H

#define GUIElementState_none 0x0
#define GUIElementState_hovered 0x1
#define GUIElementState_left_pressed 0x2

struct rect
{
    float X, Y;
    float Width, Height;
};

struct gui_draw_command
{
    color DiffuseColor;
    GLuint PrimitiveType;
    size_t Offset;
    size_t Count;
    float TexWeight = 0;
    texture *Texture;

    // vs shit
    gui_draw_command() {}
    gui_draw_command(color c, GLuint p, size_t o, size_t cnt, float tw, texture *t)
        : DiffuseColor(c), PrimitiveType(p), Offset(o), Count(cnt), TexWeight(tw), Texture(t) {}
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

#endif