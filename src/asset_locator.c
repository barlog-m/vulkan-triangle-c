#include "asset_locator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "constants.h"
#include "utils.h"

AssetLocator g_asset_locator = {};

void asset_locator_init()
{
    char* base_bin_path = get_base_path();
    const size_t base_bin_path_len = strlen(base_bin_path);
    constexpr size_t suffix_len = 5;  // strip trailing "/bin/"

    char base_dir[MAX_PATH];
    const size_t base_dir_len = base_bin_path_len - suffix_len;
    memcpy(base_dir, base_bin_path, base_dir_len);
    base_dir[base_dir_len] = '\0';
    free(base_bin_path);
    g_asset_locator.base_dir = strdup(base_dir);

    char shaders_dir[MAX_PATH];
    snprintf(shaders_dir, sizeof(shaders_dir), "%s%s", base_dir, "/shaders");
    g_asset_locator.shaders_dir = strdup(shaders_dir);
}

void asset_locator_fini()
{
    free(g_asset_locator.base_dir);
    g_asset_locator.base_dir = nullptr;

    free(g_asset_locator.shaders_dir);
    g_asset_locator.shaders_dir = nullptr;
}
