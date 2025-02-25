#include "cecs_spatial.h"

extern inline cecs_versor_f32 cecs_versor_f32_axis_angle(const cecs_vec3_f32 axis, const cecs_radians_f32 angle);
extern inline cecs_vec3_f32 cecs_versor_f32_rotate(const cecs_versor_f32 uq, const cecs_vec3_f32 v);

extern inline cecs_quaternion_f32 cecs_quaternion_f32_arc(const cecs_vec3_f32 to, const cecs_vec3_f32 from);
extern inline cecs_versor_f32 cecs_versor_f32_arc(const cecs_vec3_f32 to, const cecs_vec3_f32 from);


extern inline cecs_hpoint3_f32 cecs_hvec3_f32_pack(const cecs_hcoord4_f32 coord);
extern inline cecs_hvec3_f32 cecs_hvec3_f32_pack(const cecs_hcoord4_f32 coord);
extern inline cecs_hpoint3_f32 cecs_hvec3_f32_pack_ptr(const cecs_hcoord4_f32 *coord);
extern inline cecs_hvec3_f32 cecs_hvec3_f32_pack_ptr(const cecs_hcoord4_f32 *coord);

extern inline cecs_hcoord4_f32 cecs_hcoord4_f32_unpack_point(const cecs_hpoint3_f32 point);
extern inline cecs_hcoord4_f32 cecs_hcoord4_f32_unpack_direction(const cecs_hvec3_f32 vec);

extern inline cecs_hcoord4_f32 cecs_mat4_f32_mul_hcoord4_affine(const cecs_mat4_f32 m, const cecs_hcoord4_f32 c);
extern inline cecs_hcoord4_f32 cecs_mat4_f32_mul_hcoord4_affine_ptr(const cecs_mat4_f32 *m, const cecs_hcoord4_f32 c);
extern inline cecs_hcoord4_f32 cecs_mat4_f32_mul_hcoord4_projection(const cecs_mat4_f32 m, const cecs_hcoord4_f32 c);


extern inline cecs_hpoint3_f32 cecs_mat4_f32_mul_hpoint3(const cecs_mat4_f32 m, const cecs_hpoint3_f32 p);
extern inline cecs_hpoint3_f32 cecs_mat4_f32_mul_hpoint3_ptr(const cecs_mat4_f32 *m, const cecs_hpoint3_f32 p);

extern inline cecs_hvec3_f32 cecs_mat4_f32_mul_hvec3(const cecs_mat4_f32 m, const cecs_hvec3_f32 v);
extern inline cecs_hvec3_f32 cecs_mat4_f32_mul_hvec3_ptr(const cecs_mat4_f32 *m, const cecs_hvec3_f32 v);