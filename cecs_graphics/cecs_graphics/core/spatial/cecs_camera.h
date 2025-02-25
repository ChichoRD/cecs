#ifndef CECS_CAMERA_H
#define CECS_CAMERA_H

#include <cecs_math/cecs_units.h>
#include <cecs_math/algebra/linear/cecs_matrix.h>
#include <cecs_math/algebra/cecs_spatial.h>
#include "cecs_transform.h"

typedef union cecs_camera_projection_descriptor {
    float ortho_half_extent_y;
    cecs_radians_f32 proj_fov_y;
} cecs_camera_projection_descriptor;


typedef struct cecs_camera {
    cecs_camera_projection_descriptor projection;
    float depth_length;
} cecs_camera;

cecs_camera cecs_camera_create_orthographic(const float half_height, const float far, const float near);
cecs_camera cecs_camera_create_perspective(const cecs_radians_f32 fov_y, const float far, const float near);


typedef struct cecs_projection_packed4_f32 {
    cecs_vec3_f32 scale;
    float affine_offset_z;
} cecs_projection_packed4_f32;
typedef cecs_projection_packed4_f32 cecs_ortho_projection_packed4_f32;
typedef cecs_projection_packed4_f32 cecs_persp_projection_packed4_f32;

cecs_ortho_projection_packed4_f32 cecs_camera_orthographic_projection(const cecs_camera camera, const float aspect_ratio, const float near);
cecs_persp_projection_packed4_f32 cecs_camera_perspective_projection(const cecs_camera camera, const float aspect_ratio, const float near);


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

// TODO: handle infinite z planes
typedef enum cecs_camera_options {
    cecs_camera_options_none = 0,
    cecs_camera_options_orthographic = 1 << 0,
    cecs_camera_options_perspective = 1 << 1,
    cecs_camera_options_z_infinite = 1 << 2
} cecs_camera_options;
typedef uint8_t cecs_camera_flags;

typedef struct cecs_camera_bundle {
    cecs_orientation3_f32 orientation;
    cecs_position3_f32 position;
    cecs_camera camera;
} cecs_camera_bundle;
typedef struct cecs_camera_reference_bundle {
    cecs_orientation3_f32 *orientation;
    cecs_position3_f32 *position;
    cecs_camera *camera;
} cecs_camera_reference_bundle;


typedef struct cecs_camera_pack {
    cecs_camera_bundle bundle;
    float near;
    cecs_camera_flags flags;
} cecs_camera_pack;


typedef struct cecs_camera_raw_bundle {
    cecs_projection_packed4_f32 projection;
    cecs_orientation4_f32 orientation;
    cecs_position3_f32 position;
    float unused;
} cecs_camera_raw_bundle;

cecs_camera_raw_bundle cecs_camera_raw_bundle_from_pack_orthographic(const cecs_camera_pack pack, const float aspect_ratio);
cecs_camera_raw_bundle cecs_camera_raw_bundle_from_pack_perspective(const cecs_camera_pack pack, const float aspect_ratio);
cecs_camera_raw_bundle cecs_camera_raw_bundle_from_pack(const cecs_camera_pack pack, const float aspect_ratio);
#endif