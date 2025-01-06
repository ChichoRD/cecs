#ifndef CECS_DYNAMIC_WGPU_BUFFER_H
#define CECS_DYNAMIC_WGPU_BUFFER_H

#include <webgpu/webgpu.h>
#include <cecs_core/cecs_core.h>

typedef uint64_t cecs_buffer_offset_u64;
typedef uint32_t cecs_buffer_offset_u32;

// TODO: displaced buffer
// TODO: with small staging buffer for COPY_BUFFER_ALIGNMENT

#define CECS_WGPU_COPY_BUFFER_ALIGNMENT_LOG2 2
#define CECS_WGPU_COPY_BUFFER_ALIGNMENT ((cecs_buffer_offset_u64)(1 << CECS_WGPU_COPY_BUFFER_ALIGNMENT_LOG2))
static_assert(CECS_WGPU_COPY_BUFFER_ALIGNMENT == 4, "fatal error: invalid copy buffer alignment");

extern const cecs_buffer_offset_u64 cecs_webgpu_copy_buffer_alignment;

inline cecs_buffer_offset_u64 cecs_align_to_wgpu_copy_buffer_alignment(uint64_t size) {
    const cecs_buffer_offset_u64 align_mask = CECS_WGPU_COPY_BUFFER_ALIGNMENT - 1;
    const cecs_buffer_offset_u64 aligned_size = (size + align_mask) & ~align_mask;
    return max(aligned_size, CECS_WGPU_COPY_BUFFER_ALIGNMENT);
}

WGPUBuffer cecs_wgpu_buffer_create_with_data(
    WGPUDevice device,
    WGPUBufferUsageFlags usage,
    uint64_t buffer_size,
    void *data,
    size_t data_size
);

typedef struct cecs_dynamic_wgpu_buffer {
    WGPUBuffer buffer;
    cecs_displaced_set staging;
    size_t uploaded_size;
    WGPUBufferUsageFlags usage;
} cecs_dynamic_wgpu_buffer;

static inline cecs_dynamic_wgpu_buffer cecs_dynamic_wgpu_buffer_uninitialized(void) {
    return (cecs_dynamic_wgpu_buffer){
        .buffer = NULL,
        .staging = cecs_displaced_set_create(),
        .uploaded_size = 0,
        .usage = WGPUBufferUsage_None
    };
}

cecs_dynamic_wgpu_buffer cecs_dynamic_wgpu_buffer_create(WGPUDevice device, cecs_arena *arena, WGPUBufferUsageFlags usage, uint64_t size);
void cecs_dynamic_wgpu_buffer_free(cecs_dynamic_wgpu_buffer *buffer);

cecs_buffer_offset_u64 cecs_dynamic_wgpu_buffer_get_offset(cecs_dynamic_wgpu_buffer *buffer, size_t offset);

cecs_buffer_offset_u64 cecs_dynamic_wgpu_buffer_upload(
    cecs_dynamic_wgpu_buffer *buffer,
    WGPUDevice device,
    WGPUQueue queue,
    cecs_arena *arena,
    size_t offset,
    void *data,
    size_t size
);

#endif