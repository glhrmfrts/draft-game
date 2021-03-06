// Copyright

#define DRAFT_DEBUG 1

#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include "imgui.h"
#include "imgui_impl_sdl_gl3.h"
#include "draft.h"
#include "memory.cpp"
#include "thread_pool.cpp"
#include "tween.cpp"
#include "collision.cpp"
#include "render.cpp"
#include "audio.cpp"
#include "options.cpp"
#include "level.cpp"
#include "asset.cpp"
#include "gui.cpp"
#include "debug_ui.cpp"
#include "meshes.cpp"
#include "entity.cpp"
#include "generate.cpp"
#include "init.cpp"
#include "menu_state.cpp"
#include "level_state.cpp"

extern "C"
{
    export_func GAME_INIT(GameInit)
    {
		Global_DebugLogIdent = 0;
		DebugLogCall();
        auto g = game;
        int Width = g->Width;
        int Height = g->Height;
		Global_Platform = &g->Platform;

        ImGui_ImplSdlGL3_Init(g->Window);

        RegisterInputActions(g->Input);
        MakeCameraOrthographic(g->GUICamera, 0, Width, 0, Height, -1, 1);
        MakeCameraPerspective(g->Camera, (float)g->Width, (float)g->Height, 90.0f, 0.1f, 1000.0f);
        MakeCameraPerspective(g->FinalCamera, (float)g->Width, (float)g->Height, 90.0f, 0.1f, 1000.0f);
        InitRenderState(g->RenderState, Width, Height, g->ViewportWidth, g->ViewportHeight);
        InitTweenState(g->TweenState);
	    InitGUI(g->GUI, g->TweenState, g->Input);
        InitEntityWorld(g, g->World);
		MusicMasterInit(g, g->MusicMaster);
        InitLevelState(g, &g->LevelState);
		InitAssetLoader(g, g->AssetLoader, g->Platform);

        g->State = GameState_LoadingScreen;
        auto job = CreateAssetJob(g->AssetLoader, "main_assets");

        g->RenderState.RoadTangentPoint = &g->World.RoadTangentPoint;

        job->Entries.push_back(
            CreateAssetEntry(
                AssetEntryType_Texture,
                "data/textures/grid.png",
                "grid",
                (void *)(TextureFlag_Mipmap | TextureFlag_Anisotropic | TextureFlag_WrapRepeat)
            )
        );
        job->Entries.push_back(
            CreateAssetEntry(
                AssetEntryType_Texture,
                "data/textures/space6.png",
                "background",
                (void *)(TextureFlag_WrapRepeat)
            )
        );
		job->Entries.push_back(
			CreateAssetEntry(
				AssetEntryType_Texture,
				"data/textures/random.png",
				"random",
				(void *)(TextureFlag_WrapRepeat | TextureFlag_Nearest)
			)
		);
        job->Entries.push_back(
            CreateAssetEntry(
                AssetEntryType_Font,
                "data/fonts/vcr.ttf",
                "vcr_16",
                (void *)long(GetRealPixels(g, 32.0f))
            )
        );
		job->Entries.push_back(
			CreateAssetEntry(
				AssetEntryType_Font,
				"data/fonts/unispace.ttf",
				"unispace_12",
				(void *)long(GetRealPixels(g, 12.0f))
			)
		);
        job->Entries.push_back(
            CreateAssetEntry(
                AssetEntryType_Font,
                "data/fonts/unispace.ttf",
                "unispace_16",
                (void *)long(GetRealPixels(g, 16.0f))
            )
        );
        job->Entries.push_back(
            CreateAssetEntry(
                AssetEntryType_Font,
                "data/fonts/unispace.ttf",
                "unispace_24",
                (void *)long(GetRealPixels(g, 24.0f))
            )
        );
        job->Entries.push_back(
            CreateAssetEntry(
                AssetEntryType_Font,
                "data/fonts/unispace.ttf",
                "unispace_32",
                (void *)long(GetRealPixels(g, 32.0f))
            )
        );
        job->Entries.push_back(
            CreateAssetEntry(
                AssetEntryType_Font,
                "data/fonts/unispace.ttf",
                "unispace_48",
                (void *)long(GetRealPixels(g, 48.0f))
            )
        );
        job->Entries.push_back(
            CreateAssetEntry(
                AssetEntryType_Sound,
                "data/audio/boost.wav",
                "boost",
                NULL
            )
        );
		job->Entries.push_back(
			CreateAssetEntry(
				AssetEntryType_Sound,
				"data/audio/explosion3.wav",
				"explosion",
				NULL
			)
		);
		job->Entries.push_back(
			CreateAssetEntry(
				AssetEntryType_Sound,
				"data/audio/checkpoint.wav",
				"checkpoint",
				NULL
			)
		);
		job->Entries.push_back(
			CreateAssetEntry(
				AssetEntryType_Sound,
				"data/audio/crystal.wav",
				"crystal",
				NULL
			)
		);
		job->Entries.push_back(
			CreateAssetEntry(
				AssetEntryType_OptionsLoad,
				"options.json",
				"options",
				NULL
			)
		);
		job->Entries.push_back(
			CreateAssetEntry(
				AssetEntryType_Mesh,
				"data/models/deer.obj",
				"deer",
				(void *)(mesh_flags::UpY)
			)
		);
		job->Entries.push_back(
			CreateAssetEntry(
				AssetEntryType_Mesh,
				"data/models/skull.obj",
				"skull",
				(void *)(mesh_flags::UpY)
			)
		);

#define NUM_LEVELS 3
		for (int i = 1; i <= NUM_LEVELS; i++)
		{
			job->Entries.push_back(
				CreateAssetEntry(
					AssetEntryType_Level,
					"data/levels/" + std::to_string(i) + ".level",
					std::to_string(i),
					NULL
				)
			);
		}


        AddShadersAssetEntries(g, job);
        StartAssetJob(g->AssetLoader, job);
  }

	export_func GAME_RELOAD(GameReload)
	{
		Global_Platform = &game->Platform;
	}

	export_func GAME_UNLOAD(GameUnload)
	{
		MusicMasterExit(game->MusicMaster);
		StopThreadPool(game->AssetLoader.Pool);
		StopThreadPool(game->World.UpdateThreadPool);

		//game->MusicMaster.StopLoop = false;
		//RestartThreadPool(game->AssetLoader.Pool);
		//RestartThreadPool(game->World.UpdateThreadPool);
	}

    // @TODO: this exists only for imgui, remove in the future
    export_func GAME_PROCESS_EVENT(GameProcessEvent)
    {
        ImGui_ImplSdlGL3_ProcessEvent(event);
    }

    export_func GAME_UPDATE(GameUpdate)
    {
		Global_DebugLogIdent = 0;

        auto g = game;
		Global_Platform = &game->Platform;

		ResetProfileTimers();
		MusicMasterTick(g->MusicMaster, dt);
        if (IsJustPressed(g, Action_debugUI))
        {
            Global_DebugUI = !Global_DebugUI;
        }
        Update(g->TweenState, dt);
		Update(g->AssetLoader);

#ifdef DRAFT_DEBUG
        if (g->State != GameState_LoadingScreen)
        {
            static float reloadTimer = 0.0f;
            reloadTimer += dt;
            if (reloadTimer >= 1.0f)
            {
                reloadTimer = 0.0f;
                CheckAssetsChange(g->AssetLoader);
            }
        }
#endif
    }

    export_func GAME_RENDER(GameRender)
    {
        auto g = game;
        ImGui_ImplSdlGL3_NewFrame(g->Window);
        MakeCameraPerspective(g->Camera, (float)g->Width, (float)g->Height, Global_Camera_FieldOfView, 0.1f, 1000.0f);
        MakeCameraPerspective(g->FinalCamera, (float)g->Width, (float)g->Height, Global_Camera_FieldOfView, 0.1f, 1000.0f);

		if (!(g->AssetLoader.CurrentJob->Finished))
		{
			RenderLoadingScreen(g, dt);
		}
		else
		{
			glClear(GL_COLOR_BUFFER_BIT);
		}

		if (g->ScreenRectAlpha > 0.0f)
		{
			Begin(g->GUI, g->GUICamera, 0.0f);
			DrawRect(g->GUI, rect{ 0, 0, (float)g->Width, (float)g->Height }, Color_black * g->ScreenRectAlpha, GL_TRIANGLES, false);
			End(g->GUI);
		}

        DrawDebugUI(g, dt);
        ImGui::Render();
    }

    export_func GAME_DESTROY(GameDestroy)
    {
        auto g = game;
        ImGui_ImplSdlGL3_Shutdown();
        DestroyAssetLoader(g->AssetLoader);
    }
}
