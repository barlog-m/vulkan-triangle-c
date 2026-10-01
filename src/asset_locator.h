#ifndef ASSET_LOCATOR_H
#define ASSET_LOCATOR_H

typedef struct AssetLocator {
    char* base_dir;
    char* shaders_dir;
} AssetLocator;

AssetLocator* asset_locator_init();
void asset_locator_fini(AssetLocator* self);

#endif // ASSET_LOCATOR_H
