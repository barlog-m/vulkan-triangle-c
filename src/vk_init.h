#ifndef VK_INIT_H
#define VK_INIT_H

#include "rndr.h"

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

#endif  // VK_INIT_H
