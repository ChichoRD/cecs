#ifndef CECS_RELATION_H
#define CECS_RELATION_H

#include <stdint.h>
#include "../containers/cecs_flatmap.h"
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

#if (SIZE_MAX == UINT16_MAX)
    #define CECS_COMPONENT_ID_RELATION_SHIFT CECS_COMPONENT_ID_TYPE_EIGHTH_BITS
#elif (SIZE_MAX == UINT32_MAX)
    #define CECS_COMPONENT_ID_RELATION_SHIFT CECS_COMPONENT_ID_TYPE_QUATER_BITS
#elif (SIZE_MAX == UINT64_MAX)
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

typedef cecs_entity_id cecs_target_holder_id;
typedef struct cecs_target_holder_info {
    cecs_target_holder_id holder;
} cecs_target_holder_info;
typedef struct cecs_entity_associated_holders {
    cecs_sparse_set target_to_holder;
} cecs_entity_associated_holders;

typedef struct cecs_entity_associations {
    cecs_flatmap entity_to_target_holders;
} cecs_entity_associations;

typedef struct cecs_world_relations {
    cecs_arena associations_arena;
    cecs_entity_associations associations;
} cecs_world_relations;

cecs_world_relations cecs_world_relations_create(size_t entity_capacity);
void cecs_world_relations_free(cecs_world_relations *wr);

bool cecs_world_relations_add_target_holder(
    cecs_world_relations *wr,
    const cecs_entity_id source,
    const cecs_relation_target target,
    const cecs_target_holder_id holder
);

cecs_target_holder_id cecs_world_relations_add_target_expect(cecs_world_relations *wr, const cecs_entity_id source, const cecs_relation_target target);
bool cecs_world_relations_get_target(cecs_world_relations *wr, const cecs_entity_id source, const cecs_relation_target target, cecs_target_holder_id *out_target_holder);
bool cecs_world_relations_remove_target(cecs_world_relations *wr, const cecs_entity_id source, const cecs_relation_target target, cecs_target_holder_id *out_target_holder);


typedef cecs_sparse_set_iterator cecs_relation_targets_iterator;
bool cecs_world_relations_get_targets(cecs_world_relations *wr, const cecs_entity_id source, cecs_relation_targets_iterator *out_iterator);

#endif