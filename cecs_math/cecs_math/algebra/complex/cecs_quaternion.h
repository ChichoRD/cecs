#ifndef CECS_QUATERNION_H
#define CECS_QUATERNION_H

#include <math.h>
#include <assert.h>

typedef struct cecs_quaternion_f32 {
    float i;
    float j;
    float k;
    float r;
} cecs_quaternion_f32;
extern const cecs_quaternion_f32 cecs_quaternion_f32_identity;
extern const cecs_quaternion_f32 cecs_quaternion_f32_zero;

typedef cecs_quaternion_f32 cecs_quaternion_scalar_f32;
inline cecs_quaternion_scalar_f32 cecs_quaternion_f32_scalar(const float s) {
    return (cecs_quaternion_scalar_f32){0.f, 0.0f, 0.0f, s};
}
typedef cecs_quaternion_f32 cecs_quaternion_vector_f32;
inline cecs_quaternion_vector_f32 cecs_quaternion_f32_vector(const float i, const float j, const float k) {
    return (cecs_quaternion_vector_f32){i, j, k, 0.0f};
}

inline cecs_quaternion_f32 cecs_quaternion_f32_add(const cecs_quaternion_f32 p, const cecs_quaternion_f32 q) {
    return (cecs_quaternion_f32){p.i + q.i, p.j + q.j, p.k + q.k, p.r + q.r};
}
inline cecs_quaternion_f32 cecs_quaternion_f32_sub(const cecs_quaternion_f32 p, const cecs_quaternion_f32 q) {
    return (cecs_quaternion_f32){p.i - q.i, p.j - q.j, p.k - q.k, p.r - q.r};
}
inline cecs_quaternion_f32 cecs_quaternion_f32_mul(const cecs_quaternion_f32 p, const float s) {
    return (cecs_quaternion_f32){p.i * s, p.j * s, p.k * s, p.r * s};
}
inline cecs_quaternion_f32 cecs_quaternion_f32_div(const cecs_quaternion_f32 p, const float s) {
    assert(s != 0.0f);
    return (cecs_quaternion_f32){p.i / s, p.j / s, p.k / s, p.r / s};
}


inline cecs_quaternion_f32 cecs_quaternion_f32_product(const cecs_quaternion_f32 p, const cecs_quaternion_f32 q) {
    return (cecs_quaternion_f32){
        .i = p.r * q.i + p.i * q.r + p.j * q.k - p.k * q.j,
        .j = p.r * q.j - p.i * q.k + p.j * q.r + p.k * q.i,
        .k = p.r * q.k + p.i * q.j - p.j * q.i + p.k * q.r,
        .r = p.r * q.r - p.i * q.i - p.j * q.j - p.k * q.k
    };
}
inline cecs_quaternion_f32 cecs_quaternion_vector_f32_product(const cecs_quaternion_vector_f32 p, const cecs_quaternion_f32 q) {
    assert(p.r == 0.0f);
    return (cecs_quaternion_f32){
        .i =  p.i * q.r + p.j * q.k - p.k * q.j,
        .j = -p.i * q.k + p.j * q.r + p.k * q.i,
        .k =  p.i * q.j - p.j * q.i + p.k * q.r,
        .r = -p.i * q.i - p.j * q.j - p.k * q.k
    };
}


inline cecs_quaternion_f32 cecs_quaternion_f32_conjugate(const cecs_quaternion_f32 q) {
    return (cecs_quaternion_f32){-q.i, -q.j, -q.k, q.r};
}
inline float cecs_quaternion_f32_norm_sqr(const cecs_quaternion_f32 q) {
    return q.i * q.i + q.j * q.j + q.k * q.k + q.r * q.r;
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
    float i;
    float j;
    float k;
} cecs_versor_packed_f32;
inline cecs_versor_packed_f32 cecs_versor_packed_f32_pack(const cecs_versor_f32 uq) {
    return (cecs_versor_packed_f32){uq.i, uq.j, uq.k};
}
inline cecs_versor_f32 cecs_versor_f32_unpack(const cecs_versor_packed_f32 uq) {
    return (cecs_versor_f32){uq.i, uq.j, uq.k, sqrtf(1.0f - uq.i * uq.i - uq.j * uq.j - uq.k * uq.k)};
}

#endif