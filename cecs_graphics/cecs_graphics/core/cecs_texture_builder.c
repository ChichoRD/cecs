#include <assert.h>
#include <stdlib.h>

#include <cecs_math/cecs_math.h>

#include "builder/cecs_stbi.h"
#include "cecs_texture_builder.h"

extern inline cecs_texture_builder_base cecs_texture_builder_base_create(
    const WGPUTextureDescriptor descriptor,
    cecs_graphics_world *world,
    cecs_arena *texture_arena
);

WGPUTexture cecs_texture_builder_base_build_alloc(
    cecs_texture_builder_base *builder,
    cecs_graphics_context *context
) {
    WGPUTexture texture = wgpuDeviceCreateTexture(context->device, &builder->descriptor);
    if (texture == NULL) {
        assert(false && "fatal error: failed to create texture");
        exit(EXIT_FAILURE);
    }
    return texture;
}
extern inline cecs_graphics_world *cecs_texture_builder_base_world(cecs_texture_builder_base *builder);
extern inline cecs_arena *cecs_texture_builder_base_arena(cecs_texture_builder_base *builder);

cecs_texture_size_pow2 cecs_texture_builder_base_ensured_size(
    const cecs_texture_builder_base *builder,
    uint32_t *out_largest_side_size
) {
    return cecs_texture_size_pow2_ensure(builder->descriptor.size, out_largest_side_size);
}

size_t cecs_texture_builder_base_write_mipmaps(
    WGPUTexture destination,
    cecs_texture_builder_base *builder,
    cecs_graphics_context *context,
    const WGPUTextureAspect aspect,
    const cecs_mipmaps_write_descriptor mipmaps
) {
    return cecs_write_mipmaps(
        destination,
        context->queue,
        &builder->descriptor,
        aspect,
        mipmaps
    );
}

cecs_texture_builder cecs_texture_builder_create(
    cecs_graphics_world *world,
    cecs_arena *texture_arena,
    const cecs_texture_builder_descriptor descriptor,
    const cecs_texture_builder_wgpu_descriptor wgpu_descriptor)
{
    assert(descriptor.bytes_per_texel > 0 && "error: bytes per pixel must be greater than 0");
    assert(descriptor.channel_count > 0 && "error: channel count must be greater than 0");

    // TODO: handle other descriptor options
    return (cecs_texture_builder){
        .world = world,
        .texture_arena = texture_arena,
        .texture_descriptor = {
            .dimension = wgpu_descriptor.dimension,
            .format = wgpu_descriptor.format,
            .usage = wgpu_descriptor.usage,
        },
        .descriptor = descriptor,
        .texture_data = {0},
        .used_texture_slots = 0,
    };
}
extern inline bool cecs_texture_builder_is_empty(const cecs_texture_builder *builder);

cecs_texture_builder *cecs_texture_builder_load_into(
    cecs_texture_builder *builder,
    const char *path,
    const uint_fast8_t texture_slot 
) {
    const bool texture_builder_empty = cecs_texture_builder_is_empty(builder);
    if (texture_slot == builder->used_texture_slots) {
        assert(texture_slot < CECS_TEXTURE_BUILDER_MAX_TEXTURE_COUNT && "error: cannot use more texture slots than the defined maximum");
        assert(builder->texture_data[builder->used_texture_slots] == NULL && "error: texture data must be NULL if texture slot is unused");
        ++builder->used_texture_slots;
    } else if (texture_slot < builder->used_texture_slots) {
        if (builder->texture_data[texture_slot] == NULL) {
            assert(false && "fatal error: texture slot is used but texture data is NULL");
            exit(EXIT_FAILURE);
        }
        
        assert(builder->used_texture_slots < CECS_TEXTURE_BUILDER_MAX_TEXTURE_COUNT && "error: cannot use more texture slots than the defined maximum");
        builder->texture_data[builder->used_texture_slots] = NULL;
        // TODO: try to reuse texture slot memory
    } else {
        assert(false && "error: can only load into either a used texture slot or the first unused slot");
        exit(EXIT_FAILURE);
    }

    int width;
    int height;
    int channels;
    cecs_stbi_allocator_get_current_allocator()->current_arena = builder->texture_arena;
    uint8_t *texture_data = stbi_load(path, &width, &height, &channels, builder->descriptor.channel_count);
    //stbi_load
    cecs_stbi_allocator_get_current_allocator()->current_arena = NULL;
    assert(channels == builder->descriptor.channel_count && "error unexpected: channel count mismatch");

    // TODO: add way for user to define padding in case texture has less channels than expected
    if (texture_data == NULL) {
        assert(false && "fatal error: failed to load texture");
        exit(EXIT_FAILURE);
        return builder;
    }
    const WGPUExtent3D size = {
        .width = (uint32_t)width,
        .height = (uint32_t)height,
        .depthOrArrayLayers = 1,
    };
    
    if (texture_builder_empty) {
        builder->texture_descriptor = (WGPUTextureDescriptor){
            .nextInChain = NULL,
            .dimension = builder->texture_descriptor.dimension,
            .format = builder->texture_descriptor.format,
            .mipLevelCount = 0,
            .sampleCount = 1,
            .size = size,
            .usage = builder->texture_descriptor.usage,
            .viewFormatCount = 0,
            .viewFormats = NULL,
        };
    } else if (builder->texture_descriptor.size.width != size.width || builder->texture_descriptor.size.height != size.height) {
        assert(false && "error: texture size must match the size of the first loaded texture");
        exit(EXIT_FAILURE);
    }

    if (builder->descriptor.flags & cecs_texture_builder_descriptor_config_generate_mipmaps) {
        builder->descriptor.flags |= cecs_texture_builder_descriptor_config_alloc_mipmaps;
    }

    //static_assert(false, "TODO: handle loading multiple (clearing builder after build); set to NULL prior");
    return cecs_texture_builder_take_into(builder, texture_data, texture_slot);
}
cecs_texture_builder *cecs_texture_builder_take_into(cecs_texture_builder *builder, uint8_t *texture_data, const uint_fast8_t texture_slot) {
    assert(texture_slot < builder->used_texture_slots && "error: texture slot must be used");
    
    builder->texture_data[texture_slot] = texture_data;
    return builder;
}

cecs_texture_builder *cecs_texture_builder_clear_and_set(
    cecs_texture_builder *builder,
    const cecs_texture_builder_descriptor descriptor,
    const cecs_texture_builder_wgpu_descriptor wgpu_descriptor
) {
    builder->descriptor = descriptor;
    builder->texture_descriptor = (WGPUTextureDescriptor){
        .dimension = wgpu_descriptor.dimension,
        .format = wgpu_descriptor.format,
        .usage = wgpu_descriptor.usage,
    };
    builder->used_texture_slots = 0;
    memset(builder->texture_data, 0, sizeof(builder->texture_data));
    return builder;
}
cecs_texture_builder *cecs_texture_builder_clear(cecs_texture_builder *builder) {
    builder->descriptor = (cecs_texture_builder_descriptor){0};
    builder->texture_descriptor = (WGPUTextureDescriptor){0};
    builder->used_texture_slots = 0;
    memset(builder->texture_data, 0, sizeof(builder->texture_data));
    return builder;
}

WGPUExtent3D cecs_generate_next_mip(
    const WGPUExtent3D mip_size,
    const uint8_t *restrict mip_texels,
    const uint_fast8_t bytes_per_texel,
    uint8_t out_next_mip_texels[const restrict])
{
    const WGPUExtent3D next_mip_size = {
        .width = cecs_max_u32(mip_size.width >> 1, 1),
        .height = cecs_max_u32(mip_size.height >> 1, 1),
        .depthOrArrayLayers = 1,
    };

    for (uint32_t i = 0; i < next_mip_size.width; ++i) {
        for (uint32_t j = 0; j < next_mip_size.height; ++j) {
            uint8_t *destination_texel =
                out_next_mip_texels + (j * next_mip_size.width + i) * bytes_per_texel;

            // Get the corresponding 4 pixels from the previous level
            const uint8_t *texel_00 = mip_texels + bytes_per_texel * ((2 * j + 0) * mip_size.width + (2 * i + 0));
            const uint8_t *texel_01 = mip_texels + bytes_per_texel * ((2 * j + 0) * mip_size.width + (2 * i + 1));
            const uint8_t *texel_10 = mip_texels + bytes_per_texel * ((2 * j + 1) * mip_size.width + (2 * i + 0));
            const uint8_t *texel_11 = mip_texels + bytes_per_texel * ((2 * j + 1) * mip_size.width + (2 * i + 1));
            
            for (uint_fast8_t k = 0; k < bytes_per_texel; ++k) {
                const uint_fast32_t average = ((uint_fast32_t)texel_00[k] + texel_01[k] + texel_10[k] + texel_11[k]) >> 2;
                destination_texel[k] = average;
            }
        }
    }

    return next_mip_size;
}

size_t cecs_generate_mipmaps(
    const WGPUExtent3D mip_size,
    const uint8_t *mip_texels,
    const uint_fast8_t bytes_per_texel,
    const uint_fast8_t mip_level_count,
    uint8_t out_mipmaps_start[]
) {
    assert(mip_texels != NULL && "error: mip texels must be set");
    assert(out_mipmaps_start != NULL && "error: out mip chain start must be set");
    
    WGPUExtent3D previous_mip_size = mip_size;
    const uint8_t *previous_mips = mip_texels;

    for (uint_fast8_t i = 1; i < mip_level_count; i++) {
        const WGPUExtent3D new_mip_size = cecs_generate_next_mip(
            previous_mip_size,
            previous_mips,
            bytes_per_texel,
            out_mipmaps_start
        );
        const uint32_t mip_bytes_per_row = new_mip_size.width * bytes_per_texel;
        const size_t mip_texture_size = new_mip_size.height * mip_bytes_per_row;

        previous_mip_size = new_mip_size;
        previous_mips = out_mipmaps_start;
        out_mipmaps_start += mip_texture_size;
    }

    return out_mipmaps_start - mip_texels;
}

size_t cecs_write_mipmaps(
    WGPUTexture destination,
    WGPUQueue queue,
    const WGPUTextureDescriptor *descriptor,
    const WGPUTextureAspect aspect,
    const cecs_mipmaps_write_descriptor mipmaps
) {
    assert(mipmaps.source_texels != NULL && "error: source texels must be set");
    assert(mipmaps.source_size > 0 && "error: source size must be greater than 0");
    assert(mipmaps.bytes_per_texel > 0 && "error: bytes per texel must be greater than 0");


    const uint8_t *source_mip_start = mipmaps.source_texels;
    const uint8_t *const source_data_end = source_mip_start + mipmaps.source_size;

    for (uint32_t i = 0; i < descriptor->mipLevelCount; i++) {
        const uint32_t mip_width = cecs_max_u32(descriptor->size.width >> i, 1);
        const uint32_t mip_height = cecs_max_u32(descriptor->size.height >> i, 1);
        const uint32_t mip_bytes_per_row = mip_width * mipmaps.bytes_per_texel;
        const size_t mip_texture_size = mip_bytes_per_row * mip_height;

        const uint8_t *const source_mip_end = source_mip_start + mip_texture_size;
        assert(source_mip_end <= source_data_end && "error: source data must not exceed source size"); 

        const WGPUImageCopyTexture destination_mip = (WGPUImageCopyTexture){
            .nextInChain = NULL,
            .texture = destination,
            .mipLevel = i,
            .origin = (WGPUOrigin3D){0, 0, (uint32_t)mipmaps.destination_layer},
            .aspect = aspect,
        };
        const WGPUTextureDataLayout source_mip = (WGPUTextureDataLayout){
            .nextInChain = NULL,
            .offset = 0,
            .bytesPerRow = mip_bytes_per_row,
            .rowsPerImage = mip_height,
        };
        const WGPUExtent3D mip_size = {
            .width = mip_width,
            .height = mip_height,
            .depthOrArrayLayers = 1,
        };
        wgpuQueueWriteTexture(queue, &destination_mip, source_mip_start, mip_texture_size, &source_mip, &mip_size);
        source_mip_start = source_mip_end;
    }
    return source_mip_start - mipmaps.source_texels;
}

static inline uint_fast8_t cecs_texture_builder_mip_count(const WGPUExtent3D size, uint32_t *out_largest_side_size) {
    *out_largest_side_size = cecs_max_u32(size.width, size.height);
    return cecs_log2_u32(*out_largest_side_size) + 1;
}

static inline cecs_texture_size_pow2 cecs_texture_builder_size_from_mip_count(const uint_fast8_t mip_count) {
    return (cecs_texture_size_pow2)(mip_count);
}

static cecs_texture_size_pow2 cecs_texture_builder_configure_mipmaps(
    cecs_texture_builder *builder,
    const uint_fast8_t mip_count,
    const uint_fast8_t texture_slot,
    size_t *out_mipmaps_size
) {
    assert(texture_slot < builder->used_texture_slots && "error: texture slot must be used");
    assert(builder->texture_descriptor.mipLevelCount == mip_count && "error: mip count must match the configured mip count");

    uint8_t *texture_data = builder->texture_data[texture_slot];
    assert(texture_data != NULL && "error: texture data must be set");
    const uint32_t min_side = cecs_min_u32(builder->texture_descriptor.size.width, builder->texture_descriptor.size.height);

    // 17px -> 5 size, 5 mip, 5 log
    // 16px -> 4 size, 5 mip, 4 log
    // 15px -> 4 size, 4 mip, 4 log
    
    const size_t texture_texels =
        builder->texture_descriptor.size.width * builder->texture_descriptor.size.height;
    const size_t texture_size =
        texture_texels * builder->descriptor.bytes_per_texel;
    if (builder->descriptor.flags & cecs_texture_builder_descriptor_config_generate_mipmaps) {

        const uint_fast8_t side_log2_difference =
            mip_count
            - cecs_log2_u32(min_side)
            - 1;

        const size_t small_side_texels = ((1 << side_log2_difference) - 1) * builder->descriptor.bytes_per_texel;
        const size_t mip_buffer_size =
            (texture_size * 4 / 3) + (small_side_texels);
        uint8_t *mip_texels;
        if (builder->descriptor.flags & cecs_texture_builder_descriptor_config_alloc_mipmaps) {
            mip_texels =
                cecs_arena_realloc(builder->texture_arena, texture_data, texture_size, mip_buffer_size);
        } else {
            mip_texels = texture_data;
        }

        const size_t mip_chain_size = cecs_generate_mipmaps(
            builder->texture_descriptor.size,
            mip_texels,
            builder->descriptor.bytes_per_texel,
            mip_count,
            mip_texels + texture_size
        );

        assert(mip_chain_size <= mip_buffer_size && "fatal error: not allocated enough memory for mip chain");
        texture_data = cecs_arena_realloc(
            builder->texture_arena,
            mip_texels,
            mip_buffer_size,
            mip_chain_size
        );

        builder->texture_data[texture_slot] = texture_data;
        *out_mipmaps_size = mip_chain_size;
    } else {
        *out_mipmaps_size = texture_size;
    }

    const uint32_t side_sum = builder->texture_descriptor.size.width + builder->texture_descriptor.size.height;
    return cecs_texture_builder_size_from_mip_count(cecs_is_pow2_u32(side_sum - min_side) ? mip_count - 1 : mip_count);
}

WGPUTexture cecs_texture_builder_build_alloc(
    cecs_texture_builder *builder,
    cecs_graphics_context *context,
    const WGPUTextureViewDescriptor *view_descriptor,
    const uint32_t write_destination_layer,
    const uint_fast8_t texture_slot
) {
    assert(texture_slot < builder->used_texture_slots && "error: texture slot must be used");
    uint8_t *texture_data = builder->texture_data[texture_slot];
    
    if (texture_data == NULL) {
        assert(false && "fatal error: texture data must be set if texture slot is used");
        exit(EXIT_FAILURE);
    }
    
    uint32_t largest_side_size;
    size_t mipmaps_size;
    cecs_texture_builder_configure_mipmaps(
        builder,
        cecs_texture_builder_mip_count(builder->texture_descriptor.size, &largest_side_size),
        texture_slot,
        &mipmaps_size
    );
    WGPUTexture texture = wgpuDeviceCreateTexture(context->device, &builder->texture_descriptor);

    assert(builder->texture_descriptor.usage & WGPUTextureUsage_CopyDst && "error: texture must be copyable");
    size_t write_size = cecs_write_mipmaps(
        context->queue,
        texture,
        &builder->texture_descriptor, 
        texture_data,
        builder->descriptor.bytes_per_texel,
        view_descriptor->aspect,
        write_destination_layer
    );
    assert(write_size == mipmaps_size && "error: writen size does not match mipmaps size");

    cecs_stbi_allocator_get_current_allocator()->current_arena = builder->texture_arena;
    stbi_image_free(texture_data);
    cecs_stbi_allocator_get_current_allocator()->current_arena = NULL;

    return texture;
}
extern inline uint_fast8_t cecs_texture_builder_build_alloc_range(
    cecs_texture_builder *builder,
    cecs_graphics_context *context,
    const WGPUTextureViewDescriptor *view_descriptor,
    const uint32_t write_destination_layer,
    const cecs_exclusive_range slot_range,
    WGPUTexture out_textures[const restrict static 1]
);

cecs_texture cecs_texture_builder_build(
    cecs_texture_builder *builder,
    cecs_graphics_context *context,
    const WGPUTextureViewDescriptor *view_descriptor,
    const uint32_t write_destination_layer,
    const uint_fast8_t texture_slot
) {
    WGPUTexture texture = cecs_texture_builder_build_alloc(builder, context, view_descriptor, write_destination_layer, texture_slot);
    WGPUTextureView texture_view = wgpuTextureCreateView(texture, view_descriptor);
    wgpuTextureRelease(texture);
    return (cecs_texture){
        .texture_view = texture_view,
        .extent = builder->texture_descriptor.size
    };
}
extern inline uint_fast8_t cecs_texture_builder_build_range(
    cecs_texture_builder *builder,
    cecs_graphics_context *context,
    const WGPUTextureViewDescriptor *view_descriptor,
    const uint32_t write_destination_layer,
    const cecs_exclusive_range slot_range,
    cecs_texture out_textures[const restrict static 1]
);

cecs_texture_bank *cecs_texture_bank_find_allocated_or_null(
    cecs_graphics_world *world,
    cecs_arena *iteration_arena,
    const cecs_texture_bank_id_descriptor bank_descriptor,
    const uint_fast8_t required_slots_count,
    cecs_entity_id *out_bank_entity_id,
    uint_fast8_t *out_first_slot_index
) {
    cecs_texture_bank *bank = NULL;
    const cecs_component_id texture_bank_id = cecs_component_id_from_texture_resource_id_descriptor(bank_descriptor);
    CECS_COMPONENT_ITERATION_HANDLE_STRUCT(cecs_texture_bank) handle;
    cecs_component_iterator it = CECS_COMPONENT_ITERATOR_CREATE_GROUPED(&world->world.components, iteration_arena, 
        CECS_COMPONENT_GROUP_FROM_IDS(
            cecs_component_access_inmmutable, cecs_component_group_search_all, CECS_RELATION_ID(cecs_texture_bank, texture_bank_id)
        )
    );
    for (
        cecs_component_iterator_begin_iter(&it, iteration_arena);
        !cecs_component_iterator_done(&it);
        cecs_component_iterator_next(&it)
    ) {
        const cecs_entity_id entity = cecs_component_iterator_current(&it, (void **)&handle);
        bank = handle.cecs_texture_bank_component;
        assert(!cecs_texture_bank_is_full(bank) && "fatal error: texture bank is full, tag mismatch");

        const uint_fast8_t slot_index = cecs_texture_bank_first_free_slot_index(bank, required_slots_count);

        extern inline uint_fast8_t cecs_texture_bank_slot_count(const cecs_texture_bank *bank);
        if (slot_index < cecs_texture_bank_slot_count(handle.cecs_texture_bank_component)) {
            *out_bank_entity_id = entity;
            *out_first_slot_index = slot_index;
            break;
        }
    }
    cecs_component_iterator_end_iter(&it);
    return bank;
}
cecs_texture_bank *cecs_texture_bank_allocate(
    cecs_graphics_world *world,
    WGPUDevice device,
    const cecs_texture_bank_id_descriptor bank_descriptor,
    const uint32_t sample_count,
    cecs_entity_id *out_bank_entity_id
) {
    cecs_texture_bank bank = cecs_texture_bank_create(device, bank_descriptor, sample_count);

    *out_bank_entity_id = cecs_world_add_entity(&world->world);
    return cecs_world_set_component_relation(
        &world->world,
        *out_bank_entity_id,
        CECS_COMPONENT_ID(cecs_texture_bank),
        &bank,
        sizeof(cecs_texture_bank),
        cecs_component_id_from_texture_resource_id_descriptor(bank_descriptor)
    );
}

cecs_texture_bank_status cecs_texture_bank_use(
    cecs_texture_bank *bank,
    const uint_fast8_t first_slot_index,
    const uint_fast8_t slot_count
) {
    const cecs_texture_bank_slot_mask mask = cecs_texture_bank_slot_mask_from_range(first_slot_index, slot_count);
    bank->used_slots_mask |= mask;
    return cecs_texture_bank_is_full(bank) ? cecs_texture_bank_status_full : cecs_texture_bank_status_free;
}
cecs_texture_bank_status cecs_texture_bank_release(
    cecs_texture_bank *bank,
    const uint_fast8_t first_slot_index,
    const uint_fast8_t slot_count
) {
    const cecs_texture_bank_slot_mask mask = cecs_texture_bank_slot_mask_from_range(first_slot_index, slot_count);
    bank->used_slots_mask &= ~mask;
    return cecs_texture_bank_is_empty(bank) ? cecs_texture_bank_status_free : cecs_texture_bank_status_free;
}
cecs_texture_bank *cecs_texture_bank_use_and_relocate(
    cecs_graphics_world *world,
    cecs_texture_bank *bank,
    cecs_texture_bank_id_descriptor bank_descriptor,
    const cecs_entity_id bank_entity_id,
    const uint_fast8_t first_slot_index,
    const uint_fast8_t slot_count
) {
    cecs_texture_bank *new_bank = bank;
    if (cecs_texture_bank_use(bank, first_slot_index, slot_count) == cecs_texture_bank_status_full) {
        cecs_world_remove_component_relation(
            &world->world,
            bank_entity_id,
            CECS_COMPONENT_ID(cecs_texture_bank),
            bank,
            cecs_component_id_from_texture_resource_id_descriptor(bank_descriptor)
        );

        bank_descriptor.flags.slots_full = cecs_texture_bank_status_full;
        new_bank = cecs_world_set_component_relation(
            &world->world,
            bank_entity_id,
            CECS_COMPONENT_ID(cecs_texture_bank),
            bank,
            sizeof(cecs_texture_bank),
            cecs_component_id_from_texture_resource_id_descriptor(bank_descriptor)
        );
        *bank = (cecs_texture_bank){0};
    }
    return new_bank;
}
cecs_texture_bank *cecs_texture_bank_release_and_relocate(
    cecs_graphics_world *world,
    cecs_texture_bank *bank,
    cecs_texture_bank_id_descriptor bank_descriptor,
    const cecs_entity_id bank_entity_id,
    const uint_fast8_t first_slot_index,
    const uint_fast8_t slot_count
) {
    cecs_texture_bank *new_bank = bank;
    if (cecs_texture_bank_release(bank, first_slot_index, slot_count) == cecs_texture_bank_status_free) {
        cecs_world_remove_component_relation(
            &world->world,
            bank_entity_id,
            CECS_COMPONENT_ID(cecs_texture_bank),
            bank,
            cecs_component_id_from_texture_resource_id_descriptor(bank_descriptor)
        );

        bank_descriptor.flags.slots_full = cecs_texture_bank_status_free;
        new_bank = cecs_world_set_component_relation(
            &world->world,
            bank_entity_id,
            CECS_COMPONENT_ID(cecs_texture_bank),
            bank,
            sizeof(cecs_texture_bank),
            cecs_component_id_from_texture_resource_id_descriptor(bank_descriptor)
        );
        *bank = (cecs_texture_bank){0};
    }
    return new_bank;
}

cecs_texture_in_bank_bundle cecs_texture_builder_build_in_bank(
    cecs_texture_builder *builder,
    cecs_graphics_context *context,
    const WGPUTextureViewDescriptor *view_descriptor
) {
    assert(builder->used_texture_slots > 0 && "error: must have at least one texture slot used");
    
    uint32_t largest_side_size;
    const uint_fast8_t mip_count = cecs_texture_builder_mip_count(builder->texture_descriptor.size, &largest_side_size);
    
    static_assert(sizeof(cecs_texture_size_pow2) == sizeof(uint32_t), "static error: expected sizeof cecs_texture_size_pow2 to be 4 bytes");
    
    size_t mipmaps_size;
    builder->texture_descriptor.mipLevelCount = mip_count;
    const cecs_texture_size_pow2 size_pow2 = cecs_texture_builder_configure_mipmaps(builder, mip_count, 0, &mipmaps_size);
    for (uint_fast8_t i = 1; i < builder->used_texture_slots; i++) {
        size_t mip_size;
        const cecs_texture_size_pow2 size = cecs_texture_builder_configure_mipmaps(builder, mip_count, i, &mip_size);
        
        assert(size == size_pow2 && "error: texture pow2 size must match for all textures");
        assert(mip_size == mipmaps_size && "error: mipmaps size must match for all textures");
    }

    const cecs_texture_bank_id_descriptor bank_descriptor = cecs_texture_bank_id_descriptor_create_free(
        size_pow2,
        mip_count,
        builder->texture_descriptor.format,
        builder->texture_descriptor.usage
    );
    cecs_entity_id bank_entity_id;
    uint_fast8_t slot_index;
    cecs_texture_bank *bank = cecs_texture_bank_find_allocated_or_null(
        builder->world,
        builder->texture_arena,
        bank_descriptor,
        builder->used_texture_slots,
        &bank_entity_id,
        &slot_index
    );
    if (bank == NULL) {
        bank = cecs_texture_bank_allocate(
            builder->world,
            context->device,
            bank_descriptor,
            builder->texture_descriptor.sampleCount,
            &bank_entity_id
        );
        slot_index = 0;
    }
    bank = cecs_texture_bank_use_and_relocate(
        builder->world,
        bank,
        bank_descriptor,
        bank_entity_id,
        slot_index,
        builder->used_texture_slots
    );

    assert(builder->texture_descriptor.usage & WGPUTextureUsage_CopyDst && "error: texture must be copyable");
    cecs_stbi_allocator_get_current_allocator()->current_arena = builder->texture_arena;
    for (uint_fast8_t i = 0; i < builder->used_texture_slots; i++) {
        uint8_t *const texture_data = builder->texture_data[i];
        size_t write_size = cecs_write_mipmaps(
            context->queue,
            bank->texture,
            &builder->texture_descriptor, 
            texture_data,
            builder->descriptor.bytes_per_texel,
            view_descriptor->aspect,
            slot_index + i
        );
        assert(write_size == mipmaps_size && "error: writen size does not match mipmaps size");
        stbi_image_free(texture_data);
    }
    cecs_stbi_allocator_get_current_allocator()->current_arena = NULL;
    
    float bank_width = (float)wgpuTextureGetWidth(bank->texture);
    float bank_height = (float)wgpuTextureGetHeight(bank->texture);
    return (cecs_texture_in_bank_bundle){
        .reference = (cecs_texture_in_bank_reference){bank_entity_id},
        .range = (cecs_texture_in_bank_range){
            .slot_index = slot_index,
            .slot_range = builder->used_texture_slots,
        },
        .subrect = (cecs_texture_subrect2_f32){
            .normalized_width = builder->texture_descriptor.size.width / bank_width,
            .normalized_height = builder->texture_descriptor.size.height / bank_height,
        },
    };
}
