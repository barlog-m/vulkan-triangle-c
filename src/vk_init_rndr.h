#ifndef VK_INIT_RNDR_H
#define VK_INIT_RNDR_H

#include <vulkan/vulkan_core.h>

#include "vk_mem_alloc.h"

void vk_allocator_init();

void vk_swap_chain_init(
    uint32_t width,
    uint32_t height,
    VkPhysicalDevice gpu,
    VkDevice device,
    VkSurfaceKHR surface,
    VkSwapchainKHR old_swap_chain,
    VkSwapchainKHR* swap_chain,
    VkImage** swap_chain_images,
    uint32_t* swap_chain_images_count,
    VkSurfaceFormatKHR* swap_chain_surface_format,
    VkExtent2D* swap_chain_extent);

void vk_swap_chain_fini(
    VkDevice device,
    VkSwapchainKHR swap_chain,
    VkImage* swap_chain_images[],
    uint32_t* swap_chain_images_count,
    VkImageView* swap_chain_image_views[],
    uint32_t* swap_chain_image_views_count);

void vk_swap_chain_images_fini(
    VkDevice device,
    VkImage* swap_chain_images[],
    uint32_t* swap_chain_images_count,
    VkImageView* swap_chain_image_views[],
    uint32_t* swap_chain_image_views_count);

void vk_swap_chain_image_views_init(
    VkDevice device,
    const VkImage* swap_chain_images,
    uint32_t swap_chain_images_count,
    const VkSurfaceFormatKHR* swap_chain_surface_format,
    VkImageView** swap_chain_image_views,
    uint32_t* swap_chain_image_views_count);

void vk_swap_chain_image_views_fini(
    VkDevice device,
    VkImageView** swap_chain_image_views,
    uint32_t* swap_chain_image_views_count);

void vk_find_supported_format(
    VkPhysicalDevice gpu,
    const VkFormat candidates[],
    uint32_t candidates_size,
    VkImageTiling tiling,
    VkFormatFeatureFlags features,
    VkFormat* out_format);

void vk_find_depth_format(VkPhysicalDevice gpu, VkFormat* out_format);

void vk_depth_resources_init(
    VkPhysicalDevice gpu,
    VkDevice device,
    VmaAllocator vma,
    const VkExtent2D* swap_chain_extent,
    VkSampleCountFlagBits msaa_samples,
    VkImage* depth_image,
    VmaAllocation* depth_image_alloc,
    VkImageView* depth_image_view);

void vk_color_resources_init(
    VkDevice device,
    VmaAllocator vma,
    const VkExtent2D* swap_chain_extent,
    VkFormat image_format,
    VkSampleCountFlagBits msaa_samples,
    VkImage* color_image,
    VmaAllocation* color_image_alloc,
    VkImageView* color_image_view);

void vk_image_resources_fini(
    VkDevice device,
    VmaAllocator vma,
    VkImage* image,
    VmaAllocation* image_alloc,
    VkImageView* image_view);

void vk_descriptor_set_layout_init(VkDevice device, VkDescriptorSetLayout* descriptor_set_layout);

void vk_graphics_pipeline_init(
    const char* shaders_dir,
    VkPhysicalDevice gpu,
    VkDevice device,
    const VkSurfaceFormatKHR* swap_chain_surface_format,
    VkDescriptorSetLayout descriptor_set_layout,
    VkSampleCountFlagBits msaa_samples,
    VkPipelineLayout* pipeline_layout,
    VkPipeline* graphics_pipeline);

void vk_command_pool_init(VkDevice device, uint32_t graphics_queue_family_index, VkCommandPool* command_pool);

void vk_command_buffers_init(VkDevice device, VkCommandPool command_pool, VkCommandBuffer* command_buffers);

#endif  // VK_INIT_RNDR_H
