// Copyright

MENU_FUNC(PlayMenuCallback)
{
	switch (itemIndex)
	{
	case 0: // campaign mode
	{
		PlaySequence(g->TweenState, &g->MenuState.FadeOutSequence, true);
		break;
	}
	}
}

MENU_FUNC(OptsMenuCallback)
{
	switch (itemIndex)
	{
	case 1:
		g->MenuState.Screen = MenuScreen_OptsGraphics;
		break;

	case 2:
		g->MenuState.Screen = MenuScreen_OptsAudio;
		break;
	}
}

static rect MainMenuPositions[] = {
	rect{ 292,32,0,0 },
	rect{ 464,32,0,0 },
	rect{ 816,32,0,0 },
	rect{ 992,32,0,0 }
};
static menu_data Menus[] = {
	{ "PLAY", 2, PlayMenuCallback,
	{
		{ "CAMPAIGN MODE", MenuItemType_Text, true },
		{ "INFINITE MODE", MenuItemType_Text, false }
	}
	},
	{ "OPTS", 3, OptsMenuCallback,{
		{ "GAME", MenuItemType_Text, true },
		{ "GRAPHICS", MenuItemType_Text, true },
		{ "AUDIO", MenuItemType_Text, true }
	} },
	{ "CRED", 0, NULL,{} },
	{ "EXIT", 0, NULL,{} },
};

static menu_screen Screens[] = {
	MenuScreen_Play,
	MenuScreen_Opts,
	MenuScreen_Cred,
	MenuScreen_Exit,
};

static menu_data *PlayMenu = &Menus[0];
static menu_data *OptsMenu = &Menus[1];
static menu_data *CredMenu = &Menus[2];
static menu_data *ExitMenu = &Menus[3];

static menu_data GraphicsMenu = {
	"GRAPHICS", 4, NULL, {
		{ "RESOLUTION", MenuItemType_Options, true },
		{ "FULLSCREEN", MenuItemType_Switch, true },
		{ "AA", MenuItemType_Switch, true },
		{ "BLOOM", MenuItemType_Switch, true },
	}
};

static menu_data AudioMenu = {
	"AUDIO", 2, NULL,{
		{ "MUSIC", MenuItemType_SliderFloat, true },
		{ "SFX",   MenuItemType_SliderFloat, true },
	}
};

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
    fc.Position = WorldToRenderTransform(c.Position, g->RenderState.BendRadius);
    fc.LookAt = WorldToRenderTransform(c.LookAt, g->RenderState.BendRadius);

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

	auto sng = FindSong(g->AssetLoader, "first_song");
	MusicMasterLoadSong(g->MusicMaster, sng);
	MusicMasterPlayTrack(g->MusicMaster, sng->Names["bg_amb"]);
	MusicMasterPlayTrack(g->MusicMaster, sng->Names["pad_dmaj"]);

	g->Options = FindOptions(g->AssetLoader, "options");
	
	GraphicsMenu.Items[0].Options.SelectedIndex = g->Options->Values["graphics/resolution"]->Int;
	for (int i = 0; i < ARRAY_COUNT(Global_Resolutions); i++)
	{
		GraphicsMenu.Items[0].Options.Values.Add(Global_ResolutionTexts[i]);
	}

	GraphicsMenu.Items[1].Switch.Value = &g->Options->Values["graphics/fullscreen"]->Bool;
	GraphicsMenu.Items[2].Switch.Value = &g->Options->Values["graphics/aa"]->Bool;
	GraphicsMenu.Items[3].Switch.Value = &g->Options->Values["graphics/bloom"]->Bool;

	AudioMenu.Items[0].SliderFloat.Min = 0;
	AudioMenu.Items[0].SliderFloat.Max = 1;
	AudioMenu.Items[0].SliderFloat.Value = &g->Options->Values["audio/music_gain"]->Float;

	AudioMenu.Items[1].SliderFloat.Min = 0;
	AudioMenu.Items[1].SliderFloat.Max = 1;
	AudioMenu.Items[1].SliderFloat.Value = &g->Options->Values["audio/sfx_gain"]->Float;
}

void SaveOptions(game_main *g)
{
	AddAssetEntry(
		g->AssetLoader,
		CreateAssetEntry(
			AssetEntryType_OptionsSave,
			"options.json",
			"options",
			(void *)g->Options
		),
		true,
		true
	);
}

void UpdateMenu(game_main *g, float dt)
{
    auto m = &g->MenuState;
    float moveX = GetMenuAxisValue(g->Input, g->GUI.HorizontalAxis, dt);
    float moveY = GetMenuAxisValue(g->Input, g->GUI.VerticalAxis, dt);
    g->World.PlayerEntity->Vel().y = PLAYER_MIN_VEL;
    g->World.PlayerEntity->Pos().y += g->World.PlayerEntity->Vel().y * dt;

    UpdateLogiclessEntities(g->World, dt);
    UpdateCameraToPlayer(g->Camera, g->World.PlayerEntity, dt);
    UpdateGenState(g, g->World.GenState, NULL, dt);

    if (m->Screen == MenuScreen_Main)
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
        m->HotMainMenu = glm::clamp(m->HotMainMenu, 0, (int)ARRAY_COUNT(Menus) - 1);
        if (prevHotMenu != m->HotMainMenu)
        {
            PlaySequence(g->TweenState, &g->GUI.MenuChangeSequence, true);
        }

        if (IsJustPressed(g, Action_select))
        {
            if (m->HotMainMenu < ARRAY_COUNT(Menus) - 1)
            {
                m->Screen = Screens[m->HotMainMenu];
            }
            else
            {
                g->Running = false;
            }
        }
    }
	else if (m->Screen == MenuScreen_OptsAudio)
	{
		MusicMasterSetGain(g->MusicMaster, g->Options->Values["audio/music_gain"]->Float);
		bool back = UpdateMenuSelection(g, AudioMenu, GetAxisValue(g->Input, Action_horizontal), moveX, moveY, /*backItemEnabled=*/true);
		if (back)
		{
			m->Screen = MenuScreen_Opts;
			SaveOptions(g);
		}
	}
	else if (m->Screen == MenuScreen_OptsGraphics)
	{
		bool back = UpdateMenuSelection(g, GraphicsMenu, GetAxisValue(g->Input, Action_horizontal), moveX, moveY, /*backItemEnabled=*/true);
		if (back)
		{
			m->Screen = MenuScreen_Opts;
			g->Options->Values["graphics/resolution"]->Int = GraphicsMenu.Items[0].Options.SelectedIndex;
			SaveOptions(g);
		}
	}
    else
    {
        auto &menu = Menus[m->HotMainMenu];
        bool back = UpdateMenuSelection(g, menu, GetAxisValue(g->Input, Action_horizontal), moveX, moveY, /*backItemEnabled=*/true);
        if (back)
        {
			m->Screen = MenuScreen_Main;
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
	switch (m->Screen)
	{
	case MenuScreen_Main:
		for (int i = 0; i < ARRAY_COUNT(Menus); i++)
		{
			auto &item = Menus[i];
			color col = textColor;
			if (i == m->HotMainMenu)
			{
				col = Lerp(col, g->GUI.MenuChangeTimer, textSelectedColor);
			}
			DrawTextCentered(g->GUI, mainMenuFont, item.Title, GetRealPixels(g, MainMenuPositions[i]), col);
		}
		break;

	case MenuScreen_Play:
		DrawMenu(g, *PlayMenu, g->GUI.MenuChangeTimer, m->Alpha, true);
		break;

	case MenuScreen_Opts:
		DrawMenu(g, *OptsMenu, g->GUI.MenuChangeTimer, m->Alpha, true);
		break;

	case MenuScreen_OptsAudio:
		DrawMenu(g, AudioMenu, g->GUI.MenuChangeTimer, m->Alpha, true);
		break;

	case MenuScreen_OptsGraphics:
	{
		float halfX = g->Width*0.5f;
		static auto font = FindBitmapFont(g->AssetLoader, "unispace_12");

		DrawMenu(g, GraphicsMenu, g->GUI.MenuChangeTimer, m->Alpha, true);
		DrawTextCentered(g->GUI, font, "Warning: graphics settings need restart to apply", rect{ halfX, GetRealPixels(g, 50), 0,0 }, Color_white);
		break;
	}

	case MenuScreen_Cred:
		DrawMenu(g, *CredMenu, g->GUI.MenuChangeTimer, m->Alpha, true);
		break;

	case MenuScreen_Exit:
		DrawMenu(g, *ExitMenu, g->GUI.MenuChangeTimer, m->Alpha, true);
		break;
	}

    End(g->GUI);
}
