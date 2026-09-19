#ifndef RNDR_H
#define RNDR_H

#include <vulkan/vulkan_core.h>

typedef struct {
    VkInstance instance;
} Rndr;

void rndr_init();
void rndr_fini();

#endif
