#include "rndr.h"

#include "SDL3/SDL_vulkan.h"

#include "app.h"
#include "assert.h"
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
        g_app.width, g_app.height, g_rndr.gpu, g_rndr.device, g_rndr.surface, VK_NULL_HANDLE, &g_rndr.swap_chain,
        &g_rndr.swap_chain_images, &g_rndr.swap_chain_images_count, &g_rndr.swap_chain_surface_format,
        &g_rndr.swap_chain_extent);

    vk_swap_chain_image_views_init(
        g_rndr.device, g_rndr.swap_chain_images, g_rndr.swap_chain_images_count, &g_rndr.swap_chain_surface_format,
        &g_rndr.swap_chain_image_views, &g_rndr.swap_chain_image_views_count);

    g_rndr.color_format = g_rndr.swap_chain_surface_format.format;

    vk_color_resources_init(
        g_rndr.device, g_rndr.vma, &g_rndr.swap_chain_extent, g_rndr.swap_chain_surface_format.format,
        g_rndr.msaa_samples, &g_rndr.color_image, &g_rndr.color_image_alloc, &g_rndr.color_image_view);

    vk_find_depth_format(g_rndr.gpu, &g_rndr.depth_format);

    vk_depth_resources_init(
        g_rndr.gpu, g_rndr.device, g_rndr.vma, &g_rndr.swap_chain_extent, g_rndr.msaa_samples, &g_rndr.depth_image,
        &g_rndr.depth_image_alloc, &g_rndr.depth_image_view);

    vk_descriptor_set_layout_init(g_rndr.device, &g_rndr.descriptor_set_layout);

    vk_graphics_pipeline_init(
        g_asset_locator.shaders_dir, g_rndr.gpu, g_rndr.device, &g_rndr.swap_chain_surface_format,
        g_rndr.descriptor_set_layout, g_rndr.msaa_samples, &g_rndr.pipeline_layout, &g_rndr.graphics_pipeline);

    vk_command_pool_init(g_rndr.device, g_rndr.queue_family_indices.graphics_family, &g_rndr.command_pool);

    vk_command_buffers_init(g_rndr.device, g_rndr.command_pool, g_rndr.command_buffers);

    vk_prepare_image_layouts(
        g_rndr.device, g_rndr.graphics_queue, g_rndr.command_pool, g_rndr.color_image, g_rndr.depth_image);

    vk_sync_objects_init(
        g_rndr.device, g_rndr.image_available_semaphores, g_rndr.render_finished_semaphores,
        &g_rndr.render_timeline_semaphore);
}

void rndr_fini()
{
    vkDeviceWaitIdle(g_rndr.device);

    vk_sync_objects_fini(
        g_rndr.device, g_rndr.image_available_semaphores, g_rndr.render_finished_semaphores,
        g_rndr.render_timeline_semaphore);

    vkDestroyCommandPool(g_rndr.device, g_rndr.command_pool, nullptr);

    vkDestroyPipeline(g_rndr.device, g_rndr.graphics_pipeline, nullptr);
    vkDestroyPipelineLayout(g_rndr.device, g_rndr.pipeline_layout, nullptr);

    vkDestroyDescriptorSetLayout(g_rndr.device, g_rndr.descriptor_set_layout, nullptr);

    vk_image_resources_fini(
        g_rndr.device, g_rndr.vma, &g_rndr.depth_image, &g_rndr.depth_image_alloc, &g_rndr.depth_image_view);

    vk_image_resources_fini(
        g_rndr.device, g_rndr.vma, &g_rndr.color_image, &g_rndr.color_image_alloc, &g_rndr.color_image_view);

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
        g_app.width, g_app.height, g_rndr.gpu, g_rndr.device, g_rndr.surface, VK_NULL_HANDLE, &g_rndr.swap_chain,
        &g_rndr.swap_chain_images, &g_rndr.swap_chain_images_count, &g_rndr.swap_chain_surface_format,
        &g_rndr.swap_chain_extent);
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

void rndr_draw_frame(const Mesh* mesh)
{
    /*
    uint32_t image_index;
    VkResult result = vkAcquireNextImageKHR(
        g_rndr.device, g_rndr.swap_chain, UINT64_MAX, g_rndr.image_available_semaphores[g_rndr.current_frame_index],
        VK_NULL_HANDLE, &image_index);
    assert(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || image_index < MAX_SWAPCHAIN_IMAGES);

    const uint32_t frame = g_rndr.current_frame_index;

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        // image_available was NOT signaled by a failed acquire — skip frame, just recreate.
        // Drain any pending compute semaphore so it can be re-signaled next frame.
        if (g_rndr.compute_submitted[frame]) {
            u_vk_consume_semaphore(g_rndr.graphics_queue, g_rndr.compute_finished_semaphores[frame]);
            g_rndr.compute_submitted[frame] = false;
        }
        g_app.is_resized = false;
        u_rndr_swap_chain_recreate();
        return;
    }

    if (result == VK_SUBOPTIMAL_KHR || g_app.is_resized) {
        // image_available WAS signaled — consume it and any compute semaphore before recreating.
        u_vk_consume_semaphore(g_rndr.graphics_queue, g_rndr.image_available_semaphores[frame]);
        if (g_rndr.compute_submitted[frame]) {
            u_vk_consume_semaphore(g_rndr.graphics_queue, g_rndr.compute_finished_semaphores[frame]);
            g_rndr.compute_submitted[frame] = false;
        }
        g_app.is_resized = false;
        u_rndr_swap_chain_recreate();
        return;
    }

    u_vk_assert(vkResetCommandBuffer(g_rndr.command_buffers[g_rndr.current_frame_index], 0));

    u_vk_record_command_buffer(
        g_rndr.swap_chain_images, g_rndr.swap_chain_image_views, g_rndr.swap_chain_extent,
        g_rndr.command_buffers[g_rndr.current_frame_index], image_index, g_rndr.current_frame_index, g_rndr.depth_image,
        g_rndr.depth_image_view, g_rndr.color_image, g_rndr.color_image_view, g_rndr.query_pool,
        g_rndr.timestamp_query_pool);

    const uint64_t signal_timeline_value = g_rndr.frame_timeline_value + 1;

    // Build wait semaphore list: always wait on image_available; also wait on
    // compute_finished when compute was submitted this frame.
    VkSemaphoreSubmitInfo wait_semaphore_infos[2];
    uint32_t wait_count = 0;

    wait_semaphore_infos[wait_count++] = (VkSemaphoreSubmitInfo){
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore = g_rndr.image_available_semaphores[frame],
        .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
    };

    if (g_rndr.compute_submitted[frame]) {
        wait_semaphore_infos[wait_count++] = (VkSemaphoreSubmitInfo){
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = g_rndr.compute_finished_semaphores[frame],
            .stageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
        };
        g_rndr.compute_submitted[frame] = false;
    }

    const VkSemaphoreSubmitInfo signal_semaphore_infos[] = {
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = g_rndr.render_finished_semaphores[image_index],
            .stageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
        },
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = g_rndr.render_timeline_semaphore,
            .value = signal_timeline_value,
            .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        },
    };
    const VkCommandBufferSubmitInfo cmd_submit_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .commandBuffer = g_rndr.command_buffers[frame],
    };
    const VkSubmitInfo2 submit_info2 = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .waitSemaphoreInfoCount = wait_count,
        .pWaitSemaphoreInfos = wait_semaphore_infos,
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &cmd_submit_info,
        .signalSemaphoreInfoCount = 2,
        .pSignalSemaphoreInfos = signal_semaphore_infos,
    };

    u_vk_assert(vkQueueSubmit2(g_rndr.graphics_queue, 1, &submit_info2, VK_NULL_HANDLE));

    g_rndr.frame_timeline_value++;

    const VkSemaphore render_finished = g_rndr.render_finished_semaphores[image_index];
    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &render_finished,
    };

    const VkSwapchainKHR swap_chains[] = { g_rndr.swap_chain };
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swap_chains;
    present_info.pImageIndices = &image_index;

    present_info.pResults = nullptr;
    result = vkQueuePresentKHR(g_rndr.present_queue, &present_info);

    if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR) {
        u_rndr_swap_chain_recreate();
    }

    g_rndr.current_frame_index = (g_rndr.current_frame_index + 1) % MAX_FRAMES_IN_FLIGHT;
    */
}
