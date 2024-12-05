#ifndef CECS_RELATION_H
#define CECS_RELATION_H

#include <stdint.h>
#include "component/entity/cecs_entity.h"
#include "component/entity/cecs_tag.h"
#include "component/cecs_component.h"

typedef union cecs_relation_target {
    cecs_entity_id entity_id;
    cecs_tag_id tag_id;
} cecs_relation_target;

typedef struct cecs_relation_id_descriptor {
    cecs_component_id component_id;
    cecs_relation_target target;
} cecs_relation_id_descriptor;

static inline cecs_relation_id_descriptor cecs_relation_id_descriptor_create_entity(cecs_component_id component_id, cecs_entity_id entity_target) {
    return (cecs_relation_id_descriptor) {
        .component_id = component_id,
        .target = (cecs_relation_target) {
            .entity_id = entity_target
        }
    };
}
static inline cecs_relation_id_descriptor cecs_relation_id_descriptor_create_tag(cecs_component_id component_id, cecs_tag_id tag_target) {
    return (cecs_relation_id_descriptor) {
        .component_id = component_id,
        .target = (cecs_relation_target) {
            .tag_id = tag_target
        }
    };
}

#define CECS_COMPONENT_ID_TYPE_HALF_BITS_LOG2 (CECS_COMPONENT_ID_TYPE_BITS_LOG2 - 1)
#define CECS_COMPONENT_ID_TYPE_HALF_BITS (1 << CECS_COMPONENT_ID_TYPE_HALF_BITS_LOG2)

#define CECS_COMPONENT_ID_TYPE_QUATER_BITS_LOG2 (CECS_COMPONENT_ID_TYPE_HALF_BITS_LOG2 - 1)
#define CECS_COMPONENT_ID_TYPE_QUATER_BITS (1 << CECS_COMPONENT_ID_TYPE_QUATER_BITS_LOG2)

#define CECS_COMPONENT_ID_TYPE_EIGHTH_BITS_LOG2 (CECS_COMPONENT_ID_TYPE_QUATER_BITS_LOG2 - 1)
#define CECS_COMPONENT_ID_TYPE_EIGHTH_BITS (1 << CECS_COMPONENT_ID_TYPE_EIGHTH_BITS_LOG2)

#if (SIZE_MAX == 0xFFFF)
    #define CECS_COMPONENT_ID_RELATION_SHIFT CECS_COMPONENT_ID_TYPE_EIGHTH_BITS
#elif (SIZE_MAX == 0xFFFFFFFF)
    #define CECS_COMPONENT_ID_RELATION_SHIFT CECS_COMPONENT_ID_TYPE_QUATER_BITS
#elif (SIZE_MAX == 0xFFFFFFFFFFFFFFFF)
    #define CECS_COMPONENT_ID_RELATION_SHIFT CECS_COMPONENT_ID_TYPE_HALF_BITS
#else
    #error TBD code SIZE_T_BITS
#endif

typedef cecs_component_id cecs_relation_id;
static inline cecs_relation_id cecs_relation_id_create(cecs_relation_id_descriptor descriptor) {
    return ((descriptor.component_id + 1) << CECS_COMPONENT_ID_RELATION_SHIFT) + descriptor.target.tag_id;
}
#define CECS_RELATION_ID(component_type, target_id) \
    cecs_relation_id_create(cecs_relation_id_descriptor_create_tag(CECS_COMPONENT_ID(component_type), target_id))

typedef cecs_counted_set cecs_associated_entity_ids;
typedef struct cecs_world_relations {
    cecs_arena entity_associated_ids_arena;
    cecs_sparse_set entity_associated_ids;
} cecs_world_relations;

cecs_world_relations cecs_world_relations_create(size_t entity_capacity);

void cecs_world_relations_free(cecs_world_relations *wr);

bool cecs_world_relations_entity_has_associated_id(cecs_world_relations *wr, cecs_entity_id entity_id, cecs_relation_target target);

cecs_entity_id cecs_world_relations_set_associated_id(cecs_world_relations *wr, cecs_entity_id id, cecs_relation_target target, cecs_entity_id associated_entity_id);

cecs_entity_id cecs_world_relations_get_associated_id(cecs_world_relations *wr, cecs_entity_id id, cecs_relation_target target);

typedef cecs_counted_set_iterator cecs_associated_entity_ids_iterator;
cecs_associated_entity_ids_iterator cecs_world_relations_get_associated_ids(cecs_world_relations *wr, cecs_entity_id id);

cecs_entity_id cecs_world_relations_increment_associated_id_or_set(
    cecs_world_relations *wr,
    cecs_entity_id id,
    cecs_relation_target target,
    cecs_entity_id otherwise_associated_id
);

bool cecs_world_relations_remove_associated_id(cecs_world_relations *wr, cecs_entity_id id, cecs_relation_target target, cecs_entity_id *out_associated_entity_id);

#endif