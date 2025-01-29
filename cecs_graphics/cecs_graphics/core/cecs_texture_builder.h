#ifndef CECS_TEXTURE_BUILDER_H
#define CECS_TEXTURE_BUILDER_H

#include <stdint.h>
#include "cecs_graphics_world.h"
#include "component/cecs_texture.h"

typedef enum cecs_texture_builder_descriptor_config {
    cecs_texture_builder_descriptor_config_none = 0,
    cecs_texture_builder_descriptor_config_generate_mipmaps = 1 << 0,
    cecs_texture_builder_descriptor_config_alloc_mipmaps = 1 << 1,
    cecs_texture_builder_descriptor_config_is_depth = 1 << 2,
    cecs_texture_builder_descriptor_config_is_stencil = 1 << 3,
} cecs_texture_builder_descriptor_config;
typedef uint8_t cecs_texture_builder_descriptor_config_flags;

typedef struct cecs_texture_builder_descriptor {
    uint8_t bytes_per_texel;
    uint8_t channel_count;
    cecs_texture_builder_descriptor_config_flags flags;
} cecs_texture_builder_descriptor;

typedef struct cecs_texture_builder {
    cecs_graphics_world *world;
    cecs_arena *texture_arena;
    WGPUTextureDescriptor texture_descriptor;
    cecs_texture_builder_descriptor descriptor;
    uint8_t *texture_data;
    //size_t texture_size;
} cecs_texture_builder;

cecs_texture_builder cecs_texture_builder_create(
    cecs_graphics_world *world,
    cecs_arena *texture_arena,
    cecs_texture_builder_descriptor descriptor
);

cecs_texture_builder *cecs_texture_builder_load_from(
    cecs_texture_builder *builder,
    const char *path,
    const WGPUTextureDimension dimension,
    const WGPUTextureFormat format,
    const WGPUTextureUsage usage
);

cecs_texture_builder *cecs_texture_builder_set_data(
    cecs_texture_builder *builder,
    uint8_t *texture_data,
    const WGPUTextureDescriptor texture_descriptor
);

WGPUExtent3D cecs_generate_next_mip(
    const WGPUExtent3D mip_size,
    const uint8_t *mip_texels,
    const uint_fast8_t bytes_per_texel,
    uint8_t out_next_mip_texels[]
);

uint_fast8_t cecs_generate_mip_chain(
    const WGPUExtent3D mip_size,
    const uint8_t *mip_texels,
    const uint_fast8_t bytes_per_texel,
    uint8_t out_mip_chain_start[restrict],
    size_t *out_mip_chain_size
);

// adapted from source: https://github.com/eliemichel/LearnWebGPU-Code/tree/step075-vanilla
void cecs_write_mipmaps(
    WGPUQueue queue,
    WGPUTexture texture,
    const WGPUTextureDescriptor *descriptor,
    const uint8_t *texture_data,
    const uint_fast8_t bytes_per_texel,
    const WGPUTextureAspect aspect,
    const uint32_t destination_layer
);

cecs_texture_reference cecs_texture_builder_build_into(
    cecs_world *world,
    cecs_texture_builder *builder,
    cecs_graphics_context *context,
    const WGPUTextureViewDescriptor *view_descriptor
);

#endif