#ifndef CECS_TEXTURE_BUILDER_H
#define CECS_TEXTURE_BUILDER_H

#include <stdint.h>
#include <memory.h>
#include "cecs_graphics_world.h"
#include "component/cecs_texture.h"


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

typedef uint8_t cecs_texture_builder_texture_count;
#define CECS_TEXTURE_BUILDER_MAX_TEXTURE_COUNT_DEFAULT 8

#ifndef CECS_TEXTURE_BUILDER_MAX_TEXTURE_COUNT
#define CECS_TEXTURE_BUILDER_MAX_TEXTURE_COUNT CECS_TEXTURE_BUILDER_MAX_TEXTURE_COUNT_DEFAULT
#endif
static_assert(
    CECS_TEXTURE_BUILDER_MAX_TEXTURE_COUNT <= CHAR_BIT * sizeof(cecs_texture_builder_texture_count),
    "CECS_TEXTURE_BUILDER_MAX_TEXTURE_COUNT must be less than or equal to CHAR_BIT * sizeof(cecs_texture_builder_texture_count)"
);

typedef struct cecs_texture_builder {
    static_assert(false);
    cecs_texture_builder_base base;
    cecs_texture_builder_descriptor descriptor;
    uint8_t *texture_data[CECS_TEXTURE_BUILDER_MAX_TEXTURE_COUNT];
    cecs_texture_builder_texture_count used_texture_slots;
} cecs_texture_builder;

cecs_texture_builder cecs_texture_builder_create(
    cecs_graphics_world *world,
    cecs_arena *texture_arena,
    const cecs_texture_builder_descriptor descriptor,
    const cecs_texture_builder_wgpu_descriptor wgpu_descriptor
);
inline bool cecs_texture_builder_is_empty(const cecs_texture_builder *builder) {
    return builder->used_texture_slots == 0;
}

cecs_texture_builder *cecs_texture_builder_load_into(
    cecs_texture_builder *builder,
    const char *path,
    const uint_fast8_t texture_slot
);
cecs_texture_builder *cecs_texture_builder_take_into(
    cecs_texture_builder *builder,
    uint8_t *texture_data,
    const uint_fast8_t texture_slot
);

cecs_texture_builder *cecs_texture_builder_clear_and_set(
    cecs_texture_builder *builder,
    const cecs_texture_builder_descriptor descriptor,
    const cecs_texture_builder_wgpu_descriptor wgpu_descriptor
);
cecs_texture_builder *cecs_texture_builder_clear(
    cecs_texture_builder *builder
);

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
    WGPUTexture destination,
    WGPUQueue queue,
    const WGPUTextureDescriptor *descriptor,
    const WGPUTextureAspect aspect,
    const cecs_mipmaps_write_descriptor mipmaps
);

WGPUTexture cecs_texture_builder_build_alloc(
    cecs_texture_builder *builder,
    cecs_graphics_context *context,
    const WGPUTextureViewDescriptor *view_descriptor,
    const uint32_t write_destination_layer,
    const uint_fast8_t texture_slot
);
inline uint_fast8_t cecs_texture_builder_build_alloc_range(
    cecs_texture_builder *builder,
    cecs_graphics_context *context,
    const WGPUTextureViewDescriptor *view_descriptor,
    const uint32_t write_destination_layer,
    const cecs_exclusive_range slot_range,
    WGPUTexture out_textures[const restrict static 1]
) {
    assert(slot_range.end <= builder->used_texture_slots && "error: slot range must be within used texture slots");
    assert(!cecs_exclusive_range_is_empty(slot_range) && "error: slot range must not be empty");
    assert(slot_range.start >= 0 && "error: slot range must be a strictly positive range");
    const uint_fast8_t count = cecs_exclusive_range_length(slot_range);
    
    for (uint_fast8_t i = 0; i < count; i++) {
        out_textures[i] = cecs_texture_builder_build_alloc(
            builder,
            context,
            view_descriptor,
            write_destination_layer,
            slot_range.start + i
        );
    }
    return count;
}

cecs_texture cecs_texture_builder_build(
    cecs_texture_builder *builder,
    cecs_graphics_context *context,
    const WGPUTextureViewDescriptor *view_descriptor,
    const uint32_t write_destination_layer,
    const uint_fast8_t texture_slot
);
inline uint_fast8_t cecs_texture_builder_build_range(
    cecs_texture_builder *builder,
    cecs_graphics_context *context,
    const WGPUTextureViewDescriptor *view_descriptor,
    const uint32_t write_destination_layer,
    const cecs_exclusive_range slot_range,
    cecs_texture out_textures[const restrict static 1]
) {
    assert(slot_range.end <= builder->used_texture_slots && "error: slot range must be within used texture slots");
    assert(!cecs_exclusive_range_is_empty(slot_range) && "error: slot range must not be empty");
    assert(slot_range.start >= 0 && "error: slot range must be a strictly positive range");
    const uint_fast8_t count = cecs_exclusive_range_length(slot_range);
    
    for (uint_fast8_t i = 0; i < count; i++) {
        out_textures[i] = cecs_texture_builder_build(
            builder,
            context,
            view_descriptor,
            write_destination_layer,
            slot_range.start + i
        );
    }
    return count;
}

cecs_texture_bank *cecs_texture_bank_find_allocated_or_null(
    cecs_graphics_world *world,
    cecs_arena *iteration_arena,
    const cecs_texture_bank_id_descriptor bank_descriptor,
    const uint_fast8_t required_slots_count,
    cecs_entity_id *out_bank_entity_id,
    uint_fast8_t *out_first_slot_index
);
cecs_texture_bank *cecs_texture_bank_allocate(
    cecs_graphics_world *world,
    WGPUDevice device,
    const cecs_texture_bank_id_descriptor bank_descriptor,
    const uint32_t sample_count,
    cecs_entity_id *out_bank_entity_id
);


cecs_texture_bank_status cecs_texture_bank_use(
    cecs_texture_bank *bank,
    const uint_fast8_t first_slot_index,
    const uint_fast8_t slot_count
);
cecs_texture_bank_status cecs_texture_bank_release(
    cecs_texture_bank *bank,
    const uint_fast8_t first_slot_index,
    const uint_fast8_t slot_count
);
cecs_texture_bank *cecs_texture_bank_use_and_relocate(
    cecs_graphics_world *world,
    cecs_texture_bank *bank,
    cecs_texture_bank_id_descriptor bank_descriptor,
    const cecs_entity_id bank_entity_id,
    const uint_fast8_t first_slot_index,
    const uint_fast8_t slot_count
);
cecs_texture_bank *cecs_texture_bank_release_and_relocate(
    cecs_graphics_world *world,
    cecs_texture_bank *bank,
    cecs_texture_bank_id_descriptor bank_descriptor,
    const cecs_entity_id bank_entity_id,
    const uint_fast8_t first_slot_index,
    const uint_fast8_t slot_count
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