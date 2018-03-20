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
        auto g = game;
        int Width = g->Width;
        int Height = g->Height;
		Global_Platform = &g->Platform;

        ImGui_ImplSdlGL3_Init(g->Window);

        RegisterInputActions(g->Input);
        InitGUI(g->GUI, g->TweenState, g->Input);
        MakeCameraOrthographic(g->GUICamera, 0, Width, 0, Height, -1, 1);
        MakeCameraPerspective(g->Camera, (float)g->Width, (float)g->Height, 90.0f, 0.1f, 1000.0f);
        MakeCameraPerspective(g->FinalCamera, (float)g->Width, (float)g->Height, 90.0f, 0.1f, 1000.0f);
        InitRenderState(g->RenderState, Width, Height, g->ViewportWidth, g->ViewportHeight);
        InitTweenState(g->TweenState);
        InitEntityWorld(g, g->World);
		MusicMasterInit(g, g->MusicMaster);

        g->RenderState.RoadTangentPoint = &g->World.RoadTangentPoint;

        g->Assets.push_back(
            CreateAssetEntry(
                AssetEntryType_Texture,
                "data/textures/grid.png",
                "grid",
                (void *)(TextureFlag_Mipmap | TextureFlag_Anisotropic | TextureFlag_WrapRepeat)
            )
        );
        g->Assets.push_back(
            CreateAssetEntry(
                AssetEntryType_Texture,
                "data/textures/space6.png",
                "background",
                (void *)(TextureFlag_WrapRepeat)
            )
        );
		g->Assets.push_back(
			CreateAssetEntry(
				AssetEntryType_Texture,
				"data/textures/random.png",
				"random",
				(void *)(TextureFlag_WrapRepeat | TextureFlag_Nearest)
			)
		);
        g->Assets.push_back(
            CreateAssetEntry(
                AssetEntryType_Font,
                "data/fonts/vcr.ttf",
                "vcr_16",
                (void *)long(GetRealPixels(g, 32.0f))
            )
        );
		g->Assets.push_back(
			CreateAssetEntry(
				AssetEntryType_Font,
				"data/fonts/unispace.ttf",
				"unispace_12",
				(void *)long(GetRealPixels(g, 12.0f))
			)
		);
        g->Assets.push_back(
            CreateAssetEntry(
                AssetEntryType_Font,
                "data/fonts/unispace.ttf",
                "unispace_16",
                (void *)long(GetRealPixels(g, 16.0f))
            )
        );
        g->Assets.push_back(
            CreateAssetEntry(
                AssetEntryType_Font,
                "data/fonts/unispace.ttf",
                "unispace_24",
                (void *)long(GetRealPixels(g, 24.0f))
            )
        );
        g->Assets.push_back(
            CreateAssetEntry(
                AssetEntryType_Font,
                "data/fonts/unispace.ttf",
                "unispace_32",
                (void *)long(GetRealPixels(g, 32.0f))
            )
        );
        g->Assets.push_back(
            CreateAssetEntry(
                AssetEntryType_Font,
                "data/fonts/unispace.ttf",
                "unispace_48",
                (void *)long(GetRealPixels(g, 48.0f))
            )
        );
        g->Assets.push_back(
            CreateAssetEntry(
                AssetEntryType_Sound,
                "data/audio/boost.wav",
                "boost",
                NULL
            )
        );
		g->Assets.push_back(
			CreateAssetEntry(
				AssetEntryType_Sound,
				"data/audio/explosion3.wav",
				"explosion",
				NULL
			)
		);
		g->Assets.push_back(
			CreateAssetEntry(
				AssetEntryType_Sound,
				"data/audio/checkpoint.wav",
				"checkpoint",
				NULL
			)
		);
		g->Assets.push_back(
			CreateAssetEntry(
				AssetEntryType_Sound,
				"data/audio/crystal.wav",
				"crystal",
				NULL
			)
		);
		g->Assets.push_back(
			CreateAssetEntry(
				AssetEntryType_OptionsLoad,
				"options.json",
				"options",
				NULL
			)
		);
		g->Assets.push_back(
			CreateAssetEntry(
				AssetEntryType_Level,
				"data/levels/1.level",
				"1",
				NULL
			)
		);

        InitLoadingScreen(g);
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

        switch (g->State)
        {
        case GameState_LoadingScreen:
            UpdateLoadingScreen(g, dt, InitMenu);
            break;

        case GameState_Level:
            UpdateLevel(g, dt);
            break;

        case GameState_Menu:
            UpdateMenu(g, dt);
            break;
        }

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
        switch (g->State)
        {
        case GameState_LoadingScreen:
            RenderLoadingScreen(g, dt);
            break;

        case GameState_Level:
            RenderLevel(g, dt);
            break;

        case GameState_Menu:
            RenderMenu(g, dt);
            break;
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
