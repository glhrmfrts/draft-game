// Copyright

void UpdateCameraToPlayer(camera &cam, entity *playerEntity, float dt)
{
    const float CAMERA_LERP = 10.0f;
    vec3 prevPosition = cam.Position - vec3{0, Global_Camera_OffsetY, Global_Camera_OffsetZ};
    vec3 d = playerEntity->Pos() - prevPosition;
    cam.Position += d * CAMERA_LERP * dt;
    cam.LookAt = cam.Position + vec3{0, -Global_Camera_OffsetY + Global_Camera_LookYOffset, -Global_Camera_OffsetZ + Global_Camera_LookZOffset};
}

void UpdateFinalCamera(game_main *g)
{
    auto &fc = g->FinalCamera;
    auto &c = g->Camera;
    fc.Position = WorldToRenderTransform(c.Position);
    fc.LookAt = WorldToRenderTransform(c.LookAt);

    if (fc.LookAt.y < fc.Position.y)
    {
        fc.Up = vec3{0,0,-1};
    }
    else
    {
        fc.Up = vec3{0,0,1};
    }
    UpdateProjectionView(fc);
}

void InitMenu(game_main *g)
{
    g->State = GameState_Menu;
    InitWorldCommonEntities(g->World, &g->AssetLoader, &g->Camera);
    
    auto m = &g->MenuState;
    if (m->FadeOutSequence.Tweens.size() == 0)
    {
        m->FadeOutSequence.Tweens.push_back(
            tween(&m->Alpha)
                .SetFrom(1.0f)
                .SetTo(0.0f)
                .SetDuration(1.0f)
                .SetEasing(TweenEasing_Linear)
        );
        AddSequences(g->TweenState, &m->FadeOutSequence, 1);
    }
    
    m->Alpha = 1.0f;
    m->FadeOutSequence.Complete = false;
}

MENU_FUNC(PlayMenuCallback)
{
    switch (itemIndex)
    {
    case 0: // classic mode
    {
        PlaySequence(g->TweenState, &g->MenuState.FadeOutSequence, true);
        break;
    }
    }
}

static rect mainMenuPositions[] = {
    rect{292,32,0,0},
    rect{464,32,0,0},
    rect{816,32,0,0},
    rect{992,32,0,0}
};
static menu_data menus[] = {
    {"PLAY", 2, PlayMenuCallback,
     {
         menu_item{"CLASSIC MODE", MenuItemType_Text, true},
         menu_item{"SCORE MODE", MenuItemType_Text, false}
     }
    },
    {"OPTS", 0, NULL, {}},
    {"CRED", 0, NULL, {}},
    {"EXIT", 0, NULL, {}},
};

void UpdateMenu(game_main *g, float dt)
{
    auto m = &g->MenuState;
    float moveX = GetMenuAxisValue(g->Input, g->GUI.HorizontalAxis, dt);
    float moveY = GetMenuAxisValue(g->Input, g->GUI.VerticalAxis, dt);
    g->World.PlayerEntity->Vel().y = PLAYER_MIN_VEL;
    g->World.PlayerEntity->Pos().y += g->World.PlayerEntity->Vel().y * dt;
    UpdateLogiclessEntities(g->World, dt);
    UpdateCameraToPlayer(g->Camera, g->World.PlayerEntity, dt);

    if (m->SelectedMainMenu == -1)
    {
        float prevHotMenu = m->HotMainMenu;
        if (moveX > 0.0f)
        {
            m->HotMainMenu++;
        }
        else if (moveX < 0.0f)
        {
            m->HotMainMenu--;
        }
        m->HotMainMenu = glm::clamp(m->HotMainMenu, 0, (int)ARRAY_COUNT(menus) - 1);
        if (prevHotMenu != m->HotMainMenu)
        {
            PlaySequence(g->TweenState, &g->GUI.MenuChangeSequence, true);
        }

        if (IsJustPressed(g, Action_select))
        {
            if (m->HotMainMenu < ARRAY_COUNT(menus) - 1)
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
        auto &menu = menus[m->SelectedMainMenu];
        bool back = UpdateMenuSelection(g, menu, moveY, /*backItemEnabled=*/true);
        if (back)
        {
            m->SelectedMainMenu = -1;
        }
    }
    
    if (m->FadeOutSequence.Complete)
    {
        InitLevel(g);
    }
}

void RenderMenu(game_main *g, float dt)
{
    static auto mainMenuFont = FindBitmapFont(g->AssetLoader, "unispace_32");
    auto m = &g->MenuState;

    UpdateProjectionView(g->Camera);
    PostProcessBegin(g->RenderState);
    RenderBackground(g, g->World);
    RenderBegin(g->RenderState, dt);
    RenderEntityWorld(g->RenderState, g->World, dt);
    UpdateFinalCamera(g);
    RenderEnd(g->RenderState, g->FinalCamera);
    PostProcessEnd(g->RenderState);

    UpdateProjectionView(g->GUICamera);
    Begin(g->GUI, g->GUICamera);
    if (m->SelectedMainMenu == -1)
    {
        for (int i = 0; i < ARRAY_COUNT(menus); i++)
        {
            auto &item = menus[i];
            color col = textColor;
            if (i == m->HotMainMenu)
            {
                col = Lerp(col, g->GUI.MenuChangeTimer, textSelectedColor);
            }
            DrawTextCentered(g->GUI, mainMenuFont, item.Title, GetRealPixels(g, mainMenuPositions[i]), col);
        }
    }
    else
    {
        auto &subMenu = menus[m->SelectedMainMenu];
        DrawMenu(g, subMenu, g->GUI.MenuChangeTimer, m->Alpha, true);
    }
    End(g->GUI);
}
