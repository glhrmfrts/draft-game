// Copyright

#define ROAD_LANE_WIDTH   2
#define LEVEL_PLANE_COUNT 5
#define SHIP_Z            0.2f

static void UpdateCameraToPlayer(camera &cam, entity *playerEntity, float dt)
{
    const float CAMERA_LERP = 10.0f;
    vec3 d = playerEntity->Pos() - cam.LookAt;
    cam.Position += d * CAMERA_LERP * dt;
    cam.LookAt = cam.Position + vec3{0, -Global_Camera_OffsetY, -Global_Camera_OffsetZ};
}

static void InitMenu(game_main *g)
{
    auto m = &g->MenuState;

    g->State = GameState_Menu;
    FreeArena(g->World.Arena);

    g->World.Camera = &g->Camera;
    g->PlayerEntity = CreateShipEntity(&g->World.Arena, GetShipMesh(g), Color_blue, IntColor(FirstPalette.Colors[1]), true);
    g->PlayerEntity->Transform.Position.z = SHIP_Z;
    g->PlayerEntity->Transform.Velocity.y = PLAYER_MIN_VEL;
    AddEntity(g->World, g->PlayerEntity);

    for (int i = 0; i < LEVEL_PLANE_COUNT; i++)
    {
        auto ent = PushStruct<entity>(g->World.Arena);
        ent->Transform.Position.y = LEVEL_PLANE_SIZE * i;
        ent->Transform.Position.z = -0.25f;
        ent->Transform.Scale = vec3{LEVEL_PLANE_SIZE, LEVEL_PLANE_SIZE, 0};
        ent->Model = CreateModel(&g->World.Arena, GetFloorMesh(g));
        ent->Repeat = PushStruct<entity_repeat>(g->World.Arena);
        ent->Repeat->Count = LEVEL_PLANE_COUNT;
        ent->Repeat->Size = LEVEL_PLANE_SIZE;
        ent->Repeat->DistanceFromCamera = LEVEL_PLANE_SIZE/2;
        AddEntity(g->World, ent);
    }
    for (int i = 0; i < LEVEL_PLANE_COUNT; i++)
    {
        auto ent = PushStruct<entity>(g->World.Arena);
        ent->Transform.Position.y = LEVEL_PLANE_SIZE * i;
        ent->Transform.Position.z = 0.0f;
        ent->Transform.Scale = vec3{ 2, LEVEL_PLANE_SIZE, 1 };
        ent->Model = CreateModel(&g->World.Arena, GetRoadMesh(g));
        ent->Repeat = PushStruct<entity_repeat>(g->World.Arena);
        ent->Repeat->Count = LEVEL_PLANE_COUNT;
        ent->Repeat->Size = LEVEL_PLANE_SIZE;
        ent->Repeat->DistanceFromCamera = LEVEL_PLANE_SIZE / 2;
        AddEntity(g->World, ent);
    }

    m->SubMenuChangeSequence.Tweens.push_back(
        tween{&m->SubMenuChangeTimer, 0.0f, 1.0f, 0.25f, 1.0f, TweenEasing_Linear}
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
};

static struct {const char *Text; rect Pos;} mainMenuTexts[] = {
    {"PLAY", rect{292,32,0,0}},
    {"OPTS", rect{464,32,0,0}},
    {"CRED", rect{816,32,0,0}},
    {"QUIT", rect{992,32,0,0}}
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
         menu_item{"CLASSIC MODE", MenuItemType_Text},
         menu_item{"SCORE MODE", MenuItemType_Text}
     }
    },
};
static menu_item backItem = {"BACK", MenuItemType_Text};
static color textColor = Color_white;
static color textSelectedColor = IntColor(FirstPalette.Colors[3]);

static void DrawSubMenu(game_main *g, bitmap_font *font, menu_item &item, float baseY, float width, float height, bool selected = false, float changeTimer = 0.0f)
{
    float textPadding = GetRealPixels(g, 10.0f);
    color bgColor = IntColor(FirstPalette.Colors[2], 0.5f);
    color bgSelectedColor = IntColor(FirstPalette.Colors[3], 1.0f);
    if (selected)
    {
        bgColor = Lerp(bgColor, changeTimer, bgSelectedColor);
    }
    DrawRect(g->GUI, rect{0,baseY,width,height}, bgColor);
    DrawText(g->GUI, font, item.Text, rect{textPadding,baseY + textPadding,0,0}, textColor);
}

static void UpdateMenu(game_main *g, float dt)
{
    auto m = &g->MenuState;
    float moveX = GetMenuAxisValue(g->Input, m->HorizontalAxis, dt);
    float moveY = GetMenuAxisValue(g->Input, m->VerticalAxis, dt);
    g->PlayerEntity->Vel().y = PLAYER_MIN_VEL;
    g->PlayerEntity->Pos().y += g->PlayerEntity->Vel().y * dt;
    UpdateLogiclessEntities(g->World, dt);
    UpdateCameraToPlayer(g->Camera, g->PlayerEntity, dt);

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
        if (moveY < 0.0f)
        {
            subMenu.HotItem++;
        }
        else if (moveY > 0.0f)
        {
            subMenu.HotItem--;
        }
        subMenu.HotItem = glm::clamp(subMenu.HotItem, 0, subMenu.NumItems);
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

static void RenderMenu(game_main *g, float dt)
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
        float width = g->Width*0.33f;
        float height = g->Height*0.075f;
        float baseY = 660.0f;
        auto &item = mainMenuTexts[m->SelectedMainMenu];
        DrawText(g->GUI, mainMenuFont, item.Text, GetRealPixels(g, rect{10,660,0,0}), textColor);
        DrawLine(g->GUI, vec2{0,GetRealPixels(g,650.0f)}, vec2{width,GetRealPixels(g,650.0f)}, textColor);

        baseY = GetRealPixels(g, baseY);
        baseY -= height + GetRealPixels(g, 20.0f);
        for (int i = 0; i < subMenu.NumItems; i++)
        {
            auto &item = subMenu.Items[i];
            bool selected = (i == subMenu.HotItem);
            DrawSubMenu(g, subMenuFont, item, baseY, width, height, selected, m->SubMenuChangeTimer);

            baseY -= height + GetRealPixels(g, 10.0f);
        }
        bool backSelected = subMenu.HotItem == subMenu.NumItems;
        DrawSubMenu(g, subMenuFont, backItem, baseY, width, height, backSelected, m->SubMenuChangeTimer);
    }
    End(g->GUI);
}
