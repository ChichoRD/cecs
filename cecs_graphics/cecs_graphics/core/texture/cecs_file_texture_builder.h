#ifndef CECS_FILE_TEXTURE_BUILDER_H
#define CECS_FILE_TEXTURE_BUILDER_H

#include "builder/cecs_texture_builder.h"
#include "cecs_texture_bank.h"

typedef enum cecs_texture_builder_descriptor_config {
    cecs_texture_builder_descriptor_config_none = 0,
    cecs_texture_builder_descriptor_config_generate_mipmaps = 1 << 0,
    cecs_texture_builder_descriptor_config_alloc_mipmaps = 1 << 1,
} cecs_texture_builder_descriptor_config;
typedef uint8_t cecs_texture_builder_descriptor_config_flags;

typedef struct cecs_texture_builder_descriptor {
    uint8_t bytes_per_texel;
    uint8_t channel_count;
    cecs_texture_builder_descriptor_config_flags flags;
} cecs_texture_builder_descriptor;
typedef struct cecs_texture_builder_wgpu_descriptor {
    WGPUTextureDimension dimension;
    WGPUTextureFormat format;
    WGPUTextureUsage usage;
} cecs_texture_builder_wgpu_descriptor;

typedef struct cecs_file_texture2_builder {
    static_assert(false);
    cecs_texture_builder builder;
    cecs_texture_builder_descriptor descriptor;
    uint8_t *texture_data[CECS_TEXTURE_BUILDER_MAX_TEXTURE_COUNT];
    cecs_texture_builder_texture_count used_texture_slots;
} cecs_file_texture2_builder;

cecs_texture_builder cecs_file_texture2_builder_create(
    cecs_graphics_world *world,
    cecs_arena *texture_arena,
    const cecs_texture_builder_descriptor descriptor,
    const cecs_texture_builder_wgpu_descriptor wgpu_descriptor
);
inline bool cecs_file_texture_builder_is_empty(const cecs_texture_builder *builder) {
    return builder->used_texture_slots == 0;
}

cecs_texture_builder *cecs_file_texture_builder_load_into(
    cecs_texture_builder *builder,
    const char *path,
    const uint_fast8_t texture_slot
);

WGPUTexture cecs_file_texture_builder_build_alloc(
    cecs_texture_builder *builder,
    cecs_graphics_context *context,
    const WGPUTextureViewDescriptor *view_descriptor,
    const uint32_t write_destination_layer,
    const uint_fast8_t texture_slot
);
cecs_texture cecs_file_texture_builder_build(
    cecs_texture_builder *builder,
    cecs_graphics_context *context,
    const WGPUTextureViewDescriptor *view_descriptor,
    const uint32_t write_destination_layer,
    const uint_fast8_t texture_slot
);
cecs_texture_in_bank_bundle cecs_file_texture_builder_build_in_bank(
    cecs_texture_builder *builder,
    cecs_graphics_context *context,
    const WGPUTextureViewDescriptor *view_descriptor
);

#endif
