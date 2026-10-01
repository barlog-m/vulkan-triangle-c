#ifndef APP_H
#define APP_H

#include "asset_locator.h"
#include "rndr.h"
#include "window.h"

typedef struct App {
    Window *window;
    AssetLocator *asset_locator;
    Rndr *rndr;
    Mesh *mesh;
    bool is_running;
} App;

App* app_init();
void app_run(App* self);
void app_fini(App* self);

#endif // APP_H
