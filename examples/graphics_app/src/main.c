#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <webgpu/webgpu.h>
#include <webgpu/wgpu.h>
#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>
#include <cecs_graphics.h>
#include <memory.h>

#include <math.h>
#include <time.h>
#include <assert.h>

#include "test_pass.h"

static cecs_quaternion_f32 rotate_camera_from_mouse_delta(
    const cecs_versor_f32 camera_uq,
    const cecs_vec2_f32 mouse_delta,
    const cecs_vec3_f32 upward,
    const cecs_vec3_f32 right
) {
    const cecs_quaternion_f32 yaw =     cecs_quaternion_f32_as(cecs_versor_f32_axis_angle(upward, mouse_delta.x));
    const cecs_quaternion_f32 pitch =   cecs_quaternion_f32_as(cecs_versor_f32_axis_angle(right, -mouse_delta.y));
    return cecs_quaternion_f32_product(yaw, cecs_quaternion_f32_product(cecs_quaternion_f32_as(camera_uq), pitch));
}

typedef struct cecs_window_resize_userdata {
    cecs_graphics_system *system;
    cecs_texture *depth_texture;
    test_pass *pass;
    cecs_world *world;
    cecs_camera_pack *camera;
} cecs_window_resize_userdata;

static bool draw(cecs_world *world, cecs_graphics_system *system, test_pass *pass, const cecs_camera_raw_bundle *camera) {
    cecs_surface_render_target surface_target;
    if (cecs_graphics_context_get_surface_render_target(&system->context, &surface_target)) {
        test_pass_draw(pass, world, system, &surface_target, *camera);
        cecs_graphics_context_present_surface_render_target(&system->context, &surface_target);
        return true;
    } else {
        return false;
    }
}

static cecs_texture create_depth_texture(cecs_graphics_system *system, const uint32_t width, const uint32_t height) {
    const WGPUExtent3D extent = {
        .width = width,
        .height = height,
        .depthOrArrayLayers = 1,
    };
    cecs_texture_builder builder_depth = cecs_texture_builder_create((WGPUTextureDescriptor) {
        .dimension = WGPUTextureDimension_2D,
        .format = WGPUTextureFormat_Depth24Plus,
        .mipLevelCount = 1,
        .nextInChain = NULL,
        .sampleCount = 1,
        .size = extent,
        .usage = WGPUTextureUsage_RenderAttachment,
        .viewFormatCount = 0,
        .viewFormats = NULL,
    }, &system->world, NULL);
    WGPUTexture depth = cecs_texture_builder_build_alloc(&builder_depth, &system->context);
    const cecs_texture texture = {
        .texture_view = wgpuTextureCreateView(depth, &(WGPUTextureViewDescriptor){
            .format = WGPUTextureFormat_Depth24Plus,
            .dimension = WGPUTextureViewDimension_2D,
            .baseMipLevel = 0,
            .mipLevelCount = 1,
            .baseArrayLayer = 0,
            .arrayLayerCount = 1,
            .aspect = WGPUTextureAspect_DepthOnly,
        }),
        .extent = extent,
    };
    wgpuTextureRelease(depth);
    return texture;
}

static void cecs_window_resize(GLFWwindow* window, int width, int height) {
    assert(width > 0);
    assert(height > 0);

    cecs_window_resize_userdata *userdata = glfwGetWindowUserPointer(window);
    assert(userdata != NULL);

    glfwGetFramebufferSize(window, &width, &height);
    assert(width > 0);
    assert(height > 0);

    cecs_surface_context *surface = &CECS_OPTION_GET_UNCHECKED(cecs_optional_surface_context, userdata->system->context.surface_context);
    wgpuSurfaceUnconfigure(surface->surface);
    
    surface->configuration.width = (uint32_t)width;
    surface->configuration.height = (uint32_t)height;
    wgpuSurfaceConfigure(surface->surface, &surface->configuration);

    wgpuTextureViewRelease(userdata->depth_texture->texture_view);
    *userdata->depth_texture = create_depth_texture(userdata->system, (uint32_t)width, (uint32_t)height);

    const cecs_camera_raw_bundle camera = cecs_camera_raw_bundle_from_pack(*userdata->camera, (float)width / (float)height);
    bool redraw = draw(userdata->world, userdata->system, userdata->pass, &camera);
    assert(redraw);
}

static bool init_window(GLFWwindow **out_window, size_t *out_width, size_t *out_height) {
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return false;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // <-- extra info for glfwCreateWindow
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    const int default_width = 1200;
    const int default_height = 675;
    {
        int width;
        int height;
        glfwGetMonitorWorkarea(glfwGetPrimaryMonitor(), NULL, NULL, &width, &height);
        glfwWindowHint(GLFW_POSITION_X, (width - default_width) / 2);
        glfwWindowHint(GLFW_POSITION_Y, (height - default_height) / 2);
    }

    GLFWwindow *window = glfwCreateWindow(default_width, default_height, "cecs_graphics with WebGPU!", NULL, NULL);
    if (window == NULL) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return false;
    }

    int width;
    int height;
    glfwGetFramebufferSize(window, &width, &height);
    assert(width > 0);
    assert(height > 0);

    *out_window = window;
    *out_width = (size_t)width;
    *out_height = (size_t)height;
    return true;
}

static cecs_mesh create_duck_mesh(cecs_world *world, cecs_graphics_system *system, cecs_arena *builder_arena, const char *filepath, cecs_index_stream *out_index_stream) {
    cecs_file_mesh_builder_gltf builder = cecs_file_mesh_builder_gltf_create(&system->world, (cecs_mesh_builder_descriptor){
        .vertex_attributes_expected_count = 2,
        .index_format = WGPUIndexFormat_Uint16,
    }, builder_arena, filepath);
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
    return cecs_mesh_builder_build_into_and_clear(world, builder.mesh_builders, &system->context, out_index_stream);
}

static cecs_texture_in_bank_bundle create_duck_file_texture(
    cecs_graphics_system *system,
    cecs_arena *builder_arena,
    const char *filepath
) {
    cecs_file_texture2_builder builder_texture = cecs_file_texture2_builder_create_for_lower(&system->world, builder_arena, (cecs_file_texture2_builder_descriptor){
        .channel_count = 4,
        .descriptor = (cecs_mem_texture_builder_descriptor){
            .descriptor = (WGPUTextureDescriptor){
                .dimension = WGPUTextureDimension_2D,
                .format = WGPUTextureFormat_RGBA8Unorm,
                .usage = WGPUTextureUsage_CopyDst | WGPUTextureUsage_TextureBinding,
                .sampleCount = 1,
                .viewFormatCount = 0,
                .size = (WGPUExtent3D){
                    .width = 1,
                    .height = 1,
                    .depthOrArrayLayers = 1,
                },
                .mipLevelCount = 0
            },
            .configuration = (cecs_mem_texture_builder_configuration_descriptor){
                .bytes_per_texel = 4,
                .max_mip_level = cecs_mem_texture_builder_max_mip_level,
                .flags = cecs_mem_texture_builder_descriptor_config_generate_mipmaps,
            }
        }
    }, filepath);
    cecs_file_texture2_builder_load_into(
        &builder_texture,
        filepath,
        0
    );

    return cecs_file_texture2_builder_build_in_bank(&builder_texture, &system->context, &(WGPUTextureViewDescriptor){
        .arrayLayerCount = 1,
        .baseArrayLayer = 0,
        .baseMipLevel = 0,
        .dimension = WGPUTextureViewDimension_2D,
        .format = WGPUTextureFormat_RGBA8Unorm,
        .mipLevelCount = builder_texture.builder.builder.descriptor.mipLevelCount,
        .aspect = WGPUTextureAspect_All,
    }, 2);
}

static cecs_texture_in_bank_bundle create_duck_pattern_texture(
    cecs_graphics_system *system,
    cecs_arena *builder_arena
) {
    cecs_mem_texture_builder builder_texture_mem = cecs_mem_texture_builder_create(&system->world, builder_arena, (cecs_mem_texture_builder_descriptor){
        .descriptor = (WGPUTextureDescriptor){
            .dimension = WGPUTextureDimension_2D,
            .format = WGPUTextureFormat_RGBA8Unorm,
            .usage = WGPUTextureUsage_CopyDst | WGPUTextureUsage_TextureBinding,
            .sampleCount = 1,
            .viewFormatCount = 0,
            .size = (WGPUExtent3D){
                .width = 512,
                .height = 512,
                .depthOrArrayLayers = 1,
            },
            .mipLevelCount = 0
        },
        .configuration = (cecs_mem_texture_builder_configuration_descriptor){
            .bytes_per_texel = 4,
            .max_mip_level = cecs_mem_texture_builder_max_mip_level,
            .flags =
                cecs_mem_texture_builder_descriptor_config_generate_mipmaps
                | cecs_mem_texture_builder_descriptor_config_alloc_mipmaps,
        }
    });
    uint8_t *pattern_texture;
    {
        const uint32_t width = builder_texture_mem.builder.descriptor.size.width;
        const uint32_t height = builder_texture_mem.builder.descriptor.size.height;
        const uint_fast8_t bytes_per_texel = builder_texture_mem.descriptor.bytes_per_texel; 
        pattern_texture = cecs_arena_alloc(
            builder_arena,
            width * height * bytes_per_texel
        );
        for (uint32_t i = 0; i < width; ++i) {
            for (uint32_t j = 0; j < height; ++j) {
                const uint32_t index = (j * width + i) * bytes_per_texel;
                pattern_texture[index + 0] = (uint8_t)(i * 255 / width);
                pattern_texture[index + 1] = (uint8_t)(j * 255 / height);
                pattern_texture[index + 2] = 0;
                pattern_texture[index + 3] = 255;
            }
        }
    }
    cecs_mem_texture_builder_take_into_mut(&builder_texture_mem, pattern_texture, 0);

    return cecs_mem_texture_builder_build_in_bank(&builder_texture_mem, &system->context, &(WGPUTextureViewDescriptor){
        .arrayLayerCount = 1,
        .baseArrayLayer = 0,
        .baseMipLevel = 0,
        .dimension = WGPUTextureViewDimension_2D,
        .format = WGPUTextureFormat_RGBA8Unorm,
        .mipLevelCount = builder_texture_mem.builder.descriptor.mipLevelCount,
        .aspect = WGPUTextureAspect_All,
    }, 1, 0);
}

static cecs_instance_group create_duck_instance(
    cecs_world *world,
    cecs_graphics_system *system,
    cecs_arena *builder_arena,
    const cecs_texture_in_bank_range2_u8_attribute textures[4],
    const cecs_texture_subrect2_f32_attribute subrects[4]
) {
    cecs_instance_builder builder_instance = cecs_instance_builder_create(&system->world, (cecs_instance_builder_descriptor){
        .instance_attributes_expected_count = 3,
    }, builder_arena);
    cecs_instance_builder_set_instance_attribute(&builder_instance, CECS_COMPONENT_ID(cecs_texture_in_bank_range2_u8_attribute),
        textures,
        4,
        sizeof(cecs_texture_in_bank_range2_u8_attribute)
    );
    cecs_instance_builder_set_instance_attribute(&builder_instance, CECS_COMPONENT_ID(cecs_texture_subrect2_f32_attribute),
        subrects,
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
    return cecs_instance_builder_build_into_and_clear(world, &builder_instance, &system->context);
}

int main(void) {
    GLFWwindow *window;
    size_t width;
    size_t height;
    if (!init_window(&window, &width, &height)) {
        return EXIT_FAILURE;
    }

    cecs_world world = cecs_world_create(64, 16, 4);
    cecs_graphics_system system = cecs_graphics_system_create(1024, 8, window);

    cecs_arena builder_arena = cecs_arena_create();
    cecs_index_stream index_stream;
    cecs_mesh mesh = create_duck_mesh(&world, &system, &builder_arena, "../../examples/graphics_app/assets/Duck.gltf", &index_stream);
    cecs_texture_in_bank_bundle bank_image = create_duck_file_texture(&system, &builder_arena, "../../examples/graphics_app/assets/DuckCM.png");
    cecs_texture_in_bank_bundle bank_pattern = create_duck_pattern_texture(&system, &builder_arena);
    assert(bank_image.reference.texture_id == bank_pattern.reference.texture_id);

    cecs_instance_group duck_instances = create_duck_instance(
        &world,
        &system,
        &builder_arena,
        (cecs_texture_in_bank_range2_u8_attribute[]){
            bank_image.range, bank_pattern.range, bank_image.range, bank_pattern.range
        },
        (cecs_texture_subrect2_f32_attribute[]){
            bank_image.subrect, bank_pattern.subrect, bank_image.subrect, bank_pattern.subrect
        }
    );
    cecs_arena_free(&builder_arena);
    
    cecs_entity_id id = cecs_world_add_entity_with_indexed_mesh(&world, &mesh, &index_stream);
    CECS_WORLD_SET_COMPONENT(cecs_texture_in_bank_reference, &world, id, &bank_image.reference);
    CECS_WORLD_SET_COMPONENT(cecs_instance_group, &world, id, &duck_instances);

    const cecs_entity_id depth_entity = cecs_world_add_entity(&system.world.world);
    cecs_texture *depth_texture; {
        cecs_texture depth = create_depth_texture(&system, (uint32_t)width, (uint32_t)height);
        depth_texture = CECS_WORLD_SET_COMPONENT(cecs_texture, &system.world.world, depth_entity, &depth);
    }
    
    test_pass pass; {
        const WGPUSurfaceConfiguration configuration = CECS_OPTION_GET(cecs_optional_surface_context, system.context.surface_context).configuration;
        const cecs_render_target_info target_info = {
            .format = configuration.format,
            .sample_count = 1,
            .aspect_ratio = (float)configuration.width / (float)configuration.height,
        };
        pass = test_pass_create(&system.context, target_info, &system.world.world.resources.resources_arena, depth_texture);
    }
    
    // cornflower blue
    WGPUColor clear_color = (WGPUColor){ 0.392f, 0.584f, 0.929f, 1.0f };
    cecs_camera_pack camera = (cecs_camera_pack){
        .bundle = (cecs_camera_bundle){
            .camera = cecs_camera_create_perspective(3.14f / 2.0f, 400.0f, 4.0f),
            .position = (cecs_position3_f32){ .x = 0.0f, .y = 0.0f, .z = -200.0f },
            .orientation = cecs_versor_packed_f32_identity,
        },
        .near = 4.0f,
        .flags = cecs_camera_options_perspective,
    };
    cecs_window_resize_userdata resize_userdata = {
        .system = &system,
        .depth_texture = depth_texture,
        .pass = &pass,
        .world = &world,
        .camera = &camera,
    };
    glfwSetWindowUserPointer(window, &resize_userdata);
    glfwSetFramebufferSizeCallback(window, cecs_window_resize);

    bool render_error = false;
    double mouse_x = 0.0;
    double mouse_y = 0.0;
    glfwGetCursorPos(window, &mouse_x, &mouse_y);
    
    cecs_vec3_f32 local_forward = { 0.0f, 0.0f, 1.0f };
    cecs_vec3_f32 local_upward = { 0.0f, 1.0f, 0.0f };
    cecs_vec3_f32 local_right = { 1.0f, 0.0f, 0.0f };
    
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
        const cecs_orientation4_f32 orientation = cecs_versor_f32_look_z_up(
            forward,
            (cecs_vec3_f32){0.0f, 1.0f, 0.0f}
        );
        
        //camera.bundle.position = position;
        //camera.bundle.orientation = cecs_versor_packed_f32_pack(orientation);
        
        double mouse_x_new;
        double mouse_y_new;
        glfwGetCursorPos(window, &mouse_x_new, &mouse_y_new);
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
            cecs_vec2_f32 mouse_delta = (cecs_vec2_f32){
                .x = (float)(mouse_x_new - mouse_x) * 0.005f,
                .y = (float)(mouse_y - mouse_y_new) * 0.005f,
            };
            
            {
                int content_width;
                int content_height;
                glfwGetWindowSize(window, &content_width, &content_height);
    
                const double wrapped_mouse_x0 = fmod(mouse_x_new, (double)(content_width - 1));
                const double wrapped_mouse_y0 = fmod(mouse_y_new, (double)(content_height - 1));
        
                const double wrapped_mouse_x1 = wrapped_mouse_x0 + content_width;
                const double wrapped_mouse_y1 = wrapped_mouse_y0 + content_height;
        
                const double wrapped_mouse_x[2] = { wrapped_mouse_x0, wrapped_mouse_x1 };
                const double wrapped_mouse_y[2] = { wrapped_mouse_y0, wrapped_mouse_y1 };
                mouse_x_new = wrapped_mouse_x[mouse_x_new < 0.0];
                mouse_y_new = wrapped_mouse_y[mouse_y_new < 0.0];
                glfwSetCursorPos(window, mouse_x_new, mouse_y_new);
            }
    
            if (mouse_delta.x != 0.0 || mouse_delta.y != 0.0) {
                const cecs_versor_f32 camera_uq = cecs_versor_f32_unpack(camera.bundle.orientation);
                const cecs_quaternion_f32 displaced_orientation = (rotate_camera_from_mouse_delta(
                    camera_uq,
                    mouse_delta,
                    (cecs_vec3_f32){ 0.0f, 1.0f, 0.0f },
                    (cecs_vec3_f32){ 1.0f, 0.0f, 0.0f }
                ));
                const cecs_versor_f32 displaced_orientation_normalized = cecs_versor_f32_of(displaced_orientation);
                local_forward = cecs_versor_f32_rotate(displaced_orientation_normalized, (cecs_vec3_f32){ 0.0f, 0.0f, 1.0f });
                local_right = cecs_versor_f32_rotate(displaced_orientation_normalized, (cecs_vec3_f32){ 1.0f, 0.0f, 0.0f });
                local_upward = cecs_vec3_f32_cross(local_forward, local_right);
    
                const cecs_orientation3_f32 orientation_packed = cecs_versor_packed_f32_pack(displaced_orientation_normalized);
    
                assert(!isnan(displaced_orientation_normalized.i));
                camera.bundle.orientation = orientation_packed;
            }
            
            
            static const float speed = 3.0f;
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
                camera.bundle.position = cecs_vec3_f32_add(camera.bundle.position, cecs_vec3_f32_mul(local_forward, speed));
            }
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
                camera.bundle.position = cecs_vec3_f32_sub(camera.bundle.position, cecs_vec3_f32_mul(local_forward, speed));
            }
            
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
                camera.bundle.position = cecs_vec3_f32_sub(camera.bundle.position, cecs_vec3_f32_mul(local_right, speed));
            }
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
                camera.bundle.position = cecs_vec3_f32_add(camera.bundle.position, cecs_vec3_f32_mul(local_right, speed));
            }
            
            if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
                camera.bundle.position = cecs_vec3_f32_add(camera.bundle.position, cecs_vec3_f32_mul(local_upward, speed));
            }
            if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
                camera.bundle.position = cecs_vec3_f32_sub(camera.bundle.position, cecs_vec3_f32_mul(local_upward, speed));
            }
        }
        mouse_x = mouse_x_new;
        mouse_y = mouse_y_new;

        const WGPUSurfaceConfiguration configuration = CECS_OPTION_GET(cecs_optional_surface_context, system.context.surface_context).configuration;
        const cecs_camera_raw_bundle camera_raw = cecs_camera_raw_bundle_from_pack(camera, (float)configuration.width / (float)configuration.height);
        render_error |= !draw(&world, &system, &pass, &camera_raw);
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