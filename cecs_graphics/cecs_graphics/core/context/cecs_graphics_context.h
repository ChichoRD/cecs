#ifndef CECS_RENDER_CONTEXT_H
#define CECS_RENDER_CONTEXT_H

#include <webgpu/webgpu.h>
#include <webgpu/wgpu.h>
#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>
#include <cecs/containers/cecs_union.h>
//#include <cecs/containers/cecs_dynamic_array.h>

typedef struct cecs_surface_context {
    WGPUSurface surface;
    WGPUSurfaceConfiguration configuration;
} cecs_surface_context;
typedef CECS_OPTION_STRUCT(cecs_surface_context, optional_cecs_surface_context) optional_cecs_surface_context;

inline cecs_surface_context cecs_surface_context_create(WGPUSurface surface, WGPUSurfaceConfiguration configuration) {
    return (cecs_surface_context){
        .surface = surface,
        .configuration = configuration,
    };
}
void cecs_surface_context_free(cecs_surface_context *sc);

typedef struct cecs_render_target {
    WGPUTextureView view;
    WGPUTextureFormat format;
    WGPUExtent3D extent;
} cecs_render_target;

// typedef struct cecs_graphics_context_descriptor {

// } cecs_graphics_context_descriptor;

typedef struct cecs_graphics_context {
    WGPUInstance instance;
    WGPUDevice device;
    WGPUQueue queue;
    optional_cecs_surface_context surface_context;
    // TODO: cecs_render_target render_target;
} cecs_graphics_context;

cecs_graphics_context cecs_graphics_context_create(GLFWwindow *window);
cecs_graphics_context cecs_graphics_context_create_offscreen(void);
void cecs_graphics_context_free(cecs_graphics_context *context);

typedef cecs_render_target cecs_surface_render_target;
bool cecs_graphics_context_get_surface_render_target(
    cecs_graphics_context *context,
    cecs_surface_render_target *out_surface_render_target
);
void cecs_graphics_context_present_surface_render_target(
    cecs_graphics_context *context,
    cecs_surface_render_target *surface_render_target
);

#endif