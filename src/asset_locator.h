#ifndef ASSET_LOCATOR_H
#define ASSET_LOCATOR_H

typedef struct {
    char* base_dir;
    char* shaders_dir;
} AssetLocator;

extern AssetLocator g_asset_locator;

void asset_locator_init();
void asset_locator_fini();

#endif // ASSET_LOCATOR_H
