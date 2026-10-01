#include "rndr.h"

#include "SDL3/SDL_vulkan.h"

#include "assert.h"
#include "mesh.h"
#include "vk_debug.h"
#include "vk_init_core.h"
#include "vk_init_rndr.h"

Rndr* rndr_init(AssetLocator* asset_locator, Window* window)
{
    Rndr* self = calloc(1, sizeof(Rndr));
    self->asset_locator = asset_locator;
    self->window = window;
    
    vk_device_init(self);
    vk_allocator_init(self);

    vk_swap_chain_init(
        window->width, window->height, self->gpu, self->device, self->surface, VK_NULL_HANDLE, &self->swap_chain,
        &self->swap_chain_images, &self->swap_chain_images_count, &self->swap_chain_surface_format,
        &self->swap_chain_extent);

    vk_swap_chain_image_views_init(
        self->device, self->swap_chain_images, self->swap_chain_images_count, &self->swap_chain_surface_format,
        &self->swap_chain_image_views, &self->swap_chain_image_views_count);

    self->color_format = self->swap_chain_surface_format.format;

    vk_color_resources_init(
        self->device, self->vma, &self->swap_chain_extent, self->swap_chain_surface_format.format,
        self->msaa_samples, &self->color_image, &self->color_image_alloc, &self->color_image_view);

    vk_find_depth_format(self->gpu, &self->depth_format);

    vk_depth_resources_init(
        self->gpu, self->device, self->vma, &self->swap_chain_extent, self->msaa_samples, &self->depth_image,
        &self->depth_image_alloc, &self->depth_image_view);

    vk_descriptor_set_layout_init(self->device, &self->descriptor_set_layout);

    vk_graphics_pipeline_init(
        self->asset_locator->shaders_dir, self->gpu, self->device, &self->swap_chain_surface_format,
        self->descriptor_set_layout, self->msaa_samples, &self->pipeline_layout, &self->graphics_pipeline);

    vk_command_pool_init(self->device, self->queue_family_indices.graphics_family, &self->command_pool);

    vk_command_buffers_init(self->device, self->command_pool, self->command_buffers);

    vk_prepare_image_layouts(
        self->device, self->graphics_queue, self->command_pool, self->color_image, self->depth_image);

    vk_sync_objects_init(
        self->device, self->image_available_semaphores, self->render_finished_semaphores,
        &self->render_timeline_semaphore);
   
    self->current_frame_index = 0;
    self->frame_number = 0;
    
    return self;
}

void rndr_fini(Rndr* self)
{
    vkDeviceWaitIdle(self->device);

    vk_sync_objects_fini(
        self->device, self->image_available_semaphores, self->render_finished_semaphores,
        self->render_timeline_semaphore);

    vkDestroyCommandPool(self->device, self->command_pool, nullptr);

    vkDestroyPipeline(self->device, self->graphics_pipeline, nullptr);
    vkDestroyPipelineLayout(self->device, self->pipeline_layout, nullptr);

    vkDestroyDescriptorSetLayout(self->device, self->descriptor_set_layout, nullptr);

    vk_image_resources_fini(
        self->device, self->vma, &self->depth_image, &self->depth_image_alloc, &self->depth_image_view);

    vk_image_resources_fini(
        self->device, self->vma, &self->color_image, &self->color_image_alloc, &self->color_image_view);

    vk_swap_chain_fini(self->device, self->swap_chain, &self->swap_chain_images, &self->swap_chain_images_count,
        &self->swap_chain_image_views, &self->swap_chain_image_views_count); 

    vmaDestroyAllocator(self->vma);

    vkDestroyDevice(self->device, nullptr);

#ifndef NDEBUG
    if (self->instance != VK_NULL_HANDLE) {
        vk_debug_utils_fini(self->instance);
    }
#endif

    SDL_Vulkan_DestroySurface(self->instance, self->surface, nullptr);
    vkDestroyInstance(self->instance, nullptr);
}

static void rndr_recreate_swap_chain(Rndr* self)
{
    vkDeviceWaitIdle(self->device);

    VkSwapchainKHR old_swap_chain = self->swap_chain;
    
    vk_image_resources_fini(
        self->device, self->vma, &self->depth_image, &self->depth_image_alloc, &self->depth_image_view);

    vk_image_resources_fini(
        self->device, self->vma, &self->color_image, &self->color_image_alloc, &self->color_image_view);

    vk_swap_chain_image_resources_clean(
        self->device, &self->swap_chain_images, &self->swap_chain_images_count,
        &self->swap_chain_image_views, &self->swap_chain_image_views_count);
    
    self->swap_chain = VK_NULL_HANDLE;

    vk_swap_chain_init(
        self->window->width, self->window->height, self->gpu, self->device, self->surface, old_swap_chain, &self->swap_chain,
        &self->swap_chain_images, &self->swap_chain_images_count, &self->swap_chain_surface_format,
        &self->swap_chain_extent);
    vkDestroySwapchainKHR(self->device, old_swap_chain, nullptr);

    vk_swap_chain_image_views_init(
        self->device, self->swap_chain_images, self->swap_chain_images_count, &self->swap_chain_surface_format,
        &self->swap_chain_image_views, &self->swap_chain_image_views_count);

    vk_color_resources_init(
        self->device, self->vma, &self->swap_chain_extent, self->swap_chain_surface_format.format,
        self->msaa_samples, &self->color_image, &self->color_image_alloc, &self->color_image_view);

    vk_depth_resources_init(
        self->gpu, self->device, self->vma, &self->swap_chain_extent, self->msaa_samples, &self->depth_image,
        &self->depth_image_alloc, &self->depth_image_view);
    
    vk_prepare_image_layouts(
        self->device, self->graphics_queue, self->command_pool, self->color_image, self->depth_image);
}

static void rndr_record_command_buffer(Rndr* self, VkCommandBuffer command_buffer, uint32_t image_index, const Mesh* mesh)
{
    ASSERT_VK(vkResetCommandBuffer(command_buffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT));

    const VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    ASSERT_VK(vkBeginCommandBuffer(command_buffer, &begin_info));

    VkImage swap_chain_image = self->swap_chain_images[image_index];
    VkImageView swap_chain_image_view = self->swap_chain_image_views[image_index];

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
        .imageView = self->color_image_view,
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
        .imageView = self->depth_image_view,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .clearValue = depth_clear_value,
    };

    const VkRenderingInfo rendering_info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = { .offset = { .x = 0, .y = 0 }, .extent = self->swap_chain_extent },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_info,
        .pDepthAttachment = &depth_attachment_info,
    };

    vkCmdBeginRendering(command_buffer, &rendering_info);
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, self->graphics_pipeline);

    const VkViewport viewport = {
        .x = 0.0F,
        .y = 0.0F,
        .width = (float)self->swap_chain_extent.width,
        .height = (float)self->swap_chain_extent.height,
        .minDepth = 0.0F,
        .maxDepth = 1.0F,
    };
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    const VkRect2D scissor = {
        .offset = { 0, 0 },
        .extent = self->swap_chain_extent,
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

void rndr_draw_frame(Rndr* self, const Mesh* mesh)
{
    if (window_is_zero_size(self->window)) {
        return;
    }

    const uint32_t frame_index = self->current_frame_index;

    const uint64_t wait_value =
        self->frame_number > MAX_FRAMES_IN_FLIGHT - 1
            ? self->frame_number - (MAX_FRAMES_IN_FLIGHT - 1)
            : 0;
    const VkSemaphoreWaitInfo timeline_wait_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
        .semaphoreCount = 1,
        .pSemaphores = &self->render_timeline_semaphore,
        .pValues = &wait_value,
    };
    ASSERT_VK(vkWaitSemaphores(self->device, &timeline_wait_info, UINT64_MAX));

    bool needs_recreate =
        self->window->width != self->swap_chain_extent.width
        || self->window->height != self->swap_chain_extent.height;

    // Note: image_available_semaphores and command_buffers are indexed by frame_index,
    //       while render_finished_semaphores is indexed by image_index
    VkSemaphore present_complete_semaphore = self->image_available_semaphores[frame_index];

    uint32_t image_index;
    const VkResult acquire_result = vkAcquireNextImageKHR(
        self->device, self->swap_chain, UINT64_MAX,
        present_complete_semaphore, VK_NULL_HANDLE, &image_index);

    switch (acquire_result) {
        case VK_SUCCESS:
            break;
        case VK_SUBOPTIMAL_KHR:
            needs_recreate = true;
            break;
        case VK_ERROR_OUT_OF_DATE_KHR:
            rndr_recreate_swap_chain(self);
            return;
        case VK_NOT_READY:
        case VK_TIMEOUT:
            return;
        default:
            LOG_FATAL("vkAcquireNextImageKHR failed");
            abort();
    }

    ASSERT_MSG(image_index < MAX_SWAPCHAIN_IMAGES, "image_index out of render_finished_semaphores bounds");

    VkCommandBuffer command_buffer = self->command_buffers[frame_index];

    rndr_record_command_buffer(self, command_buffer, image_index, mesh);

    // Submit
    const uint64_t signal_value = self->frame_number + 1;

    const VkSemaphoreSubmitInfo wait_semaphore_infos[] = {
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = present_complete_semaphore,
            .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        },
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = self->render_timeline_semaphore,
            .value = self->frame_number,
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
            .semaphore = self->render_finished_semaphores[image_index],
            .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        },
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = self->render_timeline_semaphore,
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
    ASSERT_VK(vkQueueSubmit2(self->graphics_queue, 1, &submit_info, VK_NULL_HANDLE));

    // Present
    const VkSemaphore wait_semaphores[] = { self->render_finished_semaphores[image_index] };
    const VkSwapchainKHR swapchains[] = { self->swap_chain };
    const VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = wait_semaphores,
        .swapchainCount = 1,
        .pSwapchains = swapchains,
        .pImageIndices = &image_index,
    };

    const VkResult present_result = vkQueuePresentKHR(self->graphics_queue, &present_info);
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
        rndr_recreate_swap_chain(self);
    }

    self->frame_number += 1;
    self->current_frame_index = (self->current_frame_index + 1) % MAX_FRAMES_IN_FLIGHT;
}
