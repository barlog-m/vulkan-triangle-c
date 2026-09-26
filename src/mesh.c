#include "mesh.h"

#include <stdlib.h>
#include <string.h>

#include "assert.h"
#include "rndr.h"
#include "vk_utils.h"

void mesh_fini(const Mesh* mesh)
{
    vkDeviceWaitIdle(g_rndr.device);

    vmaDestroyBuffer(g_rndr.vma, mesh->index_buffer, mesh->index_buffer_alloc);
    vmaDestroyBuffer(g_rndr.vma, mesh->vertex_buffer, mesh->vertex_buffer_alloc);
}

static void mesh_create_vertex_buffer(
    Mesh* mesh,
    VmaAllocator vma,
    VkDevice device,
    VkQueue graphics_queue,
    VkCommandPool command_pool,
    const Vertex* vertices,
    uint32_t count)
{
    const VkDeviceSize buffer_size = (VkDeviceSize)count * sizeof(Vertex);

    VkBuffer staging_buffer;
    VmaAllocation staging_alloc;
    vk_buffer_init(
        vma, buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, &staging_buffer, &staging_alloc, nullptr);

    void* data;
    ASSERT_VK(vmaMapMemory(vma, staging_alloc, &data));
    memcpy(data, vertices, buffer_size);
    vmaUnmapMemory(vma, staging_alloc);

    vk_buffer_init(
        vma, buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, 0, &mesh->vertex_buffer, &mesh->vertex_buffer_alloc, nullptr);

    vk_buffer_copy(device, graphics_queue, command_pool, staging_buffer, mesh->vertex_buffer, buffer_size);
    vmaDestroyBuffer(vma, staging_buffer, staging_alloc);
}

static void mesh_create_index_buffer(
    Mesh* mesh,
    VmaAllocator vma,
    VkDevice device,
    VkQueue graphics_queue,
    VkCommandPool command_pool,
    const uint32_t* indices,
    uint32_t count)
{
    const VkDeviceSize buffer_size = (VkDeviceSize)count * sizeof(uint32_t);

    VkBuffer staging_index_buffer;
    VmaAllocation staging_index_alloc;
    vk_buffer_init(
        vma, buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, &staging_index_buffer, &staging_index_alloc, nullptr);

    void* index_data;
    ASSERT_VK(vmaMapMemory(vma, staging_index_alloc, &index_data));
    memcpy(index_data, indices, buffer_size);
    vmaUnmapMemory(vma, staging_index_alloc);

    vk_buffer_init(
        vma, buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, 0, &mesh->index_buffer, &mesh->index_buffer_alloc, nullptr);

    vk_buffer_copy(device, graphics_queue, command_pool, staging_index_buffer, mesh->index_buffer, buffer_size);
    vmaDestroyBuffer(vma, staging_index_buffer, staging_index_alloc);
}

void mesh_init(Mesh* mesh)
{
    uint32_t indices[] = { 0, 1, 2, 2, 3, 0 };
    mesh->indices_count = sizeof(indices) / sizeof(indices[0]);

    mesh_create_index_buffer(
        mesh, g_rndr.vma, g_rndr.device, g_rndr.graphics_queue, g_rndr.command_pool, indices, mesh->indices_count);

    Vertex vertices[] = {
        (Vertex){
            .position = { .x = -0.5F, .y = -0.5F },
            .color = { .r = 1.0F, .g = 0.0F, .b = 0.0F },
        },
        (Vertex){
            .position = { .x = 0.5F, .y = -0.5F },
            .color = { .r = 0.0F, .g = 1.0F, .b = 0.0F },
        },
        (Vertex){
            .position = { .x = 0.5F, .y = 0.5F },
            .color = { .r = 0.0F, .g = 0.0F, .b = 1.0F },
        },
        (Vertex){
            .position = { .x = -0.5F, .y = 0.5F },
            .color = { .r = 1.0F, .g = 1.0F, .b = 1.0F },
        },
    };
    mesh->vertices_count = sizeof(vertices) / sizeof(vertices[0]);

    mesh_create_vertex_buffer(
        mesh, g_rndr.vma, g_rndr.device, g_rndr.graphics_queue, g_rndr.command_pool, vertices, mesh->vertices_count);
}
