#ifndef VERTEX_H
#define VERTEX_H

#include <stdint.h>
#include <cglm/types-struct.h>
#include <vulkan/vulkan_core.h>

typedef struct {
    vec2s position;
    vec3s color;
} Vertex;

uint64_t vertex_hash(const Vertex* v);
bool     vertex_equals(const Vertex* a, const Vertex* b);

constexpr VkVertexInputBindingDescription VK_VERTEX_BINDING_DESCRIPTION[] = {
    {
        .binding = 0,
        .stride = sizeof(Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    },
};
constexpr size_t VK_VERTEX_BINDING_DESCRIPTION_COUNT = sizeof(VK_VERTEX_BINDING_DESCRIPTION) / sizeof(VK_VERTEX_BINDING_DESCRIPTION[0]);

constexpr VkVertexInputAttributeDescription VK_VERTEX_ATTRIBUTE_DESCRIPTION[] = {
    {
        .location = 0,
        .binding = 0,
        .format   = VK_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(Vertex, position),
    },
    {
        .location = 1,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(Vertex, color),
    },
};
constexpr size_t VK_VERTEX_ATTRIBUTE_DESCRIPTION_COUNT = sizeof(VK_VERTEX_ATTRIBUTE_DESCRIPTION) / sizeof(VK_VERTEX_ATTRIBUTE_DESCRIPTION[0]);

#endif  // VERTEX_H
