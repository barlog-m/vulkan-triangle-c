#ifndef RNDR_H
#define RNDR_H

#include <vulkan/vulkan_core.h>

typedef struct {
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR* formats;
    size_t formats_count;
    VkPresentModeKHR* present_modes;
    size_t present_modes_count;
} UVkSwapChainSupportDetails;

typedef struct {
    uint32_t graphics_family;
    uint32_t compute_family;
    uint32_t transfer_family;
    bool has_graphics;
    bool has_compute;
    bool has_transfer;
} UVkQueueFamilyIndices;

typedef struct {
    VkInstance instance;
    VkPhysicalDevice gpu;
    VkDevice device;
    VkSurfaceKHR surface;
    VkSwapchainKHR swap_chain;
    VkQueue graphics_queue;
    VkQueue compute_queue;
    UVkQueueFamilyIndices queue_family_indices;
    VkImage* swap_chain_images;
    uint32_t swap_chain_images_count;
    uint32_t swap_chain_image_views_count;
    VkSurfaceFormatKHR swap_chain_surface_format;
    VkExtent2D swap_chain_extent;
    VkImageView* swap_chain_image_views;
    VkDescriptorSetLayout descriptor_set_layout;
    VkPipelineLayout pipeline_layout;
    VkPipeline graphics_pipeline;
    VkSampleCountFlagBits msaa_samples;
} Rndr;

extern Rndr g_rndr;

void rndr_init();
void rndr_fini();

#endif
