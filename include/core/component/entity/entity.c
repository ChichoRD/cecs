#include "entity.h"

world_entities world_entities_create(size_t entity_capacity) {
    world_entities we;
    we.entity_ids_arena = cecs_arena_create_with_capacity(sizeof(entity_id) * entity_capacity);
    we.entity_ids = sparse_set_create_of_integers_with_capacity(&we.entity_ids_arena, entity_capacity, sizeof(entity_id));
    we.free_entity_ids = queue_create_with_capacity(&we.entity_ids_arena, sizeof(entity_id) * entity_capacity);
    return we;
}

void world_entities_free(world_entities* we) {
    cecs_arena_free(&we->entity_ids_arena);
    we->free_entity_ids = (queue){ 0 };
    we->entity_ids = (sparse_set){ 0 };
}

entity_id world_entities_add_entity(world_entities* we) {
    entity_id id = world_entities_count(we);
    if (QUEUE_COUNT(entity_id, &we->free_entity_ids) > 0) {
        QUEUE_POP_FIRST(entity_id, &we->free_entity_ids, &we->entity_ids_arena, &id);

        if (sparse_set_contains(&we->entity_ids, (size_t)id)) {
            assert(false && "unreachable: id already in entity_ids, there is a mismatch in the integer-sparse-set indices");
            exit(EXIT_FAILURE);
        }
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

entity_id world_entities_remove_entity(world_entities* we, entity_id id) {
    entity_id removed;
    if (SPARSE_SET_REMOVE(entity_id, &we->entity_ids, &we->entity_ids_arena, (size_t)id, &removed)) {
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

entity_id_range world_entities_add_entity_range(world_entities* we, size_t count) {
    entity_id_range range = {
        .start = world_entities_count(we) + QUEUE_COUNT(entity_id, &we->free_entity_ids),
        .end = world_entities_count(we) + QUEUE_COUNT(entity_id, &we->free_entity_ids) + count
    };

    assert(
        range.start >= 0 && range.end
        && "error: both start and end of entity_id_range must be non-negative"
    );
    for (ptrdiff_t i = range.start; i < range.end; i++) {
        SPARSE_SET_SET(
            entity_id,
            &we->entity_ids,
            &we->entity_ids_arena,
            ((size_t)(range.start + i)),
            &(entity_id) {
            i
        }
        );
    }

    return range;
}

entity_id_range world_entities_remove_entity_range(world_entities* we, entity_id_range range) {
    assert(
        range.start >= 0 && range.end
        && "error: both start and end of entity_id_range must be non-negative"
    );
    for (entity_id i = (entity_id)range.start; i < (entity_id)range.end; i++) {
        world_entities_remove_entity(we, i);
    }
    return range;
}