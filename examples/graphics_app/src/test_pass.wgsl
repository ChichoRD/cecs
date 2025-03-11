struct vertex_input {
    @location(0) position: vec3<f32>,
    @location(1) uv: vec2<f32>,
};

struct instance_input {
    @location(2) position: vec3<f32>,
    @location(3) uv_subrect: vec2<f32>,
    @location(4) texture_range: vec2<u32>,
};

struct vertex_output {
    @builtin(position) position_frame: vec4<f32>,
    @location(0) position_world: vec3<f32>,
    @location(1) uv: vec2<f32>,

    @location(2) uv_subrect: vec2<f32>,
    @location(3) texture_range: vec2<u32>,
};


struct camera {
    fov: f32,
    depth_range: f32,
};

// TODO: pack
struct camera_bundle {
    orientation: vec3<f32>,
    position: vec3<f32>,
    //padding: f32,
    cam: camera,
};

struct camera_raw_bundle {
    projection_pack: vec4<f32>,
    orientation: vec4<f32>,
    @size(16) position: vec3<f32>,
};


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


@group(0) @binding(0) var<uniform> cam: camera_raw_bundle;
@group(1) @binding(1) var<uniform> position: vec4<f32>;
@vertex
fn vs_main(v_input: vertex_input, i_input: instance_input) -> vertex_output {
    var out: vertex_output;
    
    let position_world = i_input.position * 100.0 + v_input.position * 0.25;
    let position_view = cecs_versor_f32_rotate_rcp(cam.orientation, position_world.xyz - cam.position);
    let position_clip = cecs_project_point_perspective(cam.projection_pack, position_view.xyz);

    out.position_frame = position_clip;
    out.position_world = position_world;
    out.uv = v_input.uv;
    out.uv_subrect = i_input.uv_subrect;
    out.texture_range = i_input.texture_range;
    return out;
}

@group(0) @binding(1) var texture_sampler: sampler;
@group(1) @binding(0) var<uniform> color: vec4<f32>;
@group(2) @binding(0) var albedo_texture: texture_2d_array<f32>;

@fragment
fn fs_main(input: vertex_output) -> @location(0) vec4<f32> {
    let m = max(max(input.position_world.x, input.position_world.y), input.position_world.z);
    let extreme = step(vec3f(m), input.position_world);
    
    let sample = textureSample(albedo_texture, texture_sampler, input.uv * input.uv_subrect, input.texture_range[0]);
    return vec4f((sample.rgb) , sample.a);
}