#include "window.h"

#include <SDL3/SDL.h>
#include "SDL3/SDL_init.h"
#include "assert.h"

Window* window_init()
{
    ASSERT_SDL(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS));

    constexpr Uint32 window_flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE;
    SDL_Window* window = SDL_CreateWindow("Vulkan Triangle", 1920, 1080, window_flags);

    ASSERT_SDL(window != nullptr);

    Window* self = malloc(sizeof(Window));
    self->sdl_window = window;

    window_update_size(self);
    
    return self;
}

void window_fini(Window* self)
{
    SDL_DestroyWindow(self->sdl_window);
    free(self);
}

void window_update_size(Window* self)
{
    int width = 0;
    int height = 0;
    SDL_GetWindowSizeInPixels(self->sdl_window, &width, &height);
    self->width = (uint32_t)width;
    self->height = (uint32_t)height;
    self->is_resized = true;
}