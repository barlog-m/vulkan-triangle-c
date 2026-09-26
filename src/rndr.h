#ifndef RNDR_H
#define RNDR_H

#include <vulkan/vulkan_core.h>

#include "constants.h"
#include "vertex.h"
#include "vk_utils.h"

typedef struct {
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR* formats;
    size_t formats_count;
    VkPresentModeKHR* present_modes;
    size_t present_modes_count;
} VkSwapChainSupportDetails;

typedef struct {
    uint32_t graphics_family;
    uint32_t compute_family;
    uint32_t transfer_family;
    bool has_graphics;
    bool has_compute;
    bool has_transfer;
} VkQueueFamilyIndices;

typedef struct {
    VkInstance instance;
    VkPhysicalDevice gpu;
    VkDevice device;
    VmaAllocator vma;
    VkSurfaceKHR surface;
    VkSampleCountFlagBits msaa_samples;

    VkQueue graphics_queue;
    VkQueue compute_queue;
    VkQueueFamilyIndices queue_family_indices;

    VkSwapchainKHR swap_chain;
    VkImage* swap_chain_images;
    uint32_t swap_chain_images_count;
    VkImageView* swap_chain_image_views;
    uint32_t swap_chain_image_views_count;
    VkSurfaceFormatKHR swap_chain_surface_format;
    VkExtent2D swap_chain_extent;

    VkImage depth_image;
    VmaAllocation depth_image_alloc;
    VkImageView depth_image_view;
    VkFormat depth_format;

    VkImage color_image;
    VmaAllocation color_image_alloc;
    VkImageView color_image_view;
    VkFormat color_format;
    
    VkDescriptorSetLayout descriptor_set_layout;
    VkPipelineLayout pipeline_layout;
    VkPipeline graphics_pipeline;

    VkCommandPool command_pool;
    VkCommandBuffer command_buffers[MAX_FRAMES_IN_FLIGHT];
    VkCommandPool compute_command_pool;
    VkCommandBuffer compute_command_buffers[MAX_FRAMES_IN_FLIGHT];

} Rndr;

extern Rndr g_rndr;

void rndr_init();
void rndr_fini();

#endif
