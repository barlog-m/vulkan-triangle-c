#ifndef MESH_H
#define MESH_H

#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

typedef struct {
    VkBuffer vertex_buffer;
    VmaAllocation vertex_buffer_alloc;
    VkBuffer index_buffer;
    VmaAllocation index_buffer_alloc;
  
    uint32_t indices_count;
    uint32_t vertices_count;
} Mesh;

void mesh_init(Mesh* mesh);

void mesh_fini(const Mesh* mesh);

#endif // MESH_H
