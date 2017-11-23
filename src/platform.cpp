// Copyright

#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <ctime>

#define DRAFT_DEBUG
#include "draft.h"

#undef main

#ifdef _WIN32
#include "platform_win.cpp"
#else
#include "platform_linux.cpp"
#endif

static void OpenGameController(game_input &input)
{
    input.Controller.Joystick = SDL_JoystickOpen(0);
    if (input.Controller.Joystick)
    {
        printf("Joystick name: %s\n", SDL_JoystickNameForIndex(0));
    }
    else
    {
        fprintf(stderr, "Could not open joystick %i: %s\n", 0, SDL_GetError());
    }
    SDL_JoystickEventState(SDL_IGNORE);
}

static void SplitExtension(const std::string &path, std::string &filename, std::string &extension)
{
    auto idx = path.find_last_of('.');
    filename = path.substr(0, idx);
    extension = path.substr(idx+1);
}

#define GameControllerAxisDeadzone 5000
int main(int argc, char **argv)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0)
    {
        std::cout << "Failed to init display SDL" << std::endl;
    }

    int Width = GAME_BASE_WIDTH;
    int Height = GAME_BASE_HEIGHT;
    int vWidth = Width;
    int vHeight = Height;
    SDL_Window *Window = SDL_CreateWindow("Draft", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, Width, Height, SDL_WINDOW_OPENGL);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

    SDL_GLContext Context = SDL_GL_CreateContext(Window);

    SDL_GL_SetSwapInterval(0);

    ALCdevice *AudioDevice = alcOpenDevice(NULL);
    if (!AudioDevice)
    {
        std::cerr << "Error opening audio device" << std::endl;
    }

    ALCcontext *AudioContext = alcCreateContext(AudioDevice, NULL);
    if (!alcMakeContextCurrent(AudioContext))
    {
        std::cerr << "Error setting audio context" << std::endl;
    }

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        std::cout << "Error loading GL extensions" << std::endl;
        exit(EXIT_FAILURE);
    }
    printf("%s\n", glGetString(GL_VERSION));
    printf("%s\n", glGetString(GL_VENDOR));
    printf("%s\n", glGetString(GL_RENDERER));

    glEnable(GL_MULTISAMPLE);
    glViewport(0, 0, Width, Height);

    game_main game;
    game.Window = Window;
    game.RealWidth = Width;
    game.RealHeight = Height;
    game.Width = vWidth;
    game.Height = vHeight;
    game.Platform.CompareFileTime = PlatformCompareFileTime;
    game.Platform.GetFileLastWriteTime = PlatformGetFileLastWriteTime;
    game.Platform.GetMilliseconds = PlatformGetMilliseconds;

    auto &Input = game.Input;
    if (SDL_NumJoysticks() > 0)
    {
        OpenGameController(Input);
    }

    game_library Lib;
    std::string LibPath = GameLibraryPath;
    std::string TempPath;
    std::string LibFilename;
    std::string LibExtension;
    if (argc > 1)
    {
        LibPath = std::string(argv[1]);
    }
    SplitExtension(LibPath, LibFilename, LibExtension);
    TempPath = LibFilename + "_temp." + LibExtension;
    LoadGameLibrary(Lib, LibPath.c_str(), TempPath.c_str());

    Lib.GameInit(&game);

    const float deltaTime = 0.016f;
    const float deltaTimeMS = deltaTime * 1000.0f;
    float reloadTimer = GameLibraryReloadTime;
    float accul = deltaTime;
    uint32 previousTime = SDL_GetTicks();
    while (game.Running)
    {
        uint32 currentTime = SDL_GetTicks();
        float elapsedMS = (currentTime - previousTime);
        if (elapsedMS > deltaTimeMS*2)
        {
            elapsedMS = deltaTimeMS*2;
        }
        float elapsed = (elapsedMS / 1000.0f);
        accul += elapsed;

#ifdef DRAFT_DEBUG
        reloadTimer -= elapsed;
        if (reloadTimer <= 0)
        {
            reloadTimer = GameLibraryReloadTime;
            if (GameLibraryChanged(Lib))
            {
                UnloadGameLibrary(Lib);
                LoadGameLibrary(Lib, LibPath.c_str(), TempPath.c_str());
            }
        }
#endif

        SDL_PumpEvents();

        bool HasJoystick = Input.Controller.Joystick != NULL;
        if (HasJoystick)
        {
            SDL_JoystickUpdate();
        }
        for (int i = 0; i < Action_count; i++)
        {
            auto &action = Input.Actions[i];
            action.Pressed = 0;
            action.AxisValue = 0;
            if (HasJoystick)
            {
                if (action.AxisID != Axis_Invalid)
                {
                    int Value = SDL_JoystickGetAxis(Input.Controller.Joystick, action.AxisID);
                    if (action.AxisID == Axis_RightTrigger || action.AxisID == Axis_LeftTrigger)
                    {
                        float fv = (Value + 32768) / float(65535);
                        Value = short(fv * 32767);
                    }
                    if (std::abs(Value) < GameControllerAxisDeadzone)
                    {
                        Value = 0;
                    }
                    action.AxisValue = Value / float(32767);
                }
                if (action.ButtonID != Button_Invalid)
                {
                    if (SDL_JoystickGetButton(Input.Controller.Joystick, action.ButtonID))
                    {
                        action.Pressed++;
                    }
                    else
                    {
                        action.Pressed = 0;
                    }
                }
            }

            const uint8 *keys = SDL_GetKeyboardState(&Input.KeyCount);
            uint8 positive = keys[action.Positive];
            uint8 negative = keys[action.Negative];
            action.Pressed += positive + negative;
            if (positive)
            {
                action.AxisValue = 1;
            }
            else if (negative)
            {
                action.AxisValue = -1;
            }

            Input.Keys = keys;
        }

        SDL_Event Event;
        while (SDL_PollEvent(&Event))
        {
            Lib.GameProcessEvent(&game, &Event);
            switch (Event.type) {
            case SDL_QUIT:
                game.Running = false;
                break;

            case SDL_MOUSEMOTION: {
                auto &Motion = Event.motion;
                Input.MouseState.X = Motion.x;
                Input.MouseState.Y = Motion.y;
                Input.MouseState.dX = Motion.xrel;
                Input.MouseState.dY = Motion.yrel;
                break;
            }

            case SDL_MOUSEWHEEL: {
                auto &Wheel = Event.wheel;
                Input.MouseState.ScrollY = Wheel.y;
                break;
            }

            case SDL_MOUSEBUTTONDOWN: {
                auto &Button = Event.button;
                switch (Button.button) {
                case SDL_BUTTON_LEFT:
                    Input.MouseState.Buttons |= MouseButton_Left;
                    break;

                case SDL_BUTTON_MIDDLE:
                    Input.MouseState.Buttons |= MouseButton_Middle;
                    break;

                case SDL_BUTTON_RIGHT:
                    Input.MouseState.Buttons |= MouseButton_Right;
                    break;
                }
                break;
            }

            case SDL_MOUSEBUTTONUP: {
                auto &Button = Event.button;
                switch (Button.button) {
                case SDL_BUTTON_LEFT:
                    Input.MouseState.Buttons &= ~MouseButton_Left;
                    break;

                case SDL_BUTTON_MIDDLE:
                    Input.MouseState.Buttons &= ~MouseButton_Middle;
                    break;

                case SDL_BUTTON_RIGHT:
                    Input.MouseState.Buttons &= ~MouseButton_Right;
                    break;
                }
                break;
            }
            
            case SDL_JOYBUTTONDOWN: {
                printf("%d\n", Event.jbutton.button);
                break;
            }
            }
        }

		Println(accul);

        int i = 0;
        while (accul >= deltaTime)
        {
            Lib.GameUpdate(&game, deltaTime);
            if (i == 0)
            {
                memcpy(&game.PrevInput, &game.Input, sizeof(game_input));
                Input.MouseState.dX = 0;
                Input.MouseState.dY = 0;
            }
            accul -= deltaTime;
            i++;
        }

        Lib.GameRender(&game, elapsed);
        SDL_GL_SwapWindow(Window);
        //SDL_Delay(10);
        previousTime = currentTime;
    }

    Lib.GameDestroy(&game);

    alcMakeContextCurrent(NULL);
    alcDestroyContext(AudioContext);
    alcCloseDevice(AudioDevice);

    SDL_GL_DeleteContext(Context);
    SDL_DestroyWindow(Window);
    SDL_Quit();

    return 0;
}
