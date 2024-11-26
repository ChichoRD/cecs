#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <webgpu/webgpu.h>
#include <webgpu/wgpu.h>
#include <GLFW/glfw3.h>

struct adpater_request_userdata {
    bool adapter_request_completed;
    WGPUAdapter adapter;
};

void on_adapter_request_completed(
    WGPURequestAdapterStatus status,
    WGPUAdapter adapter,
    const char *message,
    void *userdata
) {
    struct adpater_request_userdata *adapter_request_userdata = userdata;
    if (status == WGPURequestAdapterStatus_Success) {
        adapter_request_userdata->adapter = adapter;
        adapter_request_userdata->adapter_request_completed = true;
        printf("Got WebGPU adapter: %p\n", adapter);
    } else {
        adapter_request_userdata->adapter_request_completed = true;
        adapter_request_userdata->adapter = NULL;
        printf("Could not get WebGPU adapter: %s\n", message);
    }
}

WGPUAdapter request_adapter_sync(
    WGPUInstance instance,
    const WGPURequestAdapterOptions *options
) {
    struct adpater_request_userdata userdata = {
        .adapter_request_completed = false,
        .adapter = NULL
    };
    wgpuInstanceRequestAdapter(
        instance,
        options,
        on_adapter_request_completed,
        &userdata
    );
    while (!userdata.adapter_request_completed) { }
    return userdata.adapter;
}

struct device_request_userdata {
    bool device_request_completed;
    WGPUDevice device;
};

void on_device_request_completed(
    WGPURequestDeviceStatus status,
    WGPUDevice device,
    const char *message,
    void *userdata
) {
    struct device_request_userdata *device_request_userdata = userdata;
    if (status == WGPURequestDeviceStatus_Success) {
        device_request_userdata->device = device;
        device_request_userdata->device_request_completed = true;
        printf("Got WebGPU device: %p\n", device);
    } else {
        device_request_userdata->device_request_completed = true;
        device_request_userdata->device = NULL;
        printf("Could not get WebGPU device: %s\n", message);
    }
}

WGPUDevice request_device_sync(
    WGPUAdapter adapter,
    const WGPUDeviceDescriptor *descriptor
) {
    struct device_request_userdata userdata = {
        .device_request_completed = false,
        .device = NULL
    };
    wgpuAdapterRequestDevice(
        adapter,
        descriptor,
        on_device_request_completed,
        &userdata
    );
    while (!userdata.device_request_completed) { }
    return userdata.device;
}

void on_device_lost(
    WGPUDeviceLostReason reason,
    const char *message,
    void *userdata
) {
    printf("WebGPU device lost; due to WGPUDeviceLostReason (0x%X): %s\n", reason, message);
}

void on_device_error(
    WGPUErrorType type,
    const char *message,
    void *userdata
) {
    printf("WebGPU device error (0x%X): %s\n", type, message);
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

    WGPUInstanceExtras instance_extras = {
        .chain = { .next = NULL, .sType = WGPUSType_InstanceExtras },
        .backends = WGPUBackendType_Vulkan,
    };
    // unused: instance_extras
    WGPUInstanceDescriptor instance_descriptor = {
        .nextInChain = NULL,
    };
    WGPUInstance instance = wgpuCreateInstance(&instance_descriptor);
    if (instance == NULL) {
        fprintf(stderr, "Failed to create WGPU instance\n");
        return EXIT_FAILURE;
    }
    printf("WGPU Instance: %p\n", instance);


    WGPURequestAdapterOptions adapter_options = {
        .nextInChain = NULL,
        .powerPreference = WGPUPowerPreference_HighPerformance
    };
    WGPUAdapter adapter = request_adapter_sync(instance, &adapter_options);
    WGPUSupportedLimits supported_limits = { .nextInChain = NULL };

    bool adapter_limits_success = false;
#ifdef WEBGPU_BACKEND_DAWN
    adapter_limits_success = wgpuAdapterGetLimits(adapter, &supported_limits) == WGPUStatus_Success;
#else 
    adapter_limits_success = wgpuAdapterGetLimits(adapter, &supported_limits);
#endif

    WGPUFeatureName *features;
    size_t feature_count = wgpuAdapterEnumerateFeatures(adapter, NULL);
    features = calloc(feature_count, sizeof(WGPUFeatureName));
    wgpuAdapterEnumerateFeatures(adapter, features);

    printf("Adapter features: \n");
    for (size_t i = 0; i < feature_count; i++) {
        printf("\t- 0x%X\n", features[i]);
    }

    WGPUAdapterProperties adapter_properties = { .nextInChain = NULL };
    wgpuAdapterGetProperties(adapter, &adapter_properties);

    printf("Adapter properties: \n");
    printf("\t- Device ID: %d\n", adapter_properties.deviceID);

    printf("\t- Vendor ID: %d\n", adapter_properties.vendorID);
    printf("\t- Vendor name: %s\n", adapter_properties.vendorName);

    printf("\t- Adapter name: %s\n", adapter_properties.name);

    printf("\t- Backend type: 0x%X\n", adapter_properties.backendType);
    printf("\t- Adapter type: 0x%X\n", adapter_properties.adapterType);
    printf("\t- Driver description: %s\n", adapter_properties.driverDescription);

    // TODO: move creation and dbg logging to a separate function


    WGPUDeviceDescriptor device_descriptor = {
        .nextInChain = NULL,
        .label = "Learn WGPU Device",
        .requiredFeatureCount = 0,
        .requiredLimits = NULL,

        .defaultQueue.nextInChain = NULL,
        .defaultQueue.label = "Learn WGPU Queue",

        .deviceLostCallback = on_device_lost,
        .deviceLostUserdata = NULL,
    };
    WGPUDevice device = request_device_sync(adapter, &device_descriptor);
    wgpuAdapterRelease(adapter);

    wgpuDeviceSetUncapturedErrorCallback(device, on_device_error, NULL);


    WGPUQueue queue = wgpuDeviceGetQueue(device);

    while (!glfwWindowShouldClose(window)) {
        // Check whether the user clicked on the close button (and any other
        // mouse/key event, which we don't use so far)
        glfwPollEvents();
    }

    wgpuQueueRelease(queue);
    wgpuDeviceRelease(device);
    free(features);
    wgpuInstanceRelease(instance);
    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}