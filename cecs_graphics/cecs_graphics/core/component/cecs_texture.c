#include <cecs_math/cecs_math.h>
#include "cecs_texture.h"

CECS_COMPONENT_DEFINE(cecs_texture_reference);

CECS_COMPONENT_DEFINE(cecs_texture);

CECS_COMPONENT_DEFINE(cecs_texture_bank);

size_t cecs_texture_size_pow2_mipmaps_buffer_texels(const cecs_texture_size_pow2 size, const uint_fast8_t mip_count) {
#define CECS_TEXTURE_SIZE_POW2_MIP_MASK_1x1         ((size_t)0x1)
#define CECS_TEXTURE_SIZE_POW2_MIP_MASK_2x2         (CECS_TEXTURE_SIZE_POW2_MIP_MASK_1x1            | ((size_t)1 << ((size_t)cecs_texture_size_2x2          << (size_t)1)))
#define CECS_TEXTURE_SIZE_POW2_MIP_MASK_4x4         (CECS_TEXTURE_SIZE_POW2_MIP_MASK_2x2            | ((size_t)1 << ((size_t)cecs_texture_size_4x4          << (size_t)1)))
#define CECS_TEXTURE_SIZE_POW2_MIP_MASK_8x8         (CECS_TEXTURE_SIZE_POW2_MIP_MASK_4x4            | ((size_t)1 << ((size_t)cecs_texture_size_8x8          << (size_t)1)))
#define CECS_TEXTURE_SIZE_POW2_MIP_MASK_16x16       (CECS_TEXTURE_SIZE_POW2_MIP_MASK_8x8            | ((size_t)1 << ((size_t)cecs_texture_size_16x16        << (size_t)1)))
#define CECS_TEXTURE_SIZE_POW2_MIP_MASK_32x32       (CECS_TEXTURE_SIZE_POW2_MIP_MASK_16x16          | ((size_t)1 << ((size_t)cecs_texture_size_32x32        << (size_t)1)))
#define CECS_TEXTURE_SIZE_POW2_MIP_MASK_64x64       (CECS_TEXTURE_SIZE_POW2_MIP_MASK_32x32          | ((size_t)1 << ((size_t)cecs_texture_size_64x64        << (size_t)1)))
#define CECS_TEXTURE_SIZE_POW2_MIP_MASK_128x128     (CECS_TEXTURE_SIZE_POW2_MIP_MASK_64x64          | ((size_t)1 << ((size_t)cecs_texture_size_128x128      << (size_t)1)))
#define CECS_TEXTURE_SIZE_POW2_MIP_MASK_256x256     (CECS_TEXTURE_SIZE_POW2_MIP_MASK_128x128        | ((size_t)1 << ((size_t)cecs_texture_size_256x256      << (size_t)1)))
#define CECS_TEXTURE_SIZE_POW2_MIP_MASK_512x512     (CECS_TEXTURE_SIZE_POW2_MIP_MASK_256x256        | ((size_t)1 << ((size_t)cecs_texture_size_512x512      << (size_t)1)))
#define CECS_TEXTURE_SIZE_POW2_MIP_MASK_1024x1024   (CECS_TEXTURE_SIZE_POW2_MIP_MASK_512x512        | ((size_t)1 << ((size_t)cecs_texture_size_1024x1024    << (size_t)1)))
#define CECS_TEXTURE_SIZE_POW2_MIP_MASK_2048x2048   (CECS_TEXTURE_SIZE_POW2_MIP_MASK_1024x1024      | ((size_t)1 << ((size_t)cecs_texture_size_2048x2048    << (size_t)1)))
#define CECS_TEXTURE_SIZE_POW2_MIP_MASK_4096x4096   (CECS_TEXTURE_SIZE_POW2_MIP_MASK_2048x2048      | ((size_t)1 << ((size_t)cecs_texture_size_4096x4096    << (size_t)1)))
#define CECS_TEXTURE_SIZE_POW2_MIP_MASK_8192x8192   (CECS_TEXTURE_SIZE_POW2_MIP_MASK_4096x4096      | ((size_t)1 << ((size_t)cecs_texture_size_8192x8192    << (size_t)1)))
#define CECS_TEXTURE_SIZE_POW2_MIP_MASK_16384x16384 (CECS_TEXTURE_SIZE_POW2_MIP_MASK_8192x8192      | ((size_t)1 << ((size_t)cecs_texture_size_16384x16384  << (size_t)1)))
#define CECS_TEXTURE_SIZE_POW2_MIP_MASK_32768x32768 (CECS_TEXTURE_SIZE_POW2_MIP_MASK_16384x16384    | ((size_t)1 << ((size_t)cecs_texture_size_16384x16384  << (size_t)1)))

    static const size_t cecs_texture_size_pow_mip_mask_table[] = {
        CECS_TEXTURE_SIZE_POW2_MIP_MASK_1x1,
        CECS_TEXTURE_SIZE_POW2_MIP_MASK_2x2,
        CECS_TEXTURE_SIZE_POW2_MIP_MASK_4x4,
        CECS_TEXTURE_SIZE_POW2_MIP_MASK_8x8,
        CECS_TEXTURE_SIZE_POW2_MIP_MASK_16x16,
        CECS_TEXTURE_SIZE_POW2_MIP_MASK_32x32,
        CECS_TEXTURE_SIZE_POW2_MIP_MASK_64x64,
        CECS_TEXTURE_SIZE_POW2_MIP_MASK_128x128,
        CECS_TEXTURE_SIZE_POW2_MIP_MASK_256x256,
        CECS_TEXTURE_SIZE_POW2_MIP_MASK_512x512,
        CECS_TEXTURE_SIZE_POW2_MIP_MASK_1024x1024,
        CECS_TEXTURE_SIZE_POW2_MIP_MASK_2048x2048,
        CECS_TEXTURE_SIZE_POW2_MIP_MASK_4096x4096,
        CECS_TEXTURE_SIZE_POW2_MIP_MASK_8192x8192,
        CECS_TEXTURE_SIZE_POW2_MIP_MASK_16384x16384,
        CECS_TEXTURE_SIZE_POW2_MIP_MASK_32768x32768,
    };
    static_assert(
        (CECS_TEXTURE_SIZE_POW2_MIP_MASK_2x2 >> 2) == (CECS_TEXTURE_SIZE_POW2_MIP_MASK_1x1),
        "static error: mip mask table must fulfill the property that the next mip mask shifted right by 2 is equal to the previous mask"
    );
    static_assert(
        (CECS_TEXTURE_SIZE_POW2_MIP_MASK_4x4 >> 2) == (CECS_TEXTURE_SIZE_POW2_MIP_MASK_2x2),
        "static error: mip mask table must fulfill the property that the next mip mask shifted right by 2 is equal to the previous mask"
    );

    assert(mip_count <= (uint_fast8_t)(size + 1) && "error: mip_count must be no greater than size + 1");
    return cecs_texture_size_pow_mip_mask_table[(size_t)size] - (cecs_texture_size_pow_mip_mask_table[(size_t)(size + 1) - (size_t)mip_count] >> 2);
}
uint_fast8_t cecs_texture_size_pow2_max_mip_count(const cecs_texture_size_pow2 size, const uint32_t largest_side_size) {
    // 17px -> 5 size, 5 mip, 5 log
    // 16px -> 4 size, 5 mip, 4 log
    // 15px -> 4 size, 4 mip, 4 log
    assert(
        largest_side_size <= (1u << (uint32_t)size)
        && "error: largest_side_size must be less than or equal to the power of two size"
    );
    return cecs_is_pow2_u32(largest_side_size) ? (uint_fast8_t)size + (uint_fast8_t)1 : (uint_fast8_t)size;
}
cecs_texture_size_pow2 cecs_texture_size_pow2_ensure(const WGPUExtent3D size, uint32_t *out_largest_side_size) {
    *out_largest_side_size = cecs_max_u32(size.width, size.height);
    return (cecs_texture_size_pow2)(cecs_log2_u32(*out_largest_side_size) + 1);
}

cecs_texture_bank_id_descriptor cecs_texture_bank_id_descriptor_create_full(
    const cecs_texture_size_pow2 size,
    const uint_fast8_t mip_count,
    const WGPUTextureFormat format,
    const WGPUTextureUsage usage)
{
    // const uint32_t descriptor_mip_count = cecs_min_u32(
    //     (uint32_t)size, (uint32_t)mip_count
    // ) + 1;
    assert((uint32_t)size <= (uint32_t)mip_count && "error: mip count must be greater than or equal to size as uint32_t");
    return (cecs_texture_bank_id_descriptor){
        .flags = {
            .slots_full = cecs_texture_bank_status_full,
            .size = (cecs_texture_bank_size_flag)size,
            .mip_level_count = (uint8_t)mip_count,
            .usage = (cecs_texture_usage_flags_packed)usage,
        },
        .format = format,
    };
}

cecs_texture_bank_id_descriptor cecs_texture_bank_id_descriptor_create_free(
    const cecs_texture_size_pow2 size,
    const uint_fast8_t mip_count,
    const WGPUTextureFormat format,
    const WGPUTextureUsage usage
) {
    // const uint32_t descriptor_mip_count = cecs_min_u32(
    //     (uint32_t)size, (uint32_t)mip_count
    // ) + 1;
    assert((uint32_t)size <= (uint32_t)mip_count && "error: mip count must be greater than or equal to size as uint32_t");
    return (cecs_texture_bank_id_descriptor){
        .flags = {
            .slots_full = cecs_texture_bank_status_free,
            .size = (cecs_texture_bank_size_flag)size,
            .mip_level_count = (uint8_t)mip_count,
            .usage = (cecs_texture_usage_flags_packed)usage,
        },
        .format = format,
    };
}

extern inline cecs_component_id cecs_component_id_from_texture_resource_id_descriptor(cecs_texture_bank_id_descriptor descriptor);
const uint32_t cecs_texture_bank_default_array_layers = CECS_TEXTURE_BANK_DEFAULT_ARRAY_LAYERS;


extern inline uint_fast8_t cecs_texture_bank_slot_count(const cecs_texture_bank *bank);

static cecs_texture_bank_slot_mask cecs_texture_bank_max_slot_mask(const cecs_texture_bank *bank) {
    return cecs_texture_bank_slot_count(bank) - 1;
}



uint_fast8_t cecs_texture_bank_first_free_slot_index(const cecs_texture_bank *bank, const uint_fast8_t required_slots) {
    static_assert(
        sizeof(cecs_texture_bank_slot_mask) == sizeof(uint64_t),
        "static error: cecs_texture_bank_slot_mask must be the same size as uint64_t"
    );
    const uint64_t marked = cecs_mark_bit_runs_u64(~bank->used_slots_mask, required_slots);
    return cecs_log2_u64(marked & -marked);
}
cecs_texture_bank_slot_mask cecs_texture_bank_slot_mask_from_range(const uint_fast8_t start_index, const uint_fast8_t slot_count) {
    static_assert(
        sizeof(cecs_texture_bank_slot_mask) == sizeof(uint64_t),
        "static error: cecs_texture_bank_slot_mask must be the same size as uint64_t"
    );
    return (UINT64_MAX >> (CHAR_BIT * sizeof(uint64_t) - slot_count)) << start_index;
}

cecs_texture_bank cecs_texture_bank_create(
    WGPUDevice device,
    const cecs_texture_bank_id_descriptor texture_bank_id_descriptor,
    const uint32_t sample_count) {
    assert(
        texture_bank_id_descriptor.flags.size != cecs_texture_size_none
        && "error: invalid cecs_texture_resource_id_descriptor, size must be non-zero"
    );
    const uint32_t texture_size = 1 << texture_bank_id_descriptor.flags.size;
    WGPUTexture bank_texture = wgpuDeviceCreateTexture(device, &(WGPUTextureDescriptor){
        .dimension = WGPUTextureDimension_2D,
        .format = texture_bank_id_descriptor.format,
        .usage = (uint32_t)texture_bank_id_descriptor.flags.usage,
        .size = (WGPUExtent3D){
            .width = texture_size,
            .height = texture_size,
            .depthOrArrayLayers = cecs_texture_bank_default_array_layers,
        },
        .sampleCount = sample_count,
        .mipLevelCount = (uint32_t)texture_bank_id_descriptor.flags.mip_level_count,
    });
    
    WGPUTextureView bank_view = wgpuTextureCreateView(bank_texture, &(WGPUTextureViewDescriptor){
        .format = texture_bank_id_descriptor.format,
        .dimension = WGPUTextureViewDimension_2DArray,
        .baseMipLevel = 0,
        .mipLevelCount = texture_bank_id_descriptor.flags.mip_level_count,
        .baseArrayLayer = 0,
        .arrayLayerCount = cecs_texture_bank_default_array_layers,
    });

    static const cecs_texture_bank_slot_mask initial_mask[2] = {(cecs_texture_bank_slot_mask)0, (~(cecs_texture_bank_slot_mask)0)};
    return (cecs_texture_bank){
        .texture = bank_texture,
        .texture_view = bank_view,
        .used_slots_mask = initial_mask[texture_bank_id_descriptor.flags.slots_full],
    };
}

extern uint_fast8_t inline cecs_texture_bank_slot_count(const cecs_texture_bank *bank);
bool cecs_texture_bank_has_free_slots(const cecs_texture_bank *bank) {
    return bank->used_slots_mask < cecs_texture_bank_max_slot_mask(bank);
}
bool cecs_texture_bank_is_full(const cecs_texture_bank *bank) {
    return !cecs_texture_bank_has_free_slots(bank);
}
bool cecs_texture_bank_is_empty(const cecs_texture_bank *bank) {
    return bank->used_slots_mask == 0;
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
