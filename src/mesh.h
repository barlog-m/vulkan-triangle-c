#ifndef MESH_H
#define MESH_H

#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

typedef struct Rndr Rndr;

typedef struct Mesh {
    VkBuffer vertex_buffer;
    VmaAllocation vertex_buffer_alloc;
    VkBuffer index_buffer;
    VmaAllocation index_buffer_alloc;
  
    uint32_t indices_count;
    uint32_t vertices_count;
} Mesh;

Mesh* mesh_init(const Rndr* rndr);
void mesh_fini(Mesh* mesh, const Rndr* rndr);

#endif // MESH_H
