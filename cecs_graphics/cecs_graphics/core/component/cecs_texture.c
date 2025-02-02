#include <cecs_math/cecs_math.h>
#include "cecs_texture.h"

CECS_COMPONENT_DEFINE(cecs_texture_reference);

CECS_COMPONENT_DEFINE(cecs_texture);

CECS_COMPONENT_DEFINE(cecs_texture_bank);

cecs_component_id cecs_component_id_from_texture_resource_id_descriptor(cecs_texture_bank_id_descriptor descriptor) {
    assert(
        descriptor.flags.size != cecs_texture_bank_size_none
        && "error: invalid cecs_texture_resource_id_descriptor"
    );

    return (cecs_texture_bank_id) {
        .descriptor = descriptor
    }.id;
}


extern inline uint_fast8_t cecs_texture_bank_slot_count(const cecs_texture_bank *bank);

static cecs_texture_bank_slot_mask cecs_texture_bank_max_slot_mask(const cecs_texture_bank *bank) {
    return cecs_texture_bank_slot_count(bank) - 1;
}

bool cecs_texture_bank_is_empty(const cecs_texture_bank *bank) {
    return bank->used_slots_mask == 0;
}

cecs_texture_bank_slot_mask cecs_texture_bank_get_free_slot_range_mask(const cecs_texture_bank *bank, const uint_fast8_t count, uint_fast8_t *out_slot_index) {
    uint64_t mask = bank->used_slots_mask;
    uint64_t range_mask = (1 << count) - 1;
    uint_fast8_t slot_index = 0;

    while ((mask & range_mask)) {
        range_mask <<= count;
        slot_index += count;
    }

    *out_slot_index = slot_index;
    return range_mask;
}

bool cecs_texture_bank_has_free_slots(const cecs_texture_bank *bank) {
    return bank->used_slots_mask < cecs_texture_bank_max_slot_mask(bank);
}

bool cecs_texture_bank_is_full(const cecs_texture_bank *bank) {
    return !cecs_texture_bank_has_free_slots(bank);
}


CECS_COMPONENT_DEFINE(cecs_texture_in_bank_reference);

CECS_COMPONENT_DEFINE(cecs_texture_in_bank_range2_u8_attribute);
WGPUVertexBufferLayout cecs_texture_in_bank_range2_u8_attribute_layout(
    const uint32_t shader_location,
    const WGPUVertexStepMode step_mode,
    WGPUVertexAttribute out_attributes[const],
    const size_t out_attributes_capacity
) {
    assert(out_attributes_capacity >= 1 && "error: out attributes capacity must be at least 1");
    out_attributes[0] = (WGPUVertexAttribute) {
        .format = WGPUVertexFormat_Uint8x2,
        .offset = 0,
        .shaderLocation = shader_location,
    };

    return (WGPUVertexBufferLayout) {
        .arrayStride = sizeof(cecs_texture_in_bank_range2_u8_attribute),
        .stepMode = step_mode,
        .attributeCount = 1,
        .attributes = out_attributes,
    };
}

CECS_COMPONENT_DEFINE(cecs_texture_subrect2_f32_attribute);
WGPUVertexBufferLayout cecs_texture_subrect2_f32_attribute_layout(
    const uint32_t shader_location,
    const WGPUVertexStepMode step_mode,
    WGPUVertexAttribute out_attributes[const],
    const size_t out_attributes_capacity
) {
    assert(out_attributes_capacity >= 1 && "error: out attributes capacity must be at least 1");
    out_attributes[0] = (WGPUVertexAttribute) {
        .format = WGPUVertexFormat_Float32x2,
        .offset = 0,
        .shaderLocation = shader_location,
    };

    return (WGPUVertexBufferLayout) {
        .arrayStride = sizeof(cecs_texture_subrect2_f32),
        .stepMode = step_mode,
        .attributeCount = 1,
        .attributes = out_attributes,
    };
}
