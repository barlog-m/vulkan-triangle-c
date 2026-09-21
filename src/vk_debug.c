#include "vk_debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"

static VkDebugUtilsMessengerEXT vk_debug_messenger = VK_NULL_HANDLE;

bool vk_is_vulkan_debug_utils_supported()
{
    pfn_vkSetDebugUtilsObjectNameEXT = nullptr;
    vk_debug_utils_supported = false;

    uint32_t extensionCount = 0;

    VkResult result = vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    if (result != VK_SUCCESS || extensionCount == 0) {
        LOG_WARN("Failed to enumerate instance extensions");
        return false;
    }

    VkExtensionProperties* extensions = malloc(extensionCount * sizeof(VkExtensionProperties));
    if (!extensions) {
        LOG_WARN("Failed to allocate memory for extensions");
        return false;
    }

    result = vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions);
    if (result != VK_SUCCESS) {
        free(extensions);
        LOG_WARN("Failed to get instance extension properties");
        return false;
    }

    for (uint32_t i = 0; i < extensionCount; i++) {
        auto extension = &extensions[i];
        if (strcmp(extension->extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0) {
            vk_debug_utils_supported = true;
            break;
        }
    }

    free(extensions);
    return vk_debug_utils_supported;
}

void vk_setup_debug_utils(VkInstance instance)
{
    if (!vk_debug_utils_supported) {
        char msg[256];
        snprintf(msg, sizeof(msg), "%s not present, debug utils are disabled.", VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        LOG_WARN(msg);
        return;
    }

    typeof(pfn_vkSetDebugUtilsObjectNameEXT) func_ptr =
        (typeof(pfn_vkSetDebugUtilsObjectNameEXT))vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT");

    pfn_vkSetDebugUtilsObjectNameEXT = func_ptr;
    vk_debug_utils_supported = (pfn_vkSetDebugUtilsObjectNameEXT != nullptr);

    if (!vk_debug_utils_supported) {
        LOG_WARN("Failed to load vkSetDebugUtilsObjectNameEXT function");
        return;
    }

    PFN_vkCreateDebugUtilsMessengerEXT pfn_create =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (!pfn_create) {
        LOG_WARN("Failed to load vkCreateDebugUtilsMessengerEXT");
        return;
    }

    const VkDebugUtilsMessengerCreateInfoEXT create_info = vk_init_debug_messenger();
    const VkResult result = pfn_create(instance, &create_info, nullptr, &vk_debug_messenger);
    if (result != VK_SUCCESS) {
        LOG_WARN("Failed to create debug utils messenger");
        return;
    }

    LOG_DEBUG("Debug utils extension loaded successfully");
}

void vk_debug_utils_fini(VkInstance instance)
{
    if (vk_debug_messenger == VK_NULL_HANDLE) {
        return;
    }

    PFN_vkDestroyDebugUtilsMessengerEXT pfn_destroy =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (pfn_destroy) {
        pfn_destroy(instance, vk_debug_messenger, nullptr);
    }

    vk_debug_messenger = VK_NULL_HANDLE;
}

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    [[maybe_unused]] VkDebugUtilsMessageSeverityFlagBitsEXT msg_severity,
    [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT msg_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    [[maybe_unused]] void* user_data)
{
    switch (msg_severity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            LOG_VK_DEBUG(callback_data->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            LOG_VK_INFO(callback_data->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            LOG_VK_WARN(callback_data->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            LOG_VK_ERROR(callback_data->pMessage);
            break;
        default:
            LOG_VK_INFO(callback_data->pMessage);
            break;
    }

    return VK_FALSE;
}

VkDebugUtilsMessengerCreateInfoEXT vk_init_debug_messenger()
{
    const VkDebugUtilsMessengerCreateInfoEXT create_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = vk_debug_callback,
    };

    return create_info;
}

void vk_set_object_name(VkDevice device, VkObjectType object_type, uint64_t object_handle, const char* object_name)
{
    if (!vk_debug_utils_supported || !pfn_vkSetDebugUtilsObjectNameEXT) {
        LOG_WARN("debug utils not supported");
        return;
    }

    if (!object_name) {
        LOG_WARN("object_name is NULL");
        return;
    }

    VkDebugUtilsObjectNameInfoEXT name_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .pNext = nullptr,
        .objectType = object_type,
        .objectHandle = object_handle,
        .pObjectName = object_name,
    };

    const VkResult result = pfn_vkSetDebugUtilsObjectNameEXT(device, &name_info);
    if (result != VK_SUCCESS) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Failed to set object name '%s': VkResult = %d", object_name, result);
        LOG_WARN(msg);
    }
}

