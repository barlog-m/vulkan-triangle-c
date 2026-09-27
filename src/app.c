#include "app.h"

#include <SDL3/SDL.h>

#include "assert.h"
#include "asset_locator.h"
#include "rndr.h"

App g_app = {};

static Mesh mesh;

void app_init()
{
    ASSERT_SDL(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS));

    constexpr Uint32 window_flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE;
    SDL_Window *window = SDL_CreateWindow("Vulkan Triangle", 1920, 1080, window_flags);

    ASSERT_SDL(window != nullptr);
    
    int width = 0;
    int height = 0;
    SDL_GetWindowSize(window, &width, &height);
    g_app.width  = (uint32_t)width;
    g_app.height = (uint32_t)height;
    
    g_app.window = window;
    
    asset_locator_init();
    rndr_init();
    mesh_init(&mesh);

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
                    break;
                    
                case SDL_EVENT_WINDOW_RESIZED:
                case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: {
                    int width = 0;
                    int height = 0;
                    SDL_GetWindowSizeInPixels(g_app.window, &width, &height);
                    g_app.width     = (uint32_t)width;
                    g_app.height    = (uint32_t)height;
                    g_app.is_resized = true;
                    break;
                }
                    
                default:
                    break;
            }
        }

        rndr_draw_frame(&mesh);
        SDL_Delay(16);
    }
}

void app_fini()
{
    mesh_fini(&mesh);
    rndr_fini();
    asset_locator_fini();
    SDL_DestroyWindow(g_app.window);
    SDL_Quit();
}
