#include <stdbool.h>
#include <assert.h>
#include <webgpu/wgpu.h>
#include <stdlib.h>
#include <memory.h>
#include "cecs_dynamic_wgpu_buffer.h"

const cecs_buffer_offset_u64 cecs_webgpu_copy_buffer_alignment = CECS_WGPU_COPY_BUFFER_ALIGNMENT;
const cecs_buffer_offset_u64 cecs_webgpu_uniform_buffer_alignment = CECS_WGPU_UNIFORM_BUFFER_ALIGNMENT;

extern inline cecs_buffer_offset_u64 cecs_align_to_pow2(cecs_dynamic_buffer_offset size, cecs_buffer_offset_u64 align);
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

static cecs_dynamic_wgpu_buffer cecs_dynamic_wgpu_buffer_create_no_stage(
    WGPUDevice device,
    WGPUBufferUsageFlags usage,
    cecs_dynamic_buffer_offset size,
    uint16_t alignment,
    uint64_t *out_aligned_size
) {
    assert(size != 0 && "error: buffer size must be non-zero");
    assert(alignment != 0 && "error: buffer alignment must be non-zero");
    assert((alignment & (alignment - 1)) == 0 && "error: buffer alignment must be a power of two");

    *out_aligned_size = cecs_align_to_pow2(size, alignment);
    return (cecs_dynamic_wgpu_buffer){
        .buffer = wgpuDeviceCreateBuffer(
            device,
            &(WGPUBufferDescriptor){
                .label = "dynamic buffer",
                .mappedAtCreation = false,
                .nextInChain = NULL,
                .size = *out_aligned_size,
                .usage = usage
            }
        ),
        .stage = {0},
        .alignment = alignment,
        .uploaded_size = *out_aligned_size,
        .current_padding = 0,
        .usage = usage
    };
}

cecs_dynamic_wgpu_buffer cecs_dynamic_wgpu_buffer_create_owned(
    WGPUDevice device,
    cecs_arena *arena,
    WGPUBufferUsageFlags usage,
    cecs_dynamic_buffer_offset size,
    uint16_t alignment
) {
    uint64_t aligned_size;
    cecs_dynamic_wgpu_buffer buffer = cecs_dynamic_wgpu_buffer_create_no_stage(device, usage, size, alignment, &aligned_size);
    buffer.stage = CECS_COW_CREATE_OWNED_STRUCT(
        cecs_buffer_stage,
        cecs_sparse_set_create_with_capacity(arena, aligned_size, sizeof(cecs_buffer_stage_element))
    );
    return buffer;
}

cecs_dynamic_wgpu_buffer cecs_dynamic_wgpu_buffer_create_borrowed(
    WGPUDevice device,
    cecs_sparse_set *stage,
    WGPUBufferUsageFlags usage,
    cecs_dynamic_buffer_offset size,
    uint16_t alignment
) {
    uint64_t aligned_size;
    cecs_dynamic_wgpu_buffer buffer = cecs_dynamic_wgpu_buffer_create_no_stage(device, usage, size, alignment, &aligned_size);
    buffer.stage = CECS_COW_CREATE_BORROWED_STRUCT(cecs_buffer_stage, stage);
    return buffer;
}

static inline cecs_sparse_set *cecs_dynamic_wgpu_buffer_get_stage(cecs_dynamic_wgpu_buffer *buffer) {
    return CECS_COW_GET_REFERENCE(cecs_buffer_stage, buffer->stage);
}

void cecs_dynamic_wgpu_buffer_free(cecs_dynamic_wgpu_buffer *buffer) {
    wgpuBufferRelease(buffer->buffer);
    buffer->buffer = NULL;
    if (CECS_COW_IS_OWNED(cecs_buffer_stage, buffer->stage)) {
        cecs_sparse_set_clear(&CECS_COW_GET_OWNED_UNCHECKED(cecs_buffer_stage, buffer->stage));
    }
    buffer->alignment = 0;
    buffer->uploaded_size = 0;
    buffer->current_padding = 0;
    buffer->usage = WGPUBufferUsage_None;
}

cecs_buffer_offset_u64 cecs_dynamic_wgpu_buffer_get_offset(const cecs_dynamic_wgpu_buffer *buffer, cecs_dynamic_buffer_offset offset) {
    return cecs_sparse_set_index_unchecked(&buffer->stage, offset);
}

typedef struct cecs_buffer_staging_parameters {
    cecs_buffer_offset_u64 requested_offset;
    cecs_buffer_offset_u64 aligned_requested_offset;
    cecs_buffer_offset_u64 aligned_requested_size;
    cecs_buffer_offset_u64 aligned_total_size;
} cecs_buffer_staging_parameters;

static size_t cecs_dynamic_wgpu_buffer_unpad(cecs_dynamic_wgpu_buffer *buffer, cecs_arena *arena, size_t current_stage_size) {
    if (buffer->current_padding > 0) {
        cecs_dynamic_array *stage_values = &cecs_dynamic_wgpu_buffer_get_stage(buffer)->base.values;
        cecs_dynamic_array_remove_range(
            stage_values, arena, current_stage_size - buffer->current_padding, buffer->current_padding, sizeof(cecs_buffer_stage_element)
        );
    }
    return 0;
}

static size_t cecs_dynamic_wgpu_buffer_pad(cecs_dynamic_wgpu_buffer *buffer, cecs_arena *arena, size_t padding) {
    if (padding > 0) {
        cecs_dynamic_array *stage_values = &cecs_dynamic_wgpu_buffer_get_stage(buffer)->base.values;
        cecs_dynamic_array_append_empty(stage_values, arena, padding, sizeof(cecs_buffer_stage_element));
    }
    return padding;
}

static cecs_buffer_staging_parameters cecs_dynamic_wgpu_buffer_stage(
    cecs_dynamic_wgpu_buffer *buffer,
    cecs_arena *arena,
    cecs_dynamic_buffer_offset offset,
    void *data,
    cecs_dynamic_buffer_offset size,
    cecs_sparse_set **out_stage
) {
    cecs_sparse_set *stage = cecs_dynamic_wgpu_buffer_get_stage(buffer);
    const size_t stage_size = cecs_sparse_set_count_of_size(stage, sizeof(cecs_buffer_stage_element));
    buffer->current_padding = cecs_dynamic_wgpu_buffer_unpad(buffer, arena, stage_size);

    cecs_sparse_set_set_range(stage, arena, offset, data, size, sizeof(cecs_buffer_stage_element));
    const size_t new_stage_size = cecs_sparse_set_count_of_size(stage, sizeof(cecs_buffer_stage_element));
    const cecs_buffer_offset_u64 aligned_size = cecs_align_to_pow2(size, buffer->alignment);
    if (new_stage_size > stage_size) {
        const size_t padding = aligned_size - size;
        buffer->current_padding = cecs_dynamic_wgpu_buffer_pad(buffer, arena, padding);
    } else {
        assert(new_stage_size == stage_size && "error: staging buffer size must be aligned to copy buffer alignment");
    }

    const cecs_buffer_offset_u64 new_padded_stage_size =
        cecs_sparse_set_count_of_size(stage, sizeof(cecs_buffer_stage_element));
    assert(
        (new_padded_stage_size & (cecs_webgpu_copy_buffer_alignment - 1)) == 0
        && "error: staging buffer size must be aligned to copy buffer alignment"
    );

    const cecs_buffer_offset_u64 requested_offset = cecs_dynamic_wgpu_buffer_get_offset(buffer, offset);
    const cecs_buffer_offset_u64 aligned_offset =
        cecs_align_to_pow2(requested_offset + 1, buffer->alignment) - cecs_webgpu_copy_buffer_alignment;

    *out_stage = stage;
    return (cecs_buffer_staging_parameters) {
        .requested_offset = requested_offset,
        .aligned_requested_offset = aligned_offset,
        .aligned_requested_size = aligned_size,
        .aligned_total_size = new_padded_stage_size
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

    cecs_sparse_set *stage;
    cecs_buffer_staging_parameters staging = cecs_dynamic_wgpu_buffer_stage(buffer, arena, offset, data, size, &stage);
    if (buffer->uploaded_size < staging.aligned_requested_offset + staging.aligned_requested_size) {
        const uint64_t new_buffer_size = staging.aligned_total_size * 2;
        WGPUBuffer new_buffer = cecs_wgpu_buffer_create_with_data(
            device,
            buffer->usage,
            new_buffer_size,
            cecs_sparse_set_values(stage),
            staging.aligned_total_size
        );
        wgpuBufferRelease(buffer->buffer);
        buffer->buffer = new_buffer;
        buffer->uploaded_size = new_buffer_size;
    } else {
        wgpuQueueWriteBuffer(
            queue,
            buffer->buffer,
            staging.aligned_requested_offset,
            cecs_sparse_set_get_unchecked(stage, offset, sizeof(cecs_buffer_stage_element)),
            staging.aligned_requested_size
        );
    }

    if (CECS_COW_IS_BORROWED(cecs_buffer_stage, buffer->stage)) {
        buffer->current_padding = cecs_dynamic_wgpu_buffer_unpad(buffer, arena, staging.aligned_total_size);
    }
    return staging.requested_offset;
}
