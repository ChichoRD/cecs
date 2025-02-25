#ifndef CECS_CAMERA_H
#define CECS_CAMERA_H

#include <cecs_math/cecs_units.h>
#include <cecs_math/algebra/linear/cecs_matrix.h>

typedef union cecs_camera_projection_descriptor {
    float ortho_half_extent_y;
    cecs_radians_f32 proj_fov_y;
} cecs_camera_projection_descriptor;


typedef struct cecs_camera {
    cecs_camera_projection_descriptor projection;
    float far;
    float near;
} cecs_camera;

cecs_camera cecs_camera_create_orthographic(const float half_height, const float far, const float near);
cecs_camera cecs_camera_create_perspective(const cecs_radians_f32 fov_y, const float far, const float near);


typedef struct cecs_projection_packed4_f32 {
    cecs_vec3_f32 scale;
    float affine_offset_z;
} cecs_projection_packed4_f32;
typedef cecs_projection_packed4_f32 cecs_ortho_projection_packed4_f32;
typedef cecs_projection_packed4_f32 cecs_persp_projection_packed4_f32;

cecs_ortho_projection_packed4_f32 cecs_camera_orthographic_projection(const cecs_camera camera, const float aspect_ratio);
cecs_persp_projection_packed4_f32 cecs_camera_perspective_projection(const cecs_camera camera, const float aspect_ratio);


typedef cecs_mat4c_f32 cecs_projection_mat4c_f32;

typedef cecs_projection_mat4c_f32 cecs_ortho_projection_mat4c_f32;
typedef cecs_projection_mat4c_f32 cecs_persp_projection_mat4c_f32;
cecs_ortho_projection_mat4c_f32 cecs_ortho_projection_mat4c_f32_unpack(const cecs_ortho_projection_packed4_f32 packed);
cecs_persp_projection_mat4c_f32 cecs_persp_projection_mat4c_f32_unpack(const cecs_persp_projection_packed4_f32 packed);

#endif