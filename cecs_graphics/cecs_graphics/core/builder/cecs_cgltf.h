#ifndef CECS_CGLTF_H
#define CECS_CGLTF_H

#include <cgltf.h>

void *cecs_cgltf_alloc(void *userdata, cgltf_size size);
void cecs_cgltf_free(void *userdata, void *ptr);

#endif