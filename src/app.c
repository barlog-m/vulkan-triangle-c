#include "app.h"

#include <SDL3/SDL.h>

#include "log.h"
#include "assert.h"
#include "rndr.h"

App g_app = {};

void app_init()
{
    ASSERT_SDL(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS));

    constexpr Uint32 window_flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN;
    SDL_Window *window = SDL_CreateWindow("Vulkan tutorial", 1920, 1080, window_flags);

    ASSERT_SDL(window != nullptr);

    SDL_Renderer *renderer = SDL_CreateRenderer(window, nullptr);
    SDL_SetRenderVSync(renderer, 1);
    ASSERT_SDL(renderer != nullptr);

    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(window);

    rndr_init();

    g_app.is_running = true;
}

void app_run()
{
    while (g_app.is_running) {
        SDL_Event ev;

        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
                case SDL_EVENT_QUIT:
                    g_app.is_running = false;
                    break;

                case SDL_EVENT_KEY_DOWN:
                    if (ev.key.key == SDLK_ESCAPE) {
                        g_app.is_running = false;
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

                default:
                    break;
            }
        }

        // render frame here...
        SDL_Delay(16);
    }
}

void app_fini()
{
    rndr_fini();
    SDL_DestroyWindow(g_app.window);
    SDL_Quit();
}
