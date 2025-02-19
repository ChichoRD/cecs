#include <memory.h>

#include "cecs_vertex.h"

CECS_COMPONENT_DEFINE(cecs_vertex_stream);

CECS_COMPONENT_DEFINE(cecs_index_stream);

CECS_COMPONENT_DEFINE(cecs_buffer_attribute_reference);

CECS_COMPONENT_DEFINE(cecs_vertex_index_u16);
CECS_COMPONENT_DEFINE(cecs_vertex_index_u32);

cecs_buffer_stream cecs_buffer_stream_from_index(cecs_index_stream stream)
{
    return cecs_buffer_stream_from_index_size(
        stream,
        cecs_index_format_info_from(stream.format).size
    );
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

extern inline cecs_buffer_storage_attachment cecs_buffer_storage_attachment_uninitialized(const cecs_stream_storage_attachment stream, const cecs_buffer_type type);
extern inline cecs_dynamic_wgpu_buffer cecs_dynamic_wgpu_buffer_uninitialized(void);
WGPUBuffer cecs_buffer_storage_attachment_get_buffer(cecs_buffer_storage_attachment *storage) {
    assert(
        storage->buffer_flags & cecs_buffer_status_initialized
        && "error: buffer not initialized"
    );
    if (storage->buffer_flags & cecs_buffer_type_dynamic) {
        return storage->buffer.buffer.buffer;
    } else if (storage->buffer_flags & cecs_buffer_type_dynamic_element) {
        return storage->buffer.element_buffer.buffer.buffer;
    } else {
        assert(false && "fatal error: buffer type not set or invalid");
        exit(EXIT_FAILURE);
    }
}

cecs_buffer_storage_attachment cecs_buffer_storage_attachment_create_vertex_uninitialized(const cecs_vertex_storage_attachment stream) {
    return cecs_buffer_storage_attachment_uninitialized((cecs_stream_storage_attachment){.vertex = stream}, cecs_buffer_type_vertex);
}
cecs_buffer_storage_attachment cecs_buffer_storage_attachment_create_instance_uninitialized(const cecs_instance_storage_attachment stream) {
    return cecs_buffer_storage_attachment_uninitialized((cecs_stream_storage_attachment){.instance = stream}, cecs_buffer_type_instance);
}
cecs_buffer_storage_attachment cecs_buffer_storage_attachment_create_index_uninitialized(const cecs_index_storage_attachment stream) {
    return cecs_buffer_storage_attachment_uninitialized((cecs_stream_storage_attachment){.index = stream}, cecs_buffer_type_index);
}
extern inline cecs_dynamic_wgpu_element_buffer cecs_dynamic_wgpu_element_buffer_uninitialized(void);
cecs_buffer_storage_attachment cecs_buffer_storage_attachment_create_uniform_uninitialized(const cecs_uniform_storage_attachment stream) {
    return cecs_buffer_storage_attachment_uninitialized((cecs_stream_storage_attachment){.uniform = stream}, cecs_buffer_type_uniform);
}

void cecs_buffer_storage_attachment_initialize(
    cecs_buffer_storage_attachment *storage,
    WGPUDevice device,
    cecs_arena *arena,
    const WGPUBufferUsageFlags usage,
    const size_t buffer_size,
    const uint16_t buffer_alignment
){
    assert(
        !(storage->buffer_flags & cecs_buffer_status_initialized)
        && "error: buffer already initialized"
    );
    if (storage->buffer_flags & cecs_buffer_type_dynamic) {
        storage->buffer.buffer = cecs_dynamic_wgpu_buffer_create(device, arena, buffer_size, usage, buffer_alignment);
    } else if (storage->buffer_flags & cecs_buffer_type_dynamic_element) {
        storage->buffer.element_buffer = cecs_dynamic_wgpu_element_buffer_create(device, arena, buffer_size, buffer_alignment, cecs_webgpu_copy_buffer_alignment, usage);
    } else {
        assert(false && "fatal error: buffer type not set or invalid");
        exit(EXIT_FAILURE);
    }
    storage->buffer_flags |= cecs_buffer_status_initialized;
}
void cecs_buffer_storage_attachment_free(cecs_buffer_storage_attachment *storage) {
    assert(
        storage->buffer_flags & cecs_buffer_status_initialized
        && "error: buffer already deinitalized"
    );
    
    if (storage->buffer_flags & cecs_buffer_type_dynamic) {
        cecs_dynamic_wgpu_buffer_free(&storage->buffer.buffer);
    } else if (storage->buffer_flags & cecs_buffer_type_dynamic_element) {
        cecs_dynamic_wgpu_element_buffer_free(&storage->buffer.element_buffer);
    } else {
        assert(false && "fatal error: buffer type not set or invalid");
        exit(EXIT_FAILURE);
    }
    storage->buffer_flags = cecs_buffer_status_none;
}

void *cecs_buffer_storage_attachment_extend_dynamic(
    cecs_buffer_storage_attachment *storage,
    cecs_arena *arena,
    const cecs_dynamic_buffer_offset offset,
    const size_t size
) {
    assert(
        storage->buffer_flags & cecs_buffer_status_initialized
        && "error: buffer not initialized"
    );
    assert(
        storage->buffer_flags & cecs_buffer_type_dynamic
        && "error: buffer is not a dynamic buffer"
    );

    cecs_dynamic_wgpu_buffer *buffer = &storage->buffer.buffer;
    cecs_dynamic_wgpu_buffer_resize(
        buffer,
        arena,
        size + buffer->stage_size
    );
    return memmove(
        buffer->stage + offset + size,
        buffer->stage + offset,
        buffer->stage_size - offset - size
    );
}

void *cecs_buffer_storage_attachment_extend_dynamic_elements(
    cecs_buffer_storage_attachment *storage,
    cecs_arena *arena,
    const cecs_dynamic_buffer_offset element_index,
    const size_t element_count
) {
    const size_t element_size = cecs_dynamic_wgpu_element_buffer_element_size(&storage->buffer.element_buffer);
    return cecs_buffer_storage_attachment_extend_dynamic(
        storage,
        arena,
        element_index * element_size,
        element_count * element_size
    );
}

cecs_raw_stream cecs_raw_stream_from_vertex(const cecs_vertex_stream stream, const cecs_buffer_storage_attachment *vertex_buffer) {
    assert((vertex_buffer->buffer_flags & cecs_buffer_type_vertex) && "error: buffer is not a vertex buffer");
    assert((vertex_buffer->buffer_flags & cecs_buffer_type_dynamic) && "error: buffer is not a dynamic buffer");
    return (cecs_raw_stream) {
        .offset = stream.offset - vertex_buffer->offsets.offset,
        .size = stream.size
    };
}
cecs_raw_stream cecs_raw_stream_from_instance(const cecs_instance_stream stream, const cecs_buffer_storage_attachment *instance_buffer) {
    assert((instance_buffer->buffer_flags & cecs_buffer_type_instance) && "error: buffer is not an instance buffer");
    assert((instance_buffer->buffer_flags & cecs_buffer_type_dynamic) && "error: buffer is not a dynamic buffer");
    return (cecs_raw_stream) {
        .offset = stream.offset - instance_buffer->offsets.offset,
        .size = stream.size
    };
}
cecs_raw_stream cecs_raw_stream_from_index(
    const cecs_index_stream stream,
    cecs_exclusive_index_buffer_pair in_buffers,
    cecs_buffer_storage_attachment **out_index_buffer
) {
    cecs_buffer_stream index_stream;
    cecs_buffer_storage_attachment *index_buffer = NULL;
    switch (stream.format) {
    case WGPUIndexFormat_Uint16: {
        assert(in_buffers.u16 != NULL && "error: index buffer not set");
        index_buffer = in_buffers.u16;
        index_stream = cecs_buffer_stream_from_index_size(stream, sizeof(cecs_vertex_index_u16));
        break;
    }
    case WGPUIndexFormat_Uint32: {
        assert(in_buffers.u32 != NULL && "error: index buffer not set");
        index_buffer = in_buffers.u32;
        index_stream = cecs_buffer_stream_from_index_size(stream, sizeof(cecs_vertex_index_u32));
        break;
    }
    default: {
        assert(false && "fatal error: index format not set");
        exit(EXIT_FAILURE);
        break;
    }
    }

    *out_index_buffer = index_buffer;
    return (cecs_raw_stream){
        .offset = index_stream.offset - index_buffer->offsets.offset,
        .size = index_stream.size,
    };
}
