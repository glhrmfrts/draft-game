// Copyright

void RegisterInputActions(game_input &input)
{
    input.Actions[Action_camHorizontal] = action_state{ SDL_SCANCODE_D, SDL_SCANCODE_A, 0, 0, Axis_Invalid, Button_Invalid };
    input.Actions[Action_camVertical] = action_state{ SDL_SCANCODE_W, SDL_SCANCODE_S, 0, 0, Axis_Invalid, Button_Invalid};
    input.Actions[Action_horizontal] = action_state{ SDL_SCANCODE_RIGHT, SDL_SCANCODE_LEFT, 0, 0, Axis_LeftX, Button_Invalid };
    input.Actions[Action_vertical] = action_state{ SDL_SCANCODE_UP, SDL_SCANCODE_DOWN, 0, 0, Axis_RightTrigger, Button_Invalid };
    input.Actions[Action_boost] = action_state{ SDL_SCANCODE_SPACE, 0, 0, 0, Axis_Invalid, XboxButton_X };
    input.Actions[Action_debugUI] = action_state{ SDL_SCANCODE_GRAVE, 0, 0, 0, Axis_Invalid, Button_Invalid };
    input.Actions[Action_debugPause] = action_state{ SDL_SCANCODE_P, 0, 0, 0, Axis_Invalid, Button_Invalid };
    input.Actions[Action_select] = action_state{ SDL_SCANCODE_RETURN, 0, 0, 0, Axis_Invalid, XboxButton_A };
}

static void InitLoadingScreen(game_main *Game)
{
    Game->State = GameState_LoadingScreen;
    InitAssetLoader(Game->AssetLoader, Game->Platform);

    for (auto &Asset : Game->Assets)
    {
        AddAssetEntry(Game->AssetLoader, Asset);
    }
    AddShaderProgramEntries(Game->AssetLoader, Game->RenderState.ModelProgram);
    AddShaderProgramEntries(Game->AssetLoader, Game->RenderState.BlurHorizontalProgram);
    AddShaderProgramEntries(Game->AssetLoader, Game->RenderState.BlurVerticalProgram);
    AddShaderProgramEntries(Game->AssetLoader, Game->RenderState.BlendProgram);
    AddShaderProgramEntries(Game->AssetLoader, Game->RenderState.BlitProgram);
    AddShaderProgramEntries(Game->AssetLoader, Game->RenderState.ResolveMultisampleProgram);

    StartLoading(Game->AssetLoader);
}

typedef void init_func(game_main *Game);

static void UpdateLoadingScreen(game_main *g, float dt, init_func *nextModeInit)
{
    if (Update(g->AssetLoader))
    {
        nextModeInit(g);
    }
}

static void RenderLoadingScreen(game_main *Game, float DeltaTime)
{
    glClear(GL_COLOR_BUFFER_BIT);

    auto &g = Game->GUI;
    float Width = Game->Width * 0.5f;
    float Height = Game->Height * 0.05f;
    float x = (float)Game->Width/2 - Width/2;
    float y = (float)Game->Height/2 - Height/2;
    float LoadingPercentage = (float)(int)Game->AssetLoader.NumLoadedEntries / (float)Game->AssetLoader.Entries.size();
    float ProgressBarWidth = Width*LoadingPercentage;

    UpdateProjectionView(Game->GUICamera);
    Begin(g, Game->GUICamera);
    DrawRect(g, rect{ x, y, Width, Height }, Color_white, GL_LINE_LOOP);
    DrawRect(g, rect{ x + 5, y + 5, ProgressBarWidth - 10, Height - 10 }, Color_white);
    End(g);
}
