#ifndef CECS_DYNAMIC_WGPU_BUFFER_H
#define CECS_DYNAMIC_WGPU_BUFFER_H

// TODO: sparse set-like buffer staging

#include <webgpu/webgpu.h>
#include <cecs_core/cecs_core.h>
#include <stdint.h>
#include <cecs_math/arithmetic/cecs_integer_arithmetic.h>

typedef uint64_t cecs_buffer_offset_u64;
typedef uint32_t cecs_buffer_offset_u32;

typedef size_t cecs_dynamic_buffer_offset;

#define CECS_WGPU_COPY_BUFFER_ALIGNMENT_LOG2 2
#define CECS_WGPU_COPY_BUFFER_ALIGNMENT ((cecs_buffer_offset_u64)(1 << CECS_WGPU_COPY_BUFFER_ALIGNMENT_LOG2))
#define CECS_WGPU_COPY_BUFFER_ALIGNMENT_VALUE 4
static_assert(
    CECS_WGPU_COPY_BUFFER_ALIGNMENT == CECS_WGPU_COPY_BUFFER_ALIGNMENT_VALUE,
    "static error: invalid copy buffer alignment"
);

extern const cecs_buffer_offset_u64 cecs_webgpu_copy_buffer_alignment;

#define CECS_WGPU_UNIFORM_BUFFER_ALIGNMENT_LOG2 4
#define CECS_WGPU_UNIFORM_BUFFER_ALIGNMENT ((cecs_buffer_offset_u64)(1 << CECS_WGPU_UNIFORM_BUFFER_ALIGNMENT_LOG2))
#define CECS_WGPU_UNIFORM_BUFFER_ALIGNMENT_VALUE 16
static_assert(
    CECS_WGPU_UNIFORM_BUFFER_ALIGNMENT == CECS_WGPU_UNIFORM_BUFFER_ALIGNMENT_VALUE,
    "static error: invalid uniform buffer alignment"
);

#define CECS_IS_ALIGNED_TO_POW2(size, alignment) \
    ((size & ((alignment) - 1)) == 0)


#define CECS_UNIFORM_IS_ALIGNED_SIZE(size) \
    CECS_IS_ALIGNED_TO_POW2(size, CECS_WGPU_UNIFORM_BUFFER_ALIGNMENT)
#define CECS_UNIFORM_IS_ALIGNED(type) \
    CECS_UNIFORM_IS_ALIGNED_SIZE(sizeof(type))

#define CECS_UNIFORM_IS_ALIGNED_STATIC_ASSERT_INNER1(type, wgpu_u_alignment) \
    static_assert( \
        CECS_UNIFORM_IS_ALIGNED(type), \
        "static error: " #type " is not aligned to uniform buffer alignment (" #wgpu_u_alignment ")" \
    )
#define CECS_UNIFORM_IS_ALIGNED_STATIC_ASSERT_INNER0(type, wgpu_u_alignment) \
    CECS_UNIFORM_IS_ALIGNED_STATIC_ASSERT_INNER1(type, wgpu_u_alignment)
#define CECS_UNIFORM_IS_ALIGNED_STATIC_ASSERT(type) \
    CECS_UNIFORM_IS_ALIGNED_STATIC_ASSERT_INNER0(type, CECS_WGPU_UNIFORM_BUFFER_ALIGNMENT_VALUE)

extern const cecs_buffer_offset_u64 cecs_webgpu_uniform_buffer_alignment;


#define CECS_WGPU_VERTEX_STRIDE_ALIGNMENT_LOG2 2
#define CECS_WGPU_VERTEX_STRIDE_ALIGNMENT ((cecs_buffer_offset_u64)(1 << CECS_WGPU_VERTEX_STRIDE_ALIGNMENT_LOG2))
#define CECS_WGPU_VERTEX_STRIDE_ALIGNMENT_VALUE 4
static_assert(
    CECS_WGPU_VERTEX_STRIDE_ALIGNMENT == CECS_WGPU_VERTEX_STRIDE_ALIGNMENT_VALUE,
    "static error: invalid vertex stride alignment"
);

#define CECS_VERTEX_ATTRIBUTE_IS_ALIGNED_SIZE(size) \
    CECS_IS_ALIGNED_TO_POW2(size, CECS_WGPU_VERTEX_STRIDE_ALIGNMENT)
#define CECS_VERTEX_ATTRIBUTE_IS_ALIGNED(type) \
    CECS_VERTEX_ATTRIBUTE_IS_ALIGNED_SIZE(sizeof(type))

#define CECS_VERTEX_ATTRIBUTE_IS_ALIGNED_STATIC_ASSERT_INNER1(type, wgpu_v_alignment) \
    static_assert( \
        CECS_VERTEX_ATTRIBUTE_IS_ALIGNED(type), \
        "static error: " #type " is not aligned to vertex stride alignment (" #wgpu_v_alignment ")" \
    )
#define CECS_VERTEX_ATTRIBUTE_IS_ALIGNED_STATIC_ASSERT_INNER0(type, wgpu_v_alignment) \
    CECS_VERTEX_ATTRIBUTE_IS_ALIGNED_STATIC_ASSERT_INNER1(type, wgpu_v_alignment)
#define CECS_VERTEX_ATTRIBUTE_IS_ALIGNED_STATIC_ASSERT(type) \
    CECS_VERTEX_ATTRIBUTE_IS_ALIGNED_STATIC_ASSERT_INNER0(type, CECS_WGPU_VERTEX_STRIDE_ALIGNMENT_VALUE)

extern const cecs_buffer_offset_u64 cecs_webgpu_vertex_stride_alignment;

inline cecs_buffer_offset_u64 cecs_align_to_wgpu_copy_buffer_alignment(cecs_dynamic_buffer_offset size) {
    extern inline uint64_t cecs_align_to_pow2_u64(const uint64_t size, const uint64_t alignment);
    return cecs_align_to_pow2_u64(size, cecs_webgpu_copy_buffer_alignment);
}

WGPUBuffer cecs_wgpu_buffer_create_with_data(
    WGPUDevice device,
    const WGPUBufferUsage usage,
    const uint64_t buffer_size,
    const void *data,
    const size_t data_size
);

typedef uint8_t cecs_buffer_stage_value;
typedef CECS_COW_STRUCT(cecs_sparse_set, cecs_buffer_stage) cecs_buffer_stage;

typedef struct cecs_dynamic_wgpu_buffer {
    WGPUBuffer buffer;
    cecs_buffer_stage_value *stage;
    size_t stage_size;
    WGPUBufferUsageFlags usage;
    uint16_t size_alignmnent;
} cecs_dynamic_wgpu_buffer;

inline cecs_dynamic_wgpu_buffer cecs_dynamic_wgpu_buffer_uninitialized(void) {
    return (cecs_dynamic_wgpu_buffer){
        .buffer = NULL,
        .stage = NULL,
        .stage_size = 0,
        .size_alignmnent = 0,
        .usage = WGPUBufferUsage_None
    };
}

cecs_dynamic_wgpu_buffer cecs_dynamic_wgpu_buffer_create_from_stage(
    WGPUDevice device,
    cecs_buffer_stage_value *stage,
    const size_t stage_size,
    const WGPUBufferUsage usage,
    const uint16_t size_alignment
);
cecs_dynamic_wgpu_buffer cecs_dynamic_wgpu_buffer_create_from_stage_mapped(
    WGPUDevice device,
    cecs_buffer_stage_value *stage,
    const size_t stage_size,
    const WGPUBufferUsage usage,
    const uint16_t size_alignment
);
cecs_dynamic_wgpu_buffer cecs_dynamic_wgpu_buffer_create(
    WGPUDevice device,
    cecs_arena *arena,
    const size_t size,
    const WGPUBufferUsage usage,
    const uint16_t size_alignment
);

void *cecs_dynamic_wgpu_buffer_resize(
    cecs_dynamic_wgpu_buffer *buffer,
    cecs_arena *arena,
    const size_t new_size
);
void *cecs_dynamic_wgpu_buffer_stage(
    cecs_dynamic_wgpu_buffer *buffer,
    const cecs_dynamic_buffer_offset offset,
    const void *data,
    const cecs_dynamic_buffer_offset size
);
void *cecs_dynamic_wgpu_buffer_stage_or_resize(
    cecs_dynamic_wgpu_buffer *buffer,
    cecs_arena *arena,
    const cecs_dynamic_buffer_offset offset,
    const void *data,
    const cecs_dynamic_buffer_offset size
);

cecs_dynamic_buffer_offset cecs_dynamic_wgpu_buffer_upload(
    cecs_dynamic_wgpu_buffer *buffer,
    WGPUDevice device,
    WGPUQueue queue,
    cecs_arena *arena,
    const cecs_dynamic_buffer_offset offset,
    const cecs_dynamic_buffer_offset size
);
cecs_dynamic_buffer_offset cecs_dynamic_wgpu_buffer_upload_all(
    cecs_dynamic_wgpu_buffer *buffer,
    WGPUDevice device,
    WGPUQueue queue,
    cecs_arena *arena
);

void cecs_dynamic_wgpu_buffer_free(cecs_dynamic_wgpu_buffer *buffer);


typedef struct cecs_dynamic_wgpu_element_buffer {
    cecs_dynamic_wgpu_buffer buffer;
    uint8_t aligned_element_size_log2;
} cecs_dynamic_wgpu_element_buffer;

inline size_t cecs_dynamic_wgpu_element_buffer_element_size(const cecs_dynamic_wgpu_element_buffer *buffer) {
    return ((size_t)1 << (size_t)buffer->aligned_element_size_log2);
}
inline size_t cecs_dynamic_wgpu_element_buffer_element_count(const cecs_dynamic_wgpu_element_buffer *buffer) {
    return buffer->buffer.stage_size >> buffer->aligned_element_size_log2;
}

inline cecs_dynamic_wgpu_element_buffer cecs_dynamic_wgpu_element_buffer_uninitialized(void) {
    return (cecs_dynamic_wgpu_element_buffer){
        .buffer = cecs_dynamic_wgpu_buffer_uninitialized(),
        .aligned_element_size_log2 = 0
    };
}

cecs_dynamic_wgpu_element_buffer cecs_dynamic_wgpu_element_buffer_create(
    WGPUDevice device,
    cecs_arena *arena,
    const size_t element_size,
    const uint16_t element_offset_alignment,
    const uint16_t upload_alignment,
    const WGPUBufferUsageFlags usage
);
void cecs_dynamic_wgpu_element_buffer_free(cecs_dynamic_wgpu_element_buffer *buffer);

inline cecs_buffer_offset_u64 cecs_dynamic_wgpu_element_buffer_element_offset(
    const cecs_dynamic_wgpu_element_buffer *buffer,
    const cecs_dynamic_buffer_offset element_index
) {
    return element_index << buffer->aligned_element_size_log2;
}
inline cecs_dynamic_buffer_offset cecs_dynamic_wgpu_element_buffer_element_index(
    const cecs_dynamic_wgpu_element_buffer *buffer,
    const cecs_buffer_offset_u64 element_offset
)  {
    return element_offset >> buffer->aligned_element_size_log2;
}
inline cecs_buffer_offset_u64 cecs_dynamic_wgpu_element_buffer_element_offset_start(
    const cecs_dynamic_wgpu_element_buffer *buffer,
    const cecs_buffer_offset_u64 element_inner_offset
) {
    return cecs_dynamic_wgpu_element_buffer_element_offset(
        buffer,
        cecs_dynamic_wgpu_element_buffer_element_index(buffer, element_inner_offset)
    );
}

cecs_buffer_offset_u64 cecs_dynamic_wgpu_element_buffer_stage(
    cecs_dynamic_wgpu_element_buffer *buffer,
    cecs_arena *arena,
    const cecs_dynamic_buffer_offset element_index,
    const cecs_buffer_offset_u64 suboffset,
    const void *data,
    const size_t size
);
cecs_buffer_offset_u64 cecs_dynamic_wgpu_element_buffer_stage_range(
    cecs_dynamic_wgpu_element_buffer *buffer,
    cecs_arena *arena,
    const cecs_dynamic_buffer_offset element_index,
    const cecs_dynamic_buffer_offset suboffset,
    const void *data,
    const size_t count,
    const size_t size
);

cecs_buffer_offset_u64 cecs_dynamic_wgpu_element_buffer_upload(
    cecs_dynamic_wgpu_element_buffer *buffer,
    WGPUDevice device,
    WGPUQueue queue,
    cecs_arena *arena,
    const cecs_dynamic_buffer_offset element_index,
    const cecs_buffer_offset_u64 suboffset,
    const size_t size
);
cecs_buffer_offset_u64 cecs_dynamic_wgpu_element_buffer_upload_range(
    cecs_dynamic_wgpu_element_buffer *buffer,
    WGPUDevice device,
    WGPUQueue queue,
    cecs_arena *arena,
    const cecs_dynamic_buffer_offset element_index,
    const size_t count
);
cecs_buffer_offset_u64 cecs_dynamic_wgpu_element_buffer_upload_all(
    cecs_dynamic_wgpu_element_buffer *buffer,
    WGPUDevice device,
    WGPUQueue queue,
    cecs_arena *arena
);


#endif