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

static cecs_mesh_builder *mesh_builder_configure_square(cecs_mesh_builder *builder) {
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
    cecs_mesh_builder_set_vertex_attribute(builder, CECS_COMPONENT_ID(uv2_f32_attribute),
        (uv2_f32_attribute[]) {
            // quad 4 uvs
            { .u = 0.0f, .v = 1.0f },
            { .u = 1.0f, .v = 1.0f },
            { .u = 1.0f, .v = 0.0f },
            { .u = 0.0f, .v = 0.0f },
        },
        4,
        sizeof(uv2_f32_attribute)
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

    cecs_arena builder_arena = cecs_arena_create();
    cecs_file_mesh_builder_gltf builder = cecs_file_mesh_builder_gltf_create(&system.world, (cecs_mesh_builder_descriptor){
        .vertex_attributes_expected_count = 2,
        .index_format = WGPUIndexFormat_Uint16,
    }, &builder_arena, "../../examples/graphics_app/assets/Duck.gltf");
    cecs_file_mesh_builder_gltf_set_all_vertex_attributes(
        &builder,
        cgltf_attribute_type_position,
        CECS_COMPONENT_ID(position2_f32_attribute),
        sizeof(position2_f32_attribute),
        cecs_attribute_copy_expect_larger_copy_padded
    );
    cecs_file_mesh_builder_gltf_set_all_vertex_attributes(
        &builder,
        cgltf_attribute_type_texcoord,
        CECS_COMPONENT_ID(uv2_f32_attribute),
        sizeof(uv2_f32_attribute),
        cecs_attribute_copy_expect_exact
    );
    cecs_file_mesh_builder_gltf_set_all_indices(
        &builder,
        cecs_attribute_copy_expect_exact
    );
    
    assert(cecs_file_mesh_builder_gltf_mesh_count(&builder) == 1);
    cecs_index_stream index_stream;
    cecs_mesh mesh = cecs_mesh_builder_build_into_and_clear(&world, builder.mesh_builders, &system.context, &index_stream);
    cecs_entity_id id = cecs_world_add_entity_with_indexed_mesh(&world, &mesh, &index_stream);

    // TODO: add suport for multi texture functions 
    cecs_texture_builder builder_texture = cecs_texture_builder_create(&system.world, &builder_arena, (cecs_texture_builder_descriptor){
        .bytes_per_texel = 4,
        .channel_count = 4,
        .flags = cecs_texture_builder_descriptor_config_generate_mipmaps
    });
    cecs_texture_builder_load_from(
        &builder_texture,
        "../../examples/graphics_app/assets/DuckCM.png",
        WGPUTextureDimension_2D,
        WGPUTextureFormat_RGBA8Unorm,
        WGPUTextureUsage_CopyDst | WGPUTextureUsage_TextureBinding
    );
    cecs_texture_in_bank_bundle bank = cecs_texture_builder_build_in_bank(&builder_texture, &system.context, &(WGPUTextureViewDescriptor){
        .arrayLayerCount = 1,
        .baseArrayLayer = 0,
        .baseMipLevel = 0,
        .dimension = WGPUTextureViewDimension_2D,
        .format = WGPUTextureFormat_RGBA8Unorm,
        .mipLevelCount = builder_texture.texture_descriptor.mipLevelCount,
        .aspect = WGPUTextureAspect_All,
    });
    CECS_WORLD_SET_COMPONENT(cecs_texture_in_bank_reference, &world, id, &bank.reference);
    
    cecs_instance_builder builder_instance = cecs_instance_builder_create(&system.world, (cecs_instance_builder_descriptor){
        .instance_attributes_expected_count = 2,
    }, &builder_arena);
    cecs_instance_builder_set_instance_attribute(&builder_instance, CECS_COMPONENT_ID(cecs_texture_in_bank_range2_u8_attribute),
        &bank.range,
        1,
        sizeof(cecs_texture_in_bank_range2_u8_attribute)
    );
    cecs_instance_builder_set_instance_attribute(&builder_instance, CECS_COMPONENT_ID(cecs_texture_subrect2_f32_attribute),
        &bank.subrect,
        1,
        sizeof(cecs_texture_subrect2_f32_attribute)
    );
    cecs_instance_builder_set_instance_attribute(&builder_instance, CECS_COMPONENT_ID(instance_position2_f32_attribute),
        (instance_position2_f32_attribute[]) {
            { .x = 0.0f, .y = 0.0f }
        },
        1,
        sizeof(instance_position2_f32_attribute)
    );
    cecs_instance_group group = cecs_instance_builder_build_into_and_clear(&world, &builder_instance, &system.context);
    CECS_WORLD_SET_COMPONENT(cecs_instance_group, &world, id, &group);
    cecs_arena_free(&builder_arena);

    test_pass pass = test_pass_create(&system.context, (cecs_render_target_info){
        .format = CECS_OPTION_GET(cecs_optional_surface_context, system.context.surface_context).configuration.format,
        .sample_count = 1,
        .aspect_ratio = 640.0f / 480.0f,
    }, &system.world.world.resources.resources_arena);
    
    WGPUColor clear_color = { 0.9, 0.1, 0.2, 1.0 };
    bool render_error = false;
    while (!glfwWindowShouldClose(window) && !render_error) {
        glfwPollEvents();

        struct timespec t;
        timespec_get(&t, TIME_UTC);
        double t_sec = (double)t.tv_sec + (double)t.tv_nsec / 1e9;
        clear_color = (WGPUColor){ sin(t_sec) * 0.5f + 0.5f, cos(t_sec) * 0.5f + 0.5f, 0.0f, 1.0f };

        CECS_GRAPHICS_SYSTEM_SET_COMPONENT_AS_UNIFORM(color4_f32_uniform, &system, &world, id, &((color4_f32_uniform){
            .r = clear_color.r,
            .g = clear_color.g,
            .b = clear_color.b,
            .a = clear_color.a,
        }));
        CECS_GRAPHICS_SYSTEM_SET_COMPONENT_AS_UNIFORM(position4_f32_uniform, &system, &world, id, &((position4_f32_uniform){
            .x = 0.5f * sin(t_sec),
            .y = 0.5f * cos(t_sec),
            .z = 0.0f,
            .w = 0.0f,
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