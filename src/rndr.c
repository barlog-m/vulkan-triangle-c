#include "rndr.h"

#include "vk_debug.h"
#include "vk_device_init.h"
#include "SDL3/SDL_vulkan.h"

Rndr g_rndr = {};

void rndr_init()
{
    vk_device_init();
}

void rndr_fini()
{
    vkDeviceWaitIdle(g_rndr.device);

#ifndef NDEBUG
    if (g_rndr.instance != VK_NULL_HANDLE) {
        vk_debug_utils_fini(g_rndr.instance);
    }
#endif

    SDL_Vulkan_DestroySurface(g_rndr.instance, g_rndr.surface, nullptr);
    vkDestroyInstance(g_rndr.instance, nullptr);
}
