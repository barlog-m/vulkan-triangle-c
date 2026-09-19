#include <stdlib.h>

#include "SDL3/SDL.h"
#include "SDL3/SDL_main.h"

#include "log.h"

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
    {
        B_LOG_MSG(B_ERROR, "SDL_Init():", SDL_GetError());
        return -1;
    }

    constexpr Uint32 window_flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN;
    SDL_Window *window = SDL_CreateWindow("Vulkan tutorial", 1920, 1080, window_flags);
    if (window == nullptr) {
        B_LOG_MSG(B_ERROR, "SDL_CreateWindow():", SDL_GetError());
        return -1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, nullptr);
    SDL_SetRenderVSync(renderer, 1);
    if (renderer == nullptr) {
        B_LOG_MSG(B_ERROR, "SDL_CreateRenderer():", SDL_GetError());
        return -1;
    }

    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(window);

    bool running = true;

    while (running) {
        SDL_Event ev;

        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;

                case SDL_EVENT_KEY_DOWN:
                    if (ev.key.key == SDLK_ESCAPE) {
                        running = false;
                    }
                    printf("Key pressed: %s (scancode %d, mod %u)\n",
                           SDL_GetKeyName(ev.key.key),
                           ev.key.scancode,
                           ev.key.mod);
                    break;

                case SDL_EVENT_KEY_UP:
                    printf("Key released: %s\n", SDL_GetKeyName(ev.key.key));
                    break;

                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                    printf("Mouse button %d at (%f, %f)\n",
                           ev.button.button,
                           ev.button.x,
                           ev.button.y);
                    break;

                case SDL_EVENT_TEXT_INPUT:
                    printf("Text input: %s\n", ev.text.text);
                    break;

                default:
                    break;
            }
        }

        // render frame here...
        SDL_Delay(16);
    }

    SDL_DestroyWindow(window);
    SDL_Quit();

    return EXIT_SUCCESS;
}
