#include "vk_utils.h"

#include "assert.h"

static void u_vk_render_target_sampler_init(VkDevice device, VkSampler* sampler)
{
    const VkSamplerCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .mipLodBias = 0.0f,
        .anisotropyEnable = VK_FALSE,
        .compareEnable = VK_FALSE,
        .minLod = 0.0f,
        .maxLod = 0.0f,
        .borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
    };
    ASSERT_VK(vkCreateSampler(device, &info, nullptr, sampler));
}

static void u_vk_render_target_transition_to_general(
    VkDevice      device,
    VkQueue       queue,
    VkCommandPool command_pool,
    VkImage       image)
{
    const VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandPool = command_pool,
        .commandBufferCount = 1,
    };
    VkCommandBuffer cmd;
    ASSERT_VK(vkAllocateCommandBuffers(device, &alloc_info, &cmd));

    const VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    ASSERT_VK(vkBeginCommandBuffer(cmd, &begin_info));

    u_vk_transition_image_layout(
        cmd, image,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
        0, VK_ACCESS_2_SHADER_WRITE_BIT,
        VK_PIPELINE_STAGE_2_NONE, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT, 1);

    ASSERT_VK(vkEndCommandBuffer(cmd));

    const VkCommandBufferSubmitInfo cmd_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .commandBuffer = cmd,
    };
    const VkSubmitInfo2 submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &cmd_info,
    };
    ASSERT_VK(vkQueueSubmit2(queue, 1, &submit_info, VK_NULL_HANDLE));
    vkQueueWaitIdle(queue);
    vkFreeCommandBuffers(device, command_pool, 1, &cmd);
}

// Shared image-creation core used by all render target init variants.
static void u_vk_render_target_create_image(
    VmaAllocator   vma,
    VkImageType    image_type,
    VkExtent3D     extent,
    VkFormat       format,
    VkSharingMode  sharing_mode,
    const uint32_t* queue_families,
    uint32_t        queue_family_count,
    VkImage*       image_out,
    VmaAllocation* alloc_out)
{
    const VkImageCreateInfo image_info = {
        .sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType             = image_type,
        .format                = format,
        .extent                = extent,
        .mipLevels             = 1,
        .arrayLayers           = 1,
        .samples               = VK_SAMPLE_COUNT_1_BIT,
        .tiling                = VK_IMAGE_TILING_OPTIMAL,
        .usage                 = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .sharingMode           = sharing_mode,
        .queueFamilyIndexCount = queue_family_count,
        .pQueueFamilyIndices   = queue_families,
        .initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    const VmaAllocationCreateInfo alloc_info = { .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE };
    ASSERT_VK(vmaCreateImage(vma, &image_info, &alloc_info, image_out, alloc_out, nullptr));
}

void u_vk_render_target_init(
    VkDevice      device,
    VmaAllocator  vma,
    VkQueue       queue,
    VkCommandPool command_pool,
    uint32_t      width,
    uint32_t      height,
    VkFormat      format,
    URenderTarget* rt)
{
    rt->width  = width;
    rt->height = height;
    rt->depth  = 1;
    rt->format = format;

    u_vk_render_target_create_image(
        vma, VK_IMAGE_TYPE_2D,
        (VkExtent3D){ .width = width, .height = height, .depth = 1 },
        format, VK_SHARING_MODE_EXCLUSIVE, nullptr, 0,
        &rt->image, &rt->allocation);

    const VkImageViewCreateInfo view_info = {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image            = rt->image,
        .viewType         = VK_IMAGE_VIEW_TYPE_2D,
        .format           = format,
        .components       = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
                               VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY },
        .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 },
    };
    ASSERT_VK(vkCreateImageView(device, &view_info, nullptr, &rt->view));

    u_vk_render_target_sampler_init(device, &rt->sampler);
    u_vk_render_target_transition_to_general(device, queue, command_pool, rt->image);
}

void u_vk_render_target_init_concurrent(
    VkDevice       device,
    VmaAllocator   vma,
    VkQueue        queue,
    VkCommandPool  command_pool,
    uint32_t       width,
    uint32_t       height,
    VkFormat       format,
    const uint32_t families[2],
    URenderTarget* rt)
{
    // CONCURRENT requires ≥ 2 distinct queue family indices.
    // Fall back to EXCLUSIVE when both families are the same.
    const bool concurrent     = (families[0] != families[1]);
    const VkSharingMode mode  = concurrent ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
    const uint32_t      count = concurrent ? 2u : 0u;

    rt->width  = width;
    rt->height = height;
    rt->depth  = 1;
    rt->format = format;

    u_vk_render_target_create_image(
        vma, VK_IMAGE_TYPE_2D,
        (VkExtent3D){ .width = width, .height = height, .depth = 1 },
        format, mode, families, count,
        &rt->image, &rt->allocation);

    const VkImageViewCreateInfo view_info = {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image            = rt->image,
        .viewType         = VK_IMAGE_VIEW_TYPE_2D,
        .format           = format,
        .components       = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
                               VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY },
        .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 },
    };
    ASSERT_VK(vkCreateImageView(device, &view_info, nullptr, &rt->view));

    u_vk_render_target_sampler_init(device, &rt->sampler);
    u_vk_render_target_transition_to_general(device, queue, command_pool, rt->image);
}

void u_vk_render_target_3d_init(
    VkDevice      device,
    VmaAllocator  vma,
    VkQueue       queue,
    VkCommandPool command_pool,
    uint32_t      width,
    uint32_t      height,
    uint32_t      depth,
    VkFormat      format,
    URenderTarget* rt)
{
    rt->width  = width;
    rt->height = height;
    rt->depth  = depth;
    rt->format = format;

    u_vk_render_target_create_image(
        vma, VK_IMAGE_TYPE_3D,
        (VkExtent3D){ .width = width, .height = height, .depth = depth },
        format, VK_SHARING_MODE_EXCLUSIVE, nullptr, 0,
        &rt->image, &rt->allocation);

    const VkImageViewCreateInfo view_info = {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image            = rt->image,
        .viewType         = VK_IMAGE_VIEW_TYPE_3D,
        .format           = format,
        .components       = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
                               VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY },
        .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 },
    };
    ASSERT_VK(vkCreateImageView(device, &view_info, nullptr, &rt->view));

    u_vk_render_target_sampler_init(device, &rt->sampler);
    u_vk_render_target_transition_to_general(device, queue, command_pool, rt->image);
}

void u_vk_render_target_3d_init_concurrent(
    VkDevice       device,
    VmaAllocator   vma,
    VkQueue        queue,
    VkCommandPool  command_pool,
    uint32_t       width,
    uint32_t       height,
    uint32_t       depth,
    VkFormat       format,
    const uint32_t families[2],
    URenderTarget* rt)
{
    const bool concurrent     = (families[0] != families[1]);
    const VkSharingMode mode  = concurrent ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
    const uint32_t      count = concurrent ? 2u : 0u;

    rt->width  = width;
    rt->height = height;
    rt->depth  = depth;
    rt->format = format;

    u_vk_render_target_create_image(
        vma, VK_IMAGE_TYPE_3D,
        (VkExtent3D){ .width = width, .height = height, .depth = depth },
        format, mode, families, count,
        &rt->image, &rt->allocation);

    const VkImageViewCreateInfo view_info = {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image            = rt->image,
        .viewType         = VK_IMAGE_VIEW_TYPE_3D,
        .format           = format,
        .components       = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
                               VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY },
        .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 },
    };
    ASSERT_VK(vkCreateImageView(device, &view_info, nullptr, &rt->view));

    u_vk_render_target_sampler_init(device, &rt->sampler);
    u_vk_render_target_transition_to_general(device, queue, command_pool, rt->image);
}

void u_vk_render_target_fini(VkDevice device, VmaAllocator vma, URenderTarget* rt)
{
    vkDestroySampler(device, rt->sampler, nullptr);
    vkDestroyImageView(device, rt->view, nullptr);
    vmaDestroyImage(vma, rt->image, rt->allocation);
    *rt = (URenderTarget){};
}

void u_vk_buffer_init(
    VmaAllocator vma,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VmaMemoryUsage memory_usage,
    VmaAllocationCreateFlags alloc_flags,
    VkBuffer* buffer,
    VmaAllocation* allocation,
    VmaAllocationInfo* alloc_info)
{
    const VkBufferCreateInfo buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    const VmaAllocationCreateInfo alloc_create_info = {
        .flags = alloc_flags,
        .usage = memory_usage,
    };

    VmaAllocationInfo local_alloc_info = {};
    ASSERT_VK(vmaCreateBuffer(vma, &buffer_info, &alloc_create_info, buffer, allocation, &local_alloc_info));

    if (alloc_info) {
        *alloc_info = local_alloc_info;
    }
}

void u_vk_buffer_copy(
    VkDevice device,
    VkQueue graphics_queue,
    VkCommandPool command_pool,
    VkBuffer src_buffer,
    VkBuffer dst_buffer,
    VkDeviceSize size)
{
    const VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandPool = command_pool,
        .commandBufferCount = 1,
    };

    VkCommandBuffer command_buffer;
    ASSERT_VK(vkAllocateCommandBuffers(device, &alloc_info, &command_buffer));

    const VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    ASSERT_VK(vkBeginCommandBuffer(command_buffer, &begin_info));

    const VkBufferCopy copy_region = {
        .size = size,
    };
    vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);

    ASSERT_VK(vkEndCommandBuffer(command_buffer));

    const VkCommandBufferSubmitInfo cmd_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .commandBuffer = command_buffer,
    };
    const VkSubmitInfo2 submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &cmd_info,
    };

    ASSERT_VK(vkQueueSubmit2(graphics_queue, 1, &submit_info, VK_NULL_HANDLE));
    vkQueueWaitIdle(graphics_queue);

    vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
}

void u_vk_transition_image_layout(
    VkCommandBuffer command_buffer,
    VkImage image,
    VkImageLayout old_layout,
    VkImageLayout new_layout,
    VkAccessFlags2 src_access_mask,
    VkAccessFlags2 dst_access_mask,
    VkPipelineStageFlags2 src_stage_mask,
    VkPipelineStageFlags2 dst_stage_mask,
    VkImageAspectFlags image_aspect_flags,
    uint32_t mip_levels)
{
    const VkImageSubresourceRange subresource_range = {
        .aspectMask = image_aspect_flags,
        .baseMipLevel = 0,
        .levelCount = mip_levels,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };

    VkImageMemoryBarrier2 barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .pNext = nullptr,
        .srcStageMask = src_stage_mask,
        .srcAccessMask = src_access_mask,
        .dstStageMask = dst_stage_mask,
        .dstAccessMask = dst_access_mask,
        .oldLayout = old_layout,
        .newLayout = new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = subresource_range,
    };

    const VkDependencyInfo dependency_info = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext = nullptr,
        .dependencyFlags = 0,
        .memoryBarrierCount = 0,
        .pMemoryBarriers = nullptr,
        .bufferMemoryBarrierCount = 0,
        .pBufferMemoryBarriers = nullptr,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier,
    };

    vkCmdPipelineBarrier2(command_buffer, &dependency_info);
}

void u_vk_image_init(
    VmaAllocator vma,
    uint32_t width,
    uint32_t height,
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkSampleCountFlagBits msaa_samples,
    uint32_t mip_levels,
    VkImage* image,
    VmaAllocation* allocation)
{
    const VkImageCreateInfo image_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = { .width = width, .height = height, .depth = 1 },
        .mipLevels = mip_levels,
        .arrayLayers = 1,
        .samples = msaa_samples,
        .tiling = tiling,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    const VmaAllocationCreateInfo alloc_create_info = {
        .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
    };

    ASSERT_VK(vmaCreateImage(vma, &image_info, &alloc_create_info, image, allocation, nullptr));
}

void u_vk_image_view_init(
    VkDevice device,
    VkImage image,
    VkFormat image_format,
    VkImageAspectFlags aspect_flags,
    uint32_t mip_levels,
    VkImageView* image_view)
{
    const VkImageViewCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = image_format,
        .components =
            {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
        .subresourceRange =
            {
                .aspectMask = aspect_flags,
                .baseMipLevel = 0,
                .levelCount = mip_levels,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
    };

    ASSERT_VK(vkCreateImageView(device, &create_info, nullptr, image_view));
}

