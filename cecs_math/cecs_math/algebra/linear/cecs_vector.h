#ifndef CECS_VECTOR_H
#define CECS_VECTOR_H

#include <math.h>
#include "cecs_vector_def.h"

// TODO: cecs_core replace all `static inline` -> `inline` + `extern inline`

// TODO: geometric algebra extension

#define CECS_VEC2_F32_ZERO {0.0f, 0.0f}
#define CECS_VEC2_I32_ZERO {0, 0}
#define CECS_VEC2_U32_ZERO {0, 0}

#define CECS_VEC3_F32_ZERO {0.0f, 0.0f, 0.0f}
#define CECS_VEC3_I32_ZERO {0, 0, 0}
#define CECS_VEC3_U32_ZERO {0, 0, 0}

#define CECS_VEC4_F32_ZERO {0.0f, 0.0f, 0.0f, 0.0f}

extern const cecs_vec2_f32 cecs_vec2_f32_zero;
extern const cecs_vec2_i32 cecs_vec2_i32_zero;
extern const cecs_vec2_u32 cecs_vec2_u32_zero;

extern const cecs_vec3_f32 cecs_vec3_f32_zero;
extern const cecs_vec3_i32 cecs_vec3_i32_zero;
extern const cecs_vec3_u32 cecs_vec3_u32_zero;

extern const cecs_vec4_f32 cecs_vec4_f32_zero;

inline cecs_vec2_f32 cecs_vec2_f32_add(const cecs_vec2_f32 u, const cecs_vec2_f32 v) {
    return (cecs_vec2_f32){u.x + v.x, u.y + v.y};
}
inline cecs_vec2_i32 cecs_vec2_i32_add(const cecs_vec2_i32 u, const cecs_vec2_i32 v) {
    return (cecs_vec2_i32){u.x + v.x, u.y + v.y};
}
inline cecs_vec2_u32 cecs_vec2_u32_add(const cecs_vec2_u32 u, const cecs_vec2_u32 v) {
    return (cecs_vec2_u32){u.x + v.x, u.y + v.y};
}

inline cecs_vec3_f32 cecs_vec3_f32_add(const cecs_vec3_f32 u, const cecs_vec3_f32 v) {
    return (cecs_vec3_f32){u.x + v.x, u.y + v.y, u.z + v.z};
}
inline cecs_vec3_i32 cecs_vec3_i32_add(const cecs_vec3_i32 u, const cecs_vec3_i32 v) {
    return (cecs_vec3_i32){u.x + v.x, u.y + v.y, u.z + v.z};
}
inline cecs_vec3_u32 cecs_vec3_u32_add(const cecs_vec3_u32 u, const cecs_vec3_u32 v) {
    return (cecs_vec3_u32){u.x + v.x, u.y + v.y, u.z + v.z};
}

inline cecs_vec4_f32 cecs_vec4_f32_add(const cecs_vec4_f32 u, const cecs_vec4_f32 v) {
    return (cecs_vec4_f32){u.x + v.x, u.y + v.y, u.z + v.z, u.w + v.w};
}


inline cecs_vec2_f32 cecs_vec2_f32_sub(const cecs_vec2_f32 u, const cecs_vec2_f32 v) {
    return (cecs_vec2_f32){u.x - v.x, u.y - v.y};
}
inline cecs_vec2_i32 cecs_vec2_i32_sub(const cecs_vec2_i32 u, const cecs_vec2_i32 v) {
    return (cecs_vec2_i32){u.x - v.x, u.y - v.y};
}
inline cecs_vec2_u32 cecs_vec2_u32_sub(const cecs_vec2_u32 u, const cecs_vec2_u32 v) {
    return (cecs_vec2_u32){u.x - v.x, u.y - v.y};
}

inline cecs_vec3_f32 cecs_vec3_f32_sub(const cecs_vec3_f32 u, const cecs_vec3_f32 v) {
    return (cecs_vec3_f32){u.x - v.x, u.y - v.y, u.z - v.z};
}
inline cecs_vec3_i32 cecs_vec3_i32_sub(const cecs_vec3_i32 u, const cecs_vec3_i32 v) {
    return (cecs_vec3_i32){u.x - v.x, u.y - v.y, u.z - v.z};
}
inline cecs_vec3_u32 cecs_vec3_u32_sub(const cecs_vec3_u32 u, const cecs_vec3_u32 v) {
    return (cecs_vec3_u32){u.x - v.x, u.y - v.y, u.z - v.z};
}

inline cecs_vec4_f32 cecs_vec4_f32_sub(const cecs_vec4_f32 u, const cecs_vec4_f32 v) {
    return (cecs_vec4_f32){u.x - v.x, u.y - v.y, u.z - v.z, u.w - v.w};
}


inline cecs_vec2_f32 cecs_vec2_f32_mul(const cecs_vec2_f32 u, const float s) {
    return (cecs_vec2_f32){u.x * s, u.y * s};
}
inline cecs_vec2_i32 cecs_vec2_i32_mul(const cecs_vec2_i32 u, const int32_t s) {
    return (cecs_vec2_i32){u.x * s, u.y * s};
}
inline cecs_vec2_u32 cecs_vec2_u32_mul(const cecs_vec2_u32 u, const uint32_t s) {
    return (cecs_vec2_u32){u.x * s, u.y * s};
}

inline cecs_vec3_f32 cecs_vec3_f32_mul(const cecs_vec3_f32 u, const float s) {
    return (cecs_vec3_f32){u.x * s, u.y * s, u.z * s};
}
inline cecs_vec3_i32 cecs_vec3_i32_mul(const cecs_vec3_i32 u, const int32_t s) {
    return (cecs_vec3_i32){u.x * s, u.y * s, u.z * s};
}
inline cecs_vec3_u32 cecs_vec3_u32_mul(const cecs_vec3_u32 u, const uint32_t s) {
    return (cecs_vec3_u32){u.x * s, u.y * s, u.z * s};
}

inline cecs_vec4_f32 cecs_vec4_f32_mul(const cecs_vec4_f32 u, const float s) {
    return (cecs_vec4_f32){u.x * s, u.y * s, u.z * s, u.w * s};
}


inline cecs_vec2_f32 cecs_vec2_f32_div(const cecs_vec2_f32 u, const float s) {
    assert(s != 0.0f);
    return (cecs_vec2_f32){u.x / s, u.y / s};
}
inline cecs_vec2_i32 cecs_vec2_i32_div(const cecs_vec2_i32 u, const int32_t s) {
    assert(s != 0);
    return (cecs_vec2_i32){u.x / s, u.y / s};
}
inline cecs_vec2_u32 cecs_vec2_u32_div(const cecs_vec2_u32 u, const uint32_t s) {
    assert(s != 0);
    return (cecs_vec2_u32){u.x / s, u.y / s};
}

inline cecs_vec3_f32 cecs_vec3_f32_div(const cecs_vec3_f32 u, const float s) {
    assert(s != 0.0f);
    return (cecs_vec3_f32){u.x / s, u.y / s, u.z / s};
}
inline cecs_vec3_i32 cecs_vec3_i32_div(const cecs_vec3_i32 u, const int32_t s) {
    assert(s != 0);
    return (cecs_vec3_i32){u.x / s, u.y / s, u.z / s};
}
inline cecs_vec3_u32 cecs_vec3_u32_div(const cecs_vec3_u32 u, const uint32_t s) {
    assert(s != 0);
    return (cecs_vec3_u32){u.x / s, u.y / s, u.z / s};
}

inline cecs_vec4_f32 cecs_vec4_f32_div(const cecs_vec4_f32 u, const float s) {
    assert(s != 0.0f);
    return (cecs_vec4_f32){u.x / s, u.y / s, u.z / s, u.w / s};
}


inline float cecs_vec2_f32_dot(const cecs_vec2_f32 u, const cecs_vec2_f32 v) {
    return u.x * v.x + u.y * v.y;
}
inline int32_t cecs_vec2_i32_dot(const cecs_vec2_i32 u, const cecs_vec2_i32 v) {
    return u.x * v.x + u.y * v.y;
}
inline uint32_t cecs_vec2_u32_dot(const cecs_vec2_u32 u, const cecs_vec2_u32 v) {
    return u.x * v.x + u.y * v.y;
}

inline float cecs_vec3_f32_dot(const cecs_vec3_f32 u, const cecs_vec3_f32 v) {
    return u.x * v.x + u.y * v.y + u.z * v.z;
}
inline int32_t cecs_vec3_i32_dot(const cecs_vec3_i32 u, const cecs_vec3_i32 v) {
    return u.x * v.x + u.y * v.y + u.z * v.z;
}
inline uint32_t cecs_vec3_u32_dot(const cecs_vec3_u32 u, const cecs_vec3_u32 v) {
    return u.x * v.x + u.y * v.y + u.z * v.z;
}

inline float cecs_vec4_f32_dot(const cecs_vec4_f32 u, const cecs_vec4_f32 v) {
    return u.x * v.x + u.y * v.y + u.z * v.z + u.w * v.w;
}


inline cecs_vec3_f32 cecs_vec3_f32_cross(const cecs_vec3_f32 u, const cecs_vec3_f32 v) {
    return (cecs_vec3_f32){
        .x = u.y * v.z - u.z * v.y,
        .y = u.z * v.x - u.x * v.z,
        .z = u.x * v.y - u.y * v.x
    };
}


inline float cecs_vec2_f32_length(const cecs_vec2_f32 u) {
    return sqrtf(cecs_vec2_f32_dot(u, u));
}
inline float cecs_vec3_f32_length(const cecs_vec3_f32 u) {
    return sqrtf(cecs_vec3_f32_dot(u, u));
}
inline float cecs_vec4_f32_length(const cecs_vec4_f32 u) {
    return sqrtf(cecs_vec4_f32_dot(u, u));
}


inline cecs_vec2_f32 cecs_vec2_f32_normalize_length(const cecs_vec2_f32 u, const float length) {
    return cecs_vec2_f32_div(u, length);
}
inline cecs_vec3_f32 cecs_vec3_f32_normalize_length(const cecs_vec3_f32 u, const float length) {
    return cecs_vec3_f32_div(u, length);
}
inline cecs_vec4_f32 cecs_vec4_f32_normalize_length(const cecs_vec4_f32 u, const float length) {
    return cecs_vec4_f32_div(u, length);
}
inline cecs_vec2_f32 cecs_vec2_f32_normalize(const cecs_vec2_f32 u) {
    return cecs_vec2_f32_normalize_length(u, cecs_vec2_f32_length(u));
}
inline cecs_vec3_f32 cecs_vec3_f32_normalize(const cecs_vec3_f32 u) {
    return cecs_vec3_f32_normalize_length(u, cecs_vec3_f32_length(u));
}
inline cecs_vec4_f32 cecs_vec4_f32_normalize(const cecs_vec4_f32 u) {
    return cecs_vec4_f32_normalize_length(u, cecs_vec4_f32_length(u));
}
#endif