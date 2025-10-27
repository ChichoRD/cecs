#ifndef CECS_SPATIAL_H
#define CECS_SPATIAL_H

#include "complex/cecs_quaternion.h"
#include "linear/cecs_vector.h"
#include "linear/cecs_matrix.h"

#include "../cecs_units.h"


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
inline cecs_versor_packed_f32 cecs_versor_packed_f32_pack_axis_angle(const cecs_vec3_f32 axis, const cecs_radians_f32 angle) {
    const cecs_radians_f32 half_angle = angle * 0.5f;
    const float s = sinf(half_angle);
    return (cecs_versor_packed_f32){
        .i = axis.x * s,
        .j = axis.y * s,
        .k = axis.z * s
    };
}

inline cecs_quaternion_f32 cecs_quaternion_f32_axis_cos(const cecs_vec3_f32 axis, const float cos_angle) {
    const float sin_angle = sqrtf(1.0f - cos_angle * cos_angle);
    return (cecs_quaternion_f32){
        .i = axis.x * sin_angle,
        .j = axis.y * sin_angle,
        .k = axis.z * sin_angle,
        .r = cos_angle + 1.0f
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
            uq.r
        )
    );
    return cecs_vec3_f32_add(
        v,
        cecs_vec3_f32_mul(cecs_vec3_f32_cross((cecs_vec3_f32){uq.i, uq.j, uq.k}, u), 2.0f)
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
    return cecs_versor_f32_of(cecs_quaternion_f32_arc(to, from));
}
inline cecs_versor_f32 cecs_versor_f32_look_z(const cecs_vec3_f32 forward) {
    // k_cos_theta = forward.z
    const float z_sqr = forward.z * forward.z;
    const cecs_vec2_f32 axis_xy = (cecs_vec2_f32){
        .x = -forward.y,
        .y = forward.x
        // z = 0.0f
    };

    const float axis_xy_sqr = axis_xy.x * axis_xy.x + axis_xy.y * axis_xy.y;
    const float k_sqr = axis_xy_sqr + z_sqr;

    // cecs_versor_f32_of_norm(
    //     (cecs_quaternion_f32){
    //         .i = axis.x,         -> -forward.y
    //         .j = axis.y,         -> forward.x
    //         .k = axis.z,         -> 0.0f
    //         .r = k_cos_theta     -> forward.z
    //     },
    //     k                        -> cecs_vec3_f32_length(forward)
    // );

    // const cecs_quaternion_f32 q = {
    //     .i = axis_xy.x,
    //     .j = axis_xy.y,
    //     .k = 0.0f,
    //     .r = forward.z + k
    // };

    // |q|^2 = axis_xy.x^2 + axis_xy.y^2 + (forward.z + k)^2
    // |q|^2 = axis_xy.x^2 + axis_xy.y^2 + forward.z^2 + 2 * forward.z * k + k^2
    const float k = sqrtf(k_sqr);
    const float norm_rcp = 1.0f / sqrtf(axis_xy_sqr + z_sqr + 2.0f * forward.z * k + k_sqr);
    return (cecs_versor_f32) {
        .i = axis_xy.x * norm_rcp,
        .j = axis_xy.y * norm_rcp,
        .k = 0.0f,
        .r = (forward.z + k) * norm_rcp
    };
}
inline cecs_versor_packed_f32 cecs_versor_packed_f32_pack_look_z(const cecs_vec3_f32 forward) {
    // k_cos_theta = forward.z
    const float z_sqr = forward.z * forward.z;
    const cecs_vec2_f32 axis_xy = (cecs_vec2_f32){
        .x = -forward.y,
        .y = forward.x
        // z = 0.0f
    };

    const float axis_xy_sqr = axis_xy.x * axis_xy.x + axis_xy.y * axis_xy.y;
    const float k_sqr = axis_xy_sqr + z_sqr;

    // cecs_versor_f32_of_norm(
    //     (cecs_quaternion_f32){
    //         .i = axis.x,         -> -forward.y
    //         .j = axis.y,         -> forward.x
    //         .k = axis.z,         -> 0.0f
    //         .r = k_cos_theta     -> forward.z
    //     },
    //     k                        -> cecs_vec3_f32_length(forward)
    // );

    // const cecs_quaternion_f32 q = {
    //     .i = axis_xy.x,
    //     .j = axis_xy.y,
    //     .k = 0.0f,
    //     .r = forward.z + k
    // };

    // |q|^2 = axis_xy.x^2 + axis_xy.y^2 + (forward.z + k)^2
    // |q|^2 = axis_xy.x^2 + axis_xy.y^2 + forward.z^2 + 2 * forward.z * k + k^2
    const float k = sqrtf(k_sqr);
    const float norm_rcp = 1.0f / sqrtf(axis_xy_sqr + z_sqr + 2.0f * forward.z * k + k_sqr);
    return (cecs_versor_packed_f32) {
        .i = axis_xy.x * norm_rcp,
        .j = axis_xy.y * norm_rcp,
        .k = 0.0f,
    };
}

inline cecs_versor_f32 cecs_versor_f32_look_z_up(const cecs_vec3_f32 forward, const cecs_vec3_f32 upwards) {
    const cecs_vec3_f32 front = cecs_vec3_f32_normalize(forward); 
    const cecs_vec3_f32 left = cecs_vec3_f32_normalize(cecs_vec3_f32_cross(upwards, front));
    const cecs_vec3_f32 up = cecs_vec3_f32_cross(front, left);
    
    const float k = sqrtf(1.0f + left.x + up.y + front.z) * 0.5f;
    const float k4_rcp = 0.25f / k;
    return (cecs_versor_f32){
        .r = k,
        .i = (up.z - front.y) * k4_rcp,
        .j = (front.x - left.z) * k4_rcp,
        .k = (left.y - up.x) * k4_rcp,
    };
}
inline cecs_quaternion_f32 cecs_quaternion_f32_look_z_up(const cecs_vec3_f32 forward, const cecs_vec3_f32 upwards) {
    const cecs_vec3_f32 front = cecs_vec3_f32_normalize(forward); 
    const cecs_vec3_f32 left = cecs_vec3_f32_normalize(cecs_vec3_f32_cross(upwards, front));
    const cecs_vec3_f32 up = cecs_vec3_f32_cross(front, left);
    
    // const float k = sqrtf(1.0f + left.x + up.y + front.z) * 0.5f;
    // const float k4_rcp = 0.25f / k;
    // return (cecs_versor_f32){
    //     .r = k,
    //     .i = (up.z - front.y) * k4_rcp,
    //     .j = (front.x - left.z) * k4_rcp,
    //     .k = (left.y - up.x) * k4_rcp,
    // };

    // *= k / 0.25
    return (cecs_quaternion_f32){
        .r = (1.0f + left.x + up.y + front.z),
        .i = (up.z - front.y),
        .j = (front.x - left.z),
        .k = (left.y - up.x),
    };
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
