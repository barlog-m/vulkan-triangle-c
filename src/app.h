#ifndef APP_H
#define APP_H

#include "SDL3/SDL_video.h"

typedef struct {
    SDL_Window *window;
    uint32_t width;
    uint32_t height;
    bool is_resized;
    bool is_running;
} App;

extern App g_app;

void app_init();
void app_run();
void app_fini();

#endif // APP_H
