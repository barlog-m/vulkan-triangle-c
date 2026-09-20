#ifndef VK_DEBUG_H
#define VK_DEBUG_H

#include <vulkan/vulkan.h>

static PFN_vkSetDebugUtilsObjectNameEXT pfn_vkSetDebugUtilsObjectNameEXT = nullptr;
static bool vk_debug_utils_supported = false;

[[nodiscard]] bool vk_is_vulkan_debug_utils_supported();
void vk_setup_debug_utils(VkInstance instance);
void vk_debug_utils_fini(VkInstance instance);
[[nodiscard]] VkDebugUtilsMessengerCreateInfoEXT vk_init_debug_messenger();
void vk_set_object_name(VkDevice device, VkObjectType object_type, uint64_t object_handle, const char* object_name);

#endif  // VK_DEBUG_H
