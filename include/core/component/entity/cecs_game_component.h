#ifndef CECS_GAME_COMPONENT_H
#define CECS_GAME_COMPONENT_H

#include "cecs_entity.h"
#include "cecs_component_type.h"
#include "cecs_tag.h"

typedef cecs_entity_id cecs_entity_reference;
CECS_COMPONENT_IMPLEMENT(cecs_entity_reference);

typedef struct cecs_relation_entity_reference {
    cecs_entity_reference original_entity;
} cecs_relation_entity_reference;
CECS_COMPONENT_IMPLEMENT(cecs_relation_entity_reference);


typedef bool cecs_is_prefab;
CECS_TAG_IMPLEMENT(cecs_is_prefab);


typedef struct cecs_entity_flags {
    bool is_prefab : 1;
    bool is_inmutable : 1;
    bool is_permanent : 1;
    
    bool has_event_on_remove : 1;
    bool has_event_on_mutate : 1;
} cecs_entity_flags;
CECS_COMPONENT_IMPLEMENT(cecs_entity_flags);

#define CECS_ENTITY_FLAGS_DEFAULT ((cecs_entity_flags){0})
inline cecs_entity_flags cecs_entity_flags_default(void) {
    return CECS_ENTITY_FLAGS_DEFAULT;
}

#endif