#ifndef CECS_QUATERNION_H
#define CECS_QUATERNION_H

#include <math.h>
#include <assert.h>

typedef struct cecs_quaternion_f32 {
    float x;
    float y;
    float z;
    float w;
} cecs_quaternion_f32;
extern const cecs_quaternion_f32 cecs_quaternion_f32_identity;
extern const cecs_quaternion_f32 cecs_quaternion_f32_zero;

typedef cecs_quaternion_f32 cecs_quaternion_scalar_f32;
inline cecs_quaternion_scalar_f32 cecs_quaternion_f32_scalar(const float s) {
    return (cecs_quaternion_scalar_f32){0.f, 0.0f, 0.0f, s};
}
typedef cecs_quaternion_f32 cecs_quaternion_vector_f32;
inline cecs_quaternion_vector_f32 cecs_quaternion_f32_vector(const float x, const float y, const float z) {
    return (cecs_quaternion_vector_f32){x, y, z, 0.0f};
}

inline cecs_quaternion_f32 cecs_quaternion_f32_add(const cecs_quaternion_f32 p, const cecs_quaternion_f32 q) {
    return (cecs_quaternion_f32){p.x + q.x, p.y + q.y, p.z + q.z, p.w + q.w};
}
inline cecs_quaternion_f32 cecs_quaternion_f32_sub(const cecs_quaternion_f32 p, const cecs_quaternion_f32 q) {
    return (cecs_quaternion_f32){p.x - q.x, p.y - q.y, p.z - q.z, p.w - q.w};
}
inline cecs_quaternion_f32 cecs_quaternion_f32_mul(const cecs_quaternion_f32 p, const float s) {
    return (cecs_quaternion_f32){p.x * s, p.y * s, p.z * s, p.w * s};
}
inline cecs_quaternion_f32 cecs_quaternion_f32_div(const cecs_quaternion_f32 p, const float s) {
    assert(s != 0.0f);
    return (cecs_quaternion_f32){p.x / s, p.y / s, p.z / s, p.w / s};
}


inline cecs_quaternion_f32 cecs_quaternion_f32_product(const cecs_quaternion_f32 p, const cecs_quaternion_f32 q) {
    return (cecs_quaternion_f32){
        .x = p.w * q.x + p.x * q.w + p.y * q.z - p.z * q.y,
        .y = p.w * q.y - p.x * q.z + p.y * q.w + p.z * q.x,
        .z = p.w * q.z + p.x * q.y - p.y * q.x + p.z * q.w,
        .w = p.w * q.w - p.x * q.x - p.y * q.y - p.z * q.z
    };
}
inline cecs_quaternion_f32 cecs_quaternion_vector_f32_product(const cecs_quaternion_vector_f32 p, const cecs_quaternion_f32 q) {
    assert(p.w == 0.0f);
    return (cecs_quaternion_f32){
        .x = p.x * q.w + p.y * q.z - p.z * q.y,
        .y = -p.x * q.z + p.y * q.w + p.z * q.x,
        .z = p.x * q.y - p.y * q.x + p.z * q.w,
        .w = -p.x * q.x - p.y * q.y - p.z * q.z
    };
}


inline cecs_quaternion_f32 cecs_quaternion_f32_conjugate(const cecs_quaternion_f32 q) {
    return (cecs_quaternion_f32){-q.x, -q.y, -q.z, q.w};
}
inline float cecs_quaternion_f32_norm_sqr(const cecs_quaternion_f32 q) {
    return q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
}
inline float cecs_quaternion_f32_norm(const cecs_quaternion_f32 q) {
    return sqrtf(cecs_quaternion_f32_norm_sqr(q));
}

inline cecs_quaternion_f32 cecs_quaternion_f32_rcp_norm_sqr(const cecs_quaternion_f32 q, const float norm_sqr) {
    return cecs_quaternion_f32_div(cecs_quaternion_f32_conjugate(q), norm_sqr);
}
inline cecs_quaternion_f32 cecs_quaternion_f32_rcp(const cecs_quaternion_f32 q) {
    return cecs_quaternion_f32_rcp_norm_sqr(q, cecs_quaternion_f32_norm_sqr(q));
}


typedef cecs_quaternion_f32 cecs_versor_f32;
inline cecs_versor_f32 cecs_versor_f32_of_norm(const cecs_quaternion_f32 q, const float norm) {
    return cecs_quaternion_f32_div(q, norm);
}
inline cecs_versor_f32 cecs_versor_f32_of(const cecs_quaternion_f32 q) {
    return cecs_versor_f32_of_norm(q, cecs_quaternion_f32_norm(q));
}


typedef struct cecs_versor_packed_f32 {
    float x;
    float y;
    float z;
} cecs_versor_packed_f32;
inline cecs_versor_packed_f32 cecs_versor_packed_f32_pack(const cecs_versor_f32 uq) {
    return (cecs_versor_packed_f32){uq.x, uq.y, uq.z};
}
inline cecs_versor_f32 cecs_versor_f32_unpack(const cecs_versor_packed_f32 uq) {
    return (cecs_versor_f32){uq.x, uq.y, uq.z, sqrtf(1.0f - uq.x * uq.x - uq.y * uq.y - uq.z * uq.z)};
}

#endif