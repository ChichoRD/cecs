#ifndef CECS_DYNAMIC_WGPU_BUFFER_H
#define CECS_DYNAMIC_WGPU_BUFFER_H

#include <webgpu/webgpu.h>
#include <cecs_core/cecs_core.h>
#include <stdint.h>

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

inline cecs_buffer_offset_u64 cecs_align_to_pow2(cecs_dynamic_buffer_offset size, cecs_buffer_offset_u64 align) {
    const cecs_buffer_offset_u64 align_mask = align - 1;
    const cecs_buffer_offset_u64 aligned_size = (size + align_mask) & ~align_mask;
    return max(aligned_size, align);
}

inline cecs_buffer_offset_u64 cecs_align_to_wgpu_copy_buffer_alignment(cecs_dynamic_buffer_offset size) {
    extern inline cecs_buffer_offset_u64 cecs_align_to_pow2(cecs_dynamic_buffer_offset size, cecs_buffer_offset_u64 align);
    return cecs_align_to_pow2(size, cecs_webgpu_copy_buffer_alignment);
}

WGPUBuffer cecs_wgpu_buffer_create_with_data(
    WGPUDevice device,
    WGPUBufferUsageFlags usage,
    uint64_t buffer_size,
    void *data,
    size_t data_size
);

typedef uint8_t cecs_buffer_stage_element;
typedef CECS_COW_STRUCT(cecs_sparse_set, cecs_buffer_stage) cecs_buffer_stage;

typedef struct cecs_dynamic_wgpu_buffer {
    WGPUBuffer buffer;
    cecs_buffer_stage stage;
    uint16_t alignment;
    size_t uploaded_size;
    size_t current_padding;
    WGPUBufferUsage usage;
} cecs_dynamic_wgpu_buffer;

static inline cecs_dynamic_wgpu_buffer cecs_dynamic_wgpu_buffer_uninitialized(void) {
    return (cecs_dynamic_wgpu_buffer){
        .buffer = NULL,
        .stage = CECS_COW_CREATE_OWNED(cecs_buffer_stage, cecs_sparse_set_create()),
        .alignment = 0,
        .uploaded_size = 0,
        .current_padding = 0,
        .usage = WGPUBufferUsage_None
    };
}

cecs_dynamic_wgpu_buffer cecs_dynamic_wgpu_buffer_create_owned(
    WGPUDevice device,
    cecs_arena *arena,
    WGPUBufferUsageFlags usage,
    cecs_dynamic_buffer_offset size,
    uint16_t alignment
);

cecs_dynamic_wgpu_buffer cecs_dynamic_wgpu_buffer_create_borrowed(
    WGPUDevice device,
    cecs_sparse_set *stage,
    WGPUBufferUsageFlags usage,
    cecs_dynamic_buffer_offset size,
    uint16_t alignment
);

inline cecs_sparse_set *cecs_dynamic_wgpu_buffer_get_stage(cecs_dynamic_wgpu_buffer *buffer) {
    return CECS_COW_GET_REFERENCE(cecs_buffer_stage, buffer->stage);
}

static inline bool cecs_dynamic_wgpu_buffer_is_shared(const cecs_dynamic_wgpu_buffer *buffer) {
    return CECS_COW_IS_BORROWED(cecs_buffer_stage, buffer->stage);
}

void cecs_dynamic_wgpu_buffer_free(cecs_dynamic_wgpu_buffer *buffer);

cecs_buffer_offset_u64 cecs_dynamic_wgpu_buffer_get_offset(const cecs_dynamic_wgpu_buffer *buffer, cecs_dynamic_buffer_offset offset);

cecs_buffer_offset_u64 cecs_dynamic_wgpu_buffer_stage(
    cecs_dynamic_wgpu_buffer *buffer,
    cecs_arena *arena,
    cecs_dynamic_buffer_offset offset,
    void *data,
    cecs_dynamic_buffer_offset size
);
cecs_buffer_offset_u64 cecs_dynamic_wgpu_buffer_upload(
    cecs_dynamic_wgpu_buffer *buffer,
    WGPUDevice device,
    WGPUQueue queue,
    cecs_arena *arena,
    cecs_dynamic_buffer_offset offset,
    cecs_dynamic_buffer_offset size
);
cecs_buffer_offset_u64 cecs_dynamic_wgpu_buffer_upload_all(
    cecs_dynamic_wgpu_buffer *buffer,
    WGPUDevice device,
    WGPUQueue queue,
    cecs_arena *arena
);
cecs_buffer_offset_u64 cecs_dynamic_wgpu_buffer_stage_and_upload(
    cecs_dynamic_wgpu_buffer *buffer,
    WGPUDevice device,
    WGPUQueue queue,
    cecs_arena *arena,
    cecs_dynamic_buffer_offset offset,
    void *data,
    cecs_dynamic_buffer_offset size
);

#endif