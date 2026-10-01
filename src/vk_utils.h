#ifndef VK_UTILS_H
#define VK_UTILS_H

#include <stdint.h>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

void vk_create_buffer(
    VmaAllocator vma,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VmaMemoryUsage memory_usage,
    VmaAllocationCreateFlags alloc_flags,
    VkBuffer* buffer,
    VmaAllocation* allocation,
    VmaAllocationInfo* alloc_info);

void vk_copy_buffer(
    VkDevice device,
    VkQueue graphics_queue,
    VkCommandPool command_pool,
    VkBuffer src_buffer,
    VkBuffer dst_buffer,
    VkDeviceSize size);

void vk_transition_image_layout(
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

void vk_create_image(
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

void vk_create_image_view(
    VkDevice device,
    VkImage image,
    VkFormat image_format,
    VkImageAspectFlags aspect_flags,
    uint32_t mip_levels,
    VkImageView* image_view);

void vk_prepare_image_layouts(
    VkDevice device,
    VkQueue queue,
    VkCommandPool command_pool,
    VkImage color_image,
    VkImage depth_image);

void vk_buffer_init(
    VmaAllocator vma,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VmaMemoryUsage memory_usage,
    VmaAllocationCreateFlags alloc_flags,
    VkBuffer* buffer,
    VmaAllocation* allocation,
    VmaAllocationInfo* alloc_info);

void vk_buffer_copy(
    VkDevice device,
    VkQueue graphics_queue,
    VkCommandPool command_pool,
    VkBuffer src_buffer,
    VkBuffer dst_buffer,
    VkDeviceSize size);

#endif  // VK_UTILS_H
