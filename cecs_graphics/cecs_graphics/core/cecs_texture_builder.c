#include <assert.h>
#include <stdlib.h>
#include "cecs_texture_builder.h"

typedef struct cecs_stbi_allocation_header {
    uint64_t size;
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
    assert(descriptor.bytes_per_pixel > 0 && "error: bytes per pixel must be greater than 0");
    assert(descriptor.channel_count > 0 && "error: channel count must be greater than 0");

    return (cecs_texture_builder){
        .world = world,
        .texture_arena = texture_arena,
        .descriptor = descriptor,
        .texture_data = NULL,
        .texture_size = 0
    };
}

cecs_texture_builder *cecs_texture_builder_load_from(cecs_texture_builder *builder, const char *path) {
    assert(
        builder->descriptor.flags & cecs_texture_builder_descriptor_config_generate_descriptor
        && "error: descriptor must be able to be overridden to load from file. Set generate descriptor flag to allow this"
    );
    texture_builder_stbi_allocator.current_arena = builder->texture_arena;

    int width;
    int height;
    int channels;
    uint8_t *texture_data = stbi_load(path, &width, &height, &channels, builder->descriptor.channel_count);

    if (texture_data == NULL) {
        assert(false && "fatal error: failed to load texture");
        exit(EXIT_FAILURE);
        return builder;
    }

    static_assert(false, "TODO: mips and test texture load and decide TextureDescriptor when");
    static_assert(false, "TODO: handle loading multiple (clearing builder after build)");

    stbi_image_free(texture_data);
    return cecs_texture_builder_set_data(builder, texture_data, width * height * builder->descriptor.bytes_per_pixel);
}

cecs_texture_builder *cecs_texture_builder_set_data(cecs_texture_builder *builder, uint8_t *texture_data, size_t texture_size) {
    builder->texture_data = texture_data;
    builder->texture_size = texture_size;
    return builder;
}

cecs_texture_reference cecs_texture_builder_build_into(
    cecs_world *world,
    cecs_texture_builder *builder,
    cecs_graphics_context *context,
    const WGPUTextureViewDescriptor *view_descriptor
) {
    assert(builder->texture_data != NULL && "error: texture data must be set");
    assert(builder->texture_size > 0 && "error: texture size must be set");

    const uint32_t width = builder->descriptor.texture_descriptor.size.width;
    const uint32_t height = builder->descriptor.texture_descriptor.size.height;

    const uint32_t bytes_per_row = width * builder->descriptor.bytes_per_pixel;
    assert(builder->texture_size == bytes_per_row * height && "error: texture size mismatch");

    WGPUTexture texture = wgpuDeviceCreateTexture(context->device, &builder->descriptor.texture_descriptor);
    wgpuQueueWriteTexture(
        context->queue,
        &(WGPUImageCopyTexture){
            .texture = texture,
            .mipLevel = 0,
            .origin = (WGPUOrigin3D){0, 0, 0},
            .aspect = WGPUTextureAspect_All,
        },
        builder->texture_data,
        builder->texture_size,
        &(WGPUTextureDataLayout){
            .offset = 0,
            .bytesPerRow = bytes_per_row,
            .rowsPerImage = height,
        },
        &builder->descriptor.texture_descriptor.size
    );
    // TODO: watch
    wgpuTextureRelease(texture);

    WGPUTextureView texture_view = wgpuTextureCreateView(texture, view_descriptor);
    // TODO: possibility for grouping in larger atlas
    cecs_entity_id texture_id = cecs_world_add_entity(&builder->world->world);
    CECS_WORLD_SET_COMPONENT(
        cecs_texture,
        &builder->world->world,
        texture_id,
        (&(cecs_texture){
            .texture_view = texture_view,
            .extent = builder->descriptor.texture_descriptor.size
        })
    );
    
    return (cecs_texture_reference){
        .texture_id = texture_id
    };
}
