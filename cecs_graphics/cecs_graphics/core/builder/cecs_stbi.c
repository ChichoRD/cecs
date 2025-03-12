#include <stdint.h>
#include <assert.h>

#include "cecs_stbi.h"
#include <cecs_core/containers/cecs_arena.h>

typedef struct cecs_stbi_allocation_header {
    #if SIZE_MAX == UINT16_MAX
        uint16_t size;
        uint16_t padding[3];
    
    #elif SIZE_MAX == UINT32_MAX
        uint32_t size;
        uint32_t padding[1];
    
    #elif SIZE_MAX == UINT64_MAX
        uint64_t size;
    
    #else
        #error TBD code SIZE_T_BITS
    
    #endif
} cecs_stbi_allocation_header;

static cecs_stbi_allocator texture_builder_stbi_allocator = { .current_arena = NULL };
cecs_stbi_allocator *cecs_stbi_allocator_get_current_allocator(void) {
    return &texture_builder_stbi_allocator;
}

bool cecs_stbi_info_from(const char *path, cecs_stbi_info *out_info) {
    int width;
    int height;
    int channels;
    if (!stbi_info(path, &width, &height, &channels)) {
        return false;
    }
    
    out_info->width = (uint_least32_t)width;
    out_info->height = (uint_least32_t)height;
    out_info->channels = (uint_least8_t)channels;
    return true;
}
cecs_stbi_info cecs_stbi_info_from_expect(const char *path) {
    cecs_stbi_info info;
    if (!cecs_stbi_info_from(path, &info)) {
        assert(false && "fatal error: failed to get stbi info");
        exit(EXIT_FAILURE);
    }
    return info;
}

void *cecs_stbi_malloc(size_t size) {
    assert(texture_builder_stbi_allocator.current_arena != NULL && "error: stbi allocator must be set");
    const uint64_t total_size = size + sizeof(cecs_stbi_allocation_header);
    
    cecs_stbi_allocation_header *header = cecs_arena_alloc(texture_builder_stbi_allocator.current_arena, total_size); 
    header->size = total_size;
    return header + 1;
}

void *cecs_stbi_realloc(void *ptr, size_t size) {
    assert(texture_builder_stbi_allocator.current_arena != NULL && "error: stbi allocator must be set");
    if (ptr == NULL) {
        return cecs_stbi_malloc(size);
    } else {
        cecs_stbi_allocation_header *header = (cecs_stbi_allocation_header *)ptr - 1;
        const uint64_t new_total_size = size + sizeof(cecs_stbi_allocation_header);
        
        cecs_stbi_allocation_header *new_header =
        cecs_arena_realloc(texture_builder_stbi_allocator.current_arena, header, header->size, new_total_size);
        new_header->size = new_total_size;
        return new_header + 1;
    }
}

void cecs_stbi_free(void *ptr) {
    (void)ptr;
}

#define STBI_MALLOC(size) cecs_stbi_malloc(size)
#define STBI_REALLOC(ptr, size) cecs_stbi_realloc(ptr, size)
#define STBI_FREE(ptr) cecs_stbi_free(ptr)

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
