#include "cecs_spatial.h"

extern inline cecs_versor_f32 cecs_versor_f32_axis_angle(const cecs_vec3_f32 axis, const cecs_radians_f32 angle);
extern inline cecs_vec3_f32 cecs_versor_f32_rotate(const cecs_versor_f32 uq, const cecs_vec3_f32 v);

extern inline cecs_quaternion_f32 cecs_quaternion_f32_arc(const cecs_vec3_f32 to, const cecs_vec3_f32 from);
extern inline cecs_versor_f32 cecs_versor_f32_arc(const cecs_vec3_f32 to, const cecs_vec3_f32 from);