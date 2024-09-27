#ifndef ENTITY_H
#define ENTITY_H

#include <stdint.h>
#include <stdbool.h>
#include "../../../containers/sparse_set.h"
#include "../../../containers/queue.h"


#define ENTITY_ID_TYPE uint64_t
typedef ENTITY_ID_TYPE entity_id;

typedef struct world_entities {
    arena entity_ids_arena;
    sparse_set entity_ids;
    queue free_entity_ids;
} world_entities;

world_entities world_entities_create(size_t entity_capacity) {
    world_entities we;
    we.entity_ids_arena = arena_create_with_capacity(sizeof(entity_id) * entity_capacity);
    we.entity_ids = sparse_set_create_of_integers_with_capacity(&we.entity_ids_arena, entity_capacity, sizeof(entity_id));
    we.free_entity_ids = queue_create_with_capacity(&we.entity_ids_arena, sizeof(entity_id) * entity_capacity);
    return we;
}

void world_entities_free(world_entities *we) {
    arena_free(&we->entity_ids_arena);
    we->free_entity_ids = (queue){0};
    we->entity_ids = (sparse_set){0};
}

inline size_t world_entities_count(const world_entities *we) {
    return sparse_set_count_of_size(&we->entity_ids, sizeof(entity_id));
}

inline bool world_enities_has_entity(const world_entities *we, entity_id id) {
    return sparse_set_contains(&we->entity_ids, (size_t)id);
}

entity_id world_entities_add_entity(world_entities *we) {
    entity_id id = world_entities_count(we);
    if (QUEUE_COUNT(entity_id, &we->free_entity_ids) > 0) {
        QUEUE_POP_FIRST(entity_id, &we->free_entity_ids, &we->entity_ids_arena, &id);

        if (id == 0) {
            int a;
            a = 3;
        }
    }
    if (id == 0) {
        int a;
        a = 3;
    }

    SPARSE_SET_SET(
        entity_id,
        &we->entity_ids,
        &we->entity_ids_arena,
        (size_t)id,
        &id
    );
    
    return id;
}

entity_id world_entities_remove_entity(world_entities *we, entity_id id) {
    entity_id removed;
    if (SPARSE_SET_REMOVE(entity_id, &we->entity_ids, &we->entity_ids_arena, (size_t)id, &removed) ) {
        if (removed != id) {
            assert(false && "unreachable: removed entity_id != id, there is a mismatch in the integer-sparse-set indices");
            exit(EXIT_FAILURE);
        }
        if (QUEUE_COUNT(entity_id, &we->free_entity_ids) > 0 && id == *QUEUE_LAST(entity_id, &we->free_entity_ids)) {
            assert(
                false
                && "unreachable: tried to double free a removed id and sparse set returned incorrect removal indication"
            );
            exit(EXIT_FAILURE);
        }
        QUEUE_PUSH_LAST(entity_id, &we->free_entity_ids, &we->entity_ids_arena, &id);
    }
    return id;
}

#endif