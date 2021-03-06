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

uint32 CheckElementState(game_input &Input, rect Rect)
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

void CompileGUIShader(gui &g)
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

void InitGUI(gui &g, tween_state &tweenState, game_input &Input)
{
    g.Input = &Input;
    CompileGUIShader(g);
	InitBuffer(g.Buffer, GUIVertexSize, {
			   vertex_attribute{0, 2, GL_FLOAT, 8 * sizeof(float), 0},
			   vertex_attribute{1, 2, GL_FLOAT, 8 * sizeof(float), 2 * sizeof(float)},
			   vertex_attribute{2, 4, GL_FLOAT, 8 * sizeof(float), 4 * sizeof(float)}
	});

    g.MenuChangeTimer = 1.0f;
	g.MenuChangeSequence = CreateSequence(tweenState);
    g.MenuChangeSequence->Tweens.push_back(
        tween(&g.MenuChangeTimer)
        .SetFrom(0.0f)
        .SetTo(1.0f)
        .SetDuration(0.25f)
        .SetEasing(TweenEasing_Linear)
    );

    g.HorizontalAxis.Type = Action_horizontal;
    g.VerticalAxis.Type = Action_vertical;
}

void Begin(gui &g, camera &Camera, float emission = 0.0f)
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

gui_draw_command NextDrawCommand(gui &g, GLuint PrimType, float TexWeight, color DiffuseColor, texture *Texture)
{
    gui_draw_command Command{DiffuseColor, PrimType, g.Buffer.VertexCount, 0, TexWeight, Texture};
    return Command;
}

void PushDrawCommand(gui &g)
{
    if (!g.Buffer.VertexCount) return;

    auto &Curr = g.CurrentDrawCommand;
    Curr.Count = g.Buffer.VertexCount - Curr.Offset;
    g.DrawCommandList.push_back(Curr);
}

inline size_t PushVertex(vertex_buffer &buf, const gui_vertex &v)
{
	return PushVertex(buf, v.Data);
}

void DrawLine(gui &g, vec2 p1, vec2 p2, color c)
{
    auto &curr = g.CurrentDrawCommand;
    if (curr.PrimitiveType != GL_LINES)
    {
        PushDrawCommand(g);
        g.CurrentDrawCommand = NextDrawCommand(g, GL_LINES, 0.0f, Color_white, NULL);
    }

    PushVertex(g.Buffer, gui_vertex{p1.x, p1.y, 0,0, c.r, c.g, c.b, c.a});
    PushVertex(g.Buffer, gui_vertex{p2.x, p2.y, 0,0, c.r, c.g, c.b, c.a});
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
        PushVertex(g.Buffer, gui_vertex{x,   y,   u, v,   C.r, C.g, C.b, C.a});
        PushVertex(g.Buffer, gui_vertex{x+w, y,   u2, v,  C.r, C.g, C.b, C.a});
        PushVertex(g.Buffer, gui_vertex{x+w, y+h, u2, v2, C.r, C.g, C.b, C.a});
        PushVertex(g.Buffer, gui_vertex{x,   y+h, u,  v2, C.r, C.g, C.b, C.a});
        break;

    case GL_TRIANGLES:
        PushVertex(g.Buffer, gui_vertex{x,   y,   u, v,   C.r, C.g, C.b, C.a});
        PushVertex(g.Buffer, gui_vertex{x,   y+h, u,  v2, C.r, C.g, C.b, C.a});
        PushVertex(g.Buffer, gui_vertex{x+w, y,   u2, v,  C.r, C.g, C.b, C.a});
        PushVertex(g.Buffer, gui_vertex{x,   y+h, u,  v2, C.r, C.g, C.b, C.a});
        PushVertex(g.Buffer, gui_vertex{x+w, y+h, u2, v2, C.r, C.g, C.b, C.a});
        PushVertex(g.Buffer, gui_vertex{x+w, y,   u2, v,  C.r, C.g, C.b, C.a});
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

uint32 DrawCircle(gui &g, vec2 center, float radius, color c, GLuint PrimType)
{
	PushDrawCommand(g);
	g.CurrentDrawCommand = NextDrawCommand(g, PrimType, 0, Color_white, NULL);

	if (PrimType == GL_TRIANGLE_FAN)
	{
		PushVertex(g.Buffer, gui_vertex{ center.x, center.y, 0, 0, c.r, c.g, c.b, c.a });
	}

	const int segments = 24;
	const float theta = M_PI * 2 / float(segments);
	float angle = 0;
	for (int i = 0; i < segments+1; i++)
	{
		vec2 p;
		p.x = cos(angle) * radius;
		p.y = sin(angle) * radius;
		p += center;

		PushVertex(g.Buffer, gui_vertex{ p.x, p.y, 0, 0, c.r, c.g, c.b, c.a });

		angle += theta;
	}
	return GUIElementState_none;
}

uint32 DrawPolygon(gui &g, vec2 *Vertices, size_t Count, color c, GLuint PrimType = GL_LINE_LOOP)
{
	PushDrawCommand(g);
	g.CurrentDrawCommand = NextDrawCommand(g, PrimType, 0, Color_white, NULL);

	for (size_t i = 0; i < Count; i++)
	{
		vec2 v = Vertices[i];
		PushVertex(g.Buffer, gui_vertex{ v.x, v.y, 0, 0, c.r, c.g, c.b, c.a });
	}

	return GUIElementState_none;
}

uint32 DrawTexture(gui &g, rect R, texture *T, bool FlipV = false, GLuint PrimType = GL_TRIANGLES, bool CheckState = true)
{
    return DrawRect(g, R, Color_white, Color_white, T, {0, 0, 1, 1}, 1, FlipV, PrimType, CheckState);
}

vec2 MeasureText(bitmap_font *font, const char *text)
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

uint32 DrawText(gui &g, bitmap_font *Font, const char *text, rect &r, color c, bool CheckState = true)
{
    int CurX = r.X;
    int CurY = r.Y;
    for (size_t i = 0; text[i]; i++)
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

uint32 DrawTextPtr(gui &g, bitmap_font *Font, const char *text, rect &r, color c, vec2 *ptr, bool CheckState = true)
{
    uint32 result = DrawText(g, Font, text, r, c, CheckState);
    ptr->x += r.Width;
    ptr->y += r.Height;
    return result;
}

uint32 DrawTextCentered(gui &g, bitmap_font *font, const char *text, rect r, color c, bool checkState = true)
{
    vec2 size = MeasureText(font, text);
    r.X -= size.x/2;
    r.Y -= size.y/2;
    return DrawText(g, font, text, r, c, checkState);
}

void MeasureTextGroup(text_group *group)
{
    group->Size = {0, 0};
    for (size_t i = 0; i < group->Items.size(); i++)
    {
        auto item = &group->Items[i];
        item->Size = MeasureText(item->Font, item->Text);
        group->Size.x += item->Size.x;
        group->Size.y = std::max(group->Size.y, item->Size.y);
    }
}

void DrawTextGroupCentered(gui &g, text_group *group, rect r, color multiplyColor)
{
    MeasureTextGroup(group);
    r.X -= group->Size.x/2;
    r.Y -= group->Size.y/2;
    vec2 ptr = {r.X, r.Y};
    for (size_t i = 0; i < group->Items.size(); i++)
    {
		auto item = &group->Items[i];
        DrawTextPtr(g, item->Font, item->Text, r, item->Color * multiplyColor, &ptr);
        r.X = ptr.x;
        r.Y = ptr.y;
    }
}

void End(gui &g)
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

static color textColor = Color_white;
static color textSelectedColor = IntColor(ShipPalette.Colors[SHIP_ORANGE]);

void DrawMenuItem(game_main *g, bitmap_font *font, menu_item &item,
                  float centerX, float baseY, bool selected = false,
                  float changeTimer = 0.0f, float alpha = 1.0f)
{
    float textPadding = GetRealPixels(g, 10.0f);
    color col = textColor;
    if (selected)
    {
        col = Lerp(col, changeTimer, textSelectedColor);
    }
    if (!item.Enabled)
    {
        col.a = 0.25f;
    }
    col.a *= alpha;

	const float itemWidth = 500;
	const float optionWidth = 250;
	const float optionX = centerX + itemWidth / 2;
	switch (item.Type)
	{
	case MenuItemType_Text:
		DrawTextCentered(g->GUI, font, item.Text, rect{ centerX,baseY + textPadding,0,0 }, col);
		break;

	case MenuItemType_SliderFloat:
	{
		const float lineWidth = optionWidth;
		const float lineX = optionX;
		const float lineY = baseY + textPadding + 24 / 2;
		const float lineLeft = lineX - lineWidth / 2;
		const float lineRight = lineX + lineWidth / 2;

		DrawTextCentered(g->GUI, font, item.Text, rect{ centerX - itemWidth / 2, baseY + textPadding,0,0 }, col);
		DrawLine(g->GUI, vec2{ lineLeft, lineY }, vec2{ lineRight, lineY }, col);
		DrawCircle(g->GUI, vec2{ lineLeft + (lineWidth * *item.SliderFloat.Value), lineY }, 10, col, GL_TRIANGLE_FAN);
		break;
	}

	case MenuItemType_Switch:
	{
		DrawTextCentered(g->GUI, font, item.Text, rect{ centerX - itemWidth / 2, baseY + textPadding,0,0 }, col);

		color onColor = col;
		color offColor = col;
		if (*item.Switch.Value)
		{
			offColor.a = 0.25f;
		}
		else
		{
			onColor.a = 0.25f;
		}
		DrawTextCentered(g->GUI, font, "on", rect{ centerX + itemWidth/2 - optionWidth/4, baseY + textPadding, 0, 0 }, onColor);
		DrawTextCentered(g->GUI, font, "off", rect{ centerX + itemWidth/2 + optionWidth/4, baseY + textPadding, 0, 0 }, offColor);
		break;
	}

	case MenuItemType_Options:
	{
		DrawTextCentered(g->GUI, font, item.Text, rect{ centerX - itemWidth / 2, baseY + textPadding,0,0 }, col);
		DrawTextCentered(g->GUI, font, item.Options.Values[item.Options.SelectedIndex], rect{ centerX + itemWidth / 2, baseY + textPadding, 0, 0 }, col);
		break;
	}
	}
}

static menu_item backItem = {"BACK", MenuItemType_Text, true};

void DrawHeader(game_main *g, const char *text, color c, float alpha = 1.0f)
{
    static auto titleFont = FindBitmapFont(g->AssetLoader, "unispace_48", "main_assets");

    float halfX = g->Width*0.5f;
    float halfY = g->Height*0.5f;
    float baseY = 660.0f;
    float lineWidth = g->Width*0.75f;

    color _textColor = c;
    _textColor.a *= alpha;

    DrawTextCentered(g->GUI, titleFont, text, rect{halfX, GetRealPixels(g, baseY), 0, 0}, _textColor);
    DrawLine(g->GUI, vec2{halfX - lineWidth*0.5f,GetRealPixels(g,640.0f)}, vec2{halfX + lineWidth*0.5f,GetRealPixels(g,640.0f)}, _textColor);
}

void DrawContainerPolygon(game_main *g, color c, rect &r)
{
	float halfX = g->Width*0.5f;
	float halfY = g->Height*0.5f;
	float width = g->Width*0.75f;
	float height = g->Height*0.5f;
	float borderRadius = GetRealPixels(g, 100);

	static vec2 vertices[] = {
		{ halfX - width/2, halfY - height/2 },
		{ halfX + width / 2 - borderRadius, halfY - height / 2 },
		{ halfX + width / 2, halfY + height / 2 },
		{ halfX - width / 2 + borderRadius, halfY + height / 2 },
	};
	DrawPolygon(g->GUI, vertices, ARRAY_COUNT(vertices), c, GL_TRIANGLE_FAN);

	r.X = halfX - width / 2;
	r.Y = halfY - height / 2;
	r.Width = width;
	r.Height = height;
}

void DrawMenu(game_main *g, menu_data &menu, float changeTimer, float alpha = 1.0f, bool drawBackItem = false)
{
    static auto titleFont = FindBitmapFont(g->AssetLoader, "unispace_48", "main_assets");
    static auto menuItemFont = FindBitmapFont(g->AssetLoader, "unispace_24", "main_assets");

    float halfX = g->Width*0.5f;
    float halfY = g->Height*0.5f;
    float baseY = 660.0f;
    float lineWidth = g->Width*0.75f;

    color _textColor = textColor;
    _textColor.a *= alpha;

    DrawRect(g->GUI, rect{0,0, static_cast<float>(g->Width), static_cast<float>(g->Height)}, color{0,0,0,0.5f * alpha});
    DrawHeader(g, menu.Title, textColor, alpha);

    float height = g->Height*0.05f;
    baseY = GetRealPixels(g, baseY);
    baseY -= height + GetRealPixels(g, 40.0f);
    for (int i = 0; i < menu.NumItems; i++)
    {
        auto &item = menu.Items[i];
        bool selected = (i == menu.HotItem);
        DrawMenuItem(g, menuItemFont, item, halfX, baseY, selected, changeTimer, alpha);
        baseY -= height + GetRealPixels(g, 10.0f);
    }

    if (drawBackItem)
    {
        bool backSelected = menu.HotItem == menu.NumItems;
        DrawMenuItem(g, menuItemFont, backItem, halfX, baseY, backSelected, changeTimer, alpha);
    }
}

float GetMenuAxisValue(game_input &input, menu_axis &axis, float dt)
{
    const float moveInterval = 0.05f;
    float value = GetAxisValue(input, (action_type)axis.Type);

    if (value == 0.0f)
    {
        axis.Timer += dt;
    }
    else if (axis.Timer >= moveInterval)
    {
        axis.Timer = 0.0f;
        return value;
    }
    return 0.0f;
}

void UpdateMenuItem(menu_item &item, float inputX, float menuInputX)
{
	switch (item.Type)
	{
	case MenuItemType_SliderFloat:
	{
		float value = *item.SliderFloat.Value;
		value += inputX * 0.02f;
		value = glm::clamp(value, 0.0f, 1.0f);
		*item.SliderFloat.Value = value;
		break;
	}

	case MenuItemType_Switch:
	{
		bool value = *item.Switch.Value;
		if (inputX > 0)
		{
			value = false;
		}
		else if (inputX < 0)
		{
			value = true;
		}
		*item.Switch.Value = value;
		break;
	}

	case MenuItemType_Options:
	{
		if (menuInputX > 0)
		{
			item.Options.SelectedIndex++;
		}
		else if (menuInputX < 0)
		{
			item.Options.SelectedIndex--;
		}
		item.Options.SelectedIndex = glm::clamp(item.Options.SelectedIndex, 0, item.Options.Values.size() - 1);
		break;
	}
	}
}

bool UpdateMenuSelection(game_main *g, menu_data &menu, float inputX, float menuInputX, float menuInputY, bool backItemEnabled = false)
{
    int prevHotItem = menu.HotItem;
    auto item = menu.Items + menu.HotItem;
    do {
        if (menuInputY < 0.0f)
        {
            menu.HotItem++;
        }
        else if (menuInputY > 0.0f)
        {
            menu.HotItem--;
        }

        int max = menu.NumItems;
        if (!backItemEnabled)
        {
            max -= 1;
        }
        menu.HotItem = glm::clamp(menu.HotItem, 0, max);
        item = menu.Items + menu.HotItem;
    } while (!item->Enabled);

    if (prevHotItem != menu.HotItem)
    {
        PlaySequence(g->TweenState, g->GUI.MenuChangeSequence, true);
    }

    bool backSelected = menu.HotItem == menu.NumItems;
	if (!backSelected)
	{
		UpdateMenuItem(menu.Items[menu.HotItem], inputX, menuInputX);
	}

    if (IsJustPressed(g, Action_select))
    {
        if (backSelected)
        {
            return true;
        }
        else if (menu.SelectFunc)
        {
            menu.SelectFunc(g, &menu, menu.HotItem);
        }
    }
    return false;
}
