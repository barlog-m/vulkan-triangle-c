#include "app.h"

#include <SDL3/SDL.h>

#include "assert.h"
#include "asset_locator.h"
#include "mesh.h"
#include "rndr.h"

App* app_init()
{
    App* app = calloc(1, sizeof(App));
    
    app->window = window_init();
    
    app->asset_locator = asset_locator_init();
    
    app->rndr = rndr_init(app->asset_locator, app->window);
    
    app->mesh = mesh_init(app->rndr);

    app->is_running = true;

    return app;
}

void app_run(App* self)
{
    while (self->is_running) {
        SDL_Event ev;

        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
                case SDL_EVENT_QUIT:
                    self->is_running = false;
                    break;

                case SDL_EVENT_KEY_DOWN:
                    if (ev.key.key == SDLK_ESCAPE) {
                        self->is_running = false;
                    }
                    break;

                case SDL_EVENT_WINDOW_RESIZED:
                case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: {
                    window_update_size(self->window);
                    break;
                }

                default:
                    break;
            }
        }

        rndr_draw_frame(self->rndr, self->mesh);
    }
}

void app_fini(App* self)
{
    mesh_fini(self->mesh, self->rndr);
    rndr_fini(self->rndr);
    asset_locator_fini(self->asset_locator);
    window_fini(self->window);
    
    SDL_Quit();
    free(self);
}
