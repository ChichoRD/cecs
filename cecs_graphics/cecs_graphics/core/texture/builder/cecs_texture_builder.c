#include "cecs_texture_builder.h"
#include <cecs_math/relations/cecs_ordering.h>

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


extern inline cecs_texture_builder cecs_texture_builder_create(
    const WGPUTextureDescriptor descriptor,
    cecs_graphics_world *world,
    cecs_arena *texture_arena
);

WGPUTexture cecs_texture_builder_build_alloc(
    cecs_texture_builder *builder,
    cecs_graphics_context *context
) {
    WGPUTexture texture = wgpuDeviceCreateTexture(context->device, &builder->descriptor);
    if (texture == NULL) {
        assert(false && "fatal error: failed to create texture");
        exit(EXIT_FAILURE);
    }
    return texture;
}
extern inline cecs_graphics_world *cecs_texture_builder_world(cecs_texture_builder *builder);
extern inline cecs_arena *cecs_texture_builder_arena(cecs_texture_builder *builder);

cecs_texture_size_pow2 cecs_texture_builder_ensured_size(
    const cecs_texture_builder *builder,
    uint32_t *out_largest_side_size
) {
    return cecs_texture_size_pow2_ensure(builder->descriptor.size, out_largest_side_size);
}
extern inline uint64_t cecs_texture_builder_mip0_texel_count(const cecs_texture_builder *builder);
size_t cecs_texture_builder_write_mipmaps(
    WGPUTexture destination,
    cecs_texture_builder *builder,
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