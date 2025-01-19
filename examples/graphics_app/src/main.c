#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <webgpu/webgpu.h>
#include <webgpu/wgpu.h>
#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>
#include <cecs_graphics.h>

#include <math.h>
#include <time.h>
#include <assert.h>

#include "test_pass.h"

static cecs_mesh_builder *mesh_builder_configure_sqaure(cecs_mesh_builder *builder) {
    cecs_mesh_builder_set_vertex_attribute(builder, CECS_COMPONENT_ID(position2_f32_attribute),
        (position2_f32_attribute[]) {
            // quad 4 verts
            { .x = -0.5f, .y = -0.5f },
            { .x = 0.5f, .y = -0.5f },
            { .x = 0.5f, .y = 0.5f },
            { .x = -0.5f, .y = 0.5f },
        },
        4,
        sizeof(position2_f32_attribute)
    );
    cecs_mesh_builder_set_indices(builder, (cecs_vertex_index_u16[]) {
        0, 1, 2, 2, 3, 0
    }, 6);
    cecs_mesh_builder_set_vertex_attribute(builder, CECS_COMPONENT_ID(color3_f32_attribute),
        (color3_f32_attribute[]) {
            // quad 4 colors
            { .r = 1.0f, .g = 0.0f, .b = 0.0f },
            { .r = 0.0f, .g = 1.0f, .b = 0.0f },
            { .r = 0.0f, .g = 0.0f, .b = 1.0f },
            { .r = 1.0f, .g = 1.0f, .b = 1.0f },
        },
        4,
        sizeof(color3_f32_attribute)
    );
    return builder;
}

int main(void) {
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // <-- extra info for glfwCreateWindow
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow *window = glfwCreateWindow(640, 480, "Learn WebGPU", NULL, NULL);
    if (window == NULL) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return EXIT_FAILURE;
    }

    cecs_world world = cecs_world_create(64, 16, 4);
    cecs_graphics_system system = cecs_graphics_system_create(1024, 8, window);
    WGPUSupportedLimits limits;
    wgpuDeviceGetLimits(system.context.device, &limits);
    printf("maxBindingsPerBindGroup: %u\n", limits.limits.maxBindingsPerBindGroup);

    cecs_arena builder_arena = cecs_arena_create();
    
    cecs_mesh_builder builder = cecs_mesh_builder_create(&system.world, (cecs_mesh_builder_descriptor){
        .vertex_attributes_expected_count = 2,
        .index_format = WGPUIndexFormat_Uint16,
    }, &builder_arena);
    mesh_builder_configure_sqaure(&builder);
    cecs_index_stream index_stream;
    cecs_mesh mesh = cecs_mesh_builder_build_into_and_clear(&world, &builder, &system.context, &index_stream);
    cecs_entity_id id = cecs_world_add_entity_with_indexed_mesh(&world, &mesh, &index_stream);
    
    const cecs_uniform_raw_stream *stream = CECS_WORLD_SET_COMPONENT_AS_UNIFORM(color4_f32_uniform, &world, &system.context, id, &((color4_f32_uniform){
        .r = 0.5f,
        .g = 0.5f,
        .b = 0.5f,
        .a = 1.0f,
    }));
    // CECS_WORLD_SET_COMPONENT_AS_UNIFORM(position4_f32_uniform, &world, &system.context, id, &((position4_f32_uniform){
    //     .x = 0.0f,
    //     .y = 0.0f,
    //     .z = 0.0f,
    //     .w = 0.0f,
    // }));
    (void)stream;

    // builder = cecs_mesh_builder_create_from(&builder, builder.descriptor);
    // mesh_builder_configure_sqaure(&builder);
    // mesh = cecs_mesh_builder_build_into_and_clear(&world, &builder, &system.context, &index_stream);
    // cecs_world_add_entity_with_indexed_mesh(&world, &mesh, &index_stream);
    cecs_arena_free(&builder_arena);

    test_pass pass = test_pass_create(&system.context, (cecs_render_target_info){
        .format = CECS_OPTION_GET(cecs_optional_surface_context, system.context.surface_context).configuration.format,
        .sample_count = 1,
        .aspect_ratio = 640.0f / 480.0f,
    }, &system.world.world.resources.resources_arena);

    WGPUColor clear_color = { 0.9, 0.1, 0.2, 1.0 };
    (void)clear_color;

    bool render_error = false;
    while (!glfwWindowShouldClose(window) && !render_error) {
        glfwPollEvents();

        struct timespec t;
        timespec_get(&t, TIME_UTC);
        double t_sec = (double)t.tv_sec + (double)t.tv_nsec / 1e9;
        clear_color = (WGPUColor){ sin(t_sec), cos(t_sec), cos(sin(t_sec)), 1.0f };
        CECS_WORLD_SET_COMPONENT_AS_UNIFORM(color4_f32_uniform, &world, &system.context, id, &((color4_f32_uniform){
            .r = clear_color.r,
            .g = clear_color.g,
            .b = clear_color.b,
            .a = clear_color.a,
        }));

        cecs_surface_render_target surface_target;
        if (cecs_graphics_context_get_surface_render_target(&system.context, &surface_target)) {
            test_pass_draw(&pass, &world, &system, &surface_target);
            cecs_graphics_context_present_surface_render_target(&system.context, &surface_target);
        } else {
            render_error = true;
        }
    }

    cecs_graphics_system_free(&system);
    cecs_world_free(&world);
    glfwDestroyWindow(window);
    glfwTerminate();

    if (render_error) {
        fprintf(stderr, "Error rendering to surface\n");
        return EXIT_FAILURE;
    } else {
        return EXIT_SUCCESS;
    }
}