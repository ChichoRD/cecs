#include <assert.h>
#include <stdlib.h>
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
    WGPUTextureDimension dimension,
    WGPUTextureFormat format,
    WGPUTextureUsage usage
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
        .mipLevelCount = (uint32_t)cecs_log2((size_t)max(width, height)),
        .sampleCount = 1,
        .size = size,
        .usage = usage,
        .viewFormatCount = 0,
        .viewFormats = NULL,
    };

    // static_assert(false, "TODO: mips and test texture load and decide TextureDescriptor when");
    //static_assert(false, "TODO: handle loading multiple (clearing builder after build); set to NULL prior");
    return cecs_texture_builder_set_data(builder, texture_data, texture_descriptor);
}

cecs_texture_builder *cecs_texture_builder_set_data(cecs_texture_builder *builder, const uint8_t *texture_data, WGPUTextureDescriptor texture_descriptor) {
    builder->texture_data = texture_data;
    builder->texture_descriptor = texture_descriptor;
    return builder;
}

static void cecs_write_mipmaps_from_level0(
    WGPUQueue queue,
    WGPUImageCopyTexture *destination,
    WGPUExtent3D level_mip_size,
    const uint8_t *level_data,
    size_t level_size,
    uint_fast8_t bytes_per_texel,
    uint_fast8_t mip_level_count
) {
    cecs_arena arena = cecs_arena_create_with_capacity(level_size * 4 / 3);
    cecs_dynamic_array previous_mip_texels = cecs_dynamic_array_create_with_capacity(&arena, level_size);
    cecs_dynamic_array_append_empty(&previous_mip_texels, &arena, level_size, sizeof(uint8_t));
    cecs_dynamic_array_set_range(&previous_mip_texels, 0, level_data, level_size, sizeof(uint8_t));

    for (uint32_t mip_level = 1; mip_level < mip_level_count; mip_level++) {
        WGPUExtent3D previous_mip_size = level_mip_size;
        level_mip_size.width >>= 1;
        level_mip_size.height >>= 1;

        const uint32_t mip_bytes_per_row = level_mip_size.width * bytes_per_texel;
        const size_t mip_texture_size = level_mip_size.height * mip_bytes_per_row;
        cecs_dynamic_array texels = cecs_dynamic_array_create_with_capacity(&arena, mip_texture_size);
        cecs_dynamic_array_append_empty(&texels, &arena, mip_texture_size, sizeof(uint8_t));

        for (uint32_t i = 0; i < level_mip_size.width; ++i) {
            for (uint32_t j = 0; j < level_mip_size.height; ++j) {
                uint8_t* p = cecs_dynamic_array_get(&texels, bytes_per_texel * (j * level_mip_size.width + i), sizeof(uint8_t));

                // Get the corresponding 4 pixels from the previous level
                uint8_t* p00 = cecs_dynamic_array_get(&previous_mip_texels, bytes_per_texel * ((2 * j + 0) * previous_mip_size.width + (2 * i + 0)), sizeof(uint8_t));
                uint8_t* p01 = cecs_dynamic_array_get(&previous_mip_texels, bytes_per_texel * ((2 * j + 0) * previous_mip_size.width + (2 * i + 1)), sizeof(uint8_t));
                uint8_t* p10 = cecs_dynamic_array_get(&previous_mip_texels, bytes_per_texel * ((2 * j + 1) * previous_mip_size.width + (2 * i + 0)), sizeof(uint8_t));
                uint8_t* p11 = cecs_dynamic_array_get(&previous_mip_texels, bytes_per_texel * ((2 * j + 1) * previous_mip_size.width + (2 * i + 1)), sizeof(uint8_t));

                for (uint_fast8_t k = 0; k < bytes_per_texel; ++k) {
                    // Average
                    p[k] = (p00[k] + p01[k] + p10[k] + p11[k]) / 4;
                }
            }
        }

        destination->mipLevel = mip_level;
        WGPUTextureDataLayout source = (WGPUTextureDataLayout){
            .nextInChain = NULL,
            .offset = 0,
            .bytesPerRow = mip_bytes_per_row,
            .rowsPerImage = level_mip_size.height,
        };
        assert(
            cecs_dynamic_array_count_of_size(&texels, sizeof(uint8_t)) == mip_texture_size
            && "error: mismatch of total size of mip texels"
        );
        wgpuQueueWriteTexture(
            queue,
            destination,
            cecs_dynamic_array_first(&texels),
            mip_texture_size,
            &source,
            &level_mip_size
        );

        previous_mip_texels = texels;
    }

    cecs_arena_free(&arena);
}

void cecs_write_mipmaps(
    WGPUQueue queue,
    WGPUTexture texture,
    const WGPUTextureDescriptor *descriptor,
    const uint8_t *texture_data,
    uint_fast8_t bytes_per_texel,
    WGPUTextureAspect aspect
) {
    WGPUImageCopyTexture destination = (WGPUImageCopyTexture){
        .nextInChain = NULL,
        .texture = texture,
        .mipLevel = 0,
        .origin = (WGPUOrigin3D){0, 0, 0},
        .aspect = aspect,
    };

    const uint32_t width = descriptor->size.width;
    const uint32_t height = descriptor->size.height;

    const uint32_t bytes_per_row = width * bytes_per_texel;
    const size_t texture_size = bytes_per_row * height;

    WGPUTextureDataLayout source = (WGPUTextureDataLayout){
        .nextInChain = NULL,
        .offset = 0,
        .bytesPerRow = bytes_per_row,
        .rowsPerImage = height,
    };
    WGPUExtent3D mip_size = descriptor->size;
    wgpuQueueWriteTexture(queue, &destination, texture_data, texture_size, &source, &mip_size);

    if (descriptor->mipLevelCount > 1) {
        cecs_write_mipmaps_from_level0(
            queue,
            &destination,
            mip_size,
            texture_data,
            texture_size,
            bytes_per_texel,
            descriptor->mipLevelCount
        );
        (void)destination;
    }
}

cecs_texture_reference cecs_texture_builder_build_into(
    cecs_world *world,
    cecs_texture_builder *builder,
    cecs_graphics_context *context,
    const WGPUTextureViewDescriptor *view_descriptor
) {
    assert(builder->texture_data != NULL && "error: texture data must be set");
    assert(builder->texture_descriptor.usage & WGPUTextureUsage_CopyDst && "error: texture must be copyable");

    WGPUTexture texture = wgpuDeviceCreateTexture(context->device, &builder->texture_descriptor);
    cecs_write_mipmaps(context->queue, texture, &builder->texture_descriptor, builder->texture_data, builder->descriptor.bytes_per_texel, view_descriptor->aspect);

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
