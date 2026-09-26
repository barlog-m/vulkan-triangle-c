#include "vertex.h"

#include <string.h>

#include "utils.h"

uint64_t vertex_hash(const Vertex* v) {
    return fnv1a_hash(v, sizeof(Vertex));
}

bool vertex_equals(const Vertex* a, const Vertex* b) {
    return memcmp(a, b, sizeof(Vertex)) == 0;
}
