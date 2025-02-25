#include "cecs_matrix.h"


#define CECS_MAT4_F32_IDENTITY { \
    {1.0f, 0.0f, 0.0f, 0.0f}, \
    {0.0f, 1.0f, 0.0f, 0.0f}, \
    {0.0f, 0.0f, 1.0f, 0.0f}, \
    {0.0f, 0.0f, 0.0f, 1.0f} \
}
#define CECS_MAT4_F32_ZERO {CECS_VEC4_F32_ZERO, CECS_VEC4_F32_ZERO, CECS_VEC4_F32_ZERO, CECS_VEC4_F32_ZERO}

const cecs_mat4_f32 cecs_mat4_f32_identity = CECS_MAT4_F32_IDENTITY;
const cecs_mat4_f32 cecs_mat4_f32_zero = CECS_MAT4_F32_ZERO;

cecs_mat4_f32 cecs_mat4_f32_into_columns(const float raw[16]) {
    return (cecs_mat4_f32){
        .e0 = (cecs_vec4_f32){raw[0], raw[4], raw[8], raw[12]},
        .e1 = (cecs_vec4_f32){raw[1], raw[5], raw[9], raw[13]},
        .e2 = (cecs_vec4_f32){raw[2], raw[6], raw[10], raw[14]},
        .e3 = (cecs_vec4_f32){raw[3], raw[7], raw[11], raw[15]},
    };
}

extern inline cecs_mat4_f32 cecs_mat4_f32_mul(const cecs_mat4_f32 a, const cecs_mat4_f32 b);
extern inline cecs_vec4_f32 cecs_mat4_f32_mul_vec4(const cecs_mat4_f32 m, const cecs_vec4_f32 v);
extern inline cecs_vec3_f32 cecs_mat4_f32_mul_vec3(const cecs_mat4_f32 m, const cecs_vec3_f32 v);

extern inline float cecs_mat4_f32_trace(const cecs_mat4_f32 m);
extern inline cecs_mat4_f32 cecs_mat4_f32_transpose(const cecs_mat4_f32 m);

extern inline cecs_mat4_f32 cecs_mat4_f32_scale(const cecs_mat4_f32 m, const float s);