struct vertex_input {
    @location(0) position: vec2<f32>,
    @location(1) uv: vec2<f32>,
    @location(2) color: vec3<f32>,
};

struct instance_input {
    @location(3) position: vec2<f32>,
    @location(4) uv_subrect: vec2<f32>,
    @location(5) texture_range: vec2<u32>,
};

struct vertex_output {
    @builtin(position) position: vec4<f32>,
    @location(1) color: vec3<f32>,
    @location(0) uv: vec2<f32>,

    @location(2) uv_subrect: vec2<f32>,
    @location(3) texture_range: vec2<u32>,
};

@group(0) @binding(0) var<uniform> view_proj: mat4x4<f32>;
@group(1) @binding(1) var<uniform> position: vec4<f32>;
@vertex
fn vs_main(v_input: vertex_input, i_input: instance_input) -> vertex_output {
    var out: vertex_output;
    out.position = vec4<f32>(v_input.position * 0.25 + position.xy + i_input.position, 0.0, 1.0);
    out.color = v_input.color;
    out.uv = v_input.uv;
    out.uv_subrect = i_input.uv_subrect;
    out.texture_range = i_input.texture_range;
    return out;
}

fn hsv_from_rgb(color: vec3f) -> vec3f {
    let cmax = max(color.r, max(color.g, color.b));
    let cmin = min(color.r, min(color.g, color.b));
    let delta = cmax - cmin;

    var h = 0.0;
    if (delta != 0.0) {
        if (cmax == color.r) {
            h = (color.g - color.b) / delta;
        } else if (cmax == color.g) {
            h = 2.0 + (color.b - color.r) / delta;
        } else {
            h = 4.0 + (color.r - color.g) / delta;
        }
    }

    h = h * 60.0;
    if (h < 0.0) {
        h = h + 360.0;
    }

    var s = 0.0;
    if (cmax != 0.0) {
        s = delta / cmax;
    }

    let v = cmax;

    return vec3f(h, s, v);
}

@group(0) @binding(1) var texture_sampler: sampler;
@group(1) @binding(0) var<uniform> color: vec4<f32>;
@group(2) @binding(0) var albedo_texture: texture_2d_array<f32>;

@fragment
fn fs_main(input: vertex_output) -> @location(0) vec4<f32> {
    return textureSample(albedo_texture, texture_sampler, input.uv * input.uv_subrect, input.texture_range[0]);
}