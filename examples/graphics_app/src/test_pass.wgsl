struct vertex_input {
    @location(0) position: vec2<f32>,
    @location(1) uv: vec2<f32>,
};

struct instance_input {
    @location(2) position: vec2<f32>,
    @location(3) uv_subrect: vec2<f32>,
    @location(4) texture_range: vec2<u32>,
};

struct vertex_output {
    @builtin(position) position: vec4<f32>,
    @location(0) uv: vec2<f32>,

    @location(1) uv_subrect: vec2<f32>,
    @location(2) texture_range: vec2<u32>,
};

@group(0) @binding(0) var<uniform> view_proj: mat4x4<f32>;
@group(1) @binding(1) var<uniform> position: vec4<f32>;
@vertex
fn vs_main(v_input: vertex_input, i_input: instance_input) -> vertex_output {
    var out: vertex_output;
    //let up_alignment = dot(position.xy + i_input.position, vec2f(0.0, 1.0)) * 0.5 + 0.5;
    
    out.position = vec4<f32>(v_input.position * 0.005 + position.xy + i_input.position, 0.0, 1.0);
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
    //return vec4f(input.uv * input.uv_subrect, 0.0, 1.0);
    return textureSample(albedo_texture, texture_sampler, input.uv * input.uv_subrect, input.texture_range[0]);
}