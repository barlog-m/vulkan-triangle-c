#include "vk_init_rndr.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "assert.h"
#include "constants.h"
#include "rndr.h"
#include "utils.h"
#include "vertex.h"
#include "vk_debug.h"
#include "vk_utils.h"

void vk_allocator_init()
{
    const VmaVulkanFunctions vma_functions = {
        .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
        .vkGetDeviceProcAddr = vkGetDeviceProcAddr,
    };
    const VmaAllocatorCreateInfo vma_create_info = {
        .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .physicalDevice = g_rndr.gpu,
        .device = g_rndr.device,
        .instance = g_rndr.instance,
        .vulkanApiVersion = VK_API_VERSION_1_4,
        .pVulkanFunctions = &vma_functions,
    };
    ASSERT_VK(vmaCreateAllocator(&vma_create_info, &g_rndr.vma));
}

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

void vk_swap_chain_fini(
    VkDevice device,
    VkSwapchainKHR swap_chain,
    VkImage* swap_chain_images[],
    uint32_t* swap_chain_images_count,
    VkImageView* swap_chain_image_views[],
    uint32_t* swap_chain_image_views_count)
{
    vk_swap_chain_image_resources_clean(device, swap_chain_images, swap_chain_images_count, swap_chain_image_views, swap_chain_image_views_count);
    
    vkDestroySwapchainKHR(device, swap_chain, nullptr);
}

void vk_swap_chain_image_resources_clean(
    VkDevice device,
    VkImage* swap_chain_images[],
    uint32_t* swap_chain_images_count,
    VkImageView* swap_chain_image_views[],
    uint32_t* swap_chain_image_views_count)
{
    for (size_t i = 0; i < *swap_chain_image_views_count; i++) {
        vkDestroyImageView(device, (*swap_chain_image_views)[i], nullptr);
    }
   
    free(*swap_chain_image_views);
    *swap_chain_image_views = nullptr;
    *swap_chain_image_views_count = 0;

    free(*swap_chain_images);
    *swap_chain_images = nullptr;
    *swap_chain_images_count = 0;
}

void vk_swap_chain_image_views_init(
    VkDevice device,
    const VkImage* const swap_chain_images,
    uint32_t swap_chain_images_count,
    const VkSurfaceFormatKHR* swap_chain_surface_format,
    VkImageView** swap_chain_image_views,
    uint32_t* swap_chain_image_views_count)
{
    *swap_chain_image_views_count = swap_chain_images_count;
    *swap_chain_image_views = malloc(sizeof(VkImageView) * swap_chain_images_count);
    ASSERT(*swap_chain_image_views);

    for (uint32_t i = 0; i < swap_chain_images_count; ++i) {
        vk_create_image_view(
            device, swap_chain_images[i], swap_chain_surface_format->format, VK_IMAGE_ASPECT_COLOR_BIT, 1,
            &(*swap_chain_image_views)[i]);

#ifndef NDEBUG
        char object_name[256];
        snprintf(object_name, sizeof(object_name), "swap_chain_image_view_%u", i);
        vk_set_object_name(device, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)(*swap_chain_image_views)[i], object_name);
#endif
    }
}

void vk_find_supported_format(
    VkPhysicalDevice gpu,
    const VkFormat candidates[],
    uint32_t candidates_size,
    VkImageTiling tiling,
    VkFormatFeatureFlags features,
    VkFormat* out_format)
{
    for (uint32_t i = 0; i < candidates_size; i++) {
        const VkFormat format = candidates[i];
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(gpu, format, &props);

        if ((tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) ||
            (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)) {
            *out_format = format;
            return;
        }
    }

    ASSERT(false && "failed to find supported format");
}

void vk_find_depth_format(VkPhysicalDevice gpu, VkFormat* out_format)
{
    constexpr VkFormat candidates[] = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
                                        VK_FORMAT_D24_UNORM_S8_UINT };

    vk_find_supported_format(
        gpu, candidates, sizeof(candidates) / sizeof(candidates[0]), VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT, out_format);

    if (*out_format != VK_FORMAT_D32_SFLOAT) {
        ASSERT(false && "VK_FORMAT_D32_SFLOAT not supported by this device");
    }
}

void vk_depth_resources_init(
    VkPhysicalDevice gpu,
    VkDevice device,
    VmaAllocator vma,
    const VkExtent2D* swap_chain_extent,
    const VkSampleCountFlagBits msaa_samples,
    VkImage* depth_image,
    VmaAllocation* depth_image_alloc,
    VkImageView* depth_image_view)
{
    VkFormat depth_format;
    vk_find_depth_format(gpu, &depth_format);

    vk_create_image(
        vma, swap_chain_extent->width, swap_chain_extent->height, depth_format, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, msaa_samples, 1, depth_image, depth_image_alloc);

    vk_create_image_view(device, *depth_image, depth_format, VK_IMAGE_ASPECT_DEPTH_BIT, 1, depth_image_view);
}

void vk_color_resources_init(
    VkDevice device,
    VmaAllocator vma,
    const VkExtent2D* swap_chain_extent,
    VkFormat image_format,
    const VkSampleCountFlagBits msaa_samples,
    VkImage* color_image,
    VmaAllocation* color_image_alloc,
    VkImageView* color_image_view)
{
    vk_create_image(
        vma, swap_chain_extent->width, swap_chain_extent->height, image_format, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, msaa_samples, 1, color_image,
        color_image_alloc);

    vk_create_image_view(device, *color_image, image_format, VK_IMAGE_ASPECT_COLOR_BIT, 1, color_image_view);
}

void vk_image_resources_fini(
    VkDevice device,
    VmaAllocator vma,
    VkImage* image,
    VmaAllocation* image_alloc,
    VkImageView* image_view)
{
    vkDestroyImageView(device, *image_view, nullptr);
    vmaDestroyImage(vma, *image, *image_alloc);

    *image = VK_NULL_HANDLE;
    *image_alloc = nullptr;
    *image_view = VK_NULL_HANDLE;
}

void vk_descriptor_set_layout_init(VkDevice device, VkDescriptorSetLayout* descriptor_set_layout)
{
    constexpr VkDescriptorSetLayoutBinding layout_binding[] = {
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr,
        },
        {
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr,
        },
    };

    const VkDescriptorSetLayoutCreateInfo layout_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT,
        .bindingCount = sizeof(layout_binding) / sizeof(layout_binding[0]),
        .pBindings = layout_binding,
    };

    ASSERT_VK(vkCreateDescriptorSetLayout(device, &layout_info, nullptr, descriptor_set_layout));
}

static void vk_shader_module_init(
    VkDevice device,
    const uint32_t* shader_code,
    size_t shader_code_size,
    VkShaderModule* shader_module)
{
    const VkShaderModuleCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = shader_code_size,
        .pCode = shader_code,
    };

    ASSERT_VK(vkCreateShaderModule(device, &create_info, nullptr, shader_module));
}

void vk_graphics_pipeline_init(
    const char* shaders_dir,
    VkPhysicalDevice gpu,
    VkDevice device,
    const VkSurfaceFormatKHR* swap_chain_surface_format,
    VkDescriptorSetLayout descriptor_set_layout,
    VkSampleCountFlagBits msaa_samples,
    VkPipelineLayout* pipeline_layout,
    VkPipeline* graphics_pipeline)
{
    char shader_path[MAX_PATH];
    snprintf(shader_path, sizeof(shader_path), "%s/shader.spv", shaders_dir);

    size_t shader_code_size;
    uint32_t* shader_code = read_binary_file(shader_path, &shader_code_size);
    ASSERT(shader_code);

    VkShaderModule shader_module;
    vk_shader_module_init(device, shader_code, shader_code_size, &shader_module);

    free(shader_code);

    VkPipelineShaderStageCreateInfo vert_shader_stage_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = shader_module,
        .pName = "vertMain",
    };

    VkPipelineShaderStageCreateInfo frag_shader_stage_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = shader_module,
        .pName = "fragMain",
    };

    VkPipelineShaderStageCreateInfo shader_stages[] = { vert_shader_stage_info, frag_shader_stage_info };

    VkPipelineVertexInputStateCreateInfo vertex_input_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = VK_VERTEX_BINDING_DESCRIPTION_COUNT,
        .pVertexBindingDescriptions = VK_VERTEX_BINDING_DESCRIPTION,
        .vertexAttributeDescriptionCount = VK_VERTEX_ATTRIBUTE_DESCRIPTION_COUNT,
        .pVertexAttributeDescriptions = VK_VERTEX_ATTRIBUTE_DESCRIPTION,
    };

    VkPipelineInputAssemblyStateCreateInfo input_assembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };

    VkPipelineViewportStateCreateInfo state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .viewportCount = 1,
        .pViewports = nullptr,
        .scissorCount = 1,
        .pScissors = nullptr,
    };

    VkPipelineRasterizationStateCreateInfo rasterizer = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0F,
        .depthBiasClamp = 0.0F,
        .depthBiasSlopeFactor = 0.0F,
        .lineWidth = 1.0F
    };

    VkPipelineMultisampleStateCreateInfo multisampling = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .rasterizationSamples = msaa_samples,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 0.0F,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
    };

    VkPipelineDepthStencilStateCreateInfo depth_stencil = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
        .front = { 0 },
        .back = { 0 },
        .minDepthBounds = 0.0F,
        .maxDepthBounds = 1.0F
    };

    VkPipelineColorBlendAttachmentState color_blend_attachment = {
        .blendEnable = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };

    VkPipelineColorBlendStateCreateInfo color_blending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment,
        .blendConstants = { 0.0F, 0.0F, 0.0F, 0.0F },
    };

    constexpr VkDynamicState dynamic_states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

    VkPipelineDynamicStateCreateInfo dynamic_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .dynamicStateCount = sizeof(dynamic_states) / sizeof(dynamic_states[0]),
        .pDynamicStates = dynamic_states,
    };

    VkPipelineLayoutCreateInfo pipeline_layout_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .setLayoutCount = 1,
        .pSetLayouts = &descriptor_set_layout,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr,
    };

    ASSERT_VK(vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, pipeline_layout));

    VkFormat depth_format;
    vk_find_depth_format(gpu, &depth_format);

    VkPipelineRenderingCreateInfo pipeline_rendering_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .pNext = nullptr,
        .viewMask = 0,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &swap_chain_surface_format->format,
        .depthAttachmentFormat = depth_format,
        .stencilAttachmentFormat = VK_FORMAT_UNDEFINED,
    };

    VkGraphicsPipelineCreateInfo pipeline_create_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &pipeline_rendering_create_info,
        .flags = 0,
        .stageCount = 2,
        .pStages = shader_stages,
        .pVertexInputState = &vertex_input_info,
        .pInputAssemblyState = &input_assembly,
        .pTessellationState = nullptr,
        .pViewportState = &state,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = &depth_stencil,
        .pColorBlendState = &color_blending,
        .pDynamicState = &dynamic_state,
        .layout = *pipeline_layout,
        .renderPass = VK_NULL_HANDLE,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1,
    };

    ASSERT_VK(
        vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, graphics_pipeline));

    vkDestroyShaderModule(device, shader_module, nullptr);
}

void vk_command_pool_init(VkDevice device, uint32_t graphics_queue_family_index, VkCommandPool* command_pool)
{
    const VkCommandPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = graphics_queue_family_index,
    };

    ASSERT_VK(vkCreateCommandPool(device, &pool_info, nullptr, command_pool));
}

void vk_command_buffers_init(VkDevice device, VkCommandPool command_pool, VkCommandBuffer* command_buffers)
{
    const VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = MAX_FRAMES_IN_FLIGHT,
    };

    ASSERT_VK(vkAllocateCommandBuffers(device, &alloc_info, command_buffers));
}

void vk_sync_objects_init(
    VkDevice device,
    VkSemaphore image_available_semaphores[],
    VkSemaphore render_finished_semaphores[],
    VkSemaphore* render_timeline)
{
    const VkSemaphoreCreateInfo binary_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    const VkSemaphoreTypeCreateInfo timeline_type_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
        .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
        .initialValue = 0,
    };
    const VkSemaphoreCreateInfo timeline_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = &timeline_type_info,
    };

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        ASSERT_VK(vkCreateSemaphore(device, &binary_info, nullptr, &image_available_semaphores[i]));
    }

    for (size_t i = 0; i < MAX_SWAPCHAIN_IMAGES; i++) {
        ASSERT_VK(vkCreateSemaphore(device, &binary_info, nullptr, &render_finished_semaphores[i]));
    }

    ASSERT_VK(vkCreateSemaphore(device, &timeline_info, nullptr, render_timeline));
}

void vk_sync_objects_fini(
    VkDevice device,
    VkSemaphore image_available_semaphores[],
    VkSemaphore render_finished_semaphores[],
    VkSemaphore render_timeline)
{
    vkDestroySemaphore(device, render_timeline, nullptr);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, image_available_semaphores[i], nullptr);
    }
    for (size_t i = 0; i < MAX_SWAPCHAIN_IMAGES; i++) {
        vkDestroySemaphore(device, render_finished_semaphores[i], nullptr);
    }
}

