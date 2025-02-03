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
    // WGPUSupportedLimits limits;
    // wgpuDeviceGetLimits(system.context.device, &limits);
    // printf("maxBindingsPerBindGroup: %u\n", limits.limits.maxBindingsPerBindGroup);

    cecs_arena builder_arena = cecs_arena_create();
    
    cecs_mesh_builder builder = cecs_mesh_builder_create(&system.world, (cecs_mesh_builder_descriptor){
        .vertex_attributes_expected_count = 2,
        .index_format = WGPUIndexFormat_Uint16,
    }, &builder_arena);
    mesh_builder_configure_sqaure(&builder);
    cecs_index_stream index_stream;
    cecs_mesh mesh = cecs_mesh_builder_build_into_and_clear(&world, &builder, &system.context, &index_stream);
    cecs_entity_id id = cecs_world_add_entity_with_indexed_mesh(&world, &mesh, &index_stream);
    
    const cecs_uniform_raw_stream *stream = CECS_GRAPHICS_SYSTEM_SET_COMPONENT_AS_UNIFORM(color4_f32_uniform, &system, &world, id, &((color4_f32_uniform){
        .r = 0.5f,
        .g = 0.5f,
        .b = 0.5f,
        .a = 1.0f,
    }));
    CECS_GRAPHICS_SYSTEM_SET_COMPONENT_AS_UNIFORM(position4_f32_uniform, &system, &world, id, &((position4_f32_uniform){
        .x = 0.0f,
        .y = 0.0f,
        .z = 0.0f,
        .w = 0.0f,
    }));
    (void)stream;

    // builder = cecs_mesh_builder_create_from(&builder, builder.descriptor);
    // mesh_builder_configure_sqaure(&builder);
    // mesh = cecs_mesh_builder_build_into_and_clear(&world, &builder, &system.context, &index_stream);
    // cecs_world_add_entity_with_indexed_mesh(&world, &mesh, &index_stream);

    test_pass pass = test_pass_create(&system.context, (cecs_render_target_info){
        .format = CECS_OPTION_GET(cecs_optional_surface_context, system.context.surface_context).configuration.format,
        .sample_count = 1,
        .aspect_ratio = 640.0f / 480.0f,
    }, &system.world.world.resources.resources_arena);

    WGPUColor clear_color = { 0.9, 0.1, 0.2, 1.0 };
    (void)clear_color;
    
    cecs_texture_builder texture_builder = cecs_texture_builder_create(&system.world, &builder_arena, (cecs_texture_builder_descriptor){
        .bytes_per_texel = 4,
        .channel_count = 4,
        .flags = cecs_texture_builder_descriptor_config_generate_mipmaps | cecs_texture_builder_descriptor_config_alloc_mipmaps,
    });
    uint32_t *texture_data = cecs_arena_alloc(&builder_arena, 230 * 90 * sizeof(uint32_t));
    for (size_t i = 0; i < 90; i++) {
        for (size_t j = 0; j < 230; j++) {
            texture_data[i * 230 + j] = (((i / 4 + j / 4)) & 1) ? 0xFFFFFFFF : 0xFF000000;
        }
    }
    cecs_texture_builder_set_data(&texture_builder, (uint8_t *)texture_data, (WGPUTextureDescriptor){
        .dimension = WGPUTextureDimension_2D,
        .format = WGPUTextureFormat_RGBA8Unorm,
        .mipLevelCount = 1,
        .sampleCount = 1,
        .size = (WGPUExtent3D){
            .width = 230,
            .height = 90,
            .depthOrArrayLayers = 1,
        },
        .usage = WGPUTextureUsage_CopyDst | WGPUTextureUsage_TextureBinding,
        .viewFormatCount = 0
    });
    // cecs_texture texture = cecs_texture_builder_build(&texture_builder, &system.context, &(WGPUTextureViewDescriptor){
    //     .format = WGPUTextureFormat_RGBA8Unorm,
    //     .dimension = WGPUTextureViewDimension_2D,
    //     .baseMipLevel = 0,
    //     .mipLevelCount = texture_builder.texture_descriptor.mipLevelCount,
    //     .baseArrayLayer = 0,
    //     .arrayLayerCount = 1,
    // }, 0);
    //CECS_GRAPHICS_SYSTEM_SET_TEXTURE(cecs_texture, &system, &world, id, &texture);
    cecs_texture_in_bank_bundle bundle = cecs_texture_builder_build_in_bank(&texture_builder, &system.context, &(WGPUTextureViewDescriptor){
        .format = WGPUTextureFormat_RGBA8Unorm,
        .dimension = WGPUTextureViewDimension_2D,
        .baseMipLevel = 0,
        .mipLevelCount = texture_builder.texture_descriptor.mipLevelCount,
        .baseArrayLayer = 0,
        .arrayLayerCount = 1,
    });
    CECS_WORLD_SET_COMPONENT(cecs_texture_in_bank_reference, &world, id, &bundle.reference);

    cecs_texture_builder_set_descriptor_no_data(&texture_builder, (WGPUTextureDescriptor){0});
    cecs_texture_builder_load_from(
        &texture_builder,
        "../../examples/graphics_app/src/sample.png",
        WGPUTextureDimension_2D,
        WGPUTextureFormat_RGBA8Unorm,
        WGPUTextureUsage_CopyDst | WGPUTextureUsage_TextureBinding
    );
    cecs_texture_in_bank_bundle secondary_bundle = cecs_texture_builder_build_in_bank(&texture_builder, &system.context, &(WGPUTextureViewDescriptor){
        .format = WGPUTextureFormat_RGBA8Unorm,
        .dimension = WGPUTextureViewDimension_2D,
        .baseMipLevel = 0,
        .mipLevelCount = texture_builder.texture_descriptor.mipLevelCount,
        .baseArrayLayer = 0,
        .arrayLayerCount = 1,
    });


    cecs_instance_builder instance_builder = cecs_instance_builder_create(&system.world, (cecs_instance_builder_descriptor){
        .instance_attributes_expected_count = 2,
    }, &builder_arena);
    cecs_instance_builder_set_instance_attribute(&instance_builder, CECS_COMPONENT_ID(instance_position2_f32_attribute),
        (instance_position2_f32_attribute[]) {
            { .x = -0.5f, .y = -0.5f },
            { .x = 0.5f, .y = -0.5f },
            { .x = 0.5f, .y = 0.5f },
            { .x = -0.5f, .y = 0.5f },
            { .x = 0.0f, .y = 0.0f },
        },
        5,
        sizeof(instance_position2_f32_attribute)
    );
    cecs_instance_builder_set_instance_attribute(&instance_builder, CECS_COMPONENT_ID(cecs_texture_in_bank_range2_u8_attribute),
        (cecs_texture_in_bank_range2_u8_attribute[]) {
            bundle.range,
            secondary_bundle.range,
            bundle.range,
            secondary_bundle.range,
            bundle.range,
        },
        5,
        sizeof(cecs_texture_in_bank_range2_u8_attribute)
    );
    cecs_instance_builder_set_instance_attribute(&instance_builder, CECS_COMPONENT_ID(cecs_texture_subrect2_f32_attribute),
        (cecs_texture_subrect2_f32_attribute[]) {
            bundle.subrect,
            secondary_bundle.subrect,
            bundle.subrect,
            secondary_bundle.subrect,
            bundle.subrect,
        },
        5,
        sizeof(cecs_texture_subrect2_f32_attribute)
    );
    cecs_instance_group instances = cecs_instance_builder_build_into_and_clear(&world, &instance_builder, &system.context);
    CECS_WORLD_SET_COMPONENT(cecs_instance_group, &world, id, &instances);

    cecs_arena_free(&builder_arena);

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