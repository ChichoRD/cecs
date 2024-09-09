#ifndef RELATION_H
#define RELATION_H

#include <stdint.h>
#include "component/entity/entity.h"
#include "component/component.h"
#include "tag.h"

typedef union relation_target {
    entity_id entity_id;
    tag_id tag_id;
} relation_target;

typedef struct relation_id_descriptor {
    component_id component_id;
    relation_target target;
} relation_id_descriptor;

inline relation_id_descriptor relation_id_descriptor_create_with_entity_target(component_id component_id, entity_id entity_id) {
    return (relation_id_descriptor) {
        .component_id = component_id,
        .target = (relation_target) {
            .entity_id = entity_id
        }
    };
}
inline relation_id_descriptor relation_id_descriptor_create_with_tag_target(component_id component_id, tag_id tag_id) {
    return (relation_id_descriptor) {
        .component_id = component_id,
        .target = (relation_target) {
            .tag_id = tag_id
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
relation_id relation_id_create(relation_id_descriptor descriptor) {
    return (descriptor.component_id << COMPONENT_ID_RELATION_SHIFT) + descriptor.target.tag_id;
}


typedef counted_set associated_entity_ids;
typedef struct world_relations {
    arena entity_associated_ids_arena;
    sparse_set entity_associated_ids;
} world_relations;

world_relations world_relations_create(size_t entity_capacity) {
    world_relations wr;
    wr.entity_associated_ids_arena = arena_create_with_capacity(sizeof(associated_entity_ids) * entity_capacity);
    wr.entity_associated_ids = sparse_set_create_with_capacity(
        &wr.entity_associated_ids_arena,
        entity_capacity,
        sizeof(associated_entity_ids)
    );
    return wr;
}

void world_relations_free(world_relations *wr) {
    arena_free(&wr->entity_associated_ids_arena);
    wr->entity_associated_ids = (sparse_set){0};
}

bool world_relations_entity_has_associated_id(world_relations *wr, entity_id entity_id, relation_target target) {
    if (!sparse_set_contains(&wr->entity_associated_ids, entity_id)) {
        return false;
    } else {
        associated_entity_ids *ids = sparse_set_get_unchecked(&wr->entity_associated_ids, entity_id, sizeof(associated_entity_ids));
        return counted_set_contains(ids, target.tag_id);
    }
}

entity_id world_relations_set_associated_id(world_relations *wr, entity_id id, relation_target target, entity_id associated_entity_id) {
    associated_entity_ids *ids;
    if (!sparse_set_contains(&wr->entity_associated_ids, id)) {
        associated_entity_ids new_ids = counted_set_create_with_capacity(
            &wr->entity_associated_ids_arena,
            1,
            sizeof(entity_id)
        );
        ids = SPARSE_SET_SET(associated_entity_ids, &wr->entity_associated_ids, &wr->entity_associated_ids_arena, id, &new_ids);
    } else {
        ids = sparse_set_get_unchecked(&wr->entity_associated_ids, id, sizeof(associated_entity_ids));
    }

    return *COUNTED_SET_SET(entity_id, ids, &wr->entity_associated_ids_arena, target.tag_id, &associated_entity_id);
}

entity_id world_relations_get_associated_id(world_relations *wr, entity_id id, relation_target target) {
    associated_entity_ids *ids = sparse_set_get_unchecked(&wr->entity_associated_ids, id, sizeof(associated_entity_ids));
    return *COUNTED_SET_GET(entity_id, ids, target.tag_id);
}

entity_id world_relations_increment_associated_id_or_set(
    world_relations *wr,
    entity_id id,
    relation_target target,
    entity_id otherwise_associated_id
) {
    associated_entity_ids *ids;
    if (!sparse_set_contains(&wr->entity_associated_ids, id)) {
        associated_entity_ids new_ids = counted_set_create_with_capacity(
            &wr->entity_associated_ids_arena,
            1,
            sizeof(entity_id)
        );
        ids = SPARSE_SET_SET(associated_entity_ids, &wr->entity_associated_ids, &wr->entity_associated_ids_arena, id, &new_ids);
    } else {
        ids = sparse_set_get_unchecked(&wr->entity_associated_ids, id, sizeof(associated_entity_ids));
    }

    return *COUNTED_SET_INCREMENT_OR_SET(entity_id, ids, &wr->entity_associated_ids_arena, target.tag_id, &otherwise_associated_id);
}

bool world_relations_remove_associated_id(world_relations *wr, entity_id id, relation_target target, entity_id *out_associated_entity_id) {
    if (!sparse_set_contains(&wr->entity_associated_ids, id)) {
        *out_associated_entity_id = 0;
        return false;
    } else {
        associated_entity_ids *ids = sparse_set_get_unchecked(&wr->entity_associated_ids, id, sizeof(associated_entity_ids));   
        return (out_associated_entity_id == NULL)
            ? counted_set_remove(ids, target.tag_id, sizeof(entity_id))
            : counted_set_remove_out(ids, target.tag_id, out_associated_entity_id, sizeof(entity_id));
    }
}

#endif