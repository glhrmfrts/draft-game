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
    input.Actions[Action_pause] = action_state{ SDL_SCANCODE_ESCAPE, 0, 0, 0, Axis_Invalid, XboxButton_Start };
}

void AddShadersAssetEntries(game_main *g, asset_job *job)
{
    AddShaderProgramEntries(job, g->RenderState.ModelProgram);
    AddShaderProgramEntries(job, g->RenderState.BlurHorizontalProgram);
    AddShaderProgramEntries(job, g->RenderState.BlurVerticalProgram);
    AddShaderProgramEntries(job, g->RenderState.BlendProgram);
    AddShaderProgramEntries(job, g->RenderState.BlitProgram);
    AddShaderProgramEntries(job, g->RenderState.ResolveMultisampleProgram);
	  AddShaderProgramEntries(job, g->RenderState.PerlinNoiseProgram);
}

typedef void init_func(game_main *Game);

void UpdateLoadingScreen(game_main *g, float dt, init_func *nextModeInit)
{
	DebugLogCall();
    if (g->AssetLoader.CurrentJob->Finished)
    {
        nextModeInit(g);
    }
	DebugLogCallEnd();
}

void RenderLoadingScreen(game_main *Game, float DeltaTime)
{
    glClear(GL_COLOR_BUFFER_BIT);

    auto &g = Game->GUI;
    float Width = Game->Width * 0.5f;
    float Height = Game->Height * 0.05f;
    float x = (float)Game->Width/2 - Width/2;
    float y = (float)Game->Height/2 - Height/2;
    float LoadingPercentage = (float)(int)Game->AssetLoader.CurrentJob->NumLoadedEntries / (float)Game->AssetLoader.CurrentJob->Entries.size();
    float ProgressBarWidth = Width*LoadingPercentage;

    UpdateProjectionView(Game->GUICamera);
    Begin(g, Game->GUICamera);
    DrawRect(g, rect{ x, y, Width, Height }, Color_white, GL_LINE_LOOP);
    DrawRect(g, rect{ x + 5, y + 5, ProgressBarWidth - 10, Height - 10 }, Color_white);
    End(g);
}
