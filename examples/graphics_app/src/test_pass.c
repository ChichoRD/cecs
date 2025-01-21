#include <stddef.h>
#include <stdio.h>
#include <assert.h>
#include "test_pass.h"

test_pass test_pass_create(cecs_graphics_context *context, cecs_render_target_info target_info, cecs_arena *arena) {
    FILE *shader_file = NULL;
    assert(
        fopen_s(&shader_file, "../../examples/graphics_app/src/test_pass.wgsl", "r") == 0
        && "fatal error: failed to open shader file"
    );

    fseek(shader_file, 0, SEEK_END);
    size_t shader_size = ftell(shader_file);
    fseek(shader_file, 0, SEEK_SET);

    char *shader_code = cecs_arena_alloc(arena, shader_size + 1);
    const size_t readed_bytes = fread(shader_code, sizeof(char), shader_size, shader_file);
    assert(!ferror(shader_file) && "fatal error: failed to read shader file");
    assert(readed_bytes <= shader_size && "fatal error: readed bytes mismatch");
    fclose(shader_file);
    
    shader_code[readed_bytes] = '\0';


    const WGPUShaderModuleWGSLDescriptor shader_code_descriptor = {
        .chain = {
            .next = NULL,
            .sType = WGPUSType_ShaderModuleWGSLDescriptor,
        },
        .code = shader_code,
    };

    const WGPUShaderModuleDescriptor shader_module_descriptor = {
        .label = "Test Pass Shader",
        .nextInChain = &shader_code_descriptor.chain,
#ifdef WEBGPU_BACKEND_WGPU
        .hintCount = 0,
        .hints = NULL,
#endif
    };

    const WGPUShaderModule shader_module = wgpuDeviceCreateShaderModule(context->device, &shader_module_descriptor);
    WGPUBuffer global_uniform_buffer = cecs_wgpu_buffer_create_with_data(
        context->device,
        WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform,
        sizeof(camera_matrix_uniform),
        &(camera_matrix_uniform){0},
        sizeof(camera_matrix_uniform)
    );

    WGPUBindGroupLayout local_bgl = wgpuDeviceCreateBindGroupLayout(context->device, &(WGPUBindGroupLayoutDescriptor) {
        .label = "Test Pass Local Bind Group Layout",
        .nextInChain = NULL,
        .entryCount = 2,
        .entries = (WGPUBindGroupLayoutEntry[]) {
            {
                .binding = 0,
                .visibility = WGPUShaderStage_Fragment,
                .buffer = (WGPUBufferBindingLayout) {
                    .type = WGPUBufferBindingType_Uniform,
                    .hasDynamicOffset = true,
                    .minBindingSize = sizeof(color4_f32_uniform),
                    .nextInChain = NULL,
                },
            },
            {
                .binding = 1,
                .visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment,
                .buffer = (WGPUBufferBindingLayout) {
                    .type = WGPUBufferBindingType_Uniform,
                    .hasDynamicOffset = true,
                    .minBindingSize = sizeof(position4_f32_uniform),
                    .nextInChain = NULL,
                },
            }
        },
    });
    WGPUBindGroupLayout global_bgl = wgpuDeviceCreateBindGroupLayout(context->device, &(WGPUBindGroupLayoutDescriptor) {
        .label = "Test Pass Global Bind Group Layout",
        .nextInChain = NULL,
        .entryCount = 1,
        .entries = (WGPUBindGroupLayoutEntry[]) {
            {
                .binding = 0,
                .visibility = WGPUShaderStage_Vertex,
                .buffer = (WGPUBufferBindingLayout) {
                    .type = WGPUBufferBindingType_Uniform,
                    .hasDynamicOffset = false,
                    .minBindingSize = sizeof(camera_matrix_uniform),
                    .nextInChain = NULL,
                },
            },
        },
    });
    WGPUBindGroup global_bg = wgpuDeviceCreateBindGroup(context->device, &(WGPUBindGroupDescriptor) {
        .label = "Test Pass Global Bind Group",
        .layout = global_bgl,
        .entryCount = 1,
        .entries = (WGPUBindGroupEntry[]) {
            {
                .binding = 0,
                .buffer = global_uniform_buffer,
                .offset = 0,
                .size = sizeof(camera_matrix_uniform),
            },
        },
    });

    const WGPUPipelineLayout pipeline_layout = wgpuDeviceCreatePipelineLayout(context->device, &(WGPUPipelineLayoutDescriptor) {
        .label = "Test Pass Pipeline Layout",
        .nextInChain = NULL,
        .bindGroupLayoutCount = 2,
        .bindGroupLayouts = (WGPUBindGroupLayout[]){global_bgl, local_bgl},
    });
    WGPURenderPipeline pipeline = wgpuDeviceCreateRenderPipeline(
        context->device,
        &((WGPURenderPipelineDescriptor) {
            .label = "Test Pass Pipeline",
            .layout = pipeline_layout,
            .vertex = (WGPUVertexState) {
                .entryPoint = "vs_main",
                .module = shader_module,
                .bufferCount = 2,
                .buffers = (WGPUVertexBufferLayout[]) {
                    position2_f32_attribute_layout(0, (WGPUVertexAttribute[1]){0}, 1),
                    color3_f32_attribute_layout(1, (WGPUVertexAttribute[1]){0}, 1),
                },
            },
            .fragment = &(WGPUFragmentState) {
                .entryPoint = "fs_main",
                .module = shader_module,
                .targetCount = 1,
                .targets = (WGPUColorTargetState[]) {
                    {
                        .format = target_info.format,
                        .blend = NULL,
                        .writeMask = WGPUColorWriteMask_All,
                    },
                },
            },
            .primitive = (WGPUPrimitiveState) {
                .topology = WGPUPrimitiveTopology_TriangleList,
                .stripIndexFormat = WGPUIndexFormat_Undefined,
                .frontFace = WGPUFrontFace_CCW,
                .cullMode = WGPUCullMode_None,
            },
            .depthStencil = NULL, // TODO: when depth buffer is implemented
            .multisample = (WGPUMultisampleState) {
                .count = target_info.sample_count,
                .mask = ~0u,
                .alphaToCoverageEnabled = false,
            },
        })
    );
    wgpuBindGroupLayoutRelease(global_bgl);
    wgpuPipelineLayoutRelease(pipeline_layout);

    return (test_pass){
        .pipeline = pipeline,
        .local_bgl = local_bgl,
        .global_bg = global_bg,
    };
}

void test_pass_free(test_pass *pass) {
    wgpuBindGroupLayoutRelease(pass->local_bgl);
    wgpuBindGroupRelease(pass->global_bg);
    wgpuBufferRelease(pass->global_uniform_buffer);
    wgpuRenderPipelineRelease(pass->pipeline);
}

static WGPUBindGroup test_pass_create_local_bind_group(
    test_pass *pass,
    WGPUDevice device,
    cecs_buffer_storage_attachment *color_uniform_buffer,
    cecs_buffer_storage_attachment *position_uniform_buffer
) {
    return wgpuDeviceCreateBindGroup(device, &(WGPUBindGroupDescriptor) {
        .label = "Test Pass Local Bind Group",
        .layout = pass->local_bgl,
        .entryCount = 2,
        .entries = (WGPUBindGroupEntry[]) {
            {
                .binding = 0,
                .buffer = color_uniform_buffer->buffer.buffer,
                .offset = 0,
                .size = color_uniform_buffer->buffer.uploaded_size,
            },
            {
                .binding = 1,
                .buffer = position_uniform_buffer->buffer.buffer,
                .offset = 0,
                .size = position_uniform_buffer->buffer.uploaded_size,
            }
        }
    });
}

static void test_pass_draw_inner(
    test_pass *pass,
    cecs_world *world,
    cecs_graphics_system *system,
    cecs_render_target *target,
    WGPURenderPassEncoder render_pass,
    WGPUBindGroup out_local_bind_groups[],
    size_t in_local_bind_groups_capacity,
    size_t *out_local_bind_groups_count
) {
    struct ubos {
        cecs_buffer_storage_attachment *color;
        cecs_buffer_storage_attachment *position;
    } ubos;
    if (!CECS_GRAPHICS_SYSTEM_SYNC_UNIFORM_COMPONENTS_ALL(
        system, world, &ubos, color4_f32_uniform, position4_f32_uniform
    )) {
        *out_local_bind_groups_count = 0;
        return;
    }
    //wgpuDeviceCreateTexture
    //WGPUTextureDescriptor
    //WGPUSamplerDescriptor
    //WGPUTextureViewDescriptor
    //wgpuTextureCreateView()
    (void)target;
    wgpuRenderPassEncoderSetPipeline(render_pass, pass->pipeline);

    cecs_buffer_storage_attachment *position_buffer = CECS_GRAPHICS_WORLD_GET_BUFFER_ATTACHMENTS(
        position2_f32_attribute,
        &system->world
    );
    cecs_buffer_storage_attachment *color_buffer = CECS_GRAPHICS_WORLD_GET_BUFFER_ATTACHMENTS(
        color3_f32_attribute,
        &system->world
    );
    cecs_exclusive_index_buffer_pair index_buffers = cecs_graphics_world_get_index_buffers(&system->world);
    wgpuRenderPassEncoderSetBindGroup(render_pass, 0, pass->global_bg, 0, NULL);

    assert(in_local_bind_groups_capacity >= 1 && "error: out local bind groups capacity must be at least 1");
    out_local_bind_groups[0] =
        test_pass_create_local_bind_group(pass, system->context.device, ubos.color, ubos.position);
    *out_local_bind_groups_count = 1;

    typedef cecs_uniform_raw_stream cecs_raw_color_stream;
    typedef cecs_uniform_raw_stream cecs_raw_position_stream;
    CECS_COMPONENT_ITERATION_HANDLE_STRUCT(cecs_mesh, cecs_index_stream, cecs_raw_color_stream, cecs_raw_position_stream) handle;
    cecs_arena arena = cecs_arena_create();
    for (
        cecs_component_iterator it = CECS_COMPONENT_ITERATOR_CREATE_GROUPED(&world->components, &arena,
            CECS_COMPONENTS_ALL(cecs_mesh),
            CECS_COMPONENTS_AND_ANY(cecs_index_stream),
            CECS_COMPONENTS_ALL_IDS(
                CECS_RELATION_ID(cecs_uniform_raw_stream, CECS_COMPONENT_ID(color4_f32_uniform)),
                CECS_RELATION_ID(cecs_uniform_raw_stream, CECS_COMPONENT_ID(position4_f32_uniform))
            ),
        );
        !cecs_component_iterator_done(&it);
        cecs_component_iterator_next(&it)
    ) {
        // TODO: check for attributes tags too
        // TODO: jump by batches instead of by mesh
        cecs_component_iterator_current(&it, &handle);
        cecs_raw_stream position = cecs_mesh_get_raw_vertex_stream(
            *handle.cecs_mesh_component, sizeof(position2_f32_attribute), &position_buffer->buffer
        );
        cecs_raw_stream color = cecs_mesh_get_raw_vertex_stream(
            *handle.cecs_mesh_component, sizeof(color3_f32_attribute), &color_buffer->buffer
        );

        wgpuRenderPassEncoderSetBindGroup(render_pass, 1, out_local_bind_groups[0], 2, (const uint32_t[]){
            handle.cecs_raw_color_stream_component->offset,
            handle.cecs_raw_position_stream_component->offset
        });

        wgpuRenderPassEncoderSetVertexBuffer(render_pass, 0, position_buffer->buffer.buffer, position.offset, position.size);
        wgpuRenderPassEncoderSetVertexBuffer(render_pass, 1, color_buffer->buffer.buffer, color.offset, color.size);

        if (handle.cecs_index_stream_component == NULL) {
            wgpuRenderPassEncoderDraw(
                render_pass,
                cecs_mesh_vertex_count(*handle.cecs_mesh_component), 1,
                0, 0
            );
        } else  {
            cecs_dynamic_wgpu_buffer *index_buffer = NULL;
            cecs_raw_stream index_stream = cecs_raw_stream_from_index(
                *handle.cecs_index_stream_component,
                index_buffers,
                &index_buffer
            );

            WGPUIndexFormat format = handle.cecs_index_stream_component->format;
            wgpuRenderPassEncoderSetIndexBuffer(render_pass, index_buffer->buffer, format, index_stream.offset, index_stream.size);
            wgpuRenderPassEncoderDrawIndexed(
                render_pass,
                handle.cecs_index_stream_component->index_count, 1,
                0, 0, 0
            );
        }
    }
    cecs_arena_free(&arena);
}

void test_pass_draw(test_pass *pass, cecs_world *world, cecs_graphics_system *system, cecs_render_target *target) {
    // TODO: make check and sync procedure

    const WGPUCommandEncoderDescriptor render_pass_encoder_descriptor = {
        .nextInChain = NULL,
        .label = "Learn WGPU Render Pass Encoder",
    };
    WGPUCommandEncoder render_pass_encoder = wgpuDeviceCreateCommandEncoder(
        system->context.device,
        &render_pass_encoder_descriptor
    );

    WGPURenderPassColorAttachment color_attachment = {
        .nextInChain = NULL,
        .view = target->view,
        .resolveTarget = NULL,
        .loadOp = WGPULoadOp_Clear,
        .storeOp = WGPUStoreOp_Store,
        .clearValue = (WGPUColor){ .r = 0.9, .g = 0.1, .b = 0.2, .a = 1.0 }, // TODO: camera with clear color
//#ifndef WEBGPU_BACKEND_WGPU
//      .depthSlice = WGPU_DEPTH_SLICE_UNDEFINED,
//#endif // NOT WEBGPU_BACKEND_WGPU
    };

    WGPURenderPassDescriptor render_pass_descriptor = {
        .nextInChain = NULL,
        .label = "Learn WGPU Render Pass",
        .colorAttachmentCount = 1,
        .colorAttachments = &color_attachment,
        .depthStencilAttachment = NULL,
        .timestampWrites = NULL,
    };
    WGPURenderPassEncoder render_pass =
        wgpuCommandEncoderBeginRenderPass(render_pass_encoder, &render_pass_descriptor);
    

    WGPUBindGroup local_bind_groups[1];
    size_t local_bind_groups_count;
    test_pass_draw_inner(pass, world, system, target, render_pass, local_bind_groups, 1, &local_bind_groups_count);

    wgpuRenderPassEncoderEnd(render_pass);
    for (size_t i = 0; i < local_bind_groups_count; ++i) {
        wgpuBindGroupRelease(local_bind_groups[i]);
    }
    wgpuRenderPassEncoderRelease(render_pass);
    

    WGPUCommandBufferDescriptor render_command_buffer_descriptor = {
        .nextInChain = NULL,
        .label = "Learn WGPU Render Command Buffer",
    };
    WGPUCommandBuffer render_command_buffer =
        wgpuCommandEncoderFinish(render_pass_encoder, &render_command_buffer_descriptor);
    wgpuCommandEncoderRelease(render_pass_encoder);

    wgpuQueueSubmit(system->context.queue, 1, &render_command_buffer);
#if defined(WEBGPU_BACKEND_DAWN)
    wgpuDeviceTick(system->context.device);
#elif defined(WEBGPU_BACKEND_WGPU)
    wgpuDevicePoll(system->context.device, false, NULL);
#endif
    wgpuCommandBufferRelease(render_command_buffer);
}

CECS_COMPONENT_DEFINE(position2_f32_attribute);
WGPUVertexBufferLayout position2_f32_attribute_layout(
    uint32_t shader_location,
    WGPUVertexAttribute out_attributes[],
    size_t out_attributes_capacity
) {
    assert(out_attributes_capacity >= 1 && "error: out attributes capacity must be at least 1");
    out_attributes[0] = (WGPUVertexAttribute) {
        .format = WGPUVertexFormat_Float32x2,
        .offset = 0,
        .shaderLocation = shader_location,
    };

    return (WGPUVertexBufferLayout) {
        .arrayStride = sizeof(position2_f32_attribute),
        .stepMode = WGPUVertexStepMode_Vertex,
        .attributeCount = 1,
        .attributes = out_attributes,
    };
}

CECS_COMPONENT_DEFINE(color3_f32_attribute);
WGPUVertexBufferLayout color3_f32_attribute_layout(
    uint32_t shader_location,
    WGPUVertexAttribute out_attributes[],
    size_t out_attributes_capacity
) {
    assert(out_attributes_capacity >= 1 && "error: out attributes capacity must be at least 1");
    out_attributes[0] = (WGPUVertexAttribute) {
        .format = WGPUVertexFormat_Float32x3,
        .offset = 0,
        .shaderLocation = shader_location,
    };

    return (WGPUVertexBufferLayout) {
        .arrayStride = sizeof(color3_f32_attribute),
        .stepMode = WGPUVertexStepMode_Vertex,
        .attributeCount = 1,
        .attributes = out_attributes,
    };
}

CECS_COMPONENT_DEFINE(camera_matrix_uniform);
CECS_COMPONENT_DEFINE(color4_f32_uniform);

CECS_COMPONENT_DEFINE(position4_f32_uniform);