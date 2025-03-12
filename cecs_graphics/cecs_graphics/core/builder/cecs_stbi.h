#ifndef CECS_STBI_H
#define CECS_STBI_H

#include <limits.h>
#include <stb_image.h>
#include <cecs_core/containers/cecs_arena.h>

typedef struct cecs_stbi_allocator {
    cecs_arena *current_arena;
} cecs_stbi_allocator;
cecs_stbi_allocator *cecs_stbi_allocator_get_current_allocator(void);

static_assert(
    INT_MAX <= UINT_LEAST32_MAX,
    "static error: INT_MAX must be less than or equal to UINT_LEAST32_MAX"
);
typedef struct cecs_stbi_info {
    uint_least32_t width;
    uint_least32_t height;
    uint_least8_t channels;
} cecs_stbi_info;

bool cecs_stbi_info_from(const char *path, cecs_stbi_info *out_info);
cecs_stbi_info cecs_stbi_info_from_expect(const char *path);

#endif