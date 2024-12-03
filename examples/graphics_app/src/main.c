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

    cecs_graphics_system system = cecs_graphics_system_create(1024, 8, window);

    WGPUColor clear_color = { 0.9, 0.1, 0.2, 1.0 };
    bool render_error = false;
    while (!glfwWindowShouldClose(window) && !render_error) {
        glfwPollEvents();

        cecs_surface_render_target surface_target;
        if (cecs_graphics_context_get_surface_render_target(&system.context, &surface_target)) {
            
            WGPUCommandEncoderDescriptor render_pass_encoder_descriptor = {
                .nextInChain = NULL,
                .label = "Learn WGPU Render Pass Encoder",
            };
            WGPUCommandEncoder render_pass_encoder = wgpuDeviceCreateCommandEncoder(
                system.context.device,
                &render_pass_encoder_descriptor
            );

            WGPURenderPassColorAttachment color_attachment = {
                .nextInChain = NULL,
                .view = surface_target.view,
                .resolveTarget = NULL,
                .loadOp = WGPULoadOp_Clear,
                .storeOp = WGPUStoreOp_Store,
                .clearValue = clear_color,
#ifndef WEBGPU_BACKEND_WGPU
                .depthSlice = WGPU_DEPTH_SLICE_UNDEFINED,
#endif // NOT WEBGPU_BACKEND_WGPU
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
            

            wgpuRenderPassEncoderEnd(render_pass);
            wgpuRenderPassEncoderRelease(render_pass);


            WGPUCommandBufferDescriptor render_command_buffer_descriptor = {
                .nextInChain = NULL,
                .label = "Learn WGPU Render Command Buffer",
            };
            WGPUCommandBuffer render_command_buffer =
                wgpuCommandEncoderFinish(render_pass_encoder, &render_command_buffer_descriptor);
            wgpuCommandEncoderRelease(render_pass_encoder);

            wgpuQueueSubmit(system.context.queue, 1, &render_command_buffer);
#if defined(WEBGPU_BACKEND_DAWN)
            wgpuDeviceTick(system.context.device);
#elif defined(WEBGPU_BACKEND_WGPU)
            wgpuDevicePoll(system.context.device, false, NULL);
#endif
            wgpuCommandBufferRelease(render_command_buffer);
            
            cecs_graphics_context_present_surface_render_target(&system.context, &surface_target);
        } else {
            render_error = true;
        }

        struct timespec t;
        timespec_get(&t, TIME_UTC);
        double t_sec = (double)t.tv_sec + (double)t.tv_nsec / 1e9;
        clear_color = (WGPUColor){ sin(t_sec), cos(t_sec), cos(sin(t_sec)), 1.0f };
    }

    cecs_graphics_system_free(&system);
    glfwDestroyWindow(window);
    glfwTerminate();

    if (render_error) {
        fprintf(stderr, "Error rendering to surface\n");
        return EXIT_FAILURE;
    } else {
        return EXIT_SUCCESS;
    }
}