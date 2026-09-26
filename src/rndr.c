#include "rndr.h"

#include "SDL3/SDL_vulkan.h"

#include "app.h"
#include "asset_locator.h"
#include "vk_debug.h"
#include "vk_init_core.h"
#include "vk_init_rndr.h"

Rndr g_rndr = {};

void rndr_init()
{
    vk_device_init();
    vk_allocator_init();

    vk_swap_chain_init(
        g_app.width, g_app.height, g_rndr.gpu, g_rndr.device, g_rndr.surface,
        VK_NULL_HANDLE, &g_rndr.swap_chain, &g_rndr.swap_chain_images, &g_rndr.swap_chain_images_count,
        &g_rndr.swap_chain_surface_format, &g_rndr.swap_chain_extent);

    vk_swap_chain_image_views_init(
        g_rndr.device, g_rndr.swap_chain_images, g_rndr.swap_chain_images_count, &g_rndr.swap_chain_surface_format,
        &g_rndr.swap_chain_image_views, &g_rndr.swap_chain_image_views_count);

    vk_descriptor_set_layout_init(g_rndr.device, &g_rndr.descriptor_set_layout);

    vk_graphics_pipeline_init(
        g_asset_locator.shaders_dir, g_rndr.gpu, g_rndr.device, &g_rndr.swap_chain_surface_format,
        g_rndr.descriptor_set_layout, g_rndr.msaa_samples, &g_rndr.pipeline_layout, &g_rndr.graphics_pipeline);

    vk_command_pool_init(g_rndr.device, g_rndr.queue_family_indices.graphics_family, &g_rndr.command_pool);

    g_rndr.color_format = g_rndr.swap_chain_surface_format.format;

    vk_color_resources_init(
        g_rndr.device, g_rndr.vma, &g_rndr.swap_chain_extent, g_rndr.swap_chain_surface_format.format,
        g_rndr.msaa_samples, &g_rndr.color_image, &g_rndr.color_image_alloc, &g_rndr.color_image_view);

    vk_find_depth_format(g_rndr.gpu, &g_rndr.depth_format);

    vk_depth_resources_init(
        g_rndr.gpu, g_rndr.device, g_rndr.vma, &g_rndr.swap_chain_extent, g_rndr.msaa_samples, &g_rndr.depth_image,
        &g_rndr.depth_image_alloc, &g_rndr.depth_image_view);
}

void rndr_fini()
{
    vkDeviceWaitIdle(g_rndr.device);

    vk_image_resources_fini(
        g_rndr.device, g_rndr.vma, &g_rndr.depth_image, &g_rndr.depth_image_alloc, &g_rndr.depth_image_view);

    vk_image_resources_fini(
        g_rndr.device, g_rndr.vma, &g_rndr.color_image, &g_rndr.color_image_alloc, &g_rndr.color_image_view);

    vkDestroyCommandPool(g_rndr.device, g_rndr.command_pool, nullptr);

    vkDestroyPipeline(g_rndr.device, g_rndr.graphics_pipeline, nullptr);
    vkDestroyPipelineLayout(g_rndr.device, g_rndr.pipeline_layout, nullptr);

    vkDestroyDescriptorSetLayout(g_rndr.device, g_rndr.descriptor_set_layout, nullptr);

    vk_swap_chain_image_views_fini(g_rndr.device, &g_rndr.swap_chain_image_views, &g_rndr.swap_chain_image_views_count);

    vk_swap_chain_fini(
        g_rndr.device, g_rndr.swap_chain, &g_rndr.swap_chain_images, &g_rndr.swap_chain_images_count,
        &g_rndr.swap_chain_image_views, &g_rndr.swap_chain_image_views_count);

    vmaDestroyAllocator(g_rndr.vma);

    vkDestroyDevice(g_rndr.device, nullptr);

#ifndef NDEBUG
    if (g_rndr.instance != VK_NULL_HANDLE) {
        vk_debug_utils_fini(g_rndr.instance);
    }
#endif

    SDL_Vulkan_DestroySurface(g_rndr.instance, g_rndr.surface, nullptr);
    vkDestroyInstance(g_rndr.instance, nullptr);
}

static void rndr_recreate_swap_chain()
{
    vkDeviceWaitIdle(g_rndr.device);

    vk_image_resources_fini(
        g_rndr.device, g_rndr.vma, &g_rndr.depth_image, &g_rndr.depth_image_alloc, &g_rndr.depth_image_view);

    vk_image_resources_fini(
        g_rndr.device, g_rndr.vma, &g_rndr.color_image, &g_rndr.color_image_alloc, &g_rndr.color_image_view);

    vk_swap_chain_images_fini(
        g_rndr.device, &g_rndr.swap_chain_images, &g_rndr.swap_chain_images_count, &g_rndr.swap_chain_image_views,
        &g_rndr.swap_chain_image_views_count);

    VkSwapchainKHR old_swap_chain = g_rndr.swap_chain;
    g_rndr.swap_chain = VK_NULL_HANDLE;

    vk_swap_chain_init(
        g_app.width, g_app.height, g_rndr.gpu, g_rndr.device, g_rndr.surface,
        VK_NULL_HANDLE, &g_rndr.swap_chain, &g_rndr.swap_chain_images, &g_rndr.swap_chain_images_count,
        &g_rndr.swap_chain_surface_format, &g_rndr.swap_chain_extent);
    vkDestroySwapchainKHR(g_rndr.device, old_swap_chain, nullptr);

    vk_swap_chain_image_views_init(
        g_rndr.device, g_rndr.swap_chain_images, g_rndr.swap_chain_images_count, &g_rndr.swap_chain_surface_format,
        &g_rndr.swap_chain_image_views, &g_rndr.swap_chain_image_views_count);

    vk_color_resources_init(
        g_rndr.device, g_rndr.vma, &g_rndr.swap_chain_extent, g_rndr.swap_chain_surface_format.format,
        g_rndr.msaa_samples, &g_rndr.color_image, &g_rndr.color_image_alloc, &g_rndr.color_image_view);

    vk_depth_resources_init(
        g_rndr.gpu, g_rndr.device, g_rndr.vma, &g_rndr.swap_chain_extent, g_rndr.msaa_samples, &g_rndr.depth_image,
        &g_rndr.depth_image_alloc, &g_rndr.depth_image_view);
}

