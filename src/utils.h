#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdint.h>

[[nodiscard]] uint32_t* read_binary_file(const char* filename, size_t* size);
char* get_base_path(void);

static uint64_t fnv1a_hash(const void *data, size_t len) {
    uint64_t h = 0xcbf29ce484222325ULL;
    const uint8_t *p = data;
    for (size_t i = 0; i < len; i++) {
        h ^= p[i];
        h *= 0x00000100000001B3ULL;
    }
    return h;
}

#endif // UTILS_H
