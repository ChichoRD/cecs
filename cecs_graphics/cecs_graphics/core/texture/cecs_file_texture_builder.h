#ifndef CECS_FILE_TEXTURE_BUILDER_H
#define CECS_FILE_TEXTURE_BUILDER_H

#include "cecs_mem_texture_builder.h"
#include "cecs_texture_bank.h"

typedef uint8_t cecs_file_texture2_builder_descriptor_channel_count;
typedef struct cecs_file_texture2_builder_descriptor {
    cecs_mem_texture_builder_descriptor descriptor;
    cecs_file_texture2_builder_descriptor_channel_count channel_count;
} cecs_file_texture2_builder_descriptor;
typedef struct cecs_file_texture2_builder {
    cecs_mem_texture_builder builder;
    cecs_file_texture2_builder_descriptor_channel_count channel_count;
} cecs_file_texture2_builder;

inline cecs_file_texture2_builder cecs_file_texture2_builder_create(
    cecs_graphics_world *world,
    cecs_arena *texture_arena,
    const cecs_file_texture2_builder_descriptor descriptor
) {
    return (cecs_file_texture2_builder){
        .builder = cecs_mem_texture_builder_create(
            world,
            texture_arena,
            descriptor.descriptor
        ),
        .channel_count = descriptor.channel_count,
    };
}
cecs_file_texture2_builder cecs_file_texture2_builder_create_for_exact(
    cecs_graphics_world *world,
    cecs_arena *texture_arena,
    const cecs_file_texture2_builder_descriptor requirements,
    const char *filepath
);
cecs_file_texture2_builder cecs_file_texture2_builder_create_for_lower(
    cecs_graphics_world *world,
    cecs_arena *texture_arena,
    cecs_file_texture2_builder_descriptor least_requirements,
    const char *filepath
);

cecs_file_texture2_builder cecs_file_texture2_builder_create_for_upper(
    cecs_graphics_world *world,
    cecs_arena *texture_arena,
    cecs_file_texture2_builder_descriptor most_requirements,
    const char *filepath
);
inline bool cecs_file_texture2_builder_is_empty(const cecs_file_texture2_builder *builder) {
    return cecs_mem_texture_builder_is_empty(&builder->builder);
}

cecs_file_texture2_builder *cecs_file_texture2_builder_load_into(
    cecs_file_texture2_builder *builder,
    const char *filepath,
    const uint_fast8_t texture_slot
);

size_t cecs_file_texture2_builder_build_alloc(
    cecs_file_texture2_builder *builder,
    cecs_graphics_context *context,
    WGPUTextureViewDescriptor *view_descriptor,
    WGPUTexture destination_textures[const restrict static 1],
    const size_t destination_textures_capacity
);
cecs_texture_in_bank_bundle cecs_file_texture2_builder_build_in_bank(
    cecs_file_texture2_builder *builder,
    cecs_graphics_context *context,
    WGPUTextureViewDescriptor *view_descriptor,
    const size_t bundle_range_capacity
);

#endif
