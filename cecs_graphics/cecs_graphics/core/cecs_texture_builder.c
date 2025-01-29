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
    builder->texture_data = texture_data;
    builder->texture_descriptor = texture_descriptor;
    return builder;
}

WGPUExtent3D cecs_generate_next_mip(
    const WGPUExtent3D mip_size,
    const uint8_t *mip_texels,
    const uint_fast8_t bytes_per_texel,
    uint8_t out_next_mip_texels[]
) {
    const WGPUExtent3D next_mip_size = {
        .width = mip_size.width >> 1,
        .height = mip_size.height >> 1,
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
                destination_texel[k] = (texel_00[k] + texel_01[k] + texel_10[k] + texel_11[k]) / 4;
            }
        }
    }

    return next_mip_size;
}

uint_fast8_t cecs_generate_mip_chain(
    const WGPUExtent3D mip_size,
    const uint8_t *mip_texels,
    const uint_fast8_t bytes_per_texel,
    uint8_t out_mip_chain_start[restrict],
    size_t *out_mip_chain_size
) {
    extern inline int_fast8_t cecs_log2(size_t n);
    const uint_fast8_t mip_level_count =
        cecs_log2((size_t)max(mip_size.width, mip_size.height)) + 1;
    
    WGPUExtent3D previous_mip_size = mip_size;
    const uint8_t *previous_mips = mip_texels;

    for (uint_fast8_t i = 1; i < mip_level_count; i++) {
        const WGPUExtent3D new_mip_size = cecs_generate_next_mip(
            previous_mip_size,
            previous_mips,
            bytes_per_texel,
            out_mip_chain_start
        );
        const uint32_t mip_bytes_per_row = new_mip_size.width * bytes_per_texel;
        const size_t mip_texture_size = new_mip_size.height * mip_bytes_per_row;

        previous_mip_size = new_mip_size;
        previous_mips = out_mip_chain_start;
        out_mip_chain_start += mip_texture_size;
    }

    *out_mip_chain_size = out_mip_chain_start - mip_texels;
    return mip_level_count;
}

void cecs_write_mipmaps(
    WGPUQueue queue,
    WGPUTexture texture,
    const WGPUTextureDescriptor *descriptor,
    const uint8_t *texture_data,
    const uint_fast8_t bytes_per_texel,
    const WGPUTextureAspect aspect,
    const uint32_t destination_layer
) {
    for (uint32_t i = 0; i < descriptor->mipLevelCount; i++) {
        const uint32_t mip_width = descriptor->size.width >> i;
        const uint32_t mip_height = descriptor->size.height >> i;
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
        wgpuQueueWriteTexture(queue, &destination, texture_data, mip_texture_size, &source, &mip_size);
    }
}

static cecs_texture_builder *cecs_texture_builder_configure_mipmaps(cecs_texture_builder *builder) {
    if (
        (builder->descriptor.flags & cecs_texture_builder_descriptor_config_generate_mipmaps)
        && builder->texture_descriptor.mipLevelCount == 1
    ) {
        const size_t texture_size = builder->texture_descriptor.size.width
            * builder->texture_descriptor.size.height
            * builder->descriptor.bytes_per_texel;
        uint8_t *mip_texels;

        const size_t total_mips_size = texture_size * 4 / 3 - 1;
        if (builder->descriptor.flags & cecs_texture_builder_descriptor_config_alloc_mipmaps) {
            mip_texels =
                cecs_arena_realloc(builder->texture_arena, builder->texture_data, texture_size, total_mips_size);
        } else {
            mip_texels = builder->texture_data;
        }

        size_t mip_chain_size;
        builder->texture_descriptor.mipLevelCount = (uint32_t)cecs_generate_mip_chain(
            builder->texture_descriptor.size,
            mip_texels,
            builder->descriptor.bytes_per_texel,
            mip_texels + texture_size,
            &mip_chain_size
        );

        // FIXME: assert fails by one off
        assert(mip_chain_size == total_mips_size && "fatal error: mismatch of total size of mip chain");
        builder->texture_data = mip_texels;
    }
    return builder;
}

cecs_texture_reference cecs_texture_builder_build_into(
    cecs_world *world,
    cecs_texture_builder *builder,
    cecs_graphics_context *context,
    const WGPUTextureViewDescriptor *view_descriptor
) {
    assert(builder->texture_data != NULL && "error: texture data must be set");
    assert(builder->texture_descriptor.usage & WGPUTextureUsage_CopyDst && "error: texture must be copyable");
    (void)world;

    cecs_texture_builder_configure_mipmaps(builder);
    WGPUTexture texture = wgpuDeviceCreateTexture(context->device, &builder->texture_descriptor);
    cecs_write_mipmaps(context->queue, texture, &builder->texture_descriptor, builder->texture_data, builder->descriptor.bytes_per_texel, view_descriptor->aspect, 0);

    texture_builder_stbi_allocator.current_arena = builder->texture_arena;
    stbi_image_free(builder->texture_data);
    texture_builder_stbi_allocator.current_arena = NULL;

    WGPUTextureView texture_view = wgpuTextureCreateView(texture, view_descriptor);
    // TODO: watch
    wgpuTextureRelease(texture);

    // TODO: possibility for grouping in larger atlas
    cecs_entity_id texture_id = cecs_world_add_entity(&builder->world->world);
    CECS_WORLD_SET_COMPONENT(
        cecs_texture,
        &builder->world->world,
        texture_id,
        (&(cecs_texture){
            .texture_view = texture_view,
            .extent = builder->texture_descriptor.size
        })
    );
    
    return (cecs_texture_reference){
        .texture_id = texture_id
    };
}
