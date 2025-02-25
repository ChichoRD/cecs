#ifndef CECS_SPATIAL_H
#define CECS_SPATIAL_H

#include "complex/cecs_quaternion.h"
#include "linear/cecs_vector.h"
#include "linear/cecs_matrix.h"

#include "../cecs_units.h"


// TODO: cecs_core; separate unique relationship addition to own function

inline cecs_versor_f32 cecs_versor_f32_axis_angle(const cecs_vec3_f32 axis, const cecs_radians_f32 angle) {
    const cecs_radians_f32 half_angle = angle * 0.5f;
    const float s = sinf(half_angle);
    return (cecs_versor_f32){
        .i = axis.x * s,
        .j = axis.y * s,
        .k = axis.z * s,
        .r = cosf(half_angle)
    };
}

inline cecs_vec3_f32 cecs_versor_f32_rotate(const cecs_versor_f32 uq, const cecs_vec3_f32 v) {
    const cecs_vec3_f32 u = cecs_vec3_f32_add(
        cecs_vec3_f32_cross(
            (cecs_vec3_f32){uq.i, uq.j, uq.k},
            v
        ),
        cecs_vec3_f32_mul(
            v,
            uq.r * 2.0f
        )
    );
    return cecs_vec3_f32_add(
        v,
        cecs_vec3_f32_cross((cecs_vec3_f32){uq.i, uq.j, uq.k}, u)
    );
}

// source: adapted from https://stackoverflow.com/a/11741520
inline cecs_quaternion_f32 cecs_quaternion_f32_arc(const cecs_vec3_f32 to, const cecs_vec3_f32 from) {
    const float k_cos_theta = cecs_vec3_f32_dot(from, to);
    const float k = sqrtf(cecs_vec3_f32_dot(from, from) * cecs_vec3_f32_dot(to, to));

    const cecs_vec3_f32 axis = cecs_vec3_f32_cross(from, to);
    return (cecs_quaternion_f32){
        .i = axis.x,
        .j = axis.y,
        .k = axis.z,
        .r = k_cos_theta + k
    };
}
inline cecs_versor_f32 cecs_versor_f32_arc(const cecs_vec3_f32 to, const cecs_vec3_f32 from) {
    const float k_cos_theta = cecs_vec3_f32_dot(from, to);
    const float k = sqrtf(cecs_vec3_f32_dot(from, from) * cecs_vec3_f32_dot(to, to));

    const cecs_vec3_f32 axis = cecs_vec3_f32_cross(from, to);
    return cecs_versor_f32_of_norm(
        (cecs_quaternion_f32){
            .i = axis.x,
            .j = axis.y,
            .k = axis.z,
            .r = k_cos_theta
        },
        k
    );
}


typedef cecs_vec4_f32 cecs_hcoord4_f32;

typedef cecs_vec3_f32 cecs_hvec3_f32;
typedef cecs_vec3_f32 cecs_hpoint3_f32;

inline cecs_hpoint3_f32 cecs_hpoint3_f32_pack(const cecs_hcoord4_f32 coord) {
    assert(coord.w != 0.0f);
    return (cecs_hpoint3_f32){
        .x = coord.x / coord.w,
        .y = coord.y / coord.w,
        .z = coord.z / coord.w
    };
}
inline cecs_hvec3_f32 cecs_hvec3_f32_pack(const cecs_hcoord4_f32 coord) {
    assert(coord.w == 0.0f);
    return (cecs_hvec3_f32){
        .x = coord.x,
        .y = coord.y,
        .z = coord.z
    };
}
inline cecs_hpoint3_f32 cecs_hpoint3_f32_pack_ptr(const cecs_hcoord4_f32 *coord) {
    assert(coord->w != 0.0f);
    return (cecs_hpoint3_f32){
        .x = coord->x / coord->w,
        .y = coord->y / coord->w,
        .z = coord->z / coord->w
    };
}
inline cecs_hvec3_f32 cecs_hvec3_f32_pack_ptr(const cecs_hcoord4_f32 *coord) {
    assert(coord->w == 0.0f);
    return (cecs_hvec3_f32){
        .x = coord->x,
        .y = coord->y,
        .z = coord->z
    };
}

inline cecs_hcoord4_f32 cecs_hcoord4_f32_unpack_point(const cecs_hpoint3_f32 point) {
    return (cecs_hcoord4_f32){
        .x = point.x,
        .y = point.y,
        .z = point.z,
        .w = 1.0f
    };
}
inline cecs_hcoord4_f32 cecs_hcoord4_f32_unpack_direction(const cecs_hvec3_f32 vec) {
    return (cecs_hcoord4_f32){
        .x = vec.x,
        .y = vec.y,
        .z = vec.z,
        .w = 0.0f
    };
}

inline cecs_hcoord4_f32 cecs_mat4_f32_mul_hcoord4_affine(const cecs_mat4c_f32 m, const cecs_hcoord4_f32 c) {
    return (cecs_hcoord4_f32){
        .x = m.e0.x * c.x + m.e1.x * c.y + m.e2.x * c.z + m.e3.x * c.w,
        .y = m.e0.y * c.x + m.e1.y * c.y + m.e2.y * c.z + m.e3.y * c.w,
        .z = m.e0.z * c.x + m.e1.z * c.y + m.e2.z * c.z + m.e3.z * c.w,
        .w = c.w
    };
}
inline cecs_hcoord4_f32 cecs_mat4_f32_mul_hcoord4_affine_ptr(const cecs_mat4c_f32 *m, const cecs_hcoord4_f32 c) {
    return (cecs_hcoord4_f32){
        .x = m->e0.x * c.x + m->e1.x * c.y + m->e2.x * c.z + m->e3.x * c.w,
        .y = m->e0.y * c.x + m->e1.y * c.y + m->e2.y * c.z + m->e3.y * c.w,
        .z = m->e0.z * c.x + m->e1.z * c.y + m->e2.z * c.z + m->e3.z * c.w,
        .w = c.w
    };
}
inline cecs_hcoord4_f32 cecs_mat4_f32_mul_hcoord4_projection(const cecs_mat4c_f32 m, const cecs_hcoord4_f32 c) {
    return cecs_mat4c_f32_mul_vec4(m, c);
}


inline cecs_hpoint3_f32 cecs_mat4_f32_mul_hpoint3(const cecs_mat4c_f32 m, const cecs_hpoint3_f32 p) {
    return (cecs_hpoint3_f32){
        .x = m.e0.x * p.x + m.e1.x * p.y + m.e2.x * p.z + m.e3.x,
        .y = m.e0.y * p.x + m.e1.y * p.y + m.e2.y * p.z + m.e3.y,
        .z = m.e0.z * p.x + m.e1.z * p.y + m.e2.z * p.z + m.e3.z
    };
}
inline cecs_hpoint3_f32 cecs_mat4_f32_mul_hpoint3_ptr(const cecs_mat4c_f32 *m, const cecs_hpoint3_f32 p) {
    return (cecs_hpoint3_f32){
        .x = m->e0.x * p.x + m->e1.x * p.y + m->e2.x * p.z + m->e3.x,
        .y = m->e0.y * p.x + m->e1.y * p.y + m->e2.y * p.z + m->e3.y,
        .z = m->e0.z * p.x + m->e1.z * p.y + m->e2.z * p.z + m->e3.z
    };
}

inline cecs_hvec3_f32 cecs_mat4_f32_mul_hvec3(const cecs_mat4c_f32 m, const cecs_hvec3_f32 v) {
    return cecs_mat4c_f32_mul_vec3(m, v);
}
inline cecs_hvec3_f32 cecs_mat4_f32_mul_hvec3_ptr(const cecs_mat4c_f32 *m, const cecs_hvec3_f32 v) {
    return cecs_mat4c_f32_mul_vec3_ptr(m, v);
}

#endif