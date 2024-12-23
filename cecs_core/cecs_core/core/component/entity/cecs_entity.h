#ifndef CECS_ENTITY_H
#define CECS_ENTITY_H

#include <stdint.h>
#include <stdbool.h>
#include "../../../containers/cecs_sparse_set.h"
#include "../../../containers/cecs_queue.h"
#include "../../../containers/cecs_range.h"


#define CECS_ENTITY_ID_TYPE uint64_t
typedef CECS_ENTITY_ID_TYPE cecs_entity_id;

typedef struct cecs_world_entities {
    cecs_arena entity_ids_arena;
    cecs_sparse_set entity_ids;
    cecs_queue free_entity_ids;
} cecs_world_entities;

cecs_world_entities cecs_world_entities_create(size_t entity_capacity);

void cecs_world_entities_free(cecs_world_entities *we);

static inline size_t cecs_world_entities_count(const cecs_world_entities *we) {
    return cecs_sparse_set_count_of_size(&we->entity_ids, sizeof(cecs_entity_id));
}

static inline bool cecs_world_enities_has_entity(const cecs_world_entities *we, cecs_entity_id id) {
    return cecs_sparse_set_contains(&we->entity_ids, (size_t)id);
}

cecs_entity_id cecs_world_entities_add_entity(cecs_world_entities *we);

cecs_entity_id cecs_world_entities_remove_entity(cecs_world_entities *we, cecs_entity_id id);

// TODO: add a free queue for entity ranges
typedef cecs_exclusive_range cecs_entity_id_range;
cecs_entity_id_range cecs_world_entities_add_entity_range(cecs_world_entities *we, size_t count);

cecs_entity_id_range cecs_world_entities_remove_entity_range(cecs_world_entities *we, cecs_entity_id_range range);

#endif