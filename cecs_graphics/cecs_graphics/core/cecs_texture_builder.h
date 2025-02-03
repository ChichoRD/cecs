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
    cecs_texture_builder_descriptor_config_generate_empty = 1 << 4,
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


cecs_texture_builder *cecs_texture_builder_set_descriptor(
    cecs_texture_builder *builder,
    const WGPUTextureDescriptor texture_descriptor
);

static inline cecs_texture_builder *cecs_texture_builder_set_descriptor_no_data(
    cecs_texture_builder *builder,
    const WGPUTextureDescriptor texture_descriptor
) {
    builder->texture_data = NULL;
    return cecs_texture_builder_set_descriptor(builder, texture_descriptor);
}

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
    WGPUQueue queue,
    WGPUTexture texture,
    const WGPUTextureDescriptor *descriptor,
    const uint8_t *texture_data,
    const uint_fast8_t bytes_per_texel,
    const WGPUTextureAspect aspect,
    const uint32_t destination_layer
);

WGPUTexture cecs_texture_builder_build_alloc(
    cecs_texture_builder *builder,
    cecs_graphics_context *context,
    const WGPUTextureViewDescriptor *view_descriptor,
    const uint32_t write_destination_layer
);

cecs_texture cecs_texture_builder_build(
    cecs_texture_builder *builder,
    cecs_graphics_context *context,
    const WGPUTextureViewDescriptor *view_descriptor,
    const uint32_t write_destination_layer
);

typedef struct cecs_texture_in_bank_bundle {
    cecs_texture_in_bank_reference reference;
    cecs_texture_subrect2_f32 subrect;
    cecs_texture_in_bank_range range;
} cecs_texture_in_bank_bundle;

cecs_texture_in_bank_bundle cecs_texture_builder_build_in_bank(
    cecs_texture_builder *builder,
    cecs_graphics_context *context,
    const WGPUTextureViewDescriptor *view_descriptor
);

#endif