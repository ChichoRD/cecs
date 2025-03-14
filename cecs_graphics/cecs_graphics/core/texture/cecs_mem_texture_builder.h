#ifndef CECS_MEM_TEXTURE_BUILDER_H
#define CECS_MEM_TEXTURE_BUILDER_H

#include "builder/cecs_texture_builder.h"
#include "cecs_texture_bank.h"

typedef enum cecs_mem_texture_builder_descriptor_config {
    cecs_mem_texture_builder_descriptor_config_none = 0,
    cecs_mem_texture_builder_descriptor_config_generate_mipmaps = 1 << 0,
    cecs_mem_texture_builder_descriptor_config_alloc_mipmaps = 1 << 1,
} cecs_texture_builder_descriptor_config;
typedef uint8_t cecs_mem_texture_builder_descriptor_config_flags;

typedef struct cecs_mem_texture_builder_descriptor {
    WGPUTextureDescriptor descriptor;
    uint8_t bytes_per_texel;
    uint8_t max_mip_level;
    cecs_mem_texture_builder_descriptor_config_flags flags;
} cecs_mem_texture_builder_descriptor;


typedef struct cecs_mem_texture_builder {
    cecs_texture_builder builder;
    cecs_mem_texture_builder_descriptor descriptor;
    const uint8_t *textures_bytes[CECS_TEXTURE_BUILDER_MAX_TEXTURE_COUNT];
    cecs_texture_builder_texture_count used_texture_slots;
} cecs_mem_texture_builder;

cecs_mem_texture_builder cecs_mem_texture_builder_create(
    cecs_graphics_world *world,
    cecs_arena *texture_arena,
    const cecs_mem_texture_builder_descriptor descriptor
);
inline bool cecs_mem_texture_builder_is_empty(const cecs_mem_texture_builder *builder) {
    return builder->used_texture_slots == 0;
}
inline size_t cecs_mem_texture_builder_mip0_size(
    const cecs_mem_texture_builder *builder
) {
    return (size_t)cecs_texture_builder_mip0_texel_count(&builder->builder) * builder->descriptor.bytes_per_texel;
}

cecs_mem_texture_builder *cecs_mem_texture_builder_take_into_mut(
    cecs_mem_texture_builder *builder,
    uint8_t *texture_data,
    const uint_fast8_t texture_slot
);

size_t cecs_mem_texture_builder_build_alloc(
    cecs_mem_texture_builder *builder,
    cecs_graphics_context *context,
    const WGPUTextureViewDescriptor *view_descriptor,
    WGPUTexture destination_textures[const restrict static 1],
    const size_t destination_textures_capacity,
    const uint32_t depth_start_layer
);
cecs_texture_in_bank_bundle cecs_mem_texture_builder_build_in_bank(
    cecs_mem_texture_builder *builder,
    cecs_graphics_context *context,
    const WGPUTextureViewDescriptor *view_descriptor,
    const size_t bundle_range_capacity,
    const uint32_t depth_start_layer
);
#endif