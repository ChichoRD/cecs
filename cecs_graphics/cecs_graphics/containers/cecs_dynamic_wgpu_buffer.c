#include <stdbool.h>
#include <assert.h>
#include <webgpu/wgpu.h>
#include <stdlib.h>
#include <memory.h>
#include "cecs_dynamic_wgpu_buffer.h"

const cecs_buffer_offset_u64 cecs_webgpu_copy_buffer_alignment = CECS_WGPU_COPY_BUFFER_ALIGNMENT;
const cecs_buffer_offset_u64 cecs_webgpu_uniform_buffer_alignment = CECS_WGPU_UNIFORM_BUFFER_ALIGNMENT;
const cecs_buffer_offset_u64 cecs_webgpu_vertex_stride_alignment = CECS_WGPU_VERTEX_STRIDE_ALIGNMENT;

extern inline cecs_buffer_offset_u64 cecs_align_to_wgpu_copy_buffer_alignment(cecs_dynamic_buffer_offset size);

// https://github.com/luiswirth/wgpu-util/tree/main
WGPUBuffer cecs_wgpu_buffer_create_with_data(
    WGPUDevice device,
    const WGPUBufferUsage usage,
    const uint64_t buffer_size,
    const void *data,
    const size_t data_size
) {
    assert(buffer_size >= data_size && "error: buffer size must be greater than or equal to data size");
    assert(buffer_size != 0 && "error: buffer size must be non-zero");

    const uint64_t aligned_size = cecs_align_to_wgpu_copy_buffer_alignment(buffer_size);

    const WGPUBufferDescriptor descriptor = {
        .label = "wgpu buffer",
        .mappedAtCreation = true,
        .nextInChain = NULL,
        .size = aligned_size,
        .usage = usage
    };

    WGPUBuffer buffer = wgpuDeviceCreateBuffer(device, &descriptor);
    uint8_t *mapped_data = wgpuBufferGetMappedRange(buffer, 0, aligned_size);
    memcpy(mapped_data, data, data_size);
    memset(mapped_data + data_size, 0, aligned_size - data_size);

    wgpuBufferUnmap(buffer);
    return buffer;
}

extern inline bool cecs_is_pow2_u16(const uint16_t n);
extern inline size_t cecs_align_to_pow2(const size_t size, const size_t alignment);

cecs_dynamic_wgpu_buffer cecs_dynamic_wgpu_buffer_create_from_stage(
    WGPUDevice device,
    cecs_buffer_stage_value *stage,
    const size_t stage_size,
    const WGPUBufferUsage usage,
    const uint16_t size_alignment
) {
    assert(stage != NULL && "error: stage must not be NULL");
    assert(stage_size != 0 && "error: stage size must be non-zero");
    assert(size_alignment != 0 && "error: size alignment must be non-zero");
    assert(cecs_is_pow2_u16(size_alignment) && "error: size alignment must be a power of two");
    
    const uint64_t buffer_size = (uint64_t)cecs_align_to_pow2((size_t)stage_size, (size_t)size_alignment);
    return (cecs_dynamic_wgpu_buffer){
        .buffer = wgpuDeviceCreateBuffer(
            device,
            &(WGPUBufferDescriptor){
                .label = "cecs dynamic buffer",
                .mappedAtCreation = false,
                .nextInChain = NULL,
                .size = buffer_size,
                .usage = usage
            }
        ),
        .stage = stage,
        .stage_size = stage_size,
        .size_alignmnent = size_alignment,
        .usage = usage
    };
}
cecs_dynamic_wgpu_buffer cecs_dynamic_wgpu_buffer_create_from_stage_mapped(
    WGPUDevice device,
    cecs_buffer_stage_value *stage,
    const size_t stage_size,
    const WGPUBufferUsage usage,
    const uint16_t size_alignment
) {
    assert(stage != NULL && "error: stage must not be NULL");
    assert(stage_size != 0 && "error: stage size must be non-zero");
    assert(size_alignment != 0 && "error: size alignment must be non-zero");
    assert(cecs_is_pow2_u16(size_alignment) && "error: size alignment must be a power of two");

    const uint64_t buffer_size = (uint64_t)cecs_align_to_pow2((size_t)stage_size, (size_t)size_alignment);
    return (cecs_dynamic_wgpu_buffer){
        .buffer = cecs_wgpu_buffer_create_with_data(device, usage, buffer_size, stage, stage_size),
        .stage = stage,
        .stage_size = stage_size,
        .size_alignmnent = size_alignment,
        .usage = usage
    };
}
cecs_dynamic_wgpu_buffer cecs_dynamic_wgpu_buffer_create(
    WGPUDevice device,
    cecs_arena *arena,
    const size_t size,
    const WGPUBufferUsage usage,
    const uint16_t size_alignment
) {
    cecs_buffer_stage_value *stage = cecs_arena_alloc(arena, size);
    return cecs_dynamic_wgpu_buffer_create_from_stage(device, stage, size, usage, size_alignment);
}

void *cecs_dynamic_wgpu_buffer_resize(cecs_dynamic_wgpu_buffer *buffer, cecs_arena *arena, const size_t new_size) {
    buffer->stage = cecs_arena_realloc(arena, buffer->stage, buffer->stage_size, new_size);
    buffer->stage_size = new_size;
    return buffer->stage;
}

void *cecs_dynamic_wgpu_buffer_stage(
    cecs_dynamic_wgpu_buffer *buffer,
    const cecs_dynamic_buffer_offset offset,
    const void *data,
    const cecs_dynamic_buffer_offset size
) {
    assert(
        (offset + size <= buffer->stage_size)
        && "error: range to stage must be within the bounds of the staging buffer,"
        "use cecs_dynamic_wgpu_buffer_resize to resize the buffer, or cecs_dynamic_wgpu_buffer_stage_or_resize to stage and resize"
    );
    return memcpy(buffer->stage + offset, data, size);
}

void *cecs_dynamic_wgpu_buffer_stage_or_resize(
    cecs_dynamic_wgpu_buffer *buffer,
    cecs_arena *arena,
    const cecs_dynamic_buffer_offset offset,
    const void *data,
    const cecs_dynamic_buffer_offset size
) {
    const cecs_dynamic_buffer_offset data_end = offset + size;
    if (buffer->stage_size < data_end) {
        cecs_dynamic_wgpu_buffer_resize(buffer, arena, data_end);
    }
    return cecs_dynamic_wgpu_buffer_stage(buffer, offset, data, size);
}

extern inline uint64_t cecs_align_to_pow2_u64(uint64_t size, uint64_t align); 
extern inline bool cecs_is_pow2(const size_t n);
cecs_dynamic_buffer_offset cecs_dynamic_wgpu_buffer_upload(
    cecs_dynamic_wgpu_buffer *buffer,
    WGPUDevice device,
    WGPUQueue queue,
    cecs_arena *arena,
    const cecs_dynamic_buffer_offset offset,
    const cecs_dynamic_buffer_offset size
) {
    assert(
        (offset + size <= buffer->stage_size)
        && "error: range to upload must be within the bounds of the staging buffer"
    );

    const uint64_t aligned_offset = cecs_align_to_pow2_u64(offset + 1, buffer->size_alignmnent) - buffer->size_alignmnent;
    const uint64_t aligned_data_end = cecs_align_to_pow2_u64(offset + size, buffer->size_alignmnent);
    const size_t aligned_size = aligned_data_end - aligned_offset;

    assert(aligned_size != 0 && "fatal error: aligned size must be non-zero");
    extern inline bool cecs_is_aligned_to_pow2(const size_t size, const size_t alignment);
    assert(cecs_is_aligned_to_pow2(aligned_size, buffer->size_alignmnent) && "fatal error: aligned size must be a power of two");

    if (aligned_data_end > buffer->stage_size) {
        cecs_dynamic_wgpu_buffer_resize(buffer, arena, aligned_data_end);
    }
    assert(aligned_data_end <= buffer->stage_size && "fatal error: aligned data end must be less than or equal to the stage size");

    if (wgpuBufferGetSize(buffer->buffer) < aligned_data_end) {
        wgpuBufferRelease(buffer->buffer);
        buffer->buffer = cecs_wgpu_buffer_create_with_data(device, buffer->usage, buffer->stage_size, buffer->stage, aligned_data_end);
        buffer->stage_size = aligned_data_end;
    } else {
        wgpuQueueWriteBuffer(
            queue,
            buffer->buffer,
            aligned_offset,
            buffer->stage + aligned_offset,
            aligned_size
        );
    }
    return aligned_size;
}

cecs_dynamic_buffer_offset cecs_dynamic_wgpu_buffer_upload_all(
    cecs_dynamic_wgpu_buffer *buffer,
    WGPUDevice device,
    WGPUQueue queue,
    cecs_arena *arena
) {
    return cecs_dynamic_wgpu_buffer_upload(buffer, device, queue, arena, 0, buffer->stage_size);
}

void cecs_dynamic_wgpu_buffer_free(cecs_dynamic_wgpu_buffer *buffer) {
    wgpuBufferRelease(buffer->buffer);
    buffer->buffer = NULL;
    buffer->stage = NULL;
    buffer->stage_size = 0;
    buffer->size_alignmnent = 0;
    buffer->usage = WGPUBufferUsage_None;
}

extern inline uint_fast8_t cecs_log2(const size_t n);
cecs_dynamic_wgpu_element_buffer cecs_dynamic_wgpu_element_buffer_create(
    WGPUDevice device,
    cecs_arena *arena,
    const size_t element_size,
    const uint16_t element_offset_alignment,
    const uint16_t upload_alignment,
    const WGPUBufferUsageFlags usage
) {
    const size_t aligned_element_size = cecs_align_to_pow2(element_size, (size_t)element_offset_alignment);
    return (cecs_dynamic_wgpu_element_buffer){
        .buffer = cecs_dynamic_wgpu_buffer_create(device, arena, aligned_element_size, usage, upload_alignment),
        .aligned_element_size_log2 = cecs_log2(aligned_element_size)
    };
}

void cecs_dynamic_wgpu_element_buffer_free(cecs_dynamic_wgpu_element_buffer *buffer) {
    cecs_dynamic_wgpu_buffer_free(&buffer->buffer);
    buffer->aligned_element_size_log2 = 0;
}

extern inline cecs_buffer_offset_u64 cecs_dynamic_wgpu_element_buffer_element_offset(
    const cecs_dynamic_wgpu_element_buffer *buffer,
    const cecs_dynamic_buffer_offset element_index
);
extern inline cecs_dynamic_buffer_offset cecs_dynamic_wgpu_element_buffer_element_index(
    const cecs_dynamic_wgpu_element_buffer *buffer,
    const cecs_buffer_offset_u64 element_offset
);

extern inline size_t cecs_dynamic_wgpu_element_buffer_element_size(const cecs_dynamic_wgpu_element_buffer *buffer);
cecs_buffer_offset_u64 cecs_dynamic_wgpu_element_buffer_stage(
    cecs_dynamic_wgpu_element_buffer *buffer,
    cecs_arena *arena,
    const cecs_dynamic_buffer_offset element_index,
    const cecs_buffer_offset_u64 suboffset,
    const void *data,
    const size_t size
) {
    assert(
        (suboffset + size <= cecs_dynamic_wgpu_element_buffer_element_size(buffer))
        && "error: tried to upload data that exceeds the size of the element"
    );
    const cecs_buffer_offset_u64 destination_offset = cecs_dynamic_wgpu_element_buffer_element_offset(buffer, element_index) + suboffset;
    cecs_dynamic_wgpu_buffer_stage_or_resize(
        &buffer->buffer,
        arena,
        destination_offset,
        data,
        size
    );
    return destination_offset;
}
cecs_buffer_offset_u64 cecs_dynamic_wgpu_element_buffer_stage_range(
    cecs_dynamic_wgpu_element_buffer *buffer,
    cecs_arena *arena,
    const cecs_dynamic_buffer_offset element_index,
    const cecs_dynamic_buffer_offset suboffset,
    const void *data,
    const size_t count,
    const size_t size
) {
    assert(
        (suboffset + count * size <= cecs_dynamic_wgpu_element_buffer_element_size(buffer))
        && "error: tried to upload data that exceeds the size of the element"
    );
    for (ptrdiff_t i = count - 1; i >= 0; --i) {
        cecs_dynamic_wgpu_element_buffer_stage(
            buffer,
            arena,
            element_index + i,
            suboffset,
            (uint8_t *)data + i * size,
            size
        );
    }
    return cecs_dynamic_wgpu_element_buffer_element_offset(buffer, element_index) + suboffset;
}

cecs_buffer_offset_u64 cecs_dynamic_wgpu_element_buffer_upload(
    cecs_dynamic_wgpu_element_buffer *buffer,
    WGPUDevice device,
    WGPUQueue queue,
    cecs_arena *arena,
    const cecs_dynamic_buffer_offset element_index,
    const cecs_buffer_offset_u64 suboffset,
    const size_t size
) {
    assert(
        (suboffset + size <= cecs_dynamic_wgpu_element_buffer_element_size(buffer))
        && "error: tried to upload data that exceeds the size of the element"
    );
    const cecs_buffer_offset_u64 destination_offset = cecs_dynamic_wgpu_element_buffer_element_offset(buffer, element_index) + suboffset;
    cecs_dynamic_wgpu_buffer_upload(
        &buffer->buffer,
        device,
        queue,
        arena,
        destination_offset,
        size
    );
    return destination_offset;
}

extern inline size_t cecs_dynamic_wgpu_element_buffer_element_count(const cecs_dynamic_wgpu_element_buffer *buffer);
cecs_buffer_offset_u64 cecs_dynamic_wgpu_element_buffer_upload_range(
    cecs_dynamic_wgpu_element_buffer *buffer,
    WGPUDevice device,
    WGPUQueue queue,
    cecs_arena *arena,
    const cecs_dynamic_buffer_offset element_index,
    const size_t count
) {
    assert(
        (element_index + count <= cecs_dynamic_wgpu_element_buffer_element_count(buffer))
        && "error: tried to upload data that exceeds the number of elements in the buffer"
    );
    const cecs_buffer_offset_u64 element_offset = cecs_dynamic_wgpu_element_buffer_element_offset(buffer, element_index);
    const cecs_dynamic_buffer_offset upload_size = count * cecs_dynamic_wgpu_element_buffer_element_size(buffer);

    cecs_dynamic_wgpu_buffer_upload(
        &buffer->buffer,
        device,
        queue,
        arena,
        element_offset,
        upload_size
    );
    return element_offset;
}

cecs_buffer_offset_u64 cecs_dynamic_wgpu_element_buffer_upload_all(
    cecs_dynamic_wgpu_element_buffer *buffer,
    WGPUDevice device,
    WGPUQueue queue,
    cecs_arena *arena
) {
    return cecs_dynamic_wgpu_element_buffer_upload_range(
        buffer,
        device,
        queue,
        arena,
        0,
        cecs_dynamic_wgpu_element_buffer_element_count(buffer)
    );
}
