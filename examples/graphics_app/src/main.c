#include <stdio.h>
#include <stdlib.h>

#include <webgpu/webgpu.h>
#include <GLFW/glfw3.h>

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


    WGPUInstanceDescriptor instance_descriptor = {
        .nextInChain = NULL,
    };
    WGPUInstance instance = wgpuCreateInstance(&instance_descriptor);
    if (instance == NULL) {
        fprintf(stderr, "Failed to create WGPU instance\n");
        return EXIT_FAILURE;
    }
    printf("WGPU Instance: %p\n", instance);


    while (!glfwWindowShouldClose(window)) {
        // Check whether the user clicked on the close button (and any other
        // mouse/key event, which we don't use so far)
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    wgpuInstanceRelease(instance);
    glfwTerminate();
    return EXIT_SUCCESS;
}