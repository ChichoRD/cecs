#ifndef CECS_VERTEX_H
#define CECS_VERTEX_H

#include <webgpu/webgpu.h>
#include <cecs/cecs.h>

typedef uint64_t cecs_buffer_offset_u64;
typedef uint32_t cecs_buffer_offset_u32;

typedef struct cecs_vertex_stream {
    cecs_buffer_offset_u64 offset;
    cecs_buffer_offset_u64 size;
} cecs_vertex_stream;

typedef struct cecs_index_stream {
    cecs_buffer_offset_u64 offset;
    WGPUIndexFormat format;
    size_t index_count;
} cecs_index_stream;


typedef cecs_component_id cecs_vertex_attribute_id;
typedef struct cecs_vertex_attribute_reference {
    cecs_vertex_attribute_id attribute_id;
    size_t stride;
} cecs_vertex_attribute_reference;
CECS_COMPONENT_IMPLEMENT(cecs_vertex_attribute_reference);

typedef uint16_t cecs_vertex_index_u16;
CECS_COMPONENT_IMPLEMENT(cecs_vertex_index_u16);

typedef uint32_t cecs_vertex_index_u32;
CECS_COMPONENT_IMPLEMENT(cecs_vertex_index_u32);

#endif