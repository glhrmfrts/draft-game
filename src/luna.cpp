/*
  Current TODO:
  - Render entity sprites
  - Create a render command buffer
  - Create some kind of alpha map where it follows the player, so we can see behind the walls
 */

#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <AL/alc.h>
#include "luna.h"
#include "luna_level_mode.h"

#undef main

static void RegisterInputActions(game_input &Input)
{
    Input.Actions[Action_camHorizontal] = {SDLK_d, SDLK_a, 0, 0};
    Input.Actions[Action_camVertical] = {SDLK_w, SDLK_s, 0, 0};
    Input.Actions[Action_debugFreeCam] = {SDLK_SPACE, 0, 0, 0};
    Input.Actions[Action_horizontal] = {SDLK_RIGHT, SDLK_LEFT, 0, 0};
    Input.Actions[Action_vertical] = {SDLK_UP, SDLK_DOWN, 0, 0};
}

int main(int argc, char **argv)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cout << "Failed to init display SDL" << std::endl;
    }

    int Width = 1280;
    int Height = 720;
    SDL_Window *Window = SDL_CreateWindow("RGB", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          Width, Height, SDL_WINDOW_OPENGL);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

    SDL_GLContext Context = SDL_GL_CreateContext(Window);

    SDL_GL_SetSwapInterval(0);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cout << "Error loading GL extensions" << std::endl;
        exit(EXIT_FAILURE);
    }

    glEnable(GL_MULTISAMPLE);
    glViewport(0, 0, Width, Height);

    ALCdevice *AudioDevice = alcOpenDevice(NULL);
    if (!AudioDevice)
        std::cerr << "Error opening audio device" << std::endl;

    ALCcontext *AudioContext = alcCreateContext(AudioDevice, NULL);
    if (!alcMakeContextCurrent(AudioContext))
        std::cerr << "Error setting audio context" << std::endl;

    game_state GameState = {};
    GameState.Width = Width;
    GameState.Height = Height;

    auto &Input = GameState.Input;
    RegisterInputActions(Input);
    InitGUI(GameState.GUI, Input);
    MakeCameraOrthographic(GameState.GUICamera, 0, Width, 0, Height);
    InitRenderState(GameState.RenderState);
    StartLevel(GameState, GameState.LevelMode);

    clock_t PreviousTime = clock();
    float DeltaTime = 0.016f;
    float DeltaTimeMS = DeltaTime * 1000;
    while (GameState.Running) {
        clock_t CurrentTime = clock();
        float Elapsed = ((CurrentTime - PreviousTime) / (float)CLOCKS_PER_SEC * 1000);

        SDL_Event Event;
        while (SDL_PollEvent(&Event)) {
            switch (Event.type) {
            case SDL_QUIT:
                GameState.Running = false;
                break;

            case SDL_KEYDOWN: {
                SDL_Keycode Key = Event.key.keysym.sym;
                for (int i = 0; i < Action_count; i++) {
                    auto &Action = Input.Actions[i];
                    if (Action.Positive == Key || Action.Negative == Key) {
                        Action.Pressed++;

                        if (Action.Positive == Key) {
                            Action.AxisValue = 1;
                        }
                        else if (Action.Negative == Key) {
                            Action.AxisValue = -1;
                        }
                    }
                }
                break;
            }

            case SDL_KEYUP: {
                SDL_Keycode Key = Event.key.keysym.sym;
                for (int i = 0; i < Action_count; i++) {
                    auto &Action = Input.Actions[i];
                    if (Action.Positive == Key && Action.AxisValue == 1) {
                        Action.Pressed = 0;
                        Action.AxisValue = 0;
                        break;
                    }
                    else if (Action.Negative == Key && Action.AxisValue == -1) {
                        Action.Pressed = 0;
                        Action.AxisValue = 0;
                    }
                }
                break;
            }

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
                    Input.MouseState.Buttons |= MouseButton_left;
                    break;

                case SDL_BUTTON_MIDDLE:
                    Input.MouseState.Buttons |= MouseButton_middle;
                    break;

                case SDL_BUTTON_RIGHT:
                    Input.MouseState.Buttons |= MouseButton_right;
                    break;
                }
                break;
            }

            case SDL_MOUSEBUTTONUP: {
                auto &Button = Event.button;
                switch (Button.button) {
                case SDL_BUTTON_LEFT:
                    Input.MouseState.Buttons &= ~MouseButton_left;
                    break;

                case SDL_BUTTON_MIDDLE:
                    Input.MouseState.Buttons &= ~MouseButton_middle;
                    break;

                case SDL_BUTTON_RIGHT:
                    Input.MouseState.Buttons &= ~MouseButton_right;
                    break;
                }
                break;
            }
            }
        }

        switch (GameState.GameMode) {
        case GameMode_level:
            UpdateAndRenderLevel(GameState, GameState.LevelMode, DeltaTime);
            break;
        }

        memcpy(&GameState.PrevInput, &GameState.Input, sizeof(game_input));
        Input.MouseState.dX = 0;
        Input.MouseState.dY = 0;

        if (Elapsed < DeltaTimeMS) {
            SDL_Delay(DeltaTimeMS - Elapsed);
        }

        SDL_GL_SwapWindow(Window);
        PreviousTime = CurrentTime;
    }

    alcMakeContextCurrent(NULL);
    alcDestroyContext(AudioContext);
    alcCloseDevice(AudioDevice);

    SDL_GL_DeleteContext(Context);
    SDL_DestroyWindow(Window);
    SDL_Quit();

    return 0;
}
