#ifndef CECS_VERTEX_H
#define CECS_VERTEX_H

#include <webgpu/webgpu.h>
#include <cecs_core/cecs_core.h>
#include "../../containers/cecs_dynamic_wgpu_buffer.h"

typedef struct cecs_raw_stream {
    cecs_buffer_offset_u64 offset;
    cecs_buffer_offset_u64 size;
} cecs_raw_stream;

typedef struct cecs_buffer_stream {
    cecs_dynamic_buffer_offset offset;
    cecs_dynamic_buffer_offset size;
} cecs_buffer_stream;

typedef cecs_buffer_stream cecs_vertex_stream;
CECS_COMPONENT_DECLARE(cecs_vertex_stream);

typedef cecs_buffer_stream cecs_instance_stream;
CECS_COMPONENT_DECLARE(cecs_instance_stream);

static inline cecs_buffer_stream cecs_buffer_stream_create(size_t first_vertex, size_t vertex_count, size_t stride) {
    return (cecs_buffer_stream){
        .offset = first_vertex * stride,
        .size = vertex_count * stride
    };
}

typedef cecs_dynamic_wgpu_buffer cecs_vertex_buffer;
typedef cecs_dynamic_wgpu_buffer cecs_instance_buffer;
typedef struct cecs_index_stream {
    size_t first_index;
    size_t index_count;
    WGPUIndexFormat format;
} cecs_index_stream;
CECS_COMPONENT_DECLARE(cecs_index_stream);

static inline cecs_buffer_stream cecs_buffer_stream_from_index_size(cecs_index_stream stream, size_t format_size) {
    return (cecs_buffer_stream){
        .offset = stream.first_index * format_size,
        .size = stream.index_count * format_size
    };
}
cecs_buffer_stream cecs_buffer_stream_from_index(cecs_index_stream stream);

typedef cecs_dynamic_wgpu_buffer cecs_index_buffer_u16;
typedef cecs_dynamic_wgpu_buffer cecs_index_buffer_u32;


typedef cecs_component_id cecs_buffer_attribute_id;
typedef cecs_buffer_attribute_id cecs_vertex_attribute_id;
typedef cecs_buffer_attribute_id cecs_instance_attribute_id;
typedef cecs_buffer_attribute_id cecs_vertex_index_id;

typedef struct cecs_buffer_attribute_reference {
    cecs_buffer_attribute_id attribute_id;
    size_t stride;
} cecs_buffer_attribute_reference;
CECS_COMPONENT_DECLARE(cecs_buffer_attribute_reference);

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
    size_t max_attribute_count;
    size_t current_attribute_count;
    size_t attribute_stride;
} cecs_attribute_storage_attachment;
typedef cecs_attribute_storage_attachment cecs_vertex_storage_attachment;
typedef cecs_attribute_storage_attachment cecs_instance_storage_attachment;

typedef struct cecs_index_storage_attachment {
    size_t max_index_count;
    size_t current_index_count;
    WGPUIndexFormat index_format;
} cecs_index_storage_attachment;

typedef struct cecs_uniform_storage_attachment {
    size_t uniform_stride;
} cecs_uniform_storage_attachment;


typedef union cecs_stream_storage_attachment {
    cecs_vertex_storage_attachment vertex;
    cecs_instance_storage_attachment instance;
    cecs_index_storage_attachment index;
    cecs_uniform_storage_attachment uniform;
} cecs_stream_storage_attachment;
typedef enum cecs_component_storage_attachment_graphics_usage {
    cecs_component_storage_attachment_usage_graphics_buffer = 1 << 1,
} cecs_component_storage_attachment_graphics_usage;


typedef union cecs_buffer_storage_attachment_buffer {
    cecs_dynamic_wgpu_buffer buffer;
    cecs_dynamic_wgpu_element_buffer element_buffer;
} cecs_buffer_storage_attachment_buffer;
typedef union cecs_buffer_storage_attachment_offsets {
    cecs_buffer_offset_u64 offset;
    cecs_dynamic_buffer_offset element_offset;
} cecs_buffer_storage_attachment_offsets;

typedef enum cecs_buffer_status {
    cecs_buffer_status_none = 0,
    cecs_buffer_status_initialized = 1 << 0,
    cecs_buffer_status_dirty = 1 << 1,
} cecs_buffer_status;
typedef enum cecs_buffer_type {
    cecs_buffer_type_none = 0,
    cecs_buffer_type_vertex = 1 << 2,
    cecs_buffer_type_instance = 1 << 3,
    cecs_buffer_type_index = 1 << 4,
    cecs_buffer_type_uniform = 1 << 5,
    
    cecs_buffer_type_dynamic = cecs_buffer_type_vertex | cecs_buffer_type_instance | cecs_buffer_type_index,
    cecs_buffer_type_dynamic_element = cecs_buffer_type_uniform,
} cecs_buffer_type;
typedef uint8_t cecs_buffer_flags;

typedef struct cecs_buffer_storage_attachment {
    cecs_buffer_storage_attachment_buffer buffer;
    cecs_stream_storage_attachment stream;
    cecs_buffer_storage_attachment_offsets offsets;
    cecs_buffer_flags buffer_flags;
} cecs_buffer_storage_attachment;

WGPUBuffer cecs_buffer_storage_attachment_get_buffer(cecs_buffer_storage_attachment *storage);

cecs_buffer_storage_attachment cecs_buffer_storage_attachment_create_vertex_uninitialized(const cecs_vertex_storage_attachment stream);
cecs_buffer_storage_attachment cecs_buffer_storage_attachment_create_instance_uninitialized(const cecs_instance_storage_attachment stream);
cecs_buffer_storage_attachment cecs_buffer_storage_attachment_create_index_uninitialized(const cecs_index_storage_attachment stream);
cecs_buffer_storage_attachment cecs_buffer_storage_attachment_create_uniform_uninitialized(const cecs_uniform_storage_attachment stream);

inline cecs_buffer_storage_attachment cecs_buffer_storage_attachment_uninitialized(const cecs_stream_storage_attachment stream, const cecs_buffer_type type) {
    return (cecs_buffer_storage_attachment){
        .stream = stream,
        .buffer = {0},
        .offsets = {0},
        .buffer_flags = type
    };
}

void cecs_buffer_storage_attachment_initialize(
    cecs_buffer_storage_attachment *storage,
    WGPUDevice device,
    cecs_arena *arena,
    const WGPUBufferUsageFlags usage,
    const size_t buffer_size,
    const uint16_t buffer_alignment
);
void cecs_buffer_storage_attachment_free(cecs_buffer_storage_attachment *storage);

void *cecs_buffer_storage_attachment_extend_dynamic(
    cecs_buffer_storage_attachment *storage,
    cecs_arena *arena,
    const cecs_dynamic_buffer_offset offset,
    const size_t size
);
void *cecs_buffer_storage_attachment_extend_dynamic_elements(
    cecs_buffer_storage_attachment *storage,
    cecs_arena *arena,
    const cecs_dynamic_buffer_offset element_index,
    const size_t element_count
);

cecs_raw_stream cecs_raw_stream_from_vertex(
    const cecs_vertex_stream stream,
    const cecs_buffer_storage_attachment *vertex_buffer
);
cecs_raw_stream cecs_raw_stream_from_instance(
    const cecs_instance_stream stream,
    const cecs_buffer_storage_attachment *instance_buffer
);

typedef struct cecs_exclusive_index_buffer_pair {
    cecs_buffer_storage_attachment *u16;
    cecs_buffer_storage_attachment *u32;
} cecs_exclusive_index_buffer_pair;
cecs_raw_stream cecs_raw_stream_from_index(
    const cecs_index_stream stream,
    cecs_exclusive_index_buffer_pair in_buffers,
    cecs_buffer_storage_attachment **out_index_buffer
);
#endif