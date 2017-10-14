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

static void OpenGameController(game_input &Input)
{
    Input.Controller.Joystick = SDL_JoystickOpen(0);
    if (Input.Controller.Joystick)
    {
        printf("Joystick name: %s\n", SDL_JoystickNameForIndex(0));
    }
    else
    {
        fprintf(stderr, "Could not open joystick %i: %s\n", 0, SDL_GetError());
    }
    SDL_JoystickEventState(SDL_IGNORE);
}

static void SplitExtension(const std::string &Path, std::string &Filename, std::string &Extension)
{
    auto Idx = Path.find_last_of('.');
    Filename = Path.substr(0, Idx);
    Extension = Path.substr(Idx+1);
}

#define GameControllerAxisDeadzone 5000
int main(int argc, char **argv)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0)
    {
        std::cout << "Failed to init display SDL" << std::endl;
    }

    int Width = 1280;
    int Height = 720;
    int vWidth = Width/2;
    int vHeight = Height/2;
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

    game_state Game;
    Game.Window = Window;
    Game.RealWidth = Width;
    Game.RealHeight = Height;
    Game.Width = vWidth;
    Game.Height = vHeight;
    Game.Platform.CompareFileTime = PlatformCompareFileTime;
    Game.Platform.GetFileLastWriteTime = PlatformGetFileLastWriteTime;
    Game.Platform.GetMilliseconds = PlatformGetMilliseconds;

    auto &Input = Game.Input;
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

    Lib.GameInit(&Game);

    float ReloadTimer = GameLibraryReloadTime;
    float DeltaTime = 0.016f;
    float DeltaTimeMS = DeltaTime * 1000.0f;
    uint32 PreviousTime = SDL_GetTicks();
    while (Game.Running)
    {
        uint32 CurrentTime = SDL_GetTicks();
        float Elapsed = ((CurrentTime - PreviousTime) / 1000.0f);

#ifdef DRAFT_DEBUG
        ReloadTimer -= Elapsed;
        if (ReloadTimer <= 0)
        {
            ReloadTimer = GameLibraryReloadTime;
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
            auto &Action = Input.Actions[i];
            Action.Pressed = 0;
            Action.AxisValue = 0;
            if (HasJoystick)
            {
                if (Action.AxisID != Axis_Invalid)
                {
                    int Value = SDL_JoystickGetAxis(Input.Controller.Joystick, Action.AxisID);
                    if (Action.AxisID == Axis_RightTrigger || Action.AxisID == Axis_LeftTrigger)
                    {
                        float fv = (Value + 32768) / float(65535);
                        Value = short(fv * 32767);
                    }
                    if (std::abs(Value) < GameControllerAxisDeadzone)
                    {
                        Value = 0;
                    }
                    Action.AxisValue = Value / float(32767);
                }
                if (Action.ButtonID != Button_Invalid)
                {
                    if (SDL_JoystickGetButton(Input.Controller.Joystick, Action.ButtonID))
                    {
                        Action.Pressed++;
                    }
                    else
                    {
                        Action.Pressed = 0;
                    }
                }
            }

            const uint8 *Keys = SDL_GetKeyboardState(&Input.KeyCount);
            uint8 Positive = Keys[Action.Positive];
            uint8 Negative = Keys[Action.Negative];
            Action.Pressed += Positive + Negative;
            if (Positive)
            {
                Action.AxisValue = 1;
            }
            else if (Negative)
            {
                Action.AxisValue = -1;
            }

            Input.Keys = Keys;
        }

        SDL_Event Event;
        while (SDL_PollEvent(&Event))
        {
            Lib.GameProcessEvent(&Game, &Event);
            switch (Event.type) {
            case SDL_QUIT:
                Game.Running = false;
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
            }
        }
        Lib.GameRender(&Game, Elapsed);

        memcpy(&Game.PrevInput, &Game.Input, sizeof(game_input));
        Input.MouseState.dX = 0;
        Input.MouseState.dY = 0;

        SDL_GL_SwapWindow(Window);

        if (Elapsed*1000.0f < DeltaTimeMS)
        {
            SDL_Delay(DeltaTimeMS - Elapsed*1000.0f);
        }

        PreviousTime = CurrentTime;
    }

    Lib.GameDestroy(&Game);

    alcMakeContextCurrent(NULL);
    alcDestroyContext(AudioContext);
    alcCloseDevice(AudioDevice);

    SDL_GL_DeleteContext(Context);
    SDL_DestroyWindow(Window);
    SDL_Quit();

    return 0;
}
