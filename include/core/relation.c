#include "relation.h"

world_relations world_relations_create(size_t entity_capacity) {
    world_relations wr;
    wr.entity_associated_ids_arena = cecs_arena_create_with_capacity(sizeof(associated_entity_ids) * entity_capacity);
    wr.entity_associated_ids = sparse_set_create_with_capacity(
        &wr.entity_associated_ids_arena,
        entity_capacity,
        sizeof(associated_entity_ids)
    );
    return wr;
}

void world_relations_free(world_relations* wr) {
    cecs_arena_free(&wr->entity_associated_ids_arena);
    wr->entity_associated_ids = (sparse_set){ 0 };
}

bool world_relations_entity_has_associated_id(world_relations* wr, entity_id entity_id, relation_target target) {
    if (!sparse_set_contains(&wr->entity_associated_ids, (size_t)entity_id)) {
        return false;
    }
    else {
        associated_entity_ids* ids = sparse_set_get_unchecked(&wr->entity_associated_ids, (size_t)entity_id, sizeof(associated_entity_ids));
        return counted_set_contains(ids, (size_t)target.tag_id);
    }
}

entity_id world_relations_set_associated_id(world_relations* wr, entity_id id, relation_target target, entity_id associated_entity_id) {
    associated_entity_ids* ids;
    if (!sparse_set_contains(&wr->entity_associated_ids, (size_t)id)) {
        associated_entity_ids new_ids = counted_set_create_with_capacity(
            &wr->entity_associated_ids_arena,
            1,
            sizeof(entity_id)
        );
        ids = SPARSE_SET_SET(associated_entity_ids, &wr->entity_associated_ids, &wr->entity_associated_ids_arena, (size_t)id, &new_ids);
    }
    else {
        ids = sparse_set_get_unchecked(&wr->entity_associated_ids, (size_t)id, sizeof(associated_entity_ids));
    }

    return *COUNTED_SET_SET(entity_id, ids, &wr->entity_associated_ids_arena, (size_t)target.tag_id, &associated_entity_id);
}

entity_id world_relations_get_associated_id(world_relations* wr, entity_id id, relation_target target) {
    associated_entity_ids* ids = sparse_set_get_unchecked(&wr->entity_associated_ids, (size_t)id, sizeof(associated_entity_ids));
    return *COUNTED_SET_GET(entity_id, ids, (size_t)target.tag_id);
}

associated_entity_ids_iterator world_relations_get_associated_ids(world_relations* wr, entity_id id) {
    if (sparse_set_contains(&wr->entity_associated_ids, (size_t)id)) {
        associated_entity_ids* ids = sparse_set_get_unchecked(&wr->entity_associated_ids, (size_t)id, sizeof(associated_entity_ids));
        return counted_set_iterator_create_borrowed(ids);
    }
    else {
        return counted_set_iterator_create_owned(counted_set_empty());
    }
}

entity_id world_relations_increment_associated_id_or_set(world_relations* wr, entity_id id, relation_target target, entity_id otherwise_associated_id) {
    associated_entity_ids* ids;
    if (!sparse_set_contains(&wr->entity_associated_ids, (size_t)id)) {
        associated_entity_ids new_ids = counted_set_create_with_capacity(
            &wr->entity_associated_ids_arena,
            1,
            sizeof(entity_id)
        );
        ids = SPARSE_SET_SET(associated_entity_ids, &wr->entity_associated_ids, &wr->entity_associated_ids_arena, (size_t)id, &new_ids);
    }
    else {
        ids = sparse_set_get_unchecked(&wr->entity_associated_ids, (size_t)id, sizeof(associated_entity_ids));
    }

    return *COUNTED_SET_INCREMENT_OR_SET(entity_id, ids, &wr->entity_associated_ids_arena, (size_t)target.tag_id, &otherwise_associated_id);
}

bool world_relations_remove_associated_id(world_relations* wr, entity_id id, relation_target target, entity_id* out_associated_entity_id) {
    if (!sparse_set_contains(&wr->entity_associated_ids, (size_t)id)) {
        *out_associated_entity_id = 0;
        return false;
    }
    else {
        associated_entity_ids* ids = sparse_set_get_unchecked(&wr->entity_associated_ids, (size_t)id, sizeof(associated_entity_ids));
        return counted_set_remove_out(ids, (size_t)target.tag_id, out_associated_entity_id, sizeof(entity_id));
    }
}
