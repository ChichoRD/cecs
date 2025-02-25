alias cecs_versor_f32 = vec4<f32>;

fn cecs_versor_f32_rotate(uq: cecs_versor_f32, v: vec3<f32>) -> vec3<f32> {
    let u = fma(v, vec3(uq.w), cross(uq.xyz, v));
    return v + cross(2.0f * uq.xyz, u);
}
fn cecs_versor_f32_rotate_rcp(uq_rcp: cecs_versor_f32, v: vec3<f32>) -> vec3<f32> {
    let u = fma(v, vec3(-uq_rcp.w), cross(uq_rcp.xyz, v));
    return v + cross(2.0f * uq_rcp.xyz, u);
}


alias cecs_projection_packed4_f32 = vec4<f32>;

alias cecs_persp_projection_packed4_f32 = cecs_projection_packed4_f32;
alias cecs_ortho_projection_packed4_f32 = cecs_projection_packed4_f32;
fn cecs_project_point_perspective(pack: cecs_persp_projection_packed4_f32, position: vec3<f32>) -> vec4<f32> {
    let xy = position.xy * pack.xy;
    let z = fma(position.z, pack.z, pack.w);
    return vec4<f32>(xy, z, position.z);
}
fn cecs_project_point_orthographic(pack: cecs_ortho_projection_packed4_f32, position: vec3<f32>) -> vec4<f32> {
    let xy = position.xy * pack.xy;
    let z = fma(position.z, pack.z, pack.w);
    return vec4<f32>(xy, z, 1.0);
}