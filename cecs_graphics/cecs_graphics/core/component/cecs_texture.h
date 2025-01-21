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

#endif