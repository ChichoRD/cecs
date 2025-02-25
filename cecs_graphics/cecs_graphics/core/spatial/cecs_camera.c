#include "cecs_camera.h"

cecs_camera cecs_camera_create_orthographic(const float half_height, const float far, const float near) {
    assert(half_height > 0.0f && "fatal error: orthographic projection half height must be positive");
    assert(far > near && "fatal error: far plane must be greater than near plane");
    assert(near > 0.0f && "fatal error: depth planes must be strictly positive");
    return (cecs_camera){
        .projection = {.ortho_half_extent_y = half_height},
        .far = far,
        .near = near
    };
}

cecs_camera cecs_camera_create_perspective(const cecs_radians_f32 fov_y, const float far, const float near) {
    assert(fov_y > 0.0f && "fatal error: field of view must be positive");
    assert(far > near && "fatal error: far plane must be greater than near plane");
    assert(near > 0.0f && "fatal error: depth planes must be strictly positive");
    return (cecs_camera){
        .projection = {.proj_fov_y = fov_y},
        .far = far,
        .near = near
    };
}

cecs_ortho_projection_packed4_f32 cecs_camera_orthographic_projection(const cecs_camera camera, const float aspect_ratio) {
    const float height = camera.projection.ortho_half_extent_y * 2.0f;
    const float width = height * aspect_ratio;

    const float depth_length = camera.far - camera.near;
    const cecs_vec3_f32 scale = {2.0f / width, 2.0f / height, 1.0f / depth_length};
    return (cecs_ortho_projection_packed4_f32){
        .scale = scale,
        .affine_offset_z = -camera.near * scale.z
    };
}

cecs_persp_projection_packed4_f32 cecs_camera_perspective_projection(const cecs_camera camera, const float aspect_ratio) {
    const float inv_tan_half_fov = 1.0f / tanf(camera.projection.proj_fov_y * 0.5f);
    const float depth_length = camera.far - camera.near;

    cecs_vec3_f32 scale = {inv_tan_half_fov / aspect_ratio, inv_tan_half_fov, camera.far / depth_length};
    return (cecs_persp_projection_packed4_f32){
        .scale = scale,
        .affine_offset_z = -camera.near * scale.z
    };
}

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
