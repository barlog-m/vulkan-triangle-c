#ifndef VK_UTILS_H
#define VK_UTILS_H

#include <stdint.h>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

// A GPU image usable as a compute storage image output and as a sampled input.
// Initialized to VK_IMAGE_LAYOUT_GENERAL.
typedef struct {
    VkImage       image;
    VmaAllocation allocation;
    VkImageView   view;
    VkSampler     sampler;
    VkFormat      format;
    uint32_t      width;
    uint32_t      height;
    uint32_t      depth;  // 1 for 2D, >1 for 3D
} URenderTarget;

// Create a 2D render target (STORAGE | SAMPLED, linear+clamp sampler, GENERAL layout).
// Uses VK_SHARING_MODE_EXCLUSIVE — for images accessed from a single queue family only.
void u_vk_render_target_init(
    VkDevice      device,
    VmaAllocator  vma,
    VkQueue       queue,
    VkCommandPool command_pool,
    uint32_t      width,
    uint32_t      height,
    VkFormat      format,
    URenderTarget* rt);

// Create a 2D render target accessible from multiple queue families concurrently.
// When families[0] == families[1] the image is created with EXCLUSIVE sharing instead
// (CONCURRENT requires at least 2 distinct queue family indices).
void u_vk_render_target_init_concurrent(
    VkDevice       device,
    VmaAllocator   vma,
    VkQueue        queue,
    VkCommandPool  command_pool,
    uint32_t       width,
    uint32_t       height,
    VkFormat       format,
    const uint32_t families[2],
    URenderTarget* rt);

// Create a 3D render target (STORAGE | SAMPLED, linear+clamp sampler, GENERAL layout).
// Uses VK_SHARING_MODE_EXCLUSIVE — for images accessed from a single queue family only.
void u_vk_render_target_3d_init(
    VkDevice      device,
    VmaAllocator  vma,
    VkQueue       queue,
    VkCommandPool command_pool,
    uint32_t      width,
    uint32_t      height,
    uint32_t      depth,
    VkFormat      format,
    URenderTarget* rt);

// Create a 3D render target accessible from multiple queue families concurrently.
// When families[0] == families[1] the image is created with EXCLUSIVE sharing instead.
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
    URenderTarget* rt);

void u_vk_render_target_fini(VkDevice device, VmaAllocator vma, URenderTarget* rt);

void u_vk_buffer_init(
    VmaAllocator vma,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VmaMemoryUsage memory_usage,
    VmaAllocationCreateFlags alloc_flags,
    VkBuffer* buffer,
    VmaAllocation* allocation,
    VmaAllocationInfo* alloc_info_out);

void u_vk_buffer_copy(
    VkDevice device,
    VkQueue graphics_queue,
    VkCommandPool command_pool,
    VkBuffer src_buffer,
    VkBuffer dst_buffer,
    VkDeviceSize size);

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
    uint32_t mip_levels);

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
    VmaAllocation* allocation);

void u_vk_image_view_init(
    VkDevice device,
    VkImage image,
    VkFormat image_format,
    VkImageAspectFlags aspect_flags,
    uint32_t mip_levels,
    VkImageView* image_view);

#endif  // U_VK_UTILS_H
