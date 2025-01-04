#include <stdbool.h>
#include <assert.h>
#include "cecs_dynamic_wgpu_buffer.h"

cecs_dynamic_wgpu_buffer cecs_dynamic_wgpu_buffer_create(WGPUDevice device, const WGPUBufferDescriptor *descriptor) {
    return (cecs_dynamic_wgpu_buffer){
        .buffer = wgpuDeviceCreateBuffer(
            device,
            descriptor
        ),
        .size = descriptor->size,
        .usage = descriptor->usage
    };
}

void cecs_dynamic_wgpu_buffer_free(cecs_dynamic_wgpu_buffer *buffer) {
    wgpuBufferRelease(buffer->buffer);
    buffer->buffer = NULL;
    buffer->size = 0;
    buffer->usage = WGPUBufferUsage_None;
}

static cecs_dynamic_wgpu_buffer_resize(cecs_dynamic_wgpu_buffer *buffer, WGPUDevice device, WGPUQueue queue, size_t size) {
    WGPUBufferDescriptor descriptor = {
        .label = "resized buffer",
        .mappedAtCreation = false,
        .nextInChain = NULL,
        .size = size,
        .usage = buffer->usage
    };
    WGPUBuffer new_buffer = wgpuDeviceCreateBuffer(device, &descriptor);
    WGPUCommandEncoderDescriptor encoder_descriptor = {
        .label = "resize buffer encoder",
        .nextInChain = NULL
    };
    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &encoder_descriptor);
    wgpuCommandEncoderCopyBufferToBuffer(
        encoder,
        buffer->buffer,
        0,
        new_buffer,
        0,
        buffer->size
    );
    WGPUCommandBufferDescriptor command_buffer_descriptor = {
        .label = "resize buffer command buffer",
        .nextInChain = NULL,
    };
    WGPUCommandBuffer command_buffer = wgpuCommandEncoderFinish(encoder, &command_buffer_descriptor);
    wgpuCommandEncoderRelease(encoder);

    wgpuQueueSubmit(queue, 1, &command_buffer);
#if defined(WEBGPU_BACKEND_DAWN)
    wgpuDeviceTick(device);
#elif defined(WEBGPU_BACKEND_WGPU)
    wgpuDevicePoll(device, false, NULL);
#endif
    wgpuCommandBufferRelease(command_buffer);
    wgpuBufferRelease(buffer->buffer);

    buffer->buffer = new_buffer;
    buffer->size = size;
}

void cecs_dynamic_wgpu_buffer_upload(
    cecs_dynamic_wgpu_buffer *buffer,
    WGPUDevice device,
    WGPUQueue queue,
    cecs_buffer_offset_u64 offset,
    void *data,
    size_t size
) {
    if (buffer->size == 0) {
        assert(false && "error: buffer must be initialized with non-zero size before uploading data");
    } else if (buffer->size < offset + size) {
        cecs_dynamic_wgpu_buffer_resize(buffer, device, queue, offset + size);
    }

    wgpuQueueWriteBuffer(
        queue,
        buffer->buffer,
        offset,
        data,
        size
    );
}
