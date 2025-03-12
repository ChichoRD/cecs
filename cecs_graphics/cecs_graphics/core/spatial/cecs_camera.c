#include "cecs_camera.h"
#include <stdbool.h>
#include <stdlib.h>

cecs_camera cecs_camera_create_orthographic(const float half_height, const float far, const float near) {
    assert(half_height > 0.0f && "fatal error: orthographic projection half height must be positive");
    assert(far > near && "fatal error: far plane must be greater than near plane");
    assert(near > 0.0f && "fatal error: depth planes must be strictly positive");
    return (cecs_camera){
        .projection = {.ortho_half_extent_y = half_height},
        .depth_length = far - near
    };
}
cecs_camera cecs_camera_create_perspective(const cecs_radians_f32 fov_y, const float far, const float near) {
    assert(fov_y > 0.0f && "fatal error: field of view must be positive");
    assert(far > near && "fatal error: far plane must be greater than near plane");
    assert(near > 0.0f && "fatal error: depth planes must be strictly positive");
    return (cecs_camera){
        .projection = {.proj_fov_y = fov_y},
        .depth_length = far - near
    };
}

cecs_ortho_projection_packed4_f32 cecs_camera_orthographic_projection(const cecs_camera camera, const float aspect_ratio, const float near) {
    const float height = camera.projection.ortho_half_extent_y * 2.0f;
    const float width = height * aspect_ratio;

    const cecs_vec3_f32 scale = {2.0f / width, 2.0f / height, 1.0f / camera.depth_length};
    return (cecs_ortho_projection_packed4_f32){
        .scale = scale,
        .affine_offset_z = -near * scale.z
    };
}
cecs_persp_projection_packed4_f32 cecs_camera_perspective_projection(const cecs_camera camera, const float aspect_ratio, const float near) {
    const float inv_tan_half_fov = 1.0f / tanf(camera.projection.proj_fov_y * 0.5f);

    cecs_vec3_f32 scale = {inv_tan_half_fov / aspect_ratio, inv_tan_half_fov, (camera.depth_length + near) / camera.depth_length};
    return (cecs_persp_projection_packed4_f32){
        .scale = scale,
        .affine_offset_z = -near * scale.z
    };
}

extern inline cecs_hcoord4_f32 cecs_hcoord4_f32_project_point_orthographic(const cecs_ortho_projection_packed4_f32 po, const cecs_hpoint3_f32 point);
extern inline cecs_hcoord4_f32 cecs_hcoord4_f32_project_direction_orthographic(const cecs_ortho_projection_packed4_f32 po, const cecs_hvec3_f32 vec);
extern inline cecs_hcoord4_f32 cecs_hcoord4_f32_project_orthographic(const cecs_ortho_projection_packed4_f32 po, const cecs_hcoord4_f32 coord);

extern inline cecs_hcoord4_f32 cecs_hcoord4_f32_project_point_perspective(const cecs_persp_projection_packed4_f32 pp, const cecs_hpoint3_f32 point);
extern inline cecs_hcoord4_f32 cecs_hcoord4_f32_project_direction_perspective(const cecs_persp_projection_packed4_f32 pp, const cecs_hvec3_f32 vec);
extern inline cecs_hcoord4_f32 cecs_hcoord4_f32_project_perspective(const cecs_persp_projection_packed4_f32 pp, const cecs_hcoord4_f32 coord);


cecs_ortho_projection_mat4c_f32 cecs_ortho_projection_mat4c_f32_unpack(const cecs_ortho_projection_packed4_f32 packed) {
    cecs_ortho_projection_mat4c_f32 projection = {0};
    projection.e0.x = packed.scale.x;
    projection.e1.y = packed.scale.y;
    projection.e2.z = packed.scale.z;
    projection.e3.z = packed.affine_offset_z;
    projection.e3.w = 1.0f;
    return projection;
}
cecs_persp_projection_mat4c_f32 cecs_persp_projection_mat4c_f32_unpack(const cecs_persp_projection_packed4_f32 packed) {
    cecs_persp_projection_mat4c_f32 projection = {0};
    projection.e0.x = packed.scale.x;
    projection.e1.y = packed.scale.y;
    projection.e2.z = packed.scale.z;
    projection.e2.w = 1.0f;
    projection.e3.z = packed.affine_offset_z;
    return projection;
}

cecs_camera_raw_bundle cecs_camera_raw_bundle_from_pack_orthographic(const cecs_camera_pack pack, const float aspect_ratio) {
    assert(pack.flags & cecs_camera_options_orthographic && "fatal error: camera pack must be orthographic");
    return (cecs_camera_raw_bundle){
        .projection = cecs_camera_orthographic_projection(pack.bundle.camera, aspect_ratio, pack.near),
        .orientation = cecs_versor_f32_unpack(pack.bundle.orientation),
        .position = pack.bundle.position
    };
}

cecs_camera_raw_bundle cecs_camera_raw_bundle_from_pack_perspective(const cecs_camera_pack pack, const float aspect_ratio) {
    assert(pack.flags & cecs_camera_options_perspective && "fatal error: camera pack must be perspective");
    return (cecs_camera_raw_bundle){
        .projection = cecs_camera_perspective_projection(pack.bundle.camera, aspect_ratio, pack.near),
        .orientation = cecs_versor_f32_unpack(pack.bundle.orientation),
        .position = pack.bundle.position
    };
}

cecs_camera_raw_bundle cecs_camera_raw_bundle_from_pack(const cecs_camera_pack pack, const float aspect_ratio) {
    if (pack.flags & cecs_camera_options_orthographic) {
        return cecs_camera_raw_bundle_from_pack_orthographic(pack, aspect_ratio);
    } else if (pack.flags & cecs_camera_options_perspective) {
        return cecs_camera_raw_bundle_from_pack_perspective(pack, aspect_ratio);
    } else {
        assert(false && "fatal error: camera pack must be either orthographic or perspective");
        return (cecs_camera_raw_bundle){0};
    }

    if (pack.flags & cecs_camera_options_z_infinite) {
        assert(false && "unimplemented: infinite z depth");
        exit(EXIT_FAILURE);
    }
}
