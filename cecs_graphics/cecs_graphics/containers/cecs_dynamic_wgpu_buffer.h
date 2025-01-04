#ifndef CECS_DYNAMIC_WGPU_BUFFER_H
#define CECS_DYNAMIC_WGPU_BUFFER_H

#include <webgpu/webgpu.h>

typedef uint64_t cecs_buffer_offset_u64;
typedef uint32_t cecs_buffer_offset_u32;

typedef struct cecs_dynamic_wgpu_buffer {
    WGPUBuffer buffer;
    size_t size;
    WGPUBufferUsageFlags usage;
} cecs_dynamic_wgpu_buffer;

static inline cecs_dynamic_wgpu_buffer cecs_dynamic_wgpu_buffer_uninitialized() {
    return (cecs_dynamic_wgpu_buffer){
        .buffer = NULL,
        .size = 0,
        .usage = WGPUBufferUsage_None
    };
}

cecs_dynamic_wgpu_buffer cecs_dynamic_wgpu_buffer_create(WGPUDevice device, const WGPUBufferDescriptor *descriptor);
void cecs_dynamic_wgpu_buffer_free(cecs_dynamic_wgpu_buffer *buffer);

void cecs_dynamic_wgpu_buffer_upload(
    cecs_dynamic_wgpu_buffer *buffer,
    WGPUDevice device,
    WGPUQueue queue,
    cecs_buffer_offset_u64 offset,
    void *data,
    size_t size
);

#endif