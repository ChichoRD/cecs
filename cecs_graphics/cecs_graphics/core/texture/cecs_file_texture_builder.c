#include "cecs_file_texture_builder.h"
#include "../builder/cecs_stbi.h"
#include <assert.h>

extern inline cecs_file_texture2_builder cecs_file_texture2_builder_create(
    cecs_graphics_world *world,
    cecs_arena *texture_arena,
    const cecs_file_texture2_builder_descriptor descriptor
);
extern inline bool cecs_file_texture2_builder_is_empty(const cecs_file_texture2_builder *builder);
cecs_file_texture2_builder cecs_file_texture2_builder_create_for_exact(
    cecs_graphics_world *world,
    cecs_arena *texture_arena,
    const cecs_file_texture2_builder_descriptor requirements,
    const char *filepath
) {
    cecs_stbi_allocator_get_current_allocator()->current_arena = texture_arena;
    const cecs_stbi_info info = cecs_stbi_info_from_expect(filepath);
    cecs_stbi_allocator_get_current_allocator()->current_arena = NULL;

    assert(info.width == requirements.descriptor.descriptor.size.width && "error: width must match requirements");
    assert(info.height == requirements.descriptor.descriptor.size.height && "error: height must match requirements");
    assert(info.channels == requirements.channel_count && "error: channel count must match requirements");
    return cecs_file_texture2_builder_create(world, texture_arena, requirements);
}
cecs_file_texture2_builder cecs_file_texture2_builder_create_for_lower(
    cecs_graphics_world *world,
    cecs_arena *texture_arena,
    cecs_file_texture2_builder_descriptor least_requirements,
    const char *filepath
) {
    cecs_stbi_allocator_get_current_allocator()->current_arena = texture_arena;
    const cecs_stbi_info info = cecs_stbi_info_from_expect(filepath);
    cecs_stbi_allocator_get_current_allocator()->current_arena = NULL;

    assert(info.width >= least_requirements.descriptor.descriptor.size.width && "error: width must be greater than or equal to requirements");
    assert(info.height >= least_requirements.descriptor.descriptor.size.height && "error: height must be greater than or equal to requirements");
    assert(info.channels >= least_requirements.channel_count && "error: channel count must be greater than or equal to requirements");

    least_requirements.descriptor.descriptor.size.width = info.width;
    least_requirements.descriptor.descriptor.size.height = info.height;
    return cecs_file_texture2_builder_create(world, texture_arena, least_requirements);
}
cecs_file_texture2_builder cecs_file_texture2_builder_create_for_upper(
    cecs_graphics_world *world,
    cecs_arena *texture_arena,
    cecs_file_texture2_builder_descriptor most_requirements,
    const char *filepath
) {
    cecs_stbi_allocator_get_current_allocator()->current_arena = texture_arena;
    const cecs_stbi_info info = cecs_stbi_info_from_expect(filepath);
    cecs_stbi_allocator_get_current_allocator()->current_arena = NULL;

    assert(info.width <= most_requirements.descriptor.descriptor.size.width && "error: width must be less than or equal to requirements");
    assert(info.height <= most_requirements.descriptor.descriptor.size.height && "error: height must be less than or equal to requirements");
    assert(info.channels <= most_requirements.channel_count && "error: channel count must be less than or equal to requirements");

    most_requirements.descriptor.descriptor.size.width = info.width;
    most_requirements.descriptor.descriptor.size.height = info.height;
    return cecs_file_texture2_builder_create(world, texture_arena, most_requirements);
}

cecs_file_texture2_builder *cecs_file_texture2_builder_load_into(cecs_file_texture2_builder *builder, const char *path, const uint_fast8_t texture_slot) {
    int width;
    int height;
    int channels;
    cecs_stbi_allocator_get_current_allocator()->current_arena = cecs_texture_builder_arena(&builder->builder.builder);
    uint8_t *texture_data = stbi_load(path, &width, &height, &channels, builder->channel_count);
    cecs_stbi_allocator_get_current_allocator()->current_arena = NULL;

    // TODO: add way for user to define padding in case texture has less channels than expected
    if (texture_data == NULL) {
        assert(false && "fatal error: failed to load texture");
        exit(EXIT_FAILURE);
        return builder;
    }
    assert((uint32_t)width == builder->builder.descriptor.descriptor.size.width && "error: width must match requirements");
    assert((uint32_t)height == builder->builder.descriptor.descriptor.size.height && "error: height must match requirements");
    assert((uint32_t)channels == builder->channel_count && "error unexpected: channel count mismatch");

    if (builder->builder.descriptor.flags & cecs_mem_texture_builder_descriptor_config_generate_mipmaps) {
        builder->builder.descriptor.flags |= cecs_mem_texture_builder_descriptor_config_alloc_mipmaps;
    }
    return cecs_mem_texture_builder_take_into_mut(builder, texture_data, texture_slot);
}

size_t cecs_file_texture2_builder_build_alloc(
    cecs_file_texture2_builder *builder,
    cecs_graphics_context *context,
    WGPUTextureViewDescriptor *view_descriptor,
    WGPUTexture destination_textures[const restrict static 1],
    const size_t destination_textures_capacity
) {
    size_t built = cecs_mem_texture_builder_build_alloc(
        &builder->builder,
        context,
        view_descriptor,
        destination_textures,
        destination_textures_capacity,
        0
    );

    cecs_stbi_allocator_get_current_allocator()->current_arena = cecs_texture_builder_arena(&builder->builder.builder);
    for (size_t i = 0; i < built; i++) {
        stbi_image_free(builder->builder.textures_bytes[i]);
    }
    cecs_stbi_allocator_get_current_allocator()->current_arena = NULL;
    return built;
}
cecs_texture_in_bank_bundle cecs_file_texture2_builder_build_in_bank(
    cecs_file_texture2_builder *builder,
    cecs_graphics_context *context,
    WGPUTextureViewDescriptor *view_descriptor,
    const size_t bundle_range_capacity
) {
    cecs_texture_in_bank_bundle bundle = cecs_mem_texture_builder_build_in_bank(
        builder,
        context,
        view_descriptor,
        bundle_range_capacity,
        0
    );

    cecs_stbi_allocator_get_current_allocator()->current_arena = cecs_texture_builder_arena(&builder->builder.builder);
    for (size_t i = 0; i < bundle.range.slot_range; i++) {
        stbi_image_free(builder->builder.textures_bytes[i]);
    }
    cecs_stbi_allocator_get_current_allocator()->current_arena = NULL;
    return bundle;
}
