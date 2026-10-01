#ifndef WINDOW_H
#define WINDOW_H

#include "SDL3/SDL_video.h"

typedef struct Window {
    SDL_Window *sdl_window;
    uint32_t width;
    uint32_t height;
    bool is_resized;
} Window;

Window* window_init();
void window_fini(Window* self);
void window_update_size(Window* self);
bool window_is_zero_size(Window* self);

#endif  // WINDOW_H
