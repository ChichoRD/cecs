#ifndef CECS_TEXTURE_H
#define CECS_TEXTURE_H

#include <cecs_core/cecs_core.h>
#include <webgpu/webgpu.h>
#include "../../containers/cecs_dynamic_wgpu_buffer.h"

typedef struct cecs_texture_reference {
    cecs_entity_id texture_id;
} cecs_texture_reference;
CECS_COMPONENT_DECLARE(cecs_texture_reference);

typedef struct cecs_texture {
    WGPUTextureView texture_view;
    WGPUExtent3D extent;
} cecs_texture;
CECS_COMPONENT_DECLARE(cecs_texture);


typedef enum cecs_texture_bank_status {
    cecs_texture_bank_status_free,
    cecs_texture_bank_status_full,
} cecs_texture_bank_status;
typedef uint8_t cecs_texture_bank_status_flag;

typedef enum cecs_texture_size_pow2 {
    cecs_texture_bank_size_none = 0,
    cecs_texture_bank_size_2x2,
    cecs_texture_bank_size_4x4,
    cecs_texture_bank_size_8x8,
    cecs_texture_bank_size_16x16,
    cecs_texture_bank_size_32x32,
    cecs_texture_bank_size_64x64,
    cecs_texture_bank_size_128x128,
    cecs_texture_bank_size_256x256,
    cecs_texture_bank_size_512x512,
    cecs_texture_bank_size_1024x1024,
    cecs_texture_bank_size_2048x2048,
    cecs_texture_bank_size_4096x4096,
    cecs_texture_bank_size_8192x8192,
    cecs_texture_bank_size_16384x16384,
    cecs_texture_bank_size_32768x32768,
} cecs_texture_size_pow2;
typedef uint8_t cecs_texture_bank_size_flag;

typedef uint8_t cecs_texture_usage_flags_packed; 
typedef struct cecs_texture_bank_flags {
    cecs_texture_bank_status_flag slots_full : 1;
    cecs_texture_bank_size_flag size : 7;
    uint8_t : 0;

    uint8_t mip_level_count;
    cecs_texture_usage_flags_packed usage;
    uint8_t unused[1];
} cecs_texture_bank_flags;
static_assert(sizeof(cecs_texture_bank_flags) == sizeof(uint32_t), "cecs_texture_resource_flags must be 4 bytes");

typedef struct cecs_texture_bank_id_descriptor {
    cecs_texture_bank_flags flags;
    WGPUTextureFormat format;
} cecs_texture_bank_id_descriptor;
static_assert(
    sizeof(cecs_texture_bank_id_descriptor) == sizeof(cecs_component_id),
    "static error: cecs_texture_resource_id_descriptor size must be equal to cecs_component_id size"
);

typedef union cecs_texture_bank_id {
    cecs_texture_bank_id_descriptor descriptor;
    cecs_component_id id;
} cecs_texture_bank_id;

cecs_component_id cecs_component_id_from_texture_resource_id_descriptor(cecs_texture_bank_id_descriptor descriptor);

typedef uint64_t cecs_texture_bank_slot_mask;
typedef struct cecs_texture_bank {
    WGPUTexture texture;
    WGPUTextureView texture_view;
    cecs_texture_bank_slot_mask used_slots_mask;
} cecs_texture_bank;
CECS_COMPONENT_DECLARE(cecs_texture_bank);

inline uint_fast8_t cecs_texture_bank_slot_count(const cecs_texture_bank *bank) {
    return (uint_fast8_t)wgpuTextureGetDepthOrArrayLayers(bank->texture);
}

bool cecs_texture_bank_has_free_slots(const cecs_texture_bank *bank);
bool cecs_texture_bank_is_full(const cecs_texture_bank *bank);
bool cecs_texture_bank_is_empty(const cecs_texture_bank *bank);

cecs_texture_bank_slot_mask cecs_texture_bank_get_free_slot_range_mask(const cecs_texture_bank *bank, const uint_fast8_t count, uint_fast8_t *out_slot_index);


typedef cecs_texture_reference cecs_texture_in_bank_reference;
CECS_COMPONENT_DECLARE(cecs_texture_in_bank_reference);

typedef struct cecs_texture_in_bank_range {
    uint8_t slot_index;
    uint8_t slot_range;
    uint8_t padding[2];
} cecs_texture_in_bank_range;
typedef cecs_texture_in_bank_range cecs_texture_in_bank_range2_u8_attribute;
CECS_COMPONENT_DECLARE(cecs_texture_in_bank_range2_u8_attribute);
CECS_VERTEX_ATTRIBUTE_IS_ALIGNED_STATIC_ASSERT(cecs_texture_in_bank_range2_u8_attribute);

WGPUVertexBufferLayout cecs_texture_in_bank_range2_u8_attribute_layout(
    const uint32_t shader_location,
    const WGPUVertexStepMode step_mode,
    WGPUVertexAttribute out_attributes[const],
    const size_t out_attributes_capacity
);

typedef struct cecs_texture_subrect2_f32 {
    float normalized_width;
    float normalized_height;
} cecs_texture_subrect2_f32;
typedef cecs_texture_subrect2_f32 cecs_texture_subrect2_f32_attribute;
CECS_COMPONENT_DECLARE(cecs_texture_subrect2_f32_attribute);
CECS_VERTEX_ATTRIBUTE_IS_ALIGNED_STATIC_ASSERT(cecs_texture_subrect2_f32_attribute);

WGPUVertexBufferLayout cecs_texture_subrect2_f32_attribute_layout(
    const uint32_t shader_location,
    const WGPUVertexStepMode step_mode,
    WGPUVertexAttribute out_attributes[const],
    const size_t out_attributes_capacity
);

#endif