#ifndef ENTITY_H
#define ENTITY_H

#include <stdint.h>
#include <stdbool.h>
#include "../../../containers/cecs_sparse_set.h"
#include "../../../containers/cecs_queue.h"
#include "../../../containers/cecs_range.h"


#define ENTITY_ID_TYPE uint64_t
typedef ENTITY_ID_TYPE entity_id;

typedef struct world_entities {
    cecs_arena entity_ids_arena;
    cecs_sparse_set entity_ids;
    cecs_queue free_entity_ids;
} world_entities;

world_entities world_entities_create(size_t entity_capacity);

void world_entities_free(world_entities *we);

inline size_t world_entities_count(const world_entities *we) {
    return cecs_sparse_set_count_of_size(&we->entity_ids, sizeof(entity_id));
}

inline bool world_enities_has_entity(const world_entities *we, entity_id id) {
    return cecs_sparse_set_contains(&we->entity_ids, (size_t)id);
}

entity_id world_entities_add_entity(world_entities *we);

entity_id world_entities_remove_entity(world_entities *we, entity_id id);

typedef cecs_exclusive_range entity_id_range;
entity_id_range world_entities_add_entity_range(world_entities *we, size_t count);

entity_id_range world_entities_remove_entity_range(world_entities *we, entity_id_range range);

#endif