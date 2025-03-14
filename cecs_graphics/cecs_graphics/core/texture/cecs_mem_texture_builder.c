#include "cecs_mem_texture_builder.h"
#include <cecs_math/relations/cecs_ordering.h>

#define CECS_MEM_TEXTURE_BUILDER_MAX_MIP_LEVEL ((1 << CHAR_BIT) - 2)
static const uint8_t cecs_mem_texture_builder_max_mip_level = CECS_MEM_TEXTURE_BUILDER_MAX_MIP_LEVEL;

cecs_mem_texture_builder cecs_mem_texture_builder_create(
    cecs_graphics_world *world,
    cecs_arena *texture_arena,
    const cecs_mem_texture_builder_descriptor descriptor
) {
    assert(descriptor.bytes_per_texel > 0 && "error: bytes per pixel must be greater than 0");
    assert(descriptor.max_mip_level <= cecs_mem_texture_builder_max_mip_level && "error: max mip level must be less than or equal to the maximum mip level");
    return (cecs_mem_texture_builder){
        .builder = cecs_texture_builder_create(
            descriptor.descriptor,
            world,
            texture_arena
        ),
        .descriptor = descriptor,
        .textures_bytes = {0},
        .used_texture_slots = 0,
    };
}
extern inline bool cecs_mem_texture_builder_is_empty(const cecs_mem_texture_builder *builder);
extern inline size_t cecs_mem_texture_builder_mip0_size(const cecs_mem_texture_builder *builder);

cecs_mem_texture_builder *cecs_mem_texture_builder_take_into_mut(
    cecs_mem_texture_builder *builder,
    uint8_t *texture_data,
    const uint_fast8_t texture_slot
) {
    const bool builder_empty = cecs_texture_builder_is_empty(builder);
    if (texture_slot == builder->used_texture_slots) {
        assert(texture_slot < CECS_TEXTURE_BUILDER_MAX_TEXTURE_COUNT && "error: cannot use more texture slots than the defined maximum");
        assert(builder->textures_bytes[builder->used_texture_slots] == NULL && "error: texture data must be NULL if texture slot is unused");
        ++builder->used_texture_slots;
    } else if (texture_slot < builder->used_texture_slots) {
        if (builder->textures_bytes[texture_slot] == NULL) {
            assert(false && "fatal error: texture slot is used but texture data is NULL");
            exit(EXIT_FAILURE);
        }
        assert(builder->used_texture_slots < CECS_TEXTURE_BUILDER_MAX_TEXTURE_COUNT && "error: cannot use more texture slots than the defined maximum");
        builder->textures_bytes[builder->used_texture_slots] = NULL;
        // TODO: try to reuse texture slot memory
    } else {
        assert(false && "error: can only take into either a used texture slot or the first unused slot");
        exit(EXIT_FAILURE);
    }

    builder->textures_bytes[texture_slot] = texture_data;
}

static size_t cecs_mem_texture_builder_generate_mipchain(
    cecs_mem_texture_builder *builder,
    const cecs_texture_builder_texture_count count,
    const uint32_t depth_start_layer
) {
    assert(count > 0 && "error: must have at least one texture to generate mipmaps");
    assert(count <= builder->used_texture_slots && "error: cannot generate mipmaps for more textures than used texture slots");

    uint32_t largest_side_size;
    const cecs_texture_size_pow2 texture_size = cecs_texture_builder_ensured_size(&builder->builder, &largest_side_size);
    const uint_fast8_t max_mip_count = cecs_texture_size_pow2_max_mip_count(texture_size, largest_side_size);
    const uint_fast8_t mip_count = cecs_min(builder->descriptor.max_mip_level + 1, max_mip_count);

    const size_t texel_count = cecs_texture_size_pow2_mipmaps_buffer_texels(texture_size, mip_count);
    const size_t mipchain_size = texel_count * builder->descriptor.bytes_per_texel;
    if (builder->descriptor.flags & cecs_mem_texture_builder_descriptor_config_alloc_mipmaps) {
        const size_t mip0_size = cecs_mem_texture_builder_mip0_size(builder);
        
        for (cecs_texture_builder_texture_count i = 0; i < count; i++) {
            builder->textures_bytes[i] = cecs_arena_realloc(
                builder->builder.texture_arena,
                builder->textures_bytes[i],
                mip0_size,
                mipchain_size
            );

            cecs_generate_mipmaps(
                builder->builder.descriptor.size,
                builder->textures_bytes[i],
                builder->descriptor.bytes_per_texel,
                mip_count,
                builder->textures_bytes[i]
            );
        }
    } else {
        for (cecs_texture_builder_texture_count i = 0; i < count; i++) {
            cecs_generate_mipmaps(
                builder->builder.descriptor.size,
                builder->textures_bytes[i],
                builder->descriptor.bytes_per_texel,
                mip_count,
                builder->textures_bytes[i]
            );
        }
    }
    return mipchain_size;
}

size_t cecs_mem_texture_builder_build_alloc(
    cecs_mem_texture_builder *builder,
    cecs_graphics_context *context,
    const WGPUTextureViewDescriptor *view_descriptor,
    WGPUTexture destination_textures[const restrict static 1],
    const size_t destination_textures_capacity,
    const uint32_t depth_start_layer
) {
    const size_t write_count = cecs_min(
        builder->used_texture_slots,
        destination_textures_capacity
    );
    assert(write_count > 0 && "error: must have at least one texture to write");

    size_t mipmaps_size;
    if (builder->descriptor.flags & cecs_mem_texture_builder_descriptor_config_generate_mipmaps) {
        mipmaps_size = cecs_mem_texture_builder_generate_mipchain(builder, write_count, depth_start_layer, depth_start_layer);
    } else {
        mipmaps_size = cecs_mem_texture_builder_mip0_size(builder);
    }

    for (size_t i = 0; i < write_count; i++) {
        WGPUTexture texture = wgpuDeviceCreateTexture(context->device, &builder->builder.descriptor);
        
        const cecs_mipmaps_write_descriptor mipmaps = {
            .source_texels = builder->textures_bytes[i],
            .source_size = mipmaps_size,
            .bytes_per_texel = builder->descriptor.bytes_per_texel,
            .destination_layer = depth_start_layer,
        };
        const size_t write_size = cecs_write_mipmaps(
            texture,
            context->queue,
            &builder->builder.descriptor, 
            builder->textures_bytes[i],
            builder->descriptor.bytes_per_texel,
            view_descriptor->aspect,
            mipmaps
        );
        (void)write_size;

        destination_textures[i] = texture;
    }
    return write_count;
}