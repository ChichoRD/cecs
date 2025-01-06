#ifndef TEST_PASS_H
#define TEST_PASS_H

#include <webgpu/webgpu.h>
#include <cecs_graphics.h>

typedef struct test_pass {
    WGPURenderPipeline pipeline;
} test_pass;

test_pass test_pass_create(cecs_graphics_context *context, cecs_render_target_info target_info, cecs_arena *arena);

// TODO: array of targets
void test_pass_draw(test_pass *pass, cecs_world *world, cecs_graphics_system *system, cecs_render_target *target);

typedef struct position2_f32_attribute {
    float x;
    float y;
} position2_f32_attribute;
CECS_COMPONENT_DECLARE(position2_f32_attribute);
WGPUVertexBufferLayout position2_f32_attribute_layout(
    uint32_t shader_location,
    WGPUVertexAttribute out_attributes[],
    size_t out_attributes_capacity
);

typedef struct color3_f32_attribute {
    float r;
    float g;
    float b;
} color3_f32_attribute;
CECS_COMPONENT_DECLARE(color3_f32_attribute);
WGPUVertexBufferLayout color3_f32_attribute_layout(
    uint32_t shader_location,
    WGPUVertexAttribute out_attributes[],
    size_t out_attributes_capacity
);

#endif