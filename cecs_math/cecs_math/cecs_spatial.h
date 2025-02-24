#ifndef CECS_SPATIAL_H
#define CECS_SPATIAL_H

#include "complex/cecs_quaternion.h"
#include "linear/cecs_vector.h"


inline cecs_vec3_f32 cecs_versor_f32_rotate(const cecs_versor_f32 uq, const cecs_vec3_f32 v) {
    const cecs_quaternion_vector_f32 p = cecs_quaternion_f32_vector(v.x, v.y, v.z);
    const cecs_quaternion_f32 q_rcp = cecs_quaternion_f32_conjugate(uq);
    const cecs_quaternion_f32 r = cecs_quaternion_f32_product(
        uq,
        cecs_quaternion_vector_f32_product(p, q_rcp)
    );
    return (cecs_vec3_f32){r.x, r.y, r.z};
}

#endif