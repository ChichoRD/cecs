#ifndef CECS_TEXTURE_H
#define CECS_TEXTURE_H

#include <cecs_core/cecs_core.h>

typedef struct cecs_texture_reference {
    cecs_entity_id texture_id;
} cecs_texture_reference;
CECS_COMPONENT_DECLARE(cecs_texture_reference);


#endif