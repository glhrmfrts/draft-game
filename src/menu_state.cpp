// Copyright

void UpdateCameraToPlayer(camera &cam, entity *playerEntity, float dt)
{
    const float CAMERA_LERP = 10.0f;
    vec3 prevPosition = cam.Position - vec3{0, Global_Camera_OffsetY, Global_Camera_OffsetZ};
    vec3 d = playerEntity->Pos() - prevPosition;
    cam.Position += d * CAMERA_LERP * dt;
    cam.LookAt = cam.Position + vec3{0, -Global_Camera_OffsetY + Global_Camera_LookYOffset, -Global_Camera_OffsetZ + Global_Camera_LookZOffset};
}

void InitMenu(game_main *g)
{
    auto m = &g->MenuState;

    g->State = GameState_Menu;
    InitWorldCommonEntities(g->World, &g->AssetLoader, &g->Camera);

    m->SubMenuChangeTimer = 1.0f;
    m->SubMenuChangeSequence.Tweens.push_back(
        tween(&m->SubMenuChangeTimer)
        .SetFrom(0.0f)
        .SetTo(1.0f)
        .SetDuration(0.25f)
        .SetEasing(TweenEasing_Linear)
    );
    AddSequences(g->TweenState, &m->SubMenuChangeSequence, 1);

    m->HorizontalAxis.Type = Action_horizontal;
    m->VerticalAxis.Type = Action_vertical;
}

static float GetMenuAxisValue(game_input &input, menu_axis &axis, float dt)
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

#define MENU_FUNC(name) void name(game_main *g, int itemIndex)
typedef MENU_FUNC(menu_func);

MENU_FUNC(PlayMenuCallback)
{
    switch (itemIndex)
    {
    case 0: // classic mode
        InitLevel(g);
        break;
    }
}

enum menu_item_type
{
    MenuItemType_Text,
};
struct menu_item
{
    const char *Text;
    menu_item_type Type;
    bool Enabled = true;
};

static struct {const char *Text; rect Pos;} mainMenuTexts[] = {
    {"PLAY", rect{292,32,0,0}},
    {"OPTS", rect{464,32,0,0}},
    {"CRED", rect{816,32,0,0}},
    {"EXIT", rect{992,32,0,0}}
};
static struct
{
    int NumItems;
    menu_func *SelectFunc; // function called when an item in the submenu is selected
    menu_item Items[4];
    int HotItem=0;
} subMenus[] = {
    {2, PlayMenuCallback,
     {
         menu_item{"CLASSIC MODE", MenuItemType_Text, true},
         menu_item{"SCORE MODE", MenuItemType_Text, false}
     }
    },
};

void UpdateMenu(game_main *g, float dt)
{
    auto m = &g->MenuState;
    float moveX = GetMenuAxisValue(g->Input, m->HorizontalAxis, dt);
    float moveY = GetMenuAxisValue(g->Input, m->VerticalAxis, dt);
    g->World.PlayerEntity->Vel().y = PLAYER_MIN_VEL;
    g->World.PlayerEntity->Pos().y += g->World.PlayerEntity->Vel().y * dt;
    UpdateLogiclessEntities(g->World, dt);
    UpdateCameraToPlayer(g->Camera, g->World.PlayerEntity, dt);

    if (m->SelectedMainMenu == -1)
    {
        if (moveX > 0.0f)
        {
            m->HotMainMenu++;
        }
        else if (moveX < 0.0f)
        {
            m->HotMainMenu--;
        }
        m->HotMainMenu = glm::clamp(m->HotMainMenu, 0, (int)ARRAY_COUNT(mainMenuTexts) - 1);

        if (IsJustPressed(g, Action_select))
        {
            if (m->HotMainMenu < ARRAY_COUNT(mainMenuTexts) - 1)
            {
                m->SelectedMainMenu = m->HotMainMenu;
            }
            else
            {
                g->Running = false;
            }
        }
    }
    else
    {
        auto &subMenu = subMenus[m->SelectedMainMenu];
        int prevHotItem = subMenu.HotItem;
        auto item = subMenu.Items + subMenu.HotItem;
        do {
            if (moveY < 0.0f)
            {
                subMenu.HotItem++;
            }
            else if (moveY > 0.0f)
            {
                subMenu.HotItem--;
            }
            subMenu.HotItem = glm::clamp(subMenu.HotItem, 0, subMenu.NumItems);
            item = subMenu.Items + subMenu.HotItem;
        } while (!item->Enabled);
        
        if (prevHotItem != subMenu.HotItem)
        {
            PlaySequence(g->TweenState, &m->SubMenuChangeSequence, true);
        }

        bool backSelected = subMenu.HotItem == subMenu.NumItems;
        if (IsJustPressed(g, Action_select))
        {
            if (backSelected)
            {
                m->SelectedMainMenu = -1;
            }
            else
            {
                subMenu.SelectFunc(g, subMenu.HotItem);
            }
        }
    }
}

static menu_item backItem = {"BACK", MenuItemType_Text};
static color textColor = Color_white;
static color textSelectedColor = IntColor(ShipPalette.Colors[SHIP_ORANGE]);

static void DrawSubMenu(game_main *g, bitmap_font *font, menu_item &item,
                        float centerX, float baseY, bool selected = false,
                        float changeTimer = 0.0f)
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
    DrawTextCentered(g->GUI, font, item.Text, rect{centerX,baseY + textPadding,0,0}, col);
}

void RenderMenu(game_main *g, float dt)
{
    static auto mainMenuFont = FindBitmapFont(g->AssetLoader, "unispace_32");
    static auto subMenuFont = FindBitmapFont(g->AssetLoader, "unispace_24");
    auto m = &g->MenuState;

    UpdateProjectionView(g->Camera);
    PostProcessBegin(g->RenderState);
    RenderBegin(g->RenderState, dt);
    RenderEntityWorld(g->RenderState, g->World, dt);
    RenderEnd(g->RenderState, g->Camera);
    PostProcessEnd(g->RenderState);

    UpdateProjectionView(g->GUICamera);
    Begin(g->GUI, g->GUICamera);
    if (m->SelectedMainMenu == -1)
    {
        for (int i = 0; i < ARRAY_COUNT(mainMenuTexts); i++)
        {
            auto &item = mainMenuTexts[i];
            color col = textColor;
            if (i == m->HotMainMenu)
            {
                col = textSelectedColor;
            }
            DrawTextCentered(g->GUI, mainMenuFont, item.Text, GetRealPixels(g, item.Pos), col);
        }
    }
    else
    {
        auto &subMenu = subMenus[m->SelectedMainMenu];
        auto &item = mainMenuTexts[m->SelectedMainMenu];
        float halfX = g->Width*0.5f;
        float halfY = g->Height*0.5f;
        float baseY = 660.0f;
        float lineWidth = g->Width*0.75f;
        DrawRect(g->GUI, rect{0,0, static_cast<float>(g->Width), static_cast<float>(g->Height)}, color{0,0,0,0.5f});
        DrawTextCentered(g->GUI, mainMenuFont, item.Text, rect{halfX, GetRealPixels(g, baseY), 0, 0}, textColor);
        DrawLine(g->GUI, vec2{halfX - lineWidth*0.5f,GetRealPixels(g,640.0f)}, vec2{halfX + lineWidth*0.5f,GetRealPixels(g,640.0f)}, textColor);

        float height = g->Height*0.05f;
        baseY = GetRealPixels(g, baseY);
        baseY -= height + GetRealPixels(g, 40.0f);
        for (int i = 0; i < subMenu.NumItems; i++)
        {
            auto &item = subMenu.Items[i];
            bool selected = (i == subMenu.HotItem);
            DrawSubMenu(g, subMenuFont, item, halfX, baseY, selected, m->SubMenuChangeTimer);
            baseY -= height + GetRealPixels(g, 10.0f);
        }
        bool backSelected = subMenu.HotItem == subMenu.NumItems;
        DrawSubMenu(g, subMenuFont, backItem, halfX, baseY, backSelected, m->SubMenuChangeTimer);
    }
    End(g->GUI);
}
