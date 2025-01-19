#include <assert.h>
#include <stdbool.h>
#include "cecs_graphics_context.h"
//#include <stdio.h>

typedef struct cecs_adpater_request_userdata {
    bool adapter_request_completed;
    WGPUAdapter adapter;
} cecs_adpater_request_userdata;

static void cecs_on_adapter_request_completed(
    WGPURequestAdapterStatus status,
    WGPUAdapter adapter,
    const char *message,
    void *userdata
) {
    (void)message;
    cecs_adpater_request_userdata *adapter_request_userdata = userdata;
    if (status == WGPURequestAdapterStatus_Success) {
        adapter_request_userdata->adapter = adapter;
        adapter_request_userdata->adapter_request_completed = true;
    } else {
        adapter_request_userdata->adapter_request_completed = true;
        adapter_request_userdata->adapter = NULL;
    }
}

static WGPUAdapter cecs_request_adapter_sync(
    WGPUInstance instance,
    const WGPURequestAdapterOptions *options
) {
    cecs_adpater_request_userdata userdata = {
        .adapter_request_completed = false,
        .adapter = NULL
    };
    wgpuInstanceRequestAdapter(
        instance,
        options,
        cecs_on_adapter_request_completed,
        &userdata
    );
    while (!userdata.adapter_request_completed) { }

    assert(userdata.adapter != NULL && "fatal error: failed to get WGPU adapter");
    return userdata.adapter;
}


typedef struct cecs_device_request_userdata {
    bool device_request_completed;
    WGPUDevice device;
} cecs_device_request_userdata;

static void cecs_on_device_request_completed(
    WGPURequestDeviceStatus status,
    WGPUDevice device,
    const char *message,
    void *userdata
) {
    (void)message;
    cecs_device_request_userdata *device_request_userdata = userdata;
    if (status == WGPURequestDeviceStatus_Success) {
        device_request_userdata->device = device;
        device_request_userdata->device_request_completed = true;
    } else {
        device_request_userdata->device_request_completed = true;
        device_request_userdata->device = NULL;
    }
}

static WGPUDevice cecs_request_device_sync(
    WGPUAdapter adapter,
    const WGPUDeviceDescriptor *descriptor
) {
    cecs_device_request_userdata userdata = {
        .device_request_completed = false,
        .device = NULL
    };
    wgpuAdapterRequestDevice(
        adapter,
        descriptor,
        cecs_on_device_request_completed,
        &userdata
    );
    while (!userdata.device_request_completed) { }
    assert(userdata.device != NULL && "fatal error: failed to get WGPU device");
    return userdata.device;
}

void cecs_surface_context_free(cecs_surface_context *sc) {
    wgpuSurfaceUnconfigure(sc->surface);
    wgpuSurfaceRelease(sc->surface);
    sc->configuration = (WGPUSurfaceConfiguration){0};
    sc->surface = NULL;
}

cecs_graphics_context cecs_graphics_context_create(GLFWwindow *window)
{
    WGPUInstanceDescriptor instance_descriptor = {
        .nextInChain = NULL,
    };
    WGPUInstance instance = wgpuCreateInstance(&instance_descriptor);
    assert(instance != NULL && "fatal error: failed to create WGPU instance");

    WGPUSurface surface = glfwGetWGPUSurface(instance, window);

    // TODO: decriptor
    WGPURequestAdapterOptions adapter_options = {
        .nextInChain = NULL,
        .powerPreference = WGPUPowerPreference_HighPerformance,
        .compatibleSurface = surface,
    };
    WGPUAdapter adapter = cecs_request_adapter_sync(instance, &adapter_options);

    WGPUDeviceDescriptor device_descriptor = {
        .nextInChain = NULL,
        .label = "Learn WGPU Device",
        .requiredFeatureCount = 0,
        .requiredLimits = NULL,

        .defaultQueue.nextInChain = NULL,
        .defaultQueue.label = "Learn WGPU Queue",

        .deviceLostCallback = NULL, // TODO: on_device_lost,
        .deviceLostUserdata = NULL,
    };
    WGPUDevice device = cecs_request_device_sync(adapter, &device_descriptor);

    int window_width;
    int window_height;
    glfwGetWindowSize(window, &window_width, &window_height);
    WGPUSurfaceConfiguration surface_configuration = {
        .nextInChain = NULL,
        .width = (uint32_t)window_width,
        .height = (uint32_t)window_height,
        .format = wgpuSurfaceGetPreferredFormat(surface, adapter),
        .viewFormatCount = 0,
        .viewFormats = NULL,
        .usage = WGPUTextureUsage_RenderAttachment,
        .device = device,
        .presentMode = WGPUPresentMode_Fifo,
        .alphaMode = WGPUCompositeAlphaMode_Auto
    };
    wgpuSurfaceConfigure(surface, &surface_configuration);
    wgpuAdapterRelease(adapter);
    // TODO: wgpuDeviceSetUncapturedErrorCallback(device, on_device_error, NULL);

    WGPUQueue queue = wgpuDeviceGetQueue(device);
    return (cecs_graphics_context){
        .instance = instance,
        .device = device,
        .queue = queue,
        .surface_context = CECS_OPTION_CREATE_SOME(cecs_optional_surface_context, cecs_surface_context_create(
            surface,
            surface_configuration
        )),
    };
}

cecs_graphics_context cecs_graphics_context_create_offscreen(void) {
    WGPUInstanceDescriptor instance_descriptor = {
        .nextInChain = NULL,
    };
    WGPUInstance instance = wgpuCreateInstance(&instance_descriptor);
    assert(instance != NULL && "fatal error: failed to create WGPU instance");

    // TODO: decriptor
    WGPURequestAdapterOptions adapter_options = {
        .nextInChain = NULL,
        .powerPreference = WGPUPowerPreference_HighPerformance,
    };
    WGPUAdapter adapter = cecs_request_adapter_sync(instance, &adapter_options);

    WGPUDeviceDescriptor device_descriptor = {
        .nextInChain = NULL,
        .label = "Learn WGPU Device",
        .requiredFeatureCount = 0,
        .requiredLimits = NULL,

        .defaultQueue.nextInChain = NULL,
        .defaultQueue.label = "Learn WGPU Queue",

        .deviceLostCallback = NULL, // TODO: on_device_lost,
        .deviceLostUserdata = NULL,
    };
    WGPUDevice device = cecs_request_device_sync(adapter, &device_descriptor);
    wgpuAdapterRelease(adapter);
    // TODO: wgpuDeviceSetUncapturedErrorCallback(device, on_device_error, NULL);

    WGPUQueue queue = wgpuDeviceGetQueue(device);
    return (cecs_graphics_context){
        .instance = instance,
        .device = device,
        .queue = queue,
        .surface_context = CECS_OPTION_CREATE_NONE(cecs_optional_surface_context),
    };
}

void cecs_graphics_context_free(cecs_graphics_context *context) {
    wgpuQueueRelease(context->queue);
    wgpuDeviceRelease(context->device);
    if (CECS_OPTION_IS_SOME(cecs_optional_surface_context, context->surface_context)) {
        cecs_surface_context_free(
            &CECS_OPTION_GET_UNCHECKED(cecs_optional_surface_context, context->surface_context)
        );
    }
    wgpuInstanceRelease(context->instance);
    context->queue = NULL;
    context->device = NULL;
    context->instance = NULL;
    context->surface_context = CECS_OPTION_CREATE_NONE_STRUCT(cecs_optional_surface_context);
}

bool cecs_graphics_context_get_surface_render_target(
    cecs_graphics_context *context,
    cecs_surface_render_target *out_surface_render_target
) {
    CECS_OPTION_IS_SOME_ASSERT(cecs_optional_surface_context, context->surface_context);
    cecs_surface_context surface_context = CECS_OPTION_GET_UNCHECKED(cecs_optional_surface_context, context->surface_context);

    WGPUSurfaceTexture surface_texture;
    wgpuSurfaceGetCurrentTexture(surface_context.surface, &surface_texture);

    if (surface_texture.status == WGPUSurfaceGetCurrentTextureStatus_Success) {
        WGPUTextureViewDescriptor texture_view_descriptor = {
            .nextInChain = NULL,
            .label = "Learn WGPU Texture View",
            .format = wgpuTextureGetFormat(surface_texture.texture),
            .dimension = WGPUTextureViewDimension_2D,
            .baseMipLevel = 0,
            .mipLevelCount = 1,
            .baseArrayLayer = 0,
            .arrayLayerCount = 1,
            .aspect = WGPUTextureAspect_All,
        };
        WGPUTextureView target_view = wgpuTextureCreateView(surface_texture.texture, &texture_view_descriptor);
#ifndef WEBGPU_BACKEND_WGPU
        // We no longer need the texture, only its view
        // (NB: with wgpu-native, surface textures must not be manually released)
        wgpuTextureRelease(surface_texture.texture);
#endif
        *out_surface_render_target = (cecs_surface_render_target){
            .view = target_view,
            .format = surface_context.configuration.format,
            .extent = (WGPUExtent3D){
                .width = surface_context.configuration.width,
                .height = surface_context.configuration.height,
                .depthOrArrayLayers = 1,
            },
        };
        return true;
    } else {
        //printf("error: failed to get surface texture. Status: %X\n", surface_texture.status);
        *out_surface_render_target = (cecs_surface_render_target){0};
        return false;
    }
}

void cecs_graphics_context_present_surface_render_target(
    cecs_graphics_context *context,
    cecs_surface_render_target *surface_render_target
) {
    CECS_OPTION_IS_SOME_ASSERT(cecs_optional_surface_context, context->surface_context);
    cecs_surface_context surface_context = CECS_OPTION_GET_UNCHECKED(cecs_optional_surface_context, context->surface_context);

    wgpuTextureViewRelease(surface_render_target->view);
    wgpuSurfacePresent(surface_context.surface);
}
