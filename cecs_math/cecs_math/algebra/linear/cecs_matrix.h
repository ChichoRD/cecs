#ifndef CECS_MATRIX_H
#define CECS_MATRIX_H

#include "cecs_vector.h"

typedef struct cecs_mat4_f32 {
    cecs_vec4_f32 e0;
    cecs_vec4_f32 e1;
    cecs_vec4_f32 e2;
    cecs_vec4_f32 e3;
} cecs_mat4_f32;

extern const cecs_mat4_f32 cecs_mat4_f32_identity;
extern const cecs_mat4_f32 cecs_mat4_f32_zero;

cecs_mat4_f32 cecs_mat4_f32_into_columns(const float raw[16]);

inline cecs_mat4_f32 cecs_mat4_f32_mul(const cecs_mat4_f32 a, const cecs_mat4_f32 b) {
    return (cecs_mat4_f32){
        .e0 = {
            .x = a.e0.x * b.e0.x + a.e1.x * b.e0.y + a.e2.x * b.e0.z + a.e3.x * b.e0.w,
            .y = a.e0.y * b.e0.x + a.e1.y * b.e0.y + a.e2.y * b.e0.z + a.e3.y * b.e0.w,
            .z = a.e0.z * b.e0.x + a.e1.z * b.e0.y + a.e2.z * b.e0.z + a.e3.z * b.e0.w,
            .w = a.e0.w * b.e0.x + a.e1.w * b.e0.y + a.e2.w * b.e0.z + a.e3.w * b.e0.w
        },
        .e1 = {
            .x = a.e0.x * b.e1.x + a.e1.x * b.e1.y + a.e2.x * b.e1.z + a.e3.x * b.e1.w,
            .y = a.e0.y * b.e1.x + a.e1.y * b.e1.y + a.e2.y * b.e1.z + a.e3.y * b.e1.w,
            .z = a.e0.z * b.e1.x + a.e1.z * b.e1.y + a.e2.z * b.e1.z + a.e3.z * b.e1.w,
            .w = a.e0.w * b.e1.x + a.e1.w * b.e1.y + a.e2.w * b.e1.z + a.e3.w * b.e1.w
        },
        .e2 = {
            .x = a.e0.x * b.e2.x + a.e1.x * b.e2.y + a.e2.x * b.e2.z + a.e3.x * b.e2.w,
            .y = a.e0.y * b.e2.x + a.e1.y * b.e2.y + a.e2.y * b.e2.z + a.e3.y * b.e2.w,
            .z = a.e0.z * b.e2.x + a.e1.z * b.e2.y + a.e2.z * b.e2.z + a.e3.z * b.e2.w,
            .w = a.e0.w * b.e2.x + a.e1.w * b.e2.y + a.e2.w * b.e2.z + a.e3.w * b.e2.w
        },
        .e3 = {
            .x = a.e0.x * b.e3.x + a.e1.x * b.e3.y + a.e2.x * b.e3.z + a.e3.x * b.e3.w,
            .y = a.e0.y * b.e3.x + a.e1.y * b.e3.y + a.e2.y * b.e3.z + a.e3.y * b.e3.w,
            .z = a.e0.z * b.e3.x + a.e1.z * b.e3.y + a.e2.z * b.e3.z + a.e3.z * b.e3.w,
            .w = a.e0.w * b.e3.x + a.e1.w * b.e3.y + a.e2.w * b.e3.z + a.e3.w * b.e3.w
        }
    };
}

inline cecs_vec4_f32 cecs_mat4_f32_mul_vec4(const cecs_mat4_f32 m, const cecs_vec4_f32 v) {
    return (cecs_vec4_f32){
        .x = m.e0.x * v.x + m.e1.x * v.y + m.e2.x * v.z + m.e3.x * v.w,
        .y = m.e0.y * v.x + m.e1.y * v.y + m.e2.y * v.z + m.e3.y * v.w,
        .z = m.e0.z * v.x + m.e1.z * v.y + m.e2.z * v.z + m.e3.z * v.w,
        .w = m.e0.w * v.x + m.e1.w * v.y + m.e2.w * v.z + m.e3.w * v.w
    };
}
inline cecs_vec3_f32 cecs_mat4_f32_mul_vec3(const cecs_mat4_f32 m, const cecs_vec3_f32 v) {
    return (cecs_vec3_f32){
        .x = m.e0.x * v.x + m.e1.x * v.y + m.e2.x * v.z,
        .y = m.e0.y * v.x + m.e1.y * v.y + m.e2.y * v.z,
        .z = m.e0.z * v.x + m.e1.z * v.y + m.e2.z * v.z
    };
}

inline cecs_mat4_f32 cecs_mat4_f32_scale(const cecs_mat4_f32 m, const float s) {
    return (cecs_mat4_f32){
        .e0 = {m.e0.x * s, m.e0.y * s, m.e0.z * s, m.e0.w * s},
        .e1 = {m.e1.x * s, m.e1.y * s, m.e1.z * s, m.e1.w * s},
        .e2 = {m.e2.x * s, m.e2.y * s, m.e2.z * s, m.e2.w * s},
        .e3 = {m.e3.x * s, m.e3.y * s, m.e3.z * s, m.e3.w * s}
    };
}

inline float cecs_mat4_f32_trace(const cecs_mat4_f32 m) {
    return m.e0.x + m.e1.y + m.e2.z + m.e3.w;
}

inline cecs_mat4_f32 cecs_mat4_f32_transpose(const cecs_mat4_f32 m) {
    return (cecs_mat4_f32){
        .e0 = {m.e0.x, m.e1.x, m.e2.x, m.e3.x},
        .e1 = {m.e0.y, m.e1.y, m.e2.y, m.e3.y},
        .e2 = {m.e0.z, m.e1.z, m.e2.z, m.e3.z},
        .e3 = {m.e0.w, m.e1.w, m.e2.w, m.e3.w}
    };
}
#endif