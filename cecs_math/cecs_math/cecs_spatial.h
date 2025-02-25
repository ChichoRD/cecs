#ifndef CECS_SPATIAL_H
#define CECS_SPATIAL_H

#include "complex/cecs_quaternion.h"
#include "linear/cecs_vector.h"
#include "cecs_units.h"

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
    const k_cos_theta = cecs_vec3_f32_dot(from, to);
    const k = sqrtf(cecs_vec3_f32_dot(from, from) * cecs_vec3_f32_dot(to, to));

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


#endif