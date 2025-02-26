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
    cecs_mesh_builder_set_vertex_attribute(builder, CECS_COMPONENT_ID(position3_f32_attribute),
        (position3_f32_attribute[]) {
            // quad 4 verts
            { .x = -0.5f, .y = -0.5f, .z = -0.5f },
            { .x =  0.5f, .y = -0.5f, .z = -0.5f },
            { .x =  0.5f, .y =  0.5f, .z = -0.5f },
            { .x = -0.5f, .y =  0.5f, .z = -0.5f },
        },
        4,
        sizeof(position3_f32_attribute)
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
        CECS_COMPONENT_ID(position3_f32_attribute),
        sizeof(position3_f32_attribute),
        cecs_attribute_copy_expect_exact
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
        (cecs_texture_in_bank_range2_u8_attribute[]){
            bank.range, bank.range, bank.range, bank.range
        },
        4,
        sizeof(cecs_texture_in_bank_range2_u8_attribute)
    );
    cecs_instance_builder_set_instance_attribute(&builder_instance, CECS_COMPONENT_ID(cecs_texture_subrect2_f32_attribute),
        (cecs_texture_subrect2_f32_attribute[]) {
            bank.subrect, bank.subrect, bank.subrect, bank.subrect
        },
        4,
        sizeof(cecs_texture_subrect2_f32_attribute)
    );
    cecs_instance_builder_set_instance_attribute(&builder_instance, CECS_COMPONENT_ID(instance_position3_f32_attribute),
        (instance_position3_f32_attribute[]) {
            { .x = 0.0f, .y = 0.0f, .z = 0.0f},
            { .x = 1.0f, .y = 0.0f, .z = 0.0f},
            { .x = 0.0f, .y = 1.0f, .z = 0.0f},
            { .x = 0.0f, .y = 0.0f, .z = 1.0f},
        },
        4,
        sizeof(instance_position3_f32_attribute)
    );
    cecs_instance_group group = cecs_instance_builder_build_into_and_clear(&world, &builder_instance, &system.context);
    CECS_WORLD_SET_COMPONENT(cecs_instance_group, &world, id, &group);
    cecs_arena_free(&builder_arena);

    const cecs_render_target_info target_info = {
        .format = CECS_OPTION_GET(cecs_optional_surface_context, system.context.surface_context).configuration.format,
        .sample_count = 1,
        .aspect_ratio = 640.0f / 480.0f,
    };
    test_pass pass = test_pass_create(&system.context, target_info, &system.world.world.resources.resources_arena);
    
    WGPUColor clear_color = { 0.9, 0.1, 0.2, 1.0 };
    cecs_camera_pack camera = (cecs_camera_pack){
        .bundle = (cecs_camera_bundle){
            .camera = cecs_camera_create_perspective(3.14f / 2.0f, 400.0f, 0.3f),
            .position = (cecs_position3_f32){ .x = 0.0f, .y = 0.0f, .z = -200.0f },
            .orientation = cecs_versor_packed_f32_identity,
        },
        .near = 0.3f,
        .flags = cecs_camera_options_perspective,
    };
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

        const cecs_radians_f32 angle = (float)fmod(t_sec, 3.141592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342117067982148086513282306647093844609550582231725359408128481117450284102701938521105559644622948954930381964428810975665933446128475648233786783165271201909145648566923460348610454326648213393607260249141273724587006606315588174881520920962829254091715364367892590360011330530548820466521384146951941511609433057270365759591953092186117381932611793105118548074462379962749567351885752724891227938183011949129833673362440656643086021394946395224737190702179860943702770539217176293176752384674818467669405132000568127145263560827785771342757789609173637178721468440901224953430146549585371050792279689258923542019956112129021960864034418159813629774771309960518707211349999998372978049951059731732816096318595024459455346908302642522308253344685035261931188171010003137838752886587533208381420617177669147303598253490428755468731159562863882353787593751957781857780532171226806613001927876611195909216420198938095257201065485863278865936153381827968230301952035301852968995773622599413891249721775283479131515574857242454150695950829533116861727855889075098381754637464939319255060400927701671139009848824012858361603563707660104710181942955596198946767837449448255379774726847104047534646208046684259069491293313677028989152104752162056966024058038150193511253382430035587640247496473263914199272604269922796782354781636009341721641219924586315030286182974555706749838505494588586926995690927210797 * 2.0f)
            - 3.141592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342117067982148086513282306647093844609550582231725359408128481117450284102701938521105559644622948954930381964428810975665933446128475648233786783165271201909145648566923460348610454326648213393607260249141273724587006606315588174881520920962829254091715364367892590360011330530548820466521384146951941511609433057270365759591953092186117381932611793105118548074462379962749567351885752724891227938183011949129833673362440656643086021394946395224737190702179860943702770539217176293176752384674818467669405132000568127145263560827785771342757789609173637178721468440901224953430146549585371050792279689258923542019956112129021960864034418159813629774771309960518707211349999998372978049951059731732816096318595024459455346908302642522308253344685035261931188171010003137838752886587533208381420617177669147303598253490428755468731159562863882353787593751957781857780532171226806613001927876611195909216420198938095257201065485863278865936153381827968230301952035301852968995773622599413891249721775283479131515574857242454150695950829533116861727855889075098381754637464939319255060400927701671139009848824012858361603563707660104710181942955596198946767837449448255379774726847104047534646208046684259069491293313677028989152104752162056966024058038150193511253382430035587640247496473263914199272604269922796782354781636009341721641219924586315030286182974555706749838505494588586926995690927210797;
        //const cecs_versor_f32 orientation = cecs_versor_f32_axis_angle((cecs_vec3_f32){ 0.0f, 1.0f, 0.0f }, angle);
        
        static const float radius = 200.0f; 
        const cecs_position3_f32 position = (cecs_position3_f32){
            .x = (float)cos(-angle - 3.141592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342117067982148086513282306647093844609550582231725359408128481117450284102701938521105559644622948954930381964428810975665933446128475648233786783165271201909145648566923460348610454326648213393607260249141273724587006606315588174881520920962829254091715364367892590360011330530548820466521384146951941511609433057270365759591953092186117381932611793105118548074462379962749567351885752724891227938183011949129833673362440656643086021394946395224737190702179860943702770539217176293176752384674818467669405132000568127145263560827785771342757789609173637178721468440901224953430146549585371050792279689258923542019956112129021960864034418159813629774771309960518707211349999998372978049951059731732816096318595024459455346908302642522308253344685035261931188171010003137838752886587533208381420617177669147303598253490428755468731159562863882353787593751957781857780532171226806613001927876611195909216420198938095257201065485863278865936153381827968230301952035301852968995773622599413891249721775283479131515574857242454150695950829533116861727855889075098381754637464939319255060400927701671139009848824012858361603563707660104710181942955596198946767837449448255379774726847104047534646208046684259069491293313677028989152104752162056966024058038150193511253382430035587640247496473263914199272604269922796782354781636009341721641219924586315030286182974555706749838505494588586926995690927210797 * 0.5) * radius,
            .y = (float)sin(angle * 2.0) * radius * 0.5f,
            .z = (float)sin(-angle - 3.141592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342117067982148086513282306647093844609550582231725359408128481117450284102701938521105559644622948954930381964428810975665933446128475648233786783165271201909145648566923460348610454326648213393607260249141273724587006606315588174881520920962829254091715364367892590360011330530548820466521384146951941511609433057270365759591953092186117381932611793105118548074462379962749567351885752724891227938183011949129833673362440656643086021394946395224737190702179860943702770539217176293176752384674818467669405132000568127145263560827785771342757789609173637178721468440901224953430146549585371050792279689258923542019956112129021960864034418159813629774771309960518707211349999998372978049951059731732816096318595024459455346908302642522308253344685035261931188171010003137838752886587533208381420617177669147303598253490428755468731159562863882353787593751957781857780532171226806613001927876611195909216420198938095257201065485863278865936153381827968230301952035301852968995773622599413891249721775283479131515574857242454150695950829533116861727855889075098381754637464939319255060400927701671139009848824012858361603563707660104710181942955596198946767837449448255379774726847104047534646208046684259069491293313677028989152104752162056966024058038150193511253382430035587640247496473263914199272604269922796782354781636009341721641219924586315030286182974555706749838505494588586926995690927210797 * 0.5) * radius,
        };
        const cecs_vec3_f32 forward = (cecs_vec3_f32){-position.x, -position.y, -position.z};
        const cecs_orientation3_f32 orientation_packed = cecs_versor_packed_f32_pack(cecs_versor_f32_look_z_up(
            forward,
            (cecs_vec3_f32){0.0f, 1.0f, 0.0f}
        ));
        camera.bundle.position = position;
        camera.bundle.orientation = orientation_packed;
        cecs_surface_render_target surface_target;
        if (cecs_graphics_context_get_surface_render_target(&system.context, &surface_target)) {
            test_pass_draw(&pass, &world, &system, &surface_target, &target_info, camera);
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