#ifndef CECS_VERTEX_H
#define CECS_VERTEX_H

#include <webgpu/webgpu.h>
#include <cecs_core/cecs_core.h>
#include "../../containers/cecs_dynamic_wgpu_buffer.h"

typedef struct cecs_raw_stream {
    cecs_buffer_offset_u64 offset;
    cecs_buffer_offset_u64 size;
} cecs_raw_stream;

typedef struct cecs_vertex_stream {
    cecs_dynamic_buffer_offset offset;
    cecs_dynamic_buffer_offset size;
} cecs_vertex_stream;
CECS_COMPONENT_DECLARE(cecs_vertex_stream);

typedef struct cecs_index_stream {
    cecs_dynamic_buffer_offset offset;
    cecs_dynamic_buffer_offset index_count;
    WGPUIndexFormat format;
} cecs_index_stream;
CECS_COMPONENT_DECLARE(cecs_index_stream);

typedef cecs_dynamic_wgpu_buffer cecs_index_buffer_u16;
typedef cecs_dynamic_wgpu_buffer cecs_index_buffer_u32;

typedef struct cecs_exclusive_index_buffer_pair {
    cecs_index_buffer_u16 *u16;
    cecs_index_buffer_u32 *u32;
} cecs_exclusive_index_buffer_pair;

cecs_raw_stream cecs_raw_stream_from_index(
    cecs_index_stream stream,
    cecs_exclusive_index_buffer_pair in_buffers,
    cecs_dynamic_wgpu_buffer **out_index_buffer
);

typedef cecs_component_id cecs_vertex_attribute_id;
typedef cecs_component_id cecs_vertex_index_id;
typedef struct cecs_vertex_attribute_reference {
    cecs_vertex_attribute_id attribute_id;
    size_t stride;
} cecs_vertex_attribute_reference;
CECS_COMPONENT_DECLARE(cecs_vertex_attribute_reference);

typedef uint16_t cecs_vertex_index_u16;
CECS_COMPONENT_DECLARE(cecs_vertex_index_u16);

typedef uint32_t cecs_vertex_index_u32;
CECS_COMPONENT_DECLARE(cecs_vertex_index_u32);

typedef struct cecs_index_format_info {
    cecs_vertex_index_id id;
    size_t size;
} cecs_index_format_info;

cecs_index_format_info cecs_index_format_info_from(WGPUIndexFormat format);

typedef struct cecs_attribute_storage_attachment {
    size_t max_vertex_count;
    size_t current_vertex_count;
    size_t attribute_size;
} cecs_vertex_storage_attachment;

typedef struct cecs_index_storage_attachment {
    size_t max_index_count;
    size_t current_index_count;
    WGPUIndexFormat index_format;
} cecs_index_storage_attachment;

typedef CECS_UNION_STRUCT(
    cecs_stream_storage_attachment,
    cecs_vertex_storage_attachment,
    cecs_vertex_storage_attachment,
    cecs_index_storage_attachment,
    cecs_index_storage_attachment
) cecs_stream_storage_attachment;

typedef struct cecs_buffer_storage_attachment {
    cecs_stream_storage_attachment stream;
    cecs_dynamic_wgpu_buffer buffer;
    enum cecs_buffer_storage_attachment_flags {
        cecs_buffer_flags_none = 0,
        cecs_buffer_flags_initialized = 1 << 0,
    } buffer_flags;
} cecs_buffer_storage_attachment;

inline cecs_buffer_storage_attachment cecs_buffer_storage_attachment_uninitialized(void) {
    return (cecs_buffer_storage_attachment){
        .stream = CECS_UNION_CREATE(
            cecs_vertex_storage_attachment,
            cecs_stream_storage_attachment,
            ((cecs_vertex_storage_attachment){ 0 })
        ),
        .buffer = cecs_dynamic_wgpu_buffer_uninitialized(),
        .buffer_flags = cecs_buffer_flags_none
    };
}

#endif