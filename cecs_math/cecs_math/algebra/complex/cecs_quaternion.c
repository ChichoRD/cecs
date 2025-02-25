#include "cecs_quaternion.h"

#define CECS_QUATERNION_F32_IDENTITY {0.0f, 0.0f, 0.0f, 1.0f}
#define CECS_QUATERNION_F32_ZERO {0.0f, 0.0f, 0.0f, 0.0f}

const cecs_quaternion_f32 cecs_quaternion_f32_identity = CECS_QUATERNION_F32_IDENTITY;
const cecs_quaternion_f32 cecs_quaternion_f32_zero = CECS_QUATERNION_F32_ZERO;

extern inline cecs_quaternion_f32 cecs_quaternion_f32_scalar(const float s);
extern inline cecs_quaternion_f32 cecs_quaternion_f32_vector(const float x, const float y, const float z);

extern inline cecs_quaternion_f32 cecs_quaternion_f32_add(const cecs_quaternion_f32 p, const cecs_quaternion_f32 q);
extern inline cecs_quaternion_f32 cecs_quaternion_f32_sub(const cecs_quaternion_f32 p, const cecs_quaternion_f32 q);
extern inline cecs_quaternion_f32 cecs_quaternion_f32_mul(const cecs_quaternion_f32 p, const float s);
extern inline cecs_quaternion_f32 cecs_quaternion_f32_div(const cecs_quaternion_f32 p, const float s);

extern inline cecs_quaternion_f32 cecs_quaternion_f32_product(const cecs_quaternion_f32 p, const cecs_quaternion_f32 q);
extern inline cecs_quaternion_f32 cecs_quaternion_vector_f32_product(const cecs_quaternion_vector_f32 p, const cecs_quaternion_f32 q);

extern inline cecs_quaternion_f32 cecs_quaternion_f32_conjugate(const cecs_quaternion_f32 q);


extern inline float cecs_quaternion_f32_norm_sqr(const cecs_quaternion_f32 q);
extern inline float cecs_quaternion_f32_norm(const cecs_quaternion_f32 q);

extern inline cecs_quaternion_f32 cecs_quaternion_f32_rcp_norm_sqr(const cecs_quaternion_f32 q, const float norm_sqr);
extern inline cecs_quaternion_f32 cecs_quaternion_f32_rcp(const cecs_quaternion_f32 q);


extern inline cecs_versor_f32 cecs_versor_f32_of_norm(const cecs_quaternion_f32 q, const float norm);
extern inline cecs_versor_f32 cecs_versor_f32_of(const cecs_quaternion_f32 q);


extern inline cecs_versor_packed_f32 cecs_versor_packed_f32_pack(const cecs_versor_f32 q);
extern inline cecs_versor_f32 cecs_versor_f32_unpack(const cecs_versor_packed_f32 q);