#include <assert.h>
#include <stdlib.h>

#include <cecs_math/cecs_math.h>

#include "cecs_texture_builder.h"

typedef struct cecs_stbi_allocation_header {
#if SIZE_MAX == UINT16_MAX
    uint16_t size;
    uint16_t padding[3];

#elif SIZE_MAX == UINT32_MAX
    uint32_t size;
    uint32_t padding[1];

#elif SIZE_MAX == UINT64_MAX
    uint64_t size;

#else
    #error TBD code SIZE_T_BITS

#endif
} cecs_stbi_allocation_header;

typedef struct cecs_stbi_allocator {
    cecs_arena *current_arena;
} cecs_stbi_allocator;
static cecs_stbi_allocator texture_builder_stbi_allocator = { .current_arena = NULL };

static void *cecs_stbi_malloc(size_t size) {
    assert(texture_builder_stbi_allocator.current_arena != NULL && "error: stbi allocator must be set");
    const uint64_t total_size = size + sizeof(cecs_stbi_allocation_header);

    cecs_stbi_allocation_header *header = cecs_arena_alloc(texture_builder_stbi_allocator.current_arena, total_size); 
    header->size = total_size;
    return header + 1;
}

static void *cecs_stbi_realloc(void *ptr, size_t size) {
    assert(texture_builder_stbi_allocator.current_arena != NULL && "error: stbi allocator must be set");
    if (ptr == NULL) {
        return cecs_stbi_malloc(size);
    } else {
        cecs_stbi_allocation_header *header = (cecs_stbi_allocation_header *)ptr - 1;
        const uint64_t new_total_size = size + sizeof(cecs_stbi_allocation_header);

        cecs_stbi_allocation_header *new_header =
            cecs_arena_realloc(texture_builder_stbi_allocator.current_arena, header, header->size, new_total_size);
        new_header->size = new_total_size;
        return new_header + 1;
    }
}

static void cecs_stbi_free(void *ptr) {
    (void)ptr;
}

#define STBI_MALLOC(size) cecs_stbi_malloc(size)
#define STBI_REALLOC(ptr, size) cecs_stbi_realloc(ptr, size)
#define STBI_FREE(ptr) cecs_stbi_free(ptr)

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

cecs_texture_builder cecs_texture_builder_create(
    cecs_graphics_world *world,
    cecs_arena *texture_arena,
    cecs_texture_builder_descriptor descriptor
) {
    assert(descriptor.bytes_per_texel > 0 && "error: bytes per pixel must be greater than 0");
    assert(descriptor.channel_count > 0 && "error: channel count must be greater than 0");

    // TODO: handle other descriptor options
    return (cecs_texture_builder){
        .world = world,
        .texture_arena = texture_arena,
        .texture_descriptor = {0},
        .descriptor = descriptor,
        .texture_data = NULL,
    };
}

cecs_texture_builder *cecs_texture_builder_load_from(
    cecs_texture_builder *builder,
    const char *path,
    const WGPUTextureDimension dimension,
    const WGPUTextureFormat format,
    const WGPUTextureUsage usage
) {
    assert(builder->texture_data == NULL && "error: texture data must be NULL");
    assert(
        !(builder->descriptor.flags & cecs_texture_builder_descriptor_config_generate_empty)
        && "error: flags were set to generate empty texture, cannot load it from file"
    );

    int width;
    int height;
    int channels;
    texture_builder_stbi_allocator.current_arena = builder->texture_arena;
    uint8_t *texture_data = stbi_load(path, &width, &height, &channels, builder->descriptor.channel_count);
    texture_builder_stbi_allocator.current_arena = NULL;
    assert(channels == builder->descriptor.channel_count && "error unexpected: channel count mismatch");

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
    const WGPUTextureDescriptor texture_descriptor = {
        .nextInChain = NULL,
        .dimension = dimension,
        .format = format,
        .mipLevelCount = 1,
        .sampleCount = 1,
        .size = size,
        .usage = usage,
        .viewFormatCount = 0,
        .viewFormats = NULL,
    };

    if (builder->descriptor.flags & cecs_texture_builder_descriptor_config_generate_mipmaps) {
        builder->descriptor.flags |= cecs_texture_builder_descriptor_config_alloc_mipmaps;
    }

    //static_assert(false, "TODO: handle loading multiple (clearing builder after build); set to NULL prior");
    return cecs_texture_builder_set_data(builder, texture_data, texture_descriptor);
}

cecs_texture_builder *cecs_texture_builder_set_data(cecs_texture_builder *builder, uint8_t *texture_data, const WGPUTextureDescriptor texture_descriptor) {
    assert(
        !(builder->descriptor.flags & cecs_texture_builder_descriptor_config_generate_empty)
        && "error: flags were set to generate empty texture, cannot set data"
    );
    
    builder->texture_data = texture_data;
    builder->texture_descriptor = texture_descriptor;
    return builder;
}

cecs_texture_builder *cecs_texture_builder_set_descriptor(cecs_texture_builder *builder, const WGPUTextureDescriptor texture_descriptor) {
    builder->texture_descriptor = texture_descriptor;
    return builder;
}

extern inline uint32_t cecs_max_u32(uint32_t a, uint32_t b);
extern inline uint32_t cecs_min_u32(uint32_t a, uint32_t b);

WGPUExtent3D cecs_generate_next_mip(
    const WGPUExtent3D mip_size,
    const uint8_t *restrict mip_texels,
    const uint_fast8_t bytes_per_texel,
    uint8_t out_next_mip_texels[const restrict]
) {
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
    WGPUQueue queue,
    WGPUTexture texture,
    const WGPUTextureDescriptor *descriptor,
    const uint8_t *texture_data,
    const uint_fast8_t bytes_per_texel,
    const WGPUTextureAspect aspect,
    const uint32_t destination_layer
) {
    assert(texture_data != NULL && "error: texture data must be set");

    const uint8_t *source_data = texture_data;
    for (uint32_t i = 0; i < descriptor->mipLevelCount; i++) {
        const uint32_t mip_width = cecs_max_u32(descriptor->size.width >> i, 1);
        const uint32_t mip_height = cecs_max_u32(descriptor->size.height >> i, 1);
        const uint32_t mip_bytes_per_row = mip_width * bytes_per_texel;
        const size_t mip_texture_size = mip_bytes_per_row * mip_height;

        const WGPUImageCopyTexture destination = (WGPUImageCopyTexture){
            .nextInChain = NULL,
            .texture = texture,
            .mipLevel = i,
            .origin = (WGPUOrigin3D){0, 0, destination_layer},
            .aspect = aspect,
        };
        const WGPUTextureDataLayout source = (WGPUTextureDataLayout){
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
        wgpuQueueWriteTexture(queue, &destination, source_data, mip_texture_size, &source, &mip_size);
        source_data += mip_texture_size;
    }
    return source_data - texture_data;
}

static inline uint_fast8_t cecs_texture_builder_mip_count(const WGPUExtent3D size, uint32_t *out_largest_side_size) {
    extern inline uint_fast8_t cecs_log2_u32(uint32_t n);
    *out_largest_side_size = cecs_max_u32(size.width, size.height);
    return cecs_log2_u32(*out_largest_side_size) + 1;
}

static inline cecs_texture_size_pow2 cecs_texture_builder_size_from_mip_count(const uint_fast8_t mip_count) {
    return (cecs_texture_size_pow2)(mip_count);
}

extern inline bool cecs_is_pow2_u32(uint32_t n);
static cecs_texture_size_pow2 cecs_texture_builder_configure_mipmaps(cecs_texture_builder *builder, const uint_fast8_t mip_count, size_t *out_mipmaps_size) {
    assert(builder->texture_data != NULL && "error: texture data must be set");
    const uint32_t min_side = cecs_min_u32(builder->texture_descriptor.size.width, builder->texture_descriptor.size.height);

    // 17px -> 5 size, 5 mip, 5 log
    // 16px -> 4 size, 5 mip, 4 log
    // 15px -> 4 size, 4 mip, 4 log
    
    const size_t texture_texels =
        builder->texture_descriptor.size.width * builder->texture_descriptor.size.height;
    const size_t texture_size =
        texture_texels * builder->descriptor.bytes_per_texel;
    if (
        (builder->descriptor.flags & cecs_texture_builder_descriptor_config_generate_mipmaps)
        && builder->texture_descriptor.mipLevelCount == 1
    ) {
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
                cecs_arena_realloc(builder->texture_arena, builder->texture_data, texture_size, mip_buffer_size);
        } else {
            mip_texels = builder->texture_data;
        }

        const size_t mip_chain_size = cecs_generate_mipmaps(
            builder->texture_descriptor.size,
            mip_texels,
            builder->descriptor.bytes_per_texel,
            mip_count,
            mip_texels + texture_size
        );

        assert(mip_chain_size <= mip_buffer_size && "fatal error: not allocated enough memory for mip chain");
        builder->texture_descriptor.mipLevelCount = mip_count;
        builder->texture_data = cecs_arena_realloc(
            builder->texture_arena,
            mip_texels,
            mip_buffer_size,
            mip_chain_size
        );
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
    const uint32_t write_destination_layer
) {
    assert(
        (builder->texture_data == NULL) == !!(builder->descriptor.flags & cecs_texture_builder_descriptor_config_generate_empty)
        && "error: texture data must be NULL iff flags are set to generate empty texture"
    );
    if (builder->texture_data == NULL) {
        return wgpuDeviceCreateTexture(context->device, &builder->texture_descriptor);
    } else {
        uint32_t largest_side_size;
        size_t mipmaps_size;
        cecs_texture_builder_configure_mipmaps(
            builder,
            cecs_texture_builder_mip_count(builder->texture_descriptor.size, &largest_side_size),
            &mipmaps_size
        );
        WGPUTexture texture = wgpuDeviceCreateTexture(context->device, &builder->texture_descriptor);

        assert(builder->texture_descriptor.usage & WGPUTextureUsage_CopyDst && "error: texture must be copyable");
        size_t write_size = cecs_write_mipmaps(
            context->queue,
            texture,
            &builder->texture_descriptor, 
            builder->texture_data,
            builder->descriptor.bytes_per_texel,
            view_descriptor->aspect,
            write_destination_layer
        );
        assert(write_size == mipmaps_size && "error: writen size does not match mipmaps size");

        texture_builder_stbi_allocator.current_arena = builder->texture_arena;
        stbi_image_free(builder->texture_data);
        texture_builder_stbi_allocator.current_arena = NULL;

        return texture;
    }
}

cecs_texture cecs_texture_builder_build(
    cecs_texture_builder *builder,
    cecs_graphics_context *context,
    const WGPUTextureViewDescriptor *view_descriptor,
    const uint32_t write_destination_layer
) {
    WGPUTexture texture = cecs_texture_builder_build_alloc(builder, context, view_descriptor, write_destination_layer);
    WGPUTextureView texture_view = wgpuTextureCreateView(texture, view_descriptor);
    wgpuTextureRelease(texture);
    return (cecs_texture){
        .texture_view = texture_view,
        .extent = builder->texture_descriptor.size
    };
}

static cecs_texture_bank *cecs_texture_builder_get_or_allocate_bank(
    cecs_texture_builder *builder,
    cecs_graphics_context *context,
    const cecs_texture_bank_id_descriptor texture_bank_id_descriptor,
    const uint_fast8_t required_slots_count,
    cecs_entity_id *out_bank_entity_id,
    uint_fast8_t *out_slot_index,
    uint_fast8_t *out_slot_mask
) {
    const cecs_component_id texture_bank_id = cecs_component_id_from_texture_resource_id_descriptor(texture_bank_id_descriptor);
    CECS_COMPONENT_ITERATION_HANDLE_STRUCT(cecs_texture_bank) handle;
    // FIXME: correct typo in `GROUPPED`
    cecs_component_iterator it = CECS_COMPONENT_ITERATOR_CREATE_GROUPPED(&builder->world->world.components, builder->texture_arena, 
        CECS_COMPONENT_GROUP_FROM_IDS(
            cecs_component_access_inmmutable, cecs_component_group_search_all, CECS_RELATION_ID(cecs_texture_bank, texture_bank_id)
        )
    );
    for (
        cecs_component_iterator_begin_iter(&it, builder->texture_arena);
        !cecs_component_iterator_done(&it);
        cecs_component_iterator_next(&it)
    ) {
        const cecs_entity_id entity = cecs_component_iterator_current(&it, (void **)&handle);
        assert(!cecs_texture_bank_is_full(handle.cecs_texture_bank_component) && "fatal error: texture bank is full, tag mismatch");

        uint_fast8_t slot_index;
        cecs_texture_bank_slot_mask slot_mask = cecs_texture_bank_get_free_slot_range_mask(
            handle.cecs_texture_bank_component,
            required_slots_count,
            &slot_index
        );

        extern inline uint_fast8_t cecs_texture_bank_slot_count(const cecs_texture_bank *bank);
        if (slot_index < cecs_texture_bank_slot_count(handle.cecs_texture_bank_component)) {
            *out_bank_entity_id = entity;
            *out_slot_index = slot_index;
            *out_slot_mask = slot_mask;
            return handle.cecs_texture_bank_component;
        }
    }
    cecs_component_iterator_end_iter(&it);


    static const uint32_t cecs_texture_bank_default_array_layers = 64;
    cecs_texture_builder bank_builder = cecs_texture_builder_create(
        builder->world,
        builder->texture_arena,
        (cecs_texture_builder_descriptor){
            .bytes_per_texel = builder->descriptor.bytes_per_texel,
            .channel_count = builder->descriptor.channel_count,
            .flags = cecs_texture_builder_descriptor_config_generate_empty,
        }
    );

    const uint32_t texture_size = 1 << texture_bank_id_descriptor.flags.size;
    cecs_texture_builder_set_descriptor_no_data(&bank_builder, (WGPUTextureDescriptor){
        .dimension = WGPUTextureDimension_2D,
        .format = texture_bank_id_descriptor.format,
        .usage = (uint32_t)texture_bank_id_descriptor.flags.usage,
        .size = (WGPUExtent3D){
            .width = texture_size,
            .height = texture_size,
            .depthOrArrayLayers = cecs_texture_bank_default_array_layers,
        },
        .sampleCount = builder->texture_descriptor.sampleCount,
        .mipLevelCount = (uint32_t)texture_bank_id_descriptor.flags.mip_level_count,
    });
    // TODO: cecs_texture_bank_create
    WGPUTexture bank_texture = cecs_texture_builder_build_alloc(&bank_builder, context, &(WGPUTextureViewDescriptor){
        .format = texture_bank_id_descriptor.format,
        .dimension = WGPUTextureViewDimension_2DArray,
        .baseMipLevel = 0,
        .mipLevelCount = (uint32_t)texture_bank_id_descriptor.flags.mip_level_count,
        .baseArrayLayer = 0,
        .arrayLayerCount = cecs_texture_bank_default_array_layers,
    }, 0);
    
    const cecs_entity_id bank_entity_id = cecs_world_add_entity(&builder->world->world);
    cecs_texture_bank *bank = cecs_world_set_component_relation(
        &builder->world->world,
        bank_entity_id,
        CECS_COMPONENT_ID(cecs_texture_bank),
        &(cecs_texture_bank){
            .texture = bank_texture,
            .texture_view = wgpuTextureCreateView(bank_texture, &(WGPUTextureViewDescriptor){
                .format = texture_bank_id_descriptor.format,
                .dimension = WGPUTextureViewDimension_2DArray,
                .baseMipLevel = 0,
                .mipLevelCount = builder->texture_descriptor.mipLevelCount,
                .baseArrayLayer = 0,
                .arrayLayerCount = cecs_texture_bank_default_array_layers,
            }),
            .used_slots_mask = 0,
        },
        sizeof(cecs_texture_bank),
        texture_bank_id
    );

    *out_bank_entity_id = bank_entity_id;
    *out_slot_index = 0;
    *out_slot_mask = (1 << required_slots_count) - 1;
    return bank;
}

cecs_texture_in_bank_bundle cecs_texture_builder_build_in_bank(
    cecs_texture_builder *builder,
    cecs_graphics_context *context,
    const WGPUTextureViewDescriptor *view_descriptor
) {
    assert(
        (builder->texture_data == NULL) == !!(builder->descriptor.flags & cecs_texture_builder_descriptor_config_generate_empty)
        && "error: texture data must be NULL iff flags are set to generate empty texture"
    );
    uint32_t largest_side_size;
    const uint_fast8_t mip_count = cecs_texture_builder_mip_count(builder->texture_descriptor.size, &largest_side_size);
    
    cecs_texture_size_pow2 size_pow2;
    size_t mipmaps_size = 0;
    if (builder->texture_data != NULL) {
        size_pow2 = cecs_texture_builder_configure_mipmaps(builder, mip_count, &mipmaps_size);
    } else {
        size_pow2 = cecs_texture_builder_size_from_mip_count(cecs_is_pow2_u32(largest_side_size) ? mip_count - 1 : mip_count);
    }

    const uint32_t descriptor_mip_count = cecs_min_u32(
        (uint32_t)size_pow2, (uint32_t)mip_count
    ) + 1;
    cecs_texture_bank_id_descriptor bank_descriptor = {
        .flags = {
            .slots_full = cecs_texture_bank_status_free,
            .size = size_pow2,
            .mip_level_count = descriptor_mip_count,
            .usage = builder->texture_descriptor.usage,
        },
        .format = builder->texture_descriptor.format,
    };
    
    cecs_entity_id bank_entity_id;
    uint_fast8_t slot_index;
    uint_fast8_t slot_mask;
    cecs_texture_bank *bank = cecs_texture_builder_get_or_allocate_bank(
        builder,
        context,
        bank_descriptor,
        1,
        &bank_entity_id,
        &slot_index,
        &slot_mask
    );

    bank->used_slots_mask |= slot_mask;
    if (cecs_texture_bank_is_full(bank)) {
        cecs_world_remove_component_relation(
            &builder->world->world,
            bank_entity_id,
            CECS_COMPONENT_ID(cecs_texture_bank),
            bank,
            cecs_component_id_from_texture_resource_id_descriptor(bank_descriptor)
        );

        bank_descriptor.flags.slots_full = cecs_texture_bank_status_full;
        cecs_world_set_component_relation(
            &builder->world->world,
            bank_entity_id,
            CECS_COMPONENT_ID(cecs_texture_bank),
            bank,
            sizeof(cecs_texture_bank),
            cecs_component_id_from_texture_resource_id_descriptor(bank_descriptor)
        );
    }

    if (builder->texture_data != NULL) {
        assert(builder->texture_descriptor.usage & WGPUTextureUsage_CopyDst && "error: texture must be copyable");
        size_t write_size = cecs_write_mipmaps(
            context->queue,
            bank->texture,
            &builder->texture_descriptor, 
            builder->texture_data,
            builder->descriptor.bytes_per_texel,
            view_descriptor->aspect,
            slot_index
        );
        assert(write_size == mipmaps_size && "error: writen size does not match mipmaps size");

        texture_builder_stbi_allocator.current_arena = builder->texture_arena;
        stbi_image_free(builder->texture_data);
        texture_builder_stbi_allocator.current_arena = NULL;
    }

    float bank_width = (float)wgpuTextureGetWidth(bank->texture);
    float bank_height = (float)wgpuTextureGetHeight(bank->texture);
    return (cecs_texture_in_bank_bundle){
        .reference = (cecs_texture_in_bank_reference){bank_entity_id},
        .range = (cecs_texture_in_bank_range){
            .slot_index = slot_index,
            .slot_range = 1,
        },
        .subrect = (cecs_texture_subrect2_f32){
            .normalized_width = builder->texture_descriptor.size.width / bank_width,
            .normalized_height = builder->texture_descriptor.size.height / bank_height,
        },
    };
}
