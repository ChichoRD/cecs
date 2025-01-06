#include <stddef.h>
#include <stdio.h>
#include <assert.h>
#include "test_pass.h"

test_pass test_pass_create(cecs_graphics_context *context, cecs_render_target_info target_info, cecs_arena *arena) {
    FILE *shader_file = fopen("../../examples/graphics_app/src/test_pass.wgsl", "r");
    assert(shader_file != NULL && "fatal error: failed to open shader file");

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

    const WGPUPipelineLayout pipeline_layout = wgpuDeviceCreatePipelineLayout(context->device, &(WGPUPipelineLayoutDescriptor) {
        .label = "Test Pass Pipeline Layout",
        .nextInChain = NULL,
        .bindGroupLayoutCount = 0,
        .bindGroupLayouts = NULL,
    });

    return (test_pass){
        .pipeline = wgpuDeviceCreateRenderPipeline(
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
        )
    };
}

static void test_pass_draw_inner(
    test_pass *pass,
    cecs_world *world,
    cecs_graphics_system *system,
    cecs_render_target *target,
    WGPURenderPassEncoder render_pass
) {
    wgpuRenderPassEncoderSetPipeline(render_pass, pass->pipeline);

    cecs_buffer_storage_attachment *position_buffer = CECS_GRAPHICS_WORLD_GET_BUFFER_ATTACHMENTS(
        position2_f32_attribute,
        &system->world
    );
    cecs_buffer_storage_attachment *color_buffer = CECS_GRAPHICS_WORLD_GET_BUFFER_ATTACHMENTS(
        color3_f32_attribute,
        &system->world
    );

    // cecs_buffer_storage_attachment *index_buffer_u16 = CECS_GRAPHICS_WORLD_GET_BUFFER_ATTACHMENTS(
    //     cecs_vertex_index_u16,
    //     &system->world
    // );
    // cecs_buffer_storage_attachment *index_buffer_u32 = CECS_GRAPHICS_WORLD_GET_BUFFER_ATTACHMENTS(
    //     cecs_vertex_index_u32,
    //     &system->world
    // );

    CECS_COMPONENT_ITERATION_HANDLE_STRUCT(cecs_mesh, cecs_index_stream) handle;
    cecs_arena arena = cecs_arena_create();
    for (
        cecs_component_iterator it = CECS_COMPONENT_ITERATOR_CREATE_GROUPED(&world->components, &arena,
            CECS_COMPONENTS_ALL(cecs_mesh), CECS_COMPONENTS_AND_ANY(cecs_index_stream)
        );
        !cecs_component_iterator_done(&it);
        cecs_component_iterator_next(&it)
    ) {
        // TODO: check for attributes tags too
        // TODO: jump by batches instead of by mesh
        cecs_component_iterator_current(&it, &handle);
        cecs_entity_id_range vertices = handle.cecs_mesh_component->vertex_entities;

        uint64_t position_offset =
            cecs_dynamic_wgpu_buffer_get_offset(&position_buffer->buffer, vertices.start * sizeof(position2_f32_attribute));
        uint64_t color_offset =
            cecs_dynamic_wgpu_buffer_get_offset(&color_buffer->buffer, vertices.start * sizeof(color3_f32_attribute));

        uint64_t vertex_count = cecs_exclusive_range_length(vertices);
        uint64_t position_size = vertex_count * sizeof(position2_f32_attribute);
        uint64_t color_size = vertex_count * sizeof(color3_f32_attribute);

        wgpuRenderPassEncoderSetVertexBuffer(render_pass, 0, position_buffer->buffer.buffer, position_offset, position_size);
        wgpuRenderPassEncoderSetVertexBuffer(render_pass, 1, color_buffer->buffer.buffer, color_offset, color_size);

        if (handle.cecs_index_stream_component == NULL) {
            wgpuRenderPassEncoderDraw(render_pass, vertex_count, 1, 0, 0);
        } else {
            wgpuRenderPassEncoderDraw(render_pass, vertex_count, 1, 0, 0);
            // TODO: index buffer
        }
    }
    cecs_arena_free(&arena);
}

void test_pass_draw(test_pass *pass, cecs_world *world, cecs_graphics_system *system, cecs_render_target *target) {
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
    
    test_pass_draw_inner(pass, world, system, target, render_pass);

    wgpuRenderPassEncoderEnd(render_pass);
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
