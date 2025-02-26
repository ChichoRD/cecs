#include "cecs_vector.h"


const cecs_vec2_f32 cecs_vec2_f32_zero = CECS_VEC2_F32_ZERO;
const cecs_vec2_i32 cecs_vec2_i32_zero = CECS_VEC2_I32_ZERO;
const cecs_vec2_u32 cecs_vec2_u32_zero = CECS_VEC2_U32_ZERO;

const cecs_vec3_f32 cecs_vec3_f32_zero = CECS_VEC3_F32_ZERO;
const cecs_vec3_i32 cecs_vec3_i32_zero = CECS_VEC3_I32_ZERO;
const cecs_vec3_u32 cecs_vec3_u32_zero = CECS_VEC3_U32_ZERO;

const cecs_vec4_f32 cecs_vec4_f32_zero = CECS_VEC4_F32_ZERO;


extern inline cecs_vec2_f32 cecs_vec2_f32_add(const cecs_vec2_f32 u, const cecs_vec2_f32 v);
extern inline cecs_vec2_i32 cecs_vec2_i32_add(const cecs_vec2_i32 u, const cecs_vec2_i32 v);
extern inline cecs_vec2_u32 cecs_vec2_u32_add(const cecs_vec2_u32 u, const cecs_vec2_u32 v);

extern inline cecs_vec3_f32 cecs_vec3_f32_add(const cecs_vec3_f32 u, const cecs_vec3_f32 v);
extern inline cecs_vec3_i32 cecs_vec3_i32_add(const cecs_vec3_i32 u, const cecs_vec3_i32 v);
extern inline cecs_vec3_u32 cecs_vec3_u32_add(const cecs_vec3_u32 u, const cecs_vec3_u32 v);

extern inline cecs_vec4_f32 cecs_vec4_f32_add(const cecs_vec4_f32 u, const cecs_vec4_f32 v);


extern inline cecs_vec2_f32 cecs_vec2_f32_sub(const cecs_vec2_f32 u, const cecs_vec2_f32 v);
extern inline cecs_vec2_i32 cecs_vec2_i32_sub(const cecs_vec2_i32 u, const cecs_vec2_i32 v);
extern inline cecs_vec2_u32 cecs_vec2_u32_sub(const cecs_vec2_u32 u, const cecs_vec2_u32 v);

extern inline cecs_vec3_f32 cecs_vec3_f32_sub(const cecs_vec3_f32 u, const cecs_vec3_f32 v);
extern inline cecs_vec3_i32 cecs_vec3_i32_sub(const cecs_vec3_i32 u, const cecs_vec3_i32 v);
extern inline cecs_vec3_u32 cecs_vec3_u32_sub(const cecs_vec3_u32 u, const cecs_vec3_u32 v);

extern inline cecs_vec4_f32 cecs_vec4_f32_sub(const cecs_vec4_f32 u, const cecs_vec4_f32 v);


extern inline cecs_vec2_f32 cecs_vec2_f32_mul(const cecs_vec2_f32 u, const float s);
extern inline cecs_vec2_i32 cecs_vec2_i32_mul(const cecs_vec2_i32 u, const int32_t s);
extern inline cecs_vec2_u32 cecs_vec2_u32_mul(const cecs_vec2_u32 u, const uint32_t s);

extern inline cecs_vec3_f32 cecs_vec3_f32_mul(const cecs_vec3_f32 u, const float s);
extern inline cecs_vec3_i32 cecs_vec3_i32_mul(const cecs_vec3_i32 u, const int32_t s);
extern inline cecs_vec3_u32 cecs_vec3_u32_mul(const cecs_vec3_u32 u, const uint32_t s);

extern inline cecs_vec4_f32 cecs_vec4_f32_mul(const cecs_vec4_f32 u, const float s);


extern inline cecs_vec2_f32 cecs_vec2_f32_div(const cecs_vec2_f32 u, const float s);
extern inline cecs_vec2_i32 cecs_vec2_i32_div(const cecs_vec2_i32 u, const int32_t s);
extern inline cecs_vec2_u32 cecs_vec2_u32_div(const cecs_vec2_u32 u, const uint32_t s);

extern inline cecs_vec3_f32 cecs_vec3_f32_div(const cecs_vec3_f32 u, const float s);
extern inline cecs_vec3_i32 cecs_vec3_i32_div(const cecs_vec3_i32 u, const int32_t s);
extern inline cecs_vec3_u32 cecs_vec3_u32_div(const cecs_vec3_u32 u, const uint32_t s);

extern inline cecs_vec4_f32 cecs_vec4_f32_div(const cecs_vec4_f32 u, const float s);


extern inline float cecs_vec2_f32_dot(const cecs_vec2_f32 u, const cecs_vec2_f32 v);
extern inline int32_t cecs_vec2_i32_dot(const cecs_vec2_i32 u, const cecs_vec2_i32 v);
extern inline uint32_t cecs_vec2_u32_dot(const cecs_vec2_u32 u, const cecs_vec2_u32 v);

extern inline float cecs_vec3_f32_dot(const cecs_vec3_f32 u, const cecs_vec3_f32 v);
extern inline int32_t cecs_vec3_i32_dot(const cecs_vec3_i32 u, const cecs_vec3_i32 v);
extern inline uint32_t cecs_vec3_u32_dot(const cecs_vec3_u32 u, const cecs_vec3_u32 v);

extern inline float cecs_vec4_f32_dot(const cecs_vec4_f32 u, const cecs_vec4_f32 v);


extern inline cecs_vec3_f32 cecs_vec3_f32_cross(const cecs_vec3_f32 u, const cecs_vec3_f32 v);


extern inline float cecs_vec2_f32_length(const cecs_vec2_f32 u);
extern inline float cecs_vec3_f32_length(const cecs_vec3_f32 u);
extern inline float cecs_vec4_f32_length(const cecs_vec4_f32 u);


extern inline cecs_vec2_f32 cecs_vec2_f32_normalize_from_length(const cecs_vec2_f32 u, const float length);
extern inline cecs_vec3_f32 cecs_vec3_f32_normalize_from_length(const cecs_vec3_f32 u, const float length);
extern inline cecs_vec4_f32 cecs_vec4_f32_normalize_from_length(const cecs_vec4_f32 u, const float length);
extern inline cecs_vec2_f32 cecs_vec2_f32_normalize(const cecs_vec2_f32 u);
extern inline cecs_vec3_f32 cecs_vec3_f32_normalize(const cecs_vec3_f32 u);
extern inline cecs_vec4_f32 cecs_vec4_f32_normalize(const cecs_vec4_f32 u);