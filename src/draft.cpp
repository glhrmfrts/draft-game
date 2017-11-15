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
#include "asset.cpp"
#include "gui.cpp"
#include "debug_ui.cpp"
#include "meshes.cpp"
#include "entity.cpp"
#include "init.cpp"
#include "menu_state.cpp"
#include "level_state.cpp"

#ifdef _WIN32
#define export_func __declspec(dllexport)
#else
#define export_func
#endif

extern "C"
{
    export_func GAME_INIT(GameInit)
    {
        auto g = game;
        int Width = g->Width;
        int Height = g->Height;

        ImGui_ImplSdlGL3_Init(g->Window);

        RegisterInputActions(g->Input);
        InitGUI(g->GUI, g->Input);
        MakeCameraOrthographic(g->GUICamera, 0, Width, 0, Height, -1, 1);
        MakeCameraPerspective(g->Camera, (float)g->Width, (float)g->Height, 90.0f, 0.1f, 1000.0f);
        MakeCameraPerspective(g->FinalCamera, (float)g->Width, (float)g->Height, 90.0f, 0.1f, 1000.0f);
        InitRenderState(g->RenderState, Width, Height);
        InitTweenState(g->TweenState);
        InitEntityWorld(g->World);
        g->Assets.push_back(
            CreateAssetEntry(
                AssetType_Texture,
                "data/textures/grid.png",
                "grid",
                (void *)(TextureFlag_Mipmap | TextureFlag_Anisotropic | TextureFlag_WrapRepeat)
            )
        );
        g->Assets.push_back(
            CreateAssetEntry(
                AssetType_Texture,
                "data/textures/space6.png",
                "background",
                (void *)(TextureFlag_WrapRepeat)
            )
        );
        g->Assets.push_back(
            CreateAssetEntry(
                AssetType_Font,
                "data/fonts/vcr.ttf",
                "vcr_16",
                (void *)int(GetRealPixels(g, 32.0f))
            )
        );
        g->Assets.push_back(
            CreateAssetEntry(
                AssetType_Font,
                "data/fonts/unispace.ttf",
                "unispace_16",
                (void *)int(GetRealPixels(g, 16.0f))
            )
        );
        g->Assets.push_back(
            CreateAssetEntry(
                AssetType_Font,
                "data/fonts/unispace.ttf",
                "unispace_24",
                (void *)int(GetRealPixels(g, 24.0f))
            )
        );
        g->Assets.push_back(
            CreateAssetEntry(
                AssetType_Font,
                "data/fonts/unispace.ttf",
                "unispace_32",
                (void *)int(GetRealPixels(g, 32.0f))
            )
        );
        g->Assets.push_back(
            CreateAssetEntry(
                AssetType_Sound,
                "data/audio/boost.wav",
                "boost",
                NULL
            )
        );

        InitLoadingScreen(g);
    }

    // @TODO: this exists only for imgui, remove in the future
    export_func GAME_PROCESS_EVENT(GameProcessEvent)
    {
        ImGui_ImplSdlGL3_ProcessEvent(event);
    }

    export_func GAME_UPDATE(GameUpdate)
    {
        auto g = game;
        if (IsJustPressed(g, Action_debugUI))
        {
            Global_DebugUI = !Global_DebugUI;
        }
        Update(g->TweenState, dt);
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
        FreeArena(g->Arena);
        FreeArena(g->AssetLoader.Arena);
    }
}
