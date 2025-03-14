#include "cecs_mem_texture_builder.h"
#include <cecs_math/relations/cecs_ordering.h>

#define CECS_MEM_TEXTURE_BUILDER_MAX_MIP_LEVEL ((1 << CHAR_BIT) - 2)
const uint8_t cecs_mem_texture_builder_max_mip_level = CECS_MEM_TEXTURE_BUILDER_MAX_MIP_LEVEL;

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
    const uint32_t depth_start_layer,
    const cecs_texture_size_pow2 size,
    const uint_fast8_t mip_count
) {
    assert(count > 0 && "error: must have at least one texture to generate mipmaps");
    assert(count <= builder->used_texture_slots && "error: cannot generate mipmaps for more textures than used texture slots");

    const size_t texel_count = cecs_texture_size_pow2_mipmaps_buffer_texels(size, mip_count);
    const size_t mipchain_size = texel_count * builder->descriptor.bytes_per_texel;
    const size_t mip0_size = cecs_mem_texture_builder_mip0_size(builder);

    if (builder->descriptor.flags & cecs_mem_texture_builder_descriptor_config_alloc_mipmaps) {
        builder->descriptor.descriptor.mipLevelCount = mip_count;
        for (cecs_texture_builder_texture_count i = 0; i < count; i++) {
            builder->textures_bytes[i] = cecs_arena_realloc(
                cecs_texture_builder_arena(&builder->builder),
                builder->textures_bytes[i],
                mip0_size,
                mipchain_size
            );
            
            cecs_generate_mipmaps(
                builder->builder.descriptor.size,
                builder->textures_bytes[i],
                builder->descriptor.bytes_per_texel,
                mip_count,
                builder->textures_bytes[i] + mip0_size
            );
        }
    } else {
        assert(mip0_size == mipchain_size && "error: mip0 size must match mipchain size if mipmaps have been accounted for in the allocation");
        for (cecs_texture_builder_texture_count i = 0; i < count; i++) {
            cecs_generate_mipmaps(
                builder->builder.descriptor.size,
                builder->textures_bytes[i],
                builder->descriptor.bytes_per_texel,
                mip_count,
                builder->textures_bytes[i] + mip0_size
            );
        }
    }
    return mipchain_size;
}

size_t cecs_mem_texture_builder_build_alloc(
    cecs_mem_texture_builder *builder,
    cecs_graphics_context *context,
    WGPUTextureViewDescriptor *view_descriptor,
    WGPUTexture destination_textures[const restrict static 1],
    const size_t destination_textures_capacity,
    const uint32_t depth_start_layer
) {
    assert(builder->descriptor.descriptor.usage & WGPUTextureUsage_CopyDst && "error: texture must be copyable");
    const size_t write_count = cecs_min(
        (size_t)builder->used_texture_slots,
        destination_textures_capacity
    );
    assert(write_count > 0 && "error: must have at least one texture to write");

    size_t mipmaps_size;
    if (builder->descriptor.flags & cecs_mem_texture_builder_descriptor_config_generate_mipmaps) {
        uint32_t largest_side_size;
        const cecs_texture_size_pow2 texture_size = cecs_texture_builder_ensured_size(&builder->builder, &largest_side_size);
        const uint_fast8_t max_mip_count = cecs_texture_size_pow2_max_mip_count(texture_size, largest_side_size);
        const uint_fast8_t mip_count = cecs_min(builder->descriptor.max_mip_level + 1, max_mip_count);
    
        mipmaps_size = cecs_mem_texture_builder_generate_mipchain(
            builder,
            write_count,
            depth_start_layer,
            texture_size,
            mip_count
        );
        view_descriptor->mipLevelCount = mip_count;
    } else {
        mipmaps_size = cecs_mem_texture_builder_mip0_size(builder);
        assert(
            view_descriptor->mipLevelCount == builder->descriptor.descriptor.mipLevelCount
            && "error: mip level count must match descriptor mip level count if mipmaps are not generated"
        );
    }

    for (size_t i = 0; i < write_count; i++) {
        WGPUTexture texture = cecs_texture_builder_build_alloc(
            &builder->builder,
            context
        );
        
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
            view_descriptor->aspect,
            mipmaps
        );
        (void)write_size;

        destination_textures[i] = texture;
    }
    return write_count;
}
cecs_texture_in_bank_bundle cecs_mem_texture_builder_build_in_bank(
    cecs_mem_texture_builder *builder,
    cecs_graphics_context *context,
    WGPUTextureViewDescriptor *view_descriptor,
    const size_t bundle_range_capacity,
    const uint32_t depth_start_layer
) {
    assert(builder->descriptor.descriptor.usage & WGPUTextureUsage_CopyDst && "error: texture must be copyable");
    const size_t write_count = cecs_min(
        (size_t)builder->used_texture_slots,
        bundle_range_capacity
    );
    assert(write_count > 0 && "error: must have at least one texture to write");

    size_t mipmaps_size;
    uint32_t largest_side_size;
    const cecs_texture_size_pow2 texture_size = cecs_texture_builder_ensured_size(&builder->builder, &largest_side_size);
    const uint_fast8_t max_mip_count = cecs_texture_size_pow2_max_mip_count(texture_size, largest_side_size);
    const uint_fast8_t mip_count = cecs_min(builder->descriptor.max_mip_level + 1, max_mip_count);
    if (builder->descriptor.flags & cecs_mem_texture_builder_descriptor_config_generate_mipmaps) {
        mipmaps_size = cecs_mem_texture_builder_generate_mipchain(
            builder,
            write_count,
            depth_start_layer,
            texture_size,
            mip_count
        );
        view_descriptor->mipLevelCount = mip_count;
    } else {
        mipmaps_size = cecs_mem_texture_builder_mip0_size(builder);
        assert(
            view_descriptor->mipLevelCount == builder->descriptor.descriptor.mipLevelCount
            && "error: mip level count must match descriptor mip level count if mipmaps are not generated"
        );
    }

    static_assert(sizeof(cecs_texture_size_pow2) == sizeof(uint32_t), "static error: expected sizeof cecs_texture_size_pow2 to be 4 bytes");
    const cecs_texture_bank_id_descriptor bank_descriptor = cecs_texture_bank_id_descriptor_create_free(
        texture_size,
        mip_count,
        builder->descriptor.descriptor.format,
        builder->descriptor.descriptor.usage
    );

    cecs_entity_id bank_entity_id;
    uint_fast8_t slot_index;
    cecs_world *world = cecs_texture_builder_world(&builder->builder);
    cecs_texture_bank *bank = cecs_texture_bank_find_allocated_or_null(
        world,
        cecs_texture_builder_arena(&builder->builder),
        bank_descriptor,
        bundle_range_capacity,
        &bank_entity_id,
        &slot_index
    );
    if (bank == NULL) {
        bank = cecs_texture_bank_allocate(
            world,
            context->device,
            bank_descriptor,
            builder->descriptor.descriptor.sampleCount,
            &bank_entity_id
        );
        slot_index = 0;
    }
    bank = cecs_texture_bank_use_and_relocate(
        world,
        bank,
        bank_descriptor,
        bank_entity_id,
        slot_index,
        write_count
    );

    for (uint_fast8_t i = 0; i < write_count; i++) {
        WGPUTexture texture = cecs_texture_builder_build_alloc(
            &builder->builder,
            context
        );
        
        const cecs_mipmaps_write_descriptor mipmaps = {
            .source_texels = builder->textures_bytes[i] + mipmaps_size * depth_start_layer,
            .source_size = mipmaps_size,
            .bytes_per_texel = builder->descriptor.bytes_per_texel,
            .destination_layer = slot_index + i,
        };
    
        const size_t write_size = cecs_write_mipmaps(
            texture,
            context->queue,
            &builder->builder.descriptor,
            view_descriptor->aspect,
            mipmaps
        );
    }
    
    float bank_width = (float)wgpuTextureGetWidth(bank->texture);
    float bank_height = (float)wgpuTextureGetHeight(bank->texture);
    return (cecs_texture_in_bank_bundle){
        .reference = (cecs_texture_in_bank_reference){bank_entity_id},
        .range = (cecs_texture_in_bank_range){
            .slot_index = slot_index,
            .slot_range = builder->used_texture_slots,
        },
        .subrect = (cecs_texture_subrect2_f32){
            .normalized_width = builder->descriptor.descriptor.size.width / bank_width,
            .normalized_height = builder->descriptor.descriptor.size.height / bank_height,
        },
    };
}
