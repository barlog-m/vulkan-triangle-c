#include "vk_init.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "assert.h"
#include "rndr.h"
#include "vk_debug.h"
#include "vk_utils.h"

static constexpr size_t MAX_SWAPCHAIN_IMAGES = 4;

static VkSwapChainSupportDetails vk_query_swap_chain_support(VkPhysicalDevice gpu, VkSurfaceKHR surface)
{
    VkSwapChainSupportDetails details = {};

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surface, &details.capabilities);

    uint32_t format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &format_count, nullptr);
    ASSERT(format_count > 0);

    details.formats = malloc(format_count * sizeof(VkSurfaceFormatKHR));
    ASSERT(details.formats);
    vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &format_count, details.formats);
    details.formats_count = format_count;

    uint32_t present_mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &present_mode_count, nullptr);
    ASSERT(present_mode_count > 0);

    details.present_modes = malloc(present_mode_count * sizeof(VkPresentModeKHR));
    ASSERT(details.present_modes);
    vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &present_mode_count, details.present_modes);
    details.present_modes_count = present_mode_count;

    return details;
}

static void vk_free_swap_chain_support_details(VkSwapChainSupportDetails* details)
{
    free(details->formats);
    free(details->present_modes);
    *details = (VkSwapChainSupportDetails){};
}

static VkSurfaceFormatKHR vk_choose_swap_surface_format(
    const VkSurfaceFormatKHR available_formats[],
    const size_t count)
{
    for (size_t i = 0; i < count; ++i) {
        if (available_formats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
            available_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return available_formats[i];
        }
    }

    return available_formats[0];
}

static VkPresentModeKHR vk_choose_swap_present_mode(
    const VkPresentModeKHR available_present_modes[],
    const size_t count)
{
    for (size_t i = 0; i < count; ++i) {
        if (available_present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            return available_present_modes[i];
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D vk_choose_swap_extent(const VkSurfaceCapabilitiesKHR* capabilities, uint32_t width, uint32_t height)
{
    if (capabilities->currentExtent.width != UINT32_MAX) {
        return capabilities->currentExtent;
    }

    VkExtent2D actual_extent = { width, height };

    if (actual_extent.width < capabilities->minImageExtent.width) {
        actual_extent.width = capabilities->minImageExtent.width;
    } else if (actual_extent.width > capabilities->maxImageExtent.width) {
        actual_extent.width = capabilities->maxImageExtent.width;
    }

    if (actual_extent.height < capabilities->minImageExtent.height) {
        actual_extent.height = capabilities->minImageExtent.height;
    } else if (actual_extent.height > capabilities->maxImageExtent.height) {
        actual_extent.height = capabilities->maxImageExtent.height;
    }

    return actual_extent;
}

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
    VkExtent2D* swap_chain_extent)
{
    VkSwapChainSupportDetails swap_chain_support = vk_query_swap_chain_support(gpu, surface);

    const VkSurfaceFormatKHR surface_format =
        vk_choose_swap_surface_format(swap_chain_support.formats, swap_chain_support.formats_count);
    const VkPresentModeKHR present_mode =
        vk_choose_swap_present_mode(swap_chain_support.present_modes, swap_chain_support.present_modes_count);
    const VkExtent2D extent = vk_choose_swap_extent(&swap_chain_support.capabilities, width, height);

    uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;
    if (swap_chain_support.capabilities.maxImageCount > 0 &&
        image_count > swap_chain_support.capabilities.maxImageCount) {
        image_count = swap_chain_support.capabilities.maxImageCount;
    }
    if (image_count > MAX_SWAPCHAIN_IMAGES && MAX_SWAPCHAIN_IMAGES >= swap_chain_support.capabilities.minImageCount) {
        image_count = MAX_SWAPCHAIN_IMAGES;
    }

    VkSwapchainCreateInfoKHR create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = image_count,
        .imageFormat = surface_format.format,
        .imageColorSpace = surface_format.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage =
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    create_info.preTransform = swap_chain_support.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;

    create_info.oldSwapchain = old_swap_chain;

    ASSERT_VK(vkCreateSwapchainKHR(device, &create_info, nullptr, swap_chain));

    vk_free_swap_chain_support_details(&swap_chain_support);

    vkGetSwapchainImagesKHR(device, *swap_chain, &image_count, nullptr);
    *swap_chain_images = malloc(sizeof(VkImage) * image_count);
    ASSERT(*swap_chain_images);
    ASSERT_VK(vkGetSwapchainImagesKHR(device, *swap_chain, &image_count, *swap_chain_images));

    *swap_chain_images_count = image_count;
    *swap_chain_surface_format = surface_format;
    *swap_chain_extent = extent;
}

void vk_swap_chain_image_views_init(
    VkDevice device,
    const VkImage* swap_chain_images,
    uint32_t swap_chain_images_count,
    const VkSurfaceFormatKHR* swap_chain_surface_format,
    VkImageView** swap_chain_image_views,
    uint32_t* swap_chain_image_views_count)
{
    *swap_chain_image_views_count = swap_chain_images_count;
    *swap_chain_image_views = malloc(sizeof(VkImageView) * swap_chain_images_count);
    ASSERT(*swap_chain_image_views);

    for (uint32_t i = 0; i < swap_chain_images_count; ++i) {
        vk_image_view_init(
            device, swap_chain_images[i], swap_chain_surface_format->format, VK_IMAGE_ASPECT_COLOR_BIT, 1,
            &(*swap_chain_image_views)[i]);

#ifndef NDEBUG
        char object_name[256];
        snprintf(object_name, sizeof(object_name), "swap_chain_image_view_%u", i);
        vk_set_object_name(device, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)(*swap_chain_image_views)[i], object_name);
#endif
    }
}

void vk_swap_chain_image_views_fini(
    VkDevice device,
    VkImageView* swap_chain_image_views[],
    uint32_t* swap_chain_image_views_count)
{
    for (size_t i = 0; i < *swap_chain_image_views_count; i++) {
        vkDestroyImageView(device, (*swap_chain_image_views)[i], nullptr);
    }

    free(*swap_chain_image_views);
    *swap_chain_image_views = nullptr;
    *swap_chain_image_views_count = 0;
}
