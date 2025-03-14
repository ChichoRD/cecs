#ifndef CECS_TEXTURE_BUILDER_H
#define CECS_TEXTURE_BUILDER_H

#include "../cecs_graphics_world.h"
#include "../component/cecs_texture.h"

WGPUExtent3D cecs_generate_next_mip(
    const WGPUExtent3D mip_size,
    const uint8_t *restrict mip_texels,
    const uint_fast8_t bytes_per_texel,
    uint8_t out_next_mip_texels[const restrict]
);
size_t cecs_generate_mipmaps(
    const WGPUExtent3D mip_size,
    const uint8_t *mip_texels,
    const uint_fast8_t bytes_per_texel,
    const uint_fast8_t mip_level_count,
    uint8_t out_mipmaps_start[]
);

// adapted from source: https://github.com/eliemichel/LearnWebGPU-Code/tree/step075-vanilla
size_t cecs_write_mipmaps(
    WGPUTexture destination,
    WGPUQueue queue,
    const WGPUTextureDescriptor *descriptor,
    const WGPUTextureAspect aspect,
    const cecs_mipmaps_write_descriptor mipmaps
);

// TODO: single block arena (with support for external allocation)

typedef struct cecs_texture_builder {
    WGPUTextureDescriptor descriptor;
    cecs_graphics_world *world;
    cecs_arena *texture_arena;
} cecs_texture_builder;
inline cecs_texture_builder cecs_texture_builder_create(
    const WGPUTextureDescriptor descriptor,
    cecs_graphics_world *world,
    cecs_arena *texture_arena
) {
    return (cecs_texture_builder){
        .descriptor = descriptor,
        .world = world,
        .texture_arena = texture_arena
    };
}
inline cecs_graphics_world *cecs_texture_builder_world(cecs_texture_builder *builder) {
    return builder->world;
}
inline cecs_arena *cecs_texture_builder_arena(cecs_texture_builder *builder) {
    return builder->texture_arena;
}

WGPUTexture cecs_texture_builder_build_alloc(
    cecs_texture_builder *builder,
    cecs_graphics_context *context
);
cecs_texture_size_pow2 cecs_texture_builder_ensured_size(
    const cecs_texture_builder *builder,
    uint32_t *out_largest_side_size 
);

inline uint64_t cecs_texture_builder_mip0_texel_count(const cecs_texture_builder *builder) {
    return builder->descriptor.size.width * builder->descriptor.size.height * builder->descriptor.size.depthOrArrayLayers;
}

typedef struct cecs_mipmaps_write_descriptor {
    const uint8_t *source_texels;
    size_t source_size;
    uint8_t bytes_per_texel;
    uint8_t destination_layer;
} cecs_mipmaps_write_descriptor;
size_t cecs_texture_builder_write_mipmaps(
    WGPUTexture destination,
    cecs_texture_builder *builder,
    cecs_graphics_context *context,
    const WGPUTextureAspect aspect,
    const cecs_mipmaps_write_descriptor mipmaps
);


typedef uint8_t cecs_texture_builder_texture_count;
#define CECS_TEXTURE_BUILDER_MAX_TEXTURE_COUNT_DEFAULT 8

#ifndef CECS_TEXTURE_BUILDER_MAX_TEXTURE_COUNT
#define CECS_TEXTURE_BUILDER_MAX_TEXTURE_COUNT CECS_TEXTURE_BUILDER_MAX_TEXTURE_COUNT_DEFAULT
#endif
static_assert(
    CECS_TEXTURE_BUILDER_MAX_TEXTURE_COUNT <= CHAR_BIT * sizeof(cecs_texture_builder_texture_count),
    "CECS_TEXTURE_BUILDER_MAX_TEXTURE_COUNT must be less than or equal to CHAR_BIT * sizeof(cecs_texture_builder_texture_count)"
);

#endif