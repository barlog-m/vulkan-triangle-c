#include "alloc.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "assert.h"

void* xmalloc(size_t size)
{
    void* ptr = malloc(size);

    if (ptr == NULL && size != 0) {
        ASSERT_MSG(false, "malloc failed");
    }

    return ptr;
}

void* xcalloc(size_t count, size_t size)
{
    if (count != 0 && size > SIZE_MAX / count) {
        ASSERT_MSG(false, "calloc failed");
    }

    void* ptr = calloc(count, size);

    if (ptr == NULL && count != 0 && size != 0) {
        ASSERT_MSG(false, "calloc failed");
    }

    return ptr;
}

void* xrealloc(void* ptr, size_t size)
{
    if (size == 0) {
        free(ptr);
        return NULL;
    }

    void* new_ptr = realloc(ptr, size);

    if (new_ptr == NULL) {
        ASSERT_MSG(false, "realloc failed");
    }

    return new_ptr;
}

void* xreallocn(void* ptr, size_t count, size_t size)
{
    if (count != 0 && size > SIZE_MAX / count) {
        ASSERT_MSG(false, "realloc failed");
    }

    return xrealloc(ptr, count * size);
}