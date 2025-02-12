#include <cecs_core/containers/cecs_arena.h>
#include "cecs_cgltf.h"

void *cecs_cgltf_alloc(void *userdata, cgltf_size size) {
    cecs_arena *arena = (cecs_arena *)userdata;
    return cecs_arena_alloc(arena, size);
}

void cecs_cgltf_free(void *userdata, void *ptr) {
    (void)userdata;
    (void)ptr;
}

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>