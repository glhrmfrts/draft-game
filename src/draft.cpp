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
#include "level_mode.cpp"

#ifdef _WIN32
#define export_func __declspec(dllexport)
#else
#define export_func
#endif

extern "C"
{
    export_func GAME_INIT(GameInit)
    {
        auto g = Game;
        int Width = g->Width;
        int Height = g->Height;

        ImGui_ImplSdlGL3_Init(g->Window);

        RegisterInputActions(g->Input);
        InitGUI(g->GUI, g->Input);
        MakeCameraOrthographic(g->GUICamera, 0, Width, 0, Height, -1, 1);
        MakeCameraPerspective(g->Camera, (float)g->Width, (float)g->Height, 70.0f, 0.1f, 1000.0f);
        InitRenderState(g->RenderState, Width, Height);
        InitTweenState(g->TweenState);
        InitEntityWorld(g->World);

        g->Assets.push_back(
            CreateAssetEntry(
                AssetType_Texture,
                "data/textures/grid1.png",
                "grid",
                (void *)(TextureFlag_Mipmap | TextureFlag_Anisotropic | TextureFlag_WrapRepeat)
            )
        );
        g->Assets.push_back(
            CreateAssetEntry(
                AssetType_Font,
                "data/fonts/vcr.ttf",
                "vcr_16",
                (void *)16
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

        InitLoadingScreen(*g);
    }

    // @TODO: this exists only for imgui, remove in the future
    export_func GAME_PROCESS_EVENT(GameProcessEvent)
    {
        ImGui_ImplSdlGL3_ProcessEvent(Event);
    }

    export_func GAME_RENDER(GameRender)
    {
        auto g = Game;
        float dt = DeltaTime;

        ImGui_ImplSdlGL3_NewFrame(g->Window);
        if (IsJustPressed(*g, Action_debugUI))
        {
            Global_DebugUI = !Global_DebugUI;
        }

        switch (g->Mode)
        {
        case GameMode_LoadingScreen:
            RenderLoadingScreen(*g, dt, InitLevel);
            break;

        case GameMode_Level:
            RenderLevel(*g, dt);
            break;
        }

#ifdef DRAFT_DEBUG
        if (g->Mode != GameMode_LoadingScreen)
        {
            static float ReloadTimer = 0.0f;
            ReloadTimer += dt;
            if (ReloadTimer >= 1.0f)
            {
                ReloadTimer = 0.0f;
                CheckAssetsChange(g->AssetLoader);
            }
        }
#endif

        DrawDebugUI(*g, dt);
        ImGui::Render();
    }

    export_func GAME_DESTROY(GameDestroy)
    {
        auto g = Game;
        ImGui_ImplSdlGL3_Shutdown();
        DestroyAssetLoader(g->AssetLoader);
        FreeArena(g->Arena);
        FreeArena(g->AssetLoader.Arena);
    }
}
