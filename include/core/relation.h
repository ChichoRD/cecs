#ifndef RELATION_H
#define RELATION_H

#include <stdint.h>
#include "component/entity/entity.h"
#include "component/entity/tag.h"
#include "component/component.h"

typedef union relation_target {
    entity_id entity_id;
    tag_id tag_id;
} relation_target;

typedef struct relation_id_descriptor {
    component_id component_id;
    relation_target target;
} relation_id_descriptor;

inline relation_id_descriptor relation_id_descriptor_create_entity(component_id component_id, entity_id entity_target) {
    return (relation_id_descriptor) {
        .component_id = component_id,
        .target = (relation_target) {
            .entity_id = entity_target
        }
    };
}
inline relation_id_descriptor relation_id_descriptor_create_tag(component_id component_id, tag_id tag_target) {
    return (relation_id_descriptor) {
        .component_id = component_id,
        .target = (relation_target) {
            .tag_id = tag_target
        }
    };
}


#define COMPONENT_ID_TYPE_HALF_BITS_LOG2 (COMPONENT_ID_TYPE_BITS_LOG2 - 1)
#define COMPONENT_ID_TYPE_HALF_BITS (1 << COMPONENT_ID_TYPE_HALF_BITS_LOG2)

#define COMPONENT_ID_TYPE_QUATER_BITS_LOG2 (COMPONENT_ID_TYPE_HALF_BITS_LOG2 - 1)
#define COMPONENT_ID_TYPE_QUATER_BITS (1 << COMPONENT_ID_TYPE_QUATER_BITS_LOG2)

#define COMPONENT_ID_TYPE_EIGHTH_BITS_LOG2 (COMPONENT_ID_TYPE_QUATER_BITS_LOG2 - 1)
#define COMPONENT_ID_TYPE_EIGHTH_BITS (1 << COMPONENT_ID_TYPE_EIGHTH_BITS_LOG2)

#if (SIZE_MAX == 0xFFFF)
    #define COMPONENT_ID_RELATION_SHIFT COMPONENT_ID_TYPE_EIGHTH_BITS
#elif (SIZE_MAX == 0xFFFFFFFF)
    #define COMPONENT_ID_RELATION_SHIFT COMPONENT_ID_TYPE_QUATER_BITS
#elif (SIZE_MAX == 0xFFFFFFFFFFFFFFFF)
    #define COMPONENT_ID_RELATION_SHIFT COMPONENT_ID_TYPE_HALF_BITS
#else
    #error TBD code SIZE_T_BITS
#endif

typedef component_id relation_id;
inline relation_id relation_id_create(relation_id_descriptor descriptor) {
    return ((descriptor.component_id + 1) << COMPONENT_ID_RELATION_SHIFT) + descriptor.target.tag_id;
}
#define RELATION_ID(component_type, target_id) \
    relation_id_create(relation_id_descriptor_create_tag(COMPONENT_ID(component_type), target_id))


typedef cecs_counted_set associated_entity_ids;
typedef struct world_relations {
    cecs_arena entity_associated_ids_arena;
    cecs_sparse_set entity_associated_ids;
} world_relations;

world_relations world_relations_create(size_t entity_capacity);

void world_relations_free(world_relations *wr);

bool world_relations_entity_has_associated_id(world_relations *wr, entity_id entity_id, relation_target target);

entity_id world_relations_set_associated_id(world_relations *wr, entity_id id, relation_target target, entity_id associated_entity_id);

entity_id world_relations_get_associated_id(world_relations *wr, entity_id id, relation_target target);

typedef cecs_counted_set_iterator associated_entity_ids_iterator;
associated_entity_ids_iterator world_relations_get_associated_ids(world_relations *wr, entity_id id);

entity_id world_relations_increment_associated_id_or_set(
    world_relations *wr,
    entity_id id,
    relation_target target,
    entity_id otherwise_associated_id
);

bool world_relations_remove_associated_id(world_relations *wr, entity_id id, relation_target target, entity_id *out_associated_entity_id);

#endif