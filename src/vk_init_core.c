#include "vk_init_core.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vulkan/vulkan_core.h>
#include <SDL3/SDL_vulkan.h>

#include "assert.h"
#include "rndr.h"
#include "vk_debug.h"

static const char* VALIDATION_LAYERS[] = { "VK_LAYER_KHRONOS_validation" };
static const char* DEVICE_EXTENSIONS[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_EXT_MESH_SHADER_EXTENSION_NAME };
static constexpr size_t DEVICE_EXTENSIONS_SIZE = sizeof(DEVICE_EXTENSIONS) / sizeof(DEVICE_EXTENSIONS[0]);

static bool vk_enable_validation_layers()
{
#ifdef NDEBUG
    return false;
#endif

    uint32_t available_layer_count;
    vkEnumerateInstanceLayerProperties(&available_layer_count, nullptr);

    if (available_layer_count == 0) {
        LOG_WARN("no Vulkan layers found");
        return false;
    }

    VkLayerProperties* available_layers = malloc(available_layer_count * sizeof(VkLayerProperties));
    const VkResult res = vkEnumerateInstanceLayerProperties(&available_layer_count, available_layers);
    if (res != VK_SUCCESS) {
        LOG_WARN("vkEnumerateInstanceLayerProperties failed");
        free(available_layers);
        return false;
    }

    constexpr size_t validation_layer_count = sizeof(VALIDATION_LAYERS) / sizeof(VALIDATION_LAYERS[0]);

    for (size_t v = 0; v < validation_layer_count; ++v) {
        int found = 0;
        for (uint32_t a = 0; a < available_layer_count; ++a) {
            if (strcmp(available_layers[a].layerName, VALIDATION_LAYERS[v]) == 0) {
                found = 1;
                break;
            }
        }
        if (!found) {
            free(available_layers);
            return false;
        }
    }

    free(available_layers);
    return true;
}

static void vk_create_instance(VkInstance* instance)
{
    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Utu",
        .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
        .pEngineName = "Utu",
        .engineVersion = VK_MAKE_VERSION(0, 1, 0),
        .apiVersion = VK_API_VERSION_1_4,
    };

    VkInstanceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
    };

    VkValidationFeaturesEXT validation_features = {
        .sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
        .pNext = nullptr,
    };

    constexpr VkValidationFeatureEnableEXT
        enabled_validation_features[] = { VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT,
                                          VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
                                          /*VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT*/ };

    if (vk_enable_validation_layers()) {
        validation_features.enabledValidationFeatureCount =
            sizeof(enabled_validation_features) / sizeof(enabled_validation_features[0]);
        validation_features.pEnabledValidationFeatures = enabled_validation_features;
        validation_features.disabledValidationFeatureCount = 0;
        validation_features.pDisabledValidationFeatures = nullptr;

        create_info.enabledLayerCount = sizeof(VALIDATION_LAYERS) / sizeof(VALIDATION_LAYERS[0]);
        create_info.ppEnabledLayerNames = VALIDATION_LAYERS;
        create_info.pNext = &validation_features;
    } else {
        create_info.enabledLayerCount = 0;
    }

    uint32_t sdl_extension_count = 0;
    const char* const* sdl_extensions = SDL_Vulkan_GetInstanceExtensions(&sdl_extension_count);
    uint32_t total_extension_count = sdl_extension_count;

#ifndef NDEBUG
    const bool is_debug_utils_supported = vk_is_vulkan_debug_utils_supported();
    if (is_debug_utils_supported) {
        total_extension_count += 1;
    }
#endif

    const char** all_extensions = malloc(total_extension_count * sizeof(const char*));
    ASSERT(all_extensions);

    memcpy(all_extensions, sdl_extensions, sdl_extension_count * sizeof(const char*));

#ifndef NDEBUG
    VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info = {};
    if (is_debug_utils_supported) {
        all_extensions[sdl_extension_count] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        debug_messenger_create_info = vk_init_debug_messenger();
        validation_features.pNext = &debug_messenger_create_info;
    }
#endif

    create_info.enabledExtensionCount = total_extension_count;
    create_info.ppEnabledExtensionNames = all_extensions;

    ASSERT_VK(vkCreateInstance(&create_info, nullptr, instance));

#ifndef NDEBUG
    if (is_debug_utils_supported) {
        vk_setup_debug_utils(*instance);
    }
#endif

    free(all_extensions);
}

static bool vk_is_physical_device_suitable(VkPhysicalDevice gpu)
{
    VkPhysicalDeviceProperties2 device_properties2 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
        .pNext = nullptr,
    };

    VkPhysicalDeviceMeshShaderFeaturesEXT mesh_shader_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT,
        .pNext = nullptr,
    };

    VkPhysicalDeviceUnifiedImageLayoutsFeaturesKHR unified_layouts_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_UNIFIED_IMAGE_LAYOUTS_FEATURES_KHR,
        .pNext = &mesh_shader_features,
    };

    VkPhysicalDeviceVulkan11Features vulkan11_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
        .pNext = &unified_layouts_features,
    };

    VkPhysicalDeviceVulkan12Features vulkan12_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .pNext = &vulkan11_features,
    };

    VkPhysicalDeviceVulkan13Features vulkan13_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext = &vulkan12_features,
    };

    VkPhysicalDeviceVulkan14Features vulkan14_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES,
        .pNext = &vulkan13_features,
    };

    VkPhysicalDeviceFeatures2 device_features2 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &vulkan14_features,
    };

    vkGetPhysicalDeviceProperties2(gpu, &device_properties2);
    vkGetPhysicalDeviceFeatures2(gpu, &device_features2);

    return device_properties2.properties.apiVersion >= VK_API_VERSION_1_4
        && device_features2.features.geometryShader
        && device_features2.features.samplerAnisotropy
        && vulkan11_features.shaderDrawParameters
        && vulkan12_features.bufferDeviceAddress
        && vulkan12_features.drawIndirectCount
        && vulkan13_features.synchronization2
        && vulkan13_features.dynamicRendering
        && vulkan14_features.hostImageCopy
        && vulkan14_features.pushDescriptor
        && unified_layouts_features.unifiedImageLayouts
        && mesh_shader_features.taskShader
        && mesh_shader_features.meshShader;
}

static uint8_t vk_physical_device_type_score(VkPhysicalDevice gpu)
{
    VkPhysicalDeviceProperties device_properties = {};
    vkGetPhysicalDeviceProperties(gpu, &device_properties);

    switch (device_properties.deviceType) {
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            return 4;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            return 3;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            return 2;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            return 1;
        default:
            return 0;
    }
}

static VkSampleCountFlagBits vk_get_max_usable_sample_count(VkPhysicalDevice gpu)
{
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(gpu, &physicalDeviceProperties);

    const VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts &
                                      physicalDeviceProperties.limits.framebufferDepthSampleCounts;

    if (counts & VK_SAMPLE_COUNT_64_BIT) {
        return VK_SAMPLE_COUNT_64_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_32_BIT) {
        return VK_SAMPLE_COUNT_32_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_16_BIT) {
        return VK_SAMPLE_COUNT_16_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_8_BIT) {
        return VK_SAMPLE_COUNT_8_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_4_BIT) {
        return VK_SAMPLE_COUNT_4_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_2_BIT) {
        return VK_SAMPLE_COUNT_2_BIT;
    }

    return VK_SAMPLE_COUNT_1_BIT;
}

static bool vk_is_physical_device_has_graphics_and_present_family(VkPhysicalDevice gpu, VkSurfaceKHR surface)
{
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queue_family_count, nullptr);

    VkQueueFamilyProperties queue_families[queue_family_count];
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queue_family_count, queue_families);

    for (uint32_t queue_family_index = 0; queue_family_index < queue_family_count; ++queue_family_index) {
        if (!(queue_families[queue_family_index].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
            continue;
        }

        VkBool32 present_support = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(gpu, queue_family_index, surface, &present_support);
        if (present_support) {
            return true;
        }
    }

    return false;
}

static void vk_pick_physical_device(VkInstance instance, VkSurfaceKHR surface, VkPhysicalDevice* gpu, VkSampleCountFlagBits* msaa_samples)
{
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, nullptr);

    ASSERT(device_count > 0);

    VkPhysicalDevice devices[device_count];
    vkEnumeratePhysicalDevices(instance, &device_count, devices);

    const size_t devices_count = sizeof(devices) / sizeof(devices[0]);
    uint8_t best_score = 0;
    for (size_t device_num = 0; device_num < devices_count; ++device_num) {
        if (!vk_is_physical_device_suitable(devices[device_num])
            || !vk_is_physical_device_has_graphics_and_present_family(devices[device_num], surface)) {
            continue;
        }

        const uint8_t score = vk_physical_device_type_score(devices[device_num]);

        if (*gpu == VK_NULL_HANDLE || score > best_score) {
            best_score = score;
            *gpu = devices[device_num];
        }
    }

    ASSERT(*gpu != VK_NULL_HANDLE);
    *msaa_samples = vk_get_max_usable_sample_count(*gpu);
}

static void
vk_find_best_queue_families(VkPhysicalDevice gpu, VkSurfaceKHR surface, VkQueueFamilyIndices* queue_family_indices)
{
    VkQueueFamilyIndices indices = {};

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queue_family_count, nullptr);

    VkQueueFamilyProperties queue_families[queue_family_count];
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queue_family_count, queue_families);

    uint32_t best_graphics_score = 0;
    uint32_t best_compute_score = 0;
    uint32_t best_transfer_score = 0;

    for (uint32_t i = 0; i < queue_family_count; i++) {
        const VkQueueFlags flags = queue_families[i].queueFlags;
        const uint32_t queue_count = queue_families[i].queueCount;

        // Graphics queue (highest priority) — the family we pick must also support
        // present on this surface, because we present from the graphics queue.
        if (flags & VK_QUEUE_GRAPHICS_BIT) {
            VkBool32 present_support = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(gpu, i, surface, &present_support);

            if (present_support) {
                uint32_t score = queue_count;
                if (!(flags & VK_QUEUE_COMPUTE_BIT)) {
                    score += 10;
                }
                if (!(flags & VK_QUEUE_TRANSFER_BIT)) {
                    score += 5;
                }

                if (!indices.has_graphics || score > best_graphics_score) {
                    indices.graphics_family = i;
                    indices.has_graphics = true;
                    best_graphics_score = score;
                }
            }
        }

        // Compute queue (prefer dedicated)
        if (flags & VK_QUEUE_COMPUTE_BIT) {
            uint32_t score = queue_count;
            if (!(flags & VK_QUEUE_GRAPHICS_BIT)) {
                score += 20;
            }

            if (!indices.has_compute || score > best_compute_score) {
                indices.compute_family = i;
                indices.has_compute = true;
                best_compute_score = score;
            }
        }

        // Transfer queue (prefer dedicated)
        if (flags & VK_QUEUE_TRANSFER_BIT) {
            uint32_t score = queue_count;
            if (!(flags & VK_QUEUE_GRAPHICS_BIT) && !(flags & VK_QUEUE_COMPUTE_BIT)) {
                score += 30;
            }

            if (!indices.has_transfer || score > best_transfer_score) {
                indices.transfer_family = i;
                indices.has_transfer = true;
                best_transfer_score = score;
            }
        }
    }

    *queue_family_indices = indices;
}

static void vk_create_logical_device(
    VkPhysicalDevice gpu,
    VkDevice* device,
    VkSurfaceKHR surface,
    VkQueueFamilyIndices* queue_family_indices,
    VkQueue* graphics_queue,
    VkQueue* compute_queue)
{
    vk_find_best_queue_families(gpu, surface, queue_family_indices);

    constexpr float queue_priority = 0.5F;

    VkDeviceQueueCreateInfo queue_create_infos[2] = {
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queueFamilyIndex = queue_family_indices->graphics_family,
            .queueCount = 1,
            .pQueuePriorities = &queue_priority,
        },
    };
    uint32_t queue_create_info_count = 1;

    if (queue_family_indices->has_compute &&
        queue_family_indices->compute_family != queue_family_indices->graphics_family) {
        queue_create_infos[1] = (VkDeviceQueueCreateInfo){
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queueFamilyIndex = queue_family_indices->compute_family,
            .queueCount = 1,
            .pQueuePriorities = &queue_priority,
        };
        queue_create_info_count = 2;
    }

    VkPhysicalDeviceMeshShaderFeaturesEXT mesh_shader_features = {
        .sType      = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT,
        .pNext      = nullptr,
        .taskShader = VK_TRUE,
        .meshShader = VK_TRUE,
    };

    VkPhysicalDeviceUnifiedImageLayoutsFeaturesKHR unified_layouts_features = {
        .sType               = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_UNIFIED_IMAGE_LAYOUTS_FEATURES_KHR,
        .pNext               = &mesh_shader_features,
        .unifiedImageLayouts = VK_TRUE,
    };

    VkPhysicalDeviceVulkan11Features vulkan11_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
        .pNext = &unified_layouts_features,
        .shaderDrawParameters = VK_TRUE,
    };

    VkPhysicalDeviceVulkan12Features vulkan12_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .pNext = &vulkan11_features,
        .drawIndirectCount = VK_TRUE,
        .timelineSemaphore = VK_TRUE,
        .bufferDeviceAddress = VK_TRUE,
    };

    VkPhysicalDeviceVulkan13Features vulkan13_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext = &vulkan12_features,
        .synchronization2 = VK_TRUE,
        .dynamicRendering = VK_TRUE,
    };

    VkPhysicalDeviceVulkan14Features vulkan14_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES,
        .pNext = &vulkan13_features,
        .hostImageCopy = VK_TRUE,
        .pushDescriptor = VK_TRUE,
    };

    VkPhysicalDeviceFeatures2 feature_chain = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &vulkan14_features,
        .features = {
            .samplerAnisotropy = VK_TRUE,
            .pipelineStatisticsQuery = VK_TRUE,
            .inheritedQueries = VK_TRUE,
        },
    };

    VkDeviceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &feature_chain,
        .flags = 0,
        .queueCreateInfoCount = queue_create_info_count,
        .pQueueCreateInfos = queue_create_infos,
        .enabledExtensionCount = DEVICE_EXTENSIONS_SIZE,
        .ppEnabledExtensionNames = DEVICE_EXTENSIONS,
        .pEnabledFeatures = nullptr,
    };

    ASSERT_VK(vkCreateDevice(gpu, &create_info, nullptr, device));

    ASSERT(queue_family_indices->has_graphics);

    vkGetDeviceQueue(*device, queue_family_indices->graphics_family, 0, graphics_queue);

    if (queue_family_indices->has_compute) {
        vkGetDeviceQueue(*device, queue_family_indices->compute_family, 0, compute_queue);
    } else {
        *compute_queue = *graphics_queue;
    }
}

void vk_device_init(Rndr* rndr)
{
    rndr->gpu = VK_NULL_HANDLE;

    vk_create_instance(&rndr->instance);

    ASSERT_SDL(SDL_Vulkan_CreateSurface(rndr->window->sdl_window, rndr->instance, nullptr, &rndr->surface));

    vk_pick_physical_device(rndr->instance, rndr->surface, &rndr->gpu, &rndr->msaa_samples);

    vk_create_logical_device(
        rndr->gpu, &rndr->device, rndr->surface, &rndr->queue_family_indices, &rndr->graphics_queue,
        &rndr->compute_queue);
}

void vk_allocator_init(Rndr* rndr)
{
    const VmaVulkanFunctions vma_functions = {
        .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
        .vkGetDeviceProcAddr = vkGetDeviceProcAddr,
    };
    const VmaAllocatorCreateInfo vma_create_info = {
        .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .physicalDevice = rndr->gpu,
        .device = rndr->device,
        .instance = rndr->instance,
        .vulkanApiVersion = VK_API_VERSION_1_4,
        .pVulkanFunctions = &vma_functions,
    };
    ASSERT_VK(vmaCreateAllocator(&vma_create_info, &rndr->vma));
}
