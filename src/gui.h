#ifndef DRAFT_GUI_H
#define DRAFT_GUI_H

#define GUIElementState_none 0x0
#define GUIElementState_hovered 0x1
#define GUIElementState_left_pressed 0x2

struct rect
{
    float X, Y;
    float Width, Height;

    rect operator *(float f)
    {
        return rect{X*f, Y*f, Width*f, Height*f};
    }
};

struct gui_vertex
{
	union
	{
		struct {
			float PositionX;
			float PositionY;
			float UvX;
			float UvY;
			float ColorR;
			float ColorG;
			float ColorB;
			float ColorA;
		} Floats;

		float Data[8];
	};
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

struct menu_axis
{
    uint8 Type;
    float Timer = 0.0f;
};

struct game_input;
struct gui
{
    gui_draw_command CurrentDrawCommand;
    vertex_buffer Buffer;
    vector<gui_draw_command> DrawCommandList;
    shader_program Program;
    game_input *Input;
    float EmissionValue;
    
    // menu stuff
    tween_sequence MenuChangeSequence;
    menu_axis HorizontalAxis;
    menu_axis VerticalAxis;
    float MenuChangeTimer = 0;

    // uniforms
    int ProjectionView;
    int DiffuseColor;
    int Sampler;
    int TexWeight;
    int Emission;
};

enum menu_item_type
{
    MenuItemType_Text,
	MenuItemType_SliderFloat,
	MenuItemType_Switch,
	MenuItemType_Options,
};

struct menu_item
{
    const char *Text;
    menu_item_type Type;
    bool Enabled = true;

	union
	{
		struct {
			float Min;
			float Max;
			float *Value;
		} SliderFloat;

		struct {
			bool *Value;
		} Switch;

		struct {
			fixed_array<const char *, 8> Values;
			int SelectedIndex;
		} Options;
	};

	menu_item() {}
	menu_item(const char *t, menu_item_type type, bool enabled)
		: Text(t), Type(type), Enabled(enabled)
	{
		if (type == MenuItemType_Options)
		{
			this->Options.Values = fixed_array<const char *, 8>();
		}
	}

	~menu_item() {}
};

struct game_main;
struct menu_data;

#define MENU_FUNC(name) void name(game_main *g, menu_data *menu, int itemIndex)
typedef MENU_FUNC(menu_func);

struct menu_data
{
    const char *Title;
    int NumItems;
    menu_func *SelectFunc; // function called when an item in the submenu is selected
    menu_item Items[8];
    int HotItem=0;
};

#endif
