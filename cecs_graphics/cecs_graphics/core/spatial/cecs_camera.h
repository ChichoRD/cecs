#ifndef CECS_CAMERA_H
#define CECS_CAMERA_H

#include <cecs_math/cecs_units.h>
#include <cecs_math/algebra/linear/cecs_matrix.h>
#include <cecs_math/algebra/cecs_spatial.h>

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


inline cecs_hcoord4_f32 cecs_hcoord4_f32_project_point_orthographic(const cecs_ortho_projection_packed4_f32 po, const cecs_hpoint3_f32 point) {
    return (cecs_hcoord4_f32){
        .x = point.x * po.scale.x,
        .y = point.y * po.scale.y,
        .z = point.z * po.scale.z + po.affine_offset_z,
        .w = 1.0f
    };
}
inline cecs_hcoord4_f32 cecs_hcoord4_f32_project_direction_orthographic(const cecs_ortho_projection_packed4_f32 po, const cecs_hvec3_f32 vec) {
    return (cecs_hcoord4_f32){
        .x = vec.x * po.scale.x,
        .y = vec.y * po.scale.y,
        .z = vec.z * po.scale.z,
        .w = 0.0f
    };
}
inline cecs_hcoord4_f32 cecs_hcoord4_f32_project_orthographic(const cecs_ortho_projection_packed4_f32 po, const cecs_hcoord4_f32 coord) {
    return (cecs_hcoord4_f32){
        .x = coord.x * po.scale.x,
        .y = coord.y * po.scale.y,
        .z = coord.z * po.scale.z + po.affine_offset_z * coord.w,
        .w = coord.w
    };
}

inline cecs_hcoord4_f32 cecs_hcoord4_f32_project_point_perspective(const cecs_persp_projection_packed4_f32 pp, const cecs_hpoint3_f32 point) {
    return (cecs_hcoord4_f32){
        .x = point.x * pp.scale.x,
        .y = point.y * pp.scale.y,
        .z = point.z * pp.scale.z + pp.affine_offset_z,
        .w = point.z
    };
}
inline cecs_hcoord4_f32 cecs_hcoord4_f32_project_direction_perspective(const cecs_persp_projection_packed4_f32 pp, const cecs_hvec3_f32 vec) {
    return (cecs_hcoord4_f32){
        .x = vec.x * pp.scale.x,
        .y = vec.y * pp.scale.y,
        .z = vec.z * pp.scale.z,
        .w = vec.z
    };
}
inline cecs_hcoord4_f32 cecs_hcoord4_f32_project_perspective(const cecs_persp_projection_packed4_f32 pp, const cecs_hcoord4_f32 coord) {
    return (cecs_hcoord4_f32){
        .x = coord.x * pp.scale.x,
        .y = coord.y * pp.scale.y,
        .z = coord.z * pp.scale.z + pp.affine_offset_z * coord.w,
        .w = coord.w
    };
}


typedef cecs_mat4c_f32 cecs_projection_mat4c_f32;

typedef cecs_projection_mat4c_f32 cecs_ortho_projection_mat4c_f32;
typedef cecs_projection_mat4c_f32 cecs_persp_projection_mat4c_f32;
cecs_ortho_projection_mat4c_f32 cecs_ortho_projection_mat4c_f32_unpack(const cecs_ortho_projection_packed4_f32 packed);
cecs_persp_projection_mat4c_f32 cecs_persp_projection_mat4c_f32_unpack(const cecs_persp_projection_packed4_f32 packed);

#endif