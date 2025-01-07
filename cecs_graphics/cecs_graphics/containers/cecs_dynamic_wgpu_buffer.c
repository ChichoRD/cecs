#include <stdbool.h>
#include <assert.h>
#include <webgpu/wgpu.h>
#include <stdlib.h>
#include <memory.h>
#include "cecs_dynamic_wgpu_buffer.h"

const cecs_buffer_offset_u64 cecs_webgpu_copy_buffer_alignment = CECS_WGPU_COPY_BUFFER_ALIGNMENT;

extern inline cecs_buffer_offset_u64 cecs_align_to_wgpu_copy_buffer_alignment(cecs_dynamic_buffer_offset size);

// https://github.com/luiswirth/wgpu-util/tree/main
WGPUBuffer cecs_wgpu_buffer_create_with_data(
    WGPUDevice device,
    WGPUBufferUsageFlags usage,
    uint64_t buffer_size,
    void *data,
    size_t data_size
) {
    assert(buffer_size >= data_size && "error: buffer size must be greater than or equal to data size");
    assert(buffer_size != 0 && "error: buffer size must be non-zero");

    const uint64_t max_size = max(buffer_size, (uint64_t)data_size);
    const uint64_t aligned_size = cecs_align_to_wgpu_copy_buffer_alignment(max_size);

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

cecs_dynamic_wgpu_buffer cecs_dynamic_wgpu_buffer_create(
    WGPUDevice device,
    cecs_arena *arena,
    WGPUBufferUsageFlags usage,
    cecs_dynamic_buffer_offset size
) {
    assert(size != 0 && "error: buffer size must be non-zero");
    assert(
        usage & WGPUBufferUsage_CopySrc
        && "error: buffer must be able to be used as a copy source, else it cannot be resized"
    );

    uint64_t aligned_size = cecs_align_to_wgpu_copy_buffer_alignment(size);
    return (cecs_dynamic_wgpu_buffer){
        .buffer = wgpuDeviceCreateBuffer(
            device,
            &(WGPUBufferDescriptor){
                .label = "dynamic buffer",
                .mappedAtCreation = false,
                .nextInChain = NULL,
                .size = aligned_size,
                .usage = usage
            }
        ),
        .staging = cecs_displaced_set_create_with_capacity(arena, aligned_size),
        .uploaded_size = aligned_size,
        .usage = usage
    };
}

void cecs_dynamic_wgpu_buffer_free(cecs_dynamic_wgpu_buffer *buffer) {
    wgpuBufferRelease(buffer->buffer);
    buffer->buffer = NULL;
    cecs_displaced_set_clear(&buffer->staging);
    buffer->uploaded_size = 0;
    buffer->usage = WGPUBufferUsage_None;
}

cecs_buffer_offset_u64 cecs_dynamic_wgpu_buffer_get_offset(cecs_dynamic_wgpu_buffer *buffer, cecs_dynamic_buffer_offset offset) {
    assert(
        cecs_displaced_set_contains_index(&buffer->staging, offset)
        && "error: staging buffer does not contain requested offset"
    );
    return cecs_displaced_set_cecs_dynamic_array_index(&buffer->staging, offset);
}

typedef struct cecs_buffer_staging_parameters {
    cecs_buffer_offset_u64 aligned_requested_offset;
    cecs_buffer_offset_u64 aligned_requested_size;
    cecs_buffer_offset_u64 aligned_total_size;
} cecs_buffer_staging_parameters;

static cecs_buffer_staging_parameters cecs_dynamic_wgpu_buffer_stage(
    cecs_dynamic_wgpu_buffer *buffer,
    cecs_arena *arena,
    cecs_dynamic_buffer_offset offset,
    void *data,
    cecs_dynamic_buffer_offset size
) {
    cecs_displaced_set_set_range(
        &buffer->staging, arena, cecs_inclusive_range_index_count(offset, size), data, sizeof(cecs_buffer_staging_element)
    );
    const cecs_buffer_offset_u64 aligned_size = cecs_align_to_wgpu_copy_buffer_alignment(size);
    const size_t aligned_last = offset + aligned_size - sizeof(cecs_buffer_staging_element);

    cecs_displaced_set_expand(&buffer->staging, arena, aligned_last, sizeof(cecs_buffer_staging_element));
    const cecs_buffer_offset_u64 stage_size = cecs_exclusive_range_length(buffer->staging.index_range);
    assert(
        (stage_size & (cecs_webgpu_copy_buffer_alignment - 1)) == 0
        && "error: staging buffer size must be aligned to copy buffer alignment"
    );

    const cecs_buffer_offset_u64 requested_offset = cecs_dynamic_wgpu_buffer_get_offset(buffer, offset);
    const cecs_buffer_offset_u64 aligned_offset =
        cecs_align_to_wgpu_copy_buffer_alignment(requested_offset + 1) - cecs_webgpu_copy_buffer_alignment;
    return (cecs_buffer_staging_parameters) {
        .aligned_requested_offset = aligned_offset,
        .aligned_requested_size = aligned_size,
        .aligned_total_size = stage_size
    };
}

cecs_buffer_offset_u64 cecs_dynamic_wgpu_buffer_upload(
    cecs_dynamic_wgpu_buffer *buffer,
    WGPUDevice device,
    WGPUQueue queue,
    cecs_arena *arena,
    cecs_dynamic_buffer_offset offset,
    void *data,
    cecs_dynamic_buffer_offset size
) {
    if (buffer->uploaded_size == 0) {
        assert(false && "error: buffer must be initialized with non-zero size before uploading data");
    }

    cecs_buffer_staging_parameters staging = cecs_dynamic_wgpu_buffer_stage(buffer, arena, offset, data, size);
    if (buffer->uploaded_size < staging.aligned_requested_size) {
        WGPUBuffer new_buffer = cecs_wgpu_buffer_create_with_data(
            device,
            buffer->usage,
            staging.aligned_total_size,
            cecs_displaced_set_get(&buffer->staging, 0, sizeof(cecs_buffer_staging_element)),
            staging.aligned_total_size
        );
        wgpuBufferRelease(buffer->buffer);
        buffer->buffer = new_buffer;
        buffer->uploaded_size = staging.aligned_total_size;
    } else {
        wgpuQueueWriteBuffer(
            queue,
            buffer->buffer,
            staging.aligned_requested_offset,
            cecs_displaced_set_get_range(
                &buffer->staging, cecs_inclusive_range_index_count(offset, size), sizeof(cecs_buffer_staging_element)
            ),
            staging.aligned_requested_size
        );
    }
    return staging.aligned_requested_offset;
}
