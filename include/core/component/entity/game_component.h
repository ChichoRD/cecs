#ifndef GAME_COMPONENT_H
#define GAME_COMPONENT_H

#include "entity.h"
#include "component_type.h"
#include "tag.h"

typedef entity_id entity_reference;
COMPONENT_IMPLEMENT(entity_reference);

typedef struct relation_entity_reference {
    entity_reference original_entity;
} relation_entity_reference;
COMPONENT_IMPLEMENT(relation_entity_reference);


typedef bool is_prefab;
TAG_IMPLEMENT(is_prefab);


typedef struct entity_flags {
    bool is_prefab : 1;
    bool is_inmutable : 1;
    bool is_permanent : 1;
    
    bool has_event_on_remove : 1;
    bool has_event_on_mutate : 1;
} entity_flags;
COMPONENT_IMPLEMENT(entity_flags);

#define ENTITY_FLAGS_DEFAULT ((entity_flags){0})
inline entity_flags entity_flags_default() {
    return ENTITY_FLAGS_DEFAULT;
}

#endif