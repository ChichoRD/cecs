#include "cecs_vertex.h"

CECS_COMPONENT_DEFINE(cecs_vertex_stream);

CECS_COMPONENT_DEFINE(cecs_index_stream);

CECS_COMPONENT_DEFINE(cecs_vertex_attribute_reference);

CECS_COMPONENT_DEFINE(cecs_vertex_index_u16);
CECS_COMPONENT_DEFINE(cecs_vertex_index_u32);

cecs_raw_stream cecs_raw_stream_from_index(
    cecs_index_stream stream,
    cecs_exclusive_index_buffer_pair in_buffers,
    cecs_dynamic_wgpu_buffer **out_index_buffer
) {
    cecs_buffer_offset_u64 format_size;
    switch (stream.format) {
    case WGPUIndexFormat_Uint16: {
        assert(in_buffers.u16 != NULL && "error: index buffer not set");
        *out_index_buffer = in_buffers.u16;
        format_size = sizeof(cecs_vertex_index_u16);
        break;
    }
    case WGPUIndexFormat_Uint32: {
        assert(in_buffers.u32 != NULL && "error: index buffer not set");
        *out_index_buffer = in_buffers.u32;
        format_size = sizeof(cecs_vertex_index_u32);
        break;
    }
    default: {
        assert(false && "fatal error: index format not set");
        exit(EXIT_FAILURE);
        break;
    }
    }

    return (cecs_raw_stream){
        .offset = cecs_dynamic_wgpu_buffer_get_offset(*out_index_buffer, stream.offset),
        .size = stream.index_count * format_size
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

extern inline cecs_buffer_storage_attachment cecs_buffer_storage_attachment_uninitialized(void);