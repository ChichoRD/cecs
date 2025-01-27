#include <stdlib.h>

#include "cecs_entity.h"

cecs_world_entities cecs_world_entities_create(size_t entity_capacity) {
    cecs_world_entities we;
    we.entity_ids_arena = cecs_arena_create_with_capacity(sizeof(cecs_entity_id) * entity_capacity);
    we.entity_ids = cecs_sparse_set_create_of_integers_with_capacity(&we.entity_ids_arena, entity_capacity, sizeof(cecs_entity_id));
    we.free_entity_ids = cecs_queue_create_with_capacity(&we.entity_ids_arena, sizeof(cecs_entity_id) * ((entity_capacity >> 2) + 1));
    return we;
}

void cecs_world_entities_free(cecs_world_entities* we) {
    cecs_arena_free(&we->entity_ids_arena);
    we->free_entity_ids = (cecs_queue){ 0 };
    we->entity_ids = (cecs_sparse_set){ 0 };
}

cecs_entity_id cecs_world_entities_add_entity(cecs_world_entities* we) {
    cecs_entity_id id = cecs_world_entities_count(we);
    if (CECS_QUEUE_COUNT(cecs_entity_id, &we->free_entity_ids) > 0) {
        CECS_QUEUE_POP_FIRST(cecs_entity_id, &we->free_entity_ids, &we->entity_ids_arena, &id);

        if (cecs_sparse_set_contains(&we->entity_ids, (size_t)id)) {
            assert(false && "unreachable: id already in entity_ids, there is a mismatch in the integer-sparse-set indices");
            exit(EXIT_FAILURE);
        }
    }

    CECS_SPARSE_SET_SET(
        cecs_entity_id,
        &we->entity_ids,
        &we->entity_ids_arena,
        (size_t)id,
        &id
    );

    return id;
}

cecs_entity_id cecs_world_entities_remove_entity(cecs_world_entities* we, cecs_entity_id id) {
    cecs_entity_id removed;
    if (CECS_SPARSE_SET_REMOVE(cecs_entity_id, &we->entity_ids, &we->entity_ids_arena, (size_t)id, &removed)) {
        if (removed != id) {
            assert(false && "unreachable: removed entity_id != id, there is a mismatch in the integer-sparse-set indices");
            exit(EXIT_FAILURE);
        }
        if (CECS_QUEUE_COUNT(cecs_entity_id, &we->free_entity_ids) > 0 && id == *CECS_QUEUE_LAST(cecs_entity_id, &we->free_entity_ids)) {
            assert(
                false
                && "unreachable: tried to double free a removed id and sparse set returned incorrect removal indication"
            );
            exit(EXIT_FAILURE);
        }
        CECS_QUEUE_PUSH_LAST(cecs_entity_id, &we->free_entity_ids, &we->entity_ids_arena, &id);
    }
    return id;
}

cecs_entity_id_range cecs_world_entities_add_entity_range(cecs_world_entities* we, size_t count) {
    cecs_entity_id_range range = {
        .start = cecs_world_entities_count(we) + CECS_QUEUE_COUNT(cecs_entity_id, &we->free_entity_ids),
        .end = cecs_world_entities_count(we) + CECS_QUEUE_COUNT(cecs_entity_id, &we->free_entity_ids) + count
    };

    assert(
        range.start >= 0 && range.end >= 0
        && "error: both start and end of entity_id_range must be non-negative"
    );
    for (ptrdiff_t i = range.start; i < range.end; i++) {
        CECS_SPARSE_SET_SET(
            cecs_entity_id,
            &we->entity_ids,
            &we->entity_ids_arena,
            (size_t)i,
            &(cecs_entity_id){i}
        );
    }

    return range;
}

cecs_entity_id_range cecs_world_entities_remove_entity_range(cecs_world_entities* we, cecs_entity_id_range range) {
    assert(
        range.start >= 0 && range.end >= 0
        && "error: both start and end of entity_id_range must be non-negative"
    );
    for (cecs_entity_id i = (cecs_entity_id)range.start; i < (cecs_entity_id)range.end; i++) {
        cecs_world_entities_remove_entity(we, i);
    }
    return range;
}
