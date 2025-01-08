#include "cecs_vertex.h"

CECS_COMPONENT_DEFINE(cecs_vertex_stream);

CECS_COMPONENT_DEFINE(cecs_index_stream);

CECS_COMPONENT_DEFINE(cecs_vertex_attribute_reference);

CECS_COMPONENT_DEFINE(cecs_vertex_index_u16);
CECS_COMPONENT_DEFINE(cecs_vertex_index_u32);

cecs_raw_stream cecs_raw_stream_from_vertex(cecs_vertex_stream stream, const cecs_vertex_buffer *vertex_buffer) {
    return (cecs_raw_stream){
        .offset = cecs_dynamic_wgpu_buffer_get_offset(vertex_buffer, stream.offset),
        .size = stream.size,
    };
}

cecs_buffer_stream cecs_buffer_stream_from_index(cecs_index_stream stream)
{
    return cecs_buffer_stream_from_index_size(
        stream,
        cecs_index_format_info_from(stream.format).size
    );
}

cecs_raw_stream cecs_raw_stream_from_index(
    cecs_index_stream stream,
    cecs_exclusive_index_buffer_pair in_buffers,
    cecs_dynamic_wgpu_buffer **out_index_buffer
) {
    cecs_buffer_stream index_stream;
    switch (stream.format) {
    case WGPUIndexFormat_Uint16: {
        assert(in_buffers.u16 != NULL && "error: index buffer not set");
        *out_index_buffer = in_buffers.u16;
        index_stream = cecs_buffer_stream_from_index_size(stream, sizeof(cecs_vertex_index_u16));
        break;
    }
    case WGPUIndexFormat_Uint32: {
        assert(in_buffers.u32 != NULL && "error: index buffer not set");
        *out_index_buffer = in_buffers.u32;
        index_stream = cecs_buffer_stream_from_index_size(stream, sizeof(cecs_vertex_index_u32));
        break;
    }
    default: {
        assert(false && "fatal error: index format not set");
        exit(EXIT_FAILURE);
        break;
    }
    }

    return (cecs_raw_stream){
        .offset = cecs_dynamic_wgpu_buffer_get_offset(*out_index_buffer, index_stream.offset),
        .size = index_stream.size,
    };
}

cecs_index_format_info cecs_index_format_info_from(WGPUIndexFormat format) {
    switch (format) {
    case WGPUIndexFormat_Uint16: {
        return (cecs_index_format_info){
            .id = CECS_COMPONENT_ID(cecs_vertex_index_u16),
            .size = sizeof(cecs_vertex_index_u16),
        };
    }
    case WGPUIndexFormat_Uint32: {
        return (cecs_index_format_info){
            .id = CECS_COMPONENT_ID(cecs_vertex_index_u32),
            .size = sizeof(cecs_vertex_index_u32),
        };
    }
    default: {
        assert(false && "fatal error: invalid index format");
        exit(EXIT_FAILURE);
        return (cecs_index_format_info){0};
    }
    }
}

cecs_buffer_storage_attachment cecs_buffer_storage_attachment_create_vertex_uninitialized(cecs_vertex_storage_attachment stream) {
    return (cecs_buffer_storage_attachment){
        .stream = CECS_UNION_CREATE(
            cecs_vertex_storage_attachment,
            cecs_stream_storage_attachment,
            stream
        ),
        .buffer = cecs_dynamic_wgpu_buffer_uninitialized(),
        .buffer_flags = cecs_buffer_flags_none
    };
}

cecs_buffer_storage_attachment cecs_buffer_storage_attachment_create_index_uninitialized(cecs_index_storage_attachment stream) {
    return (cecs_buffer_storage_attachment){
        .stream = CECS_UNION_CREATE(
            cecs_index_storage_attachment,
            cecs_stream_storage_attachment,
            stream
        ),
        .buffer = cecs_dynamic_wgpu_buffer_uninitialized(),
        .buffer_flags = cecs_buffer_flags_none
    };
}

void cecs_buffer_storage_attachment_initialize(
    cecs_buffer_storage_attachment *storage,
    WGPUDevice device,
    cecs_arena *arena,
    WGPUBufferUsageFlags usage,
    size_t buffer_size
){
    assert(
        !(storage->buffer_flags & cecs_buffer_flags_initialized)
        && "error: buffer already initialized"
    );
    storage->buffer = cecs_dynamic_wgpu_buffer_create(device, arena, usage, buffer_size);
    storage->buffer_flags |= cecs_buffer_flags_initialized;
}

void cecs_buffer_storage_attachment_free(cecs_buffer_storage_attachment *storage) {
    assert(
        storage->buffer_flags & cecs_buffer_flags_initialized
        && "error: buffer already deinitalized"
    );
    
    cecs_dynamic_wgpu_buffer_free(&storage->buffer);
    storage->buffer_flags = cecs_buffer_flags_none;
}
