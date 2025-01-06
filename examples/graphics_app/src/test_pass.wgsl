struct vertex_input {
    @location(0) position: vec2<f32>,
    @location(1) color: vec3<f32>,
};

struct vertex_output {
    @builtin(position) position: vec4<f32>,
    @location(0) color: vec3<f32>,
};

@vertex
fn vs_main(input: vertex_input) -> vertex_output {
    var out: vertex_output;
    out.position = vec4<f32>(input.position, 0.0, 1.0);
    out.color = input.color;
    return out;
}

@fragment
fn fs_main(input: vertex_output) -> @location(0) vec4<f32> {
    return vec4<f32>(input.color, 1.0);
}