#ifndef CECS_TEXTURE_H
#define CECS_TEXTURE_H

#include <cecs_core/cecs_core.h>
#include <webgpu/webgpu.h>

typedef struct cecs_texture_reference {
    cecs_entity_id texture_id;
} cecs_texture_reference;
CECS_COMPONENT_DECLARE(cecs_texture_reference);

typedef struct cecs_texture {
    WGPUTextureView texture_view;
    WGPUExtent3D extent;
} cecs_texture;
CECS_COMPONENT_DECLARE(cecs_texture);

typedef enum cecs_texture_resource_status {
    cecs_texture_resource_status_free,
    cecs_texture_resource_status_in_use,
} cecs_texture_resource_status;

// TODO: support!
typedef struct cecs_texture_resource {
    cecs_texture texture;
    WGPUTextureViewDescriptor descriptor;
    size_t used_layers;
} cecs_texture_resource;
CECS_COMPONENT_DECLARE(cecs_texture_resource);

#endif