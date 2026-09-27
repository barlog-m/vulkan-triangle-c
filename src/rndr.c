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

    vk_swap_chain_fini(g_rndr.device, g_rndr.swap_chain, &g_rndr.swap_chain_images, &g_rndr.swap_chain_images_count,
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

    VkSwapchainKHR old_swap_chain = g_rndr.swap_chain;
    
    vk_image_resources_fini(
        g_rndr.device, g_rndr.vma, &g_rndr.depth_image, &g_rndr.depth_image_alloc, &g_rndr.depth_image_view);

    vk_image_resources_fini(
        g_rndr.device, g_rndr.vma, &g_rndr.color_image, &g_rndr.color_image_alloc, &g_rndr.color_image_view);

    vk_swap_chain_image_resources_clean(
        g_rndr.device, &g_rndr.swap_chain_images, &g_rndr.swap_chain_images_count,
        &g_rndr.swap_chain_image_views, &g_rndr.swap_chain_image_views_count);
    
    g_rndr.swap_chain = VK_NULL_HANDLE;

    vk_swap_chain_init(
        g_app.width, g_app.height, g_rndr.gpu, g_rndr.device, g_rndr.surface, old_swap_chain, &g_rndr.swap_chain,
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
    
    vk_prepare_image_layouts(
        g_rndr.device, g_rndr.graphics_queue, g_rndr.command_pool, g_rndr.color_image, g_rndr.depth_image);
}

static void rndr_record_command_buffer(VkCommandBuffer command_buffer, uint32_t image_index, const Mesh* mesh)
{
    ASSERT_VK(vkResetCommandBuffer(command_buffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT));

    const VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    ASSERT_VK(vkBeginCommandBuffer(command_buffer, &begin_info));

    VkImage swap_chain_image = g_rndr.swap_chain_images[image_index];
    VkImageView swap_chain_image_view = g_rndr.swap_chain_image_views[image_index];

    vk_transition_image_layout(
        command_buffer, swap_chain_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, 0,
        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_IMAGE_ASPECT_COLOR_BIT, 1);

    const VkClearValue color_clear_value = {
        .color = { .float32 = { 0.0F, 0.0F, 0.0F, 1.0F } },
    };
    const VkClearValue depth_clear_value = {
        .depthStencil = { .depth = 1.0F, .stencil = 0 },
    };

    const VkRenderingAttachmentInfo color_attachment_info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = g_rndr.color_image_view,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
        .resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT,
        .resolveImageView = swap_chain_image_view,
        .resolveImageLayout = VK_IMAGE_LAYOUT_GENERAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .clearValue = color_clear_value,
    };

    const VkRenderingAttachmentInfo depth_attachment_info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = g_rndr.depth_image_view,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .clearValue = depth_clear_value,
    };

    const VkRenderingInfo rendering_info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = { .offset = { 0, 0 }, .extent = g_rndr.swap_chain_extent },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_info,
        .pDepthAttachment = &depth_attachment_info,
    };

    vkCmdBeginRendering(command_buffer, &rendering_info);
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, g_rndr.graphics_pipeline);

    const VkViewport viewport = {
        .x = 0.0F,
        .y = 0.0F,
        .width = (float)g_rndr.swap_chain_extent.width,
        .height = (float)g_rndr.swap_chain_extent.height,
        .minDepth = 0.0F,
        .maxDepth = 1.0F,
    };
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    const VkRect2D scissor = {
        .offset = { 0, 0 },
        .extent = g_rndr.swap_chain_extent,
    };
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    const VkBuffer vertex_buffers[] = { mesh->vertex_buffer };
    constexpr VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
    vkCmdBindIndexBuffer(command_buffer, mesh->index_buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(command_buffer, mesh->indices_count, 1, 0, 0, 0);

    vkCmdEndRendering(command_buffer);

    vk_transition_image_layout(
        command_buffer, swap_chain_image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, 0, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_IMAGE_ASPECT_COLOR_BIT, 1);

    ASSERT_VK(vkEndCommandBuffer(command_buffer));
}

void rndr_draw_frame(const Mesh* mesh)
{
    if (g_app.width == 0 || g_app.height == 0) {
        g_app.is_resized = true;
        return;
    }

    const uint32_t frame_index = g_rndr.current_frame_index;

    const uint64_t wait_value =
        g_rndr.frame_number > MAX_FRAMES_IN_FLIGHT - 1
            ? g_rndr.frame_number - (MAX_FRAMES_IN_FLIGHT - 1)
            : 0;
    const VkSemaphoreWaitInfo timeline_wait_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
        .semaphoreCount = 1,
        .pSemaphores = &g_rndr.render_timeline_semaphore,
        .pValues = &wait_value,
    };
    ASSERT_VK(vkWaitSemaphores(g_rndr.device, &timeline_wait_info, UINT64_MAX));

    // Note: image_available_semaphores and command_buffers are indexed by frame_index,
    //       while render_finished_semaphores is indexed by image_index
    bool needs_recreate = g_app.is_resized;
    if (g_app.width != g_rndr.swap_chain_extent.width
        || g_app.height != g_rndr.swap_chain_extent.height) {
        needs_recreate = true;
    } else {
        g_app.is_resized = false;
    }

    VkSemaphore present_complete_semaphore = g_rndr.image_available_semaphores[frame_index];

    uint32_t image_index;
    const VkResult acquire_result = vkAcquireNextImageKHR(
        g_rndr.device, g_rndr.swap_chain, UINT64_MAX,
        present_complete_semaphore, VK_NULL_HANDLE, &image_index);

    switch (acquire_result) {
        case VK_SUCCESS:
            break;
        case VK_SUBOPTIMAL_KHR:
            needs_recreate = true;
            break;
        case VK_ERROR_OUT_OF_DATE_KHR:
            rndr_recreate_swap_chain();
            return;
        case VK_NOT_READY:
        case VK_TIMEOUT:
            return;
        default:
            LOG_FATAL("vkAcquireNextImageKHR failed");
            abort();
    }

    ASSERT_MSG(image_index < MAX_SWAPCHAIN_IMAGES, "image_index out of render_finished_semaphores bounds");

    VkCommandBuffer command_buffer = g_rndr.command_buffers[frame_index];

    rndr_record_command_buffer(command_buffer, image_index, mesh);

    // Submit
    const uint64_t signal_value = g_rndr.frame_number + 1;

    const VkSemaphoreSubmitInfo wait_semaphore_infos[] = {
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = present_complete_semaphore,
            .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        },
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = g_rndr.render_timeline_semaphore,
            .value = g_rndr.frame_number,
            .stageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
        },
    };
    const VkCommandBufferSubmitInfo command_buffer_infos[] = {
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .commandBuffer = command_buffer,
        },
    };
    const VkSemaphoreSubmitInfo signal_semaphore_infos[] = {
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = g_rndr.render_finished_semaphores[image_index],
            .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        },
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = g_rndr.render_timeline_semaphore,
            .value = signal_value,
            .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        },
    };
    const VkSubmitInfo2 submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .waitSemaphoreInfoCount = 2,
        .pWaitSemaphoreInfos = wait_semaphore_infos,
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = command_buffer_infos,
        .signalSemaphoreInfoCount = 2,
        .pSignalSemaphoreInfos = signal_semaphore_infos,
    };
    ASSERT_VK(vkQueueSubmit2(g_rndr.graphics_queue, 1, &submit_info, VK_NULL_HANDLE));

    // Present
    const VkSemaphore wait_semaphores[] = { g_rndr.render_finished_semaphores[image_index] };
    const VkSwapchainKHR swapchains[] = { g_rndr.swap_chain };
    const VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = wait_semaphores,
        .swapchainCount = 1,
        .pSwapchains = swapchains,
        .pImageIndices = &image_index,
    };

    const VkResult present_result = vkQueuePresentKHR(g_rndr.graphics_queue, &present_info);
    switch (present_result) {
        case VK_SUCCESS:
            break;
        case VK_SUBOPTIMAL_KHR:
        case VK_ERROR_OUT_OF_DATE_KHR:
            needs_recreate = true;
            break;
        default:
            LOG_FATAL("vkQueuePresentKHR failed");
            abort();
    }

    if (needs_recreate) {
        rndr_recreate_swap_chain();
    }

    g_rndr.frame_number += 1;
    g_rndr.current_frame_index = (g_rndr.current_frame_index + 1) % MAX_FRAMES_IN_FLIGHT;
}
