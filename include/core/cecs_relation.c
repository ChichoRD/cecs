#include "cecs_relation.h"

cecs_world_relations cecs_world_relations_create(size_t entity_capacity) {
    cecs_world_relations wr;
    wr.entity_associated_ids_arena = cecs_arena_create_with_capacity(sizeof(cecs_associated_entity_ids) * entity_capacity);
    wr.entity_associated_ids = cecs_sparse_set_create_with_capacity(
        &wr.entity_associated_ids_arena,
        entity_capacity,
        sizeof(cecs_associated_entity_ids)
    );
    return wr;
}

void cecs_world_relations_free(cecs_world_relations* wr) {
    cecs_arena_free(&wr->entity_associated_ids_arena);
    wr->entity_associated_ids = (cecs_sparse_set){ 0 };
}

bool cecs_world_relations_entity_has_associated_id(cecs_world_relations* wr, cecs_entity_id entity_id, cecs_relation_target target) {
    if (!cecs_sparse_set_contains(&wr->entity_associated_ids, (size_t)entity_id)) {
        return false;
    }
    else {
        cecs_associated_entity_ids* ids = cecs_sparse_set_get_unchecked(&wr->entity_associated_ids, (size_t)entity_id, sizeof(cecs_associated_entity_ids));
        return cecs_counted_set_contains(ids, (size_t)target.tag_id);
    }
}

cecs_entity_id cecs_world_relations_set_associated_id(cecs_world_relations* wr, cecs_entity_id id, cecs_relation_target target, cecs_entity_id associated_entity_id) {
    cecs_associated_entity_ids* ids;
    if (!cecs_sparse_set_contains(&wr->entity_associated_ids, (size_t)id)) {
        cecs_associated_entity_ids new_ids = cecs_counted_set_create_with_capacity(
            &wr->entity_associated_ids_arena,
            1,
            sizeof(cecs_entity_id)
        );
        ids = CECS_SPARSE_SET_SET(cecs_associated_entity_ids, &wr->entity_associated_ids, &wr->entity_associated_ids_arena, (size_t)id, &new_ids);
    }
    else {
        ids = cecs_sparse_set_get_unchecked(&wr->entity_associated_ids, (size_t)id, sizeof(cecs_associated_entity_ids));
    }

    return *CECS_COUNTED_SET_SET(cecs_entity_id, ids, &wr->entity_associated_ids_arena, (size_t)target.tag_id, &associated_entity_id);
}

cecs_entity_id cecs_world_relations_get_associated_id(cecs_world_relations* wr, cecs_entity_id id, cecs_relation_target target) {
    cecs_associated_entity_ids* ids = cecs_sparse_set_get_unchecked(&wr->entity_associated_ids, (size_t)id, sizeof(cecs_associated_entity_ids));
    return *CECS_COUNTED_SET_GET(cecs_entity_id, ids, (size_t)target.tag_id);
}

cecs_associated_entity_ids_iterator cecs_world_relations_get_associated_ids(cecs_world_relations* wr, cecs_entity_id id) {
    if (cecs_sparse_set_contains(&wr->entity_associated_ids, (size_t)id)) {
        cecs_associated_entity_ids* ids = cecs_sparse_set_get_unchecked(&wr->entity_associated_ids, (size_t)id, sizeof(cecs_associated_entity_ids));
        return cecs_counted_set_iterator_create_borrowed(ids);
    }
    else {
        return cecs_counted_set_iterator_create_owned(cecs_counted_set_empty());
    }
}

cecs_entity_id cecs_world_relations_increment_associated_id_or_set(cecs_world_relations* wr, cecs_entity_id id, cecs_relation_target target, cecs_entity_id otherwise_associated_id) {
    cecs_associated_entity_ids* ids;
    if (!cecs_sparse_set_contains(&wr->entity_associated_ids, (size_t)id)) {
        cecs_associated_entity_ids new_ids = cecs_counted_set_create_with_capacity(
            &wr->entity_associated_ids_arena,
            1,
            sizeof(cecs_entity_id)
        );
        ids = CECS_SPARSE_SET_SET(cecs_associated_entity_ids, &wr->entity_associated_ids, &wr->entity_associated_ids_arena, (size_t)id, &new_ids);
    }
    else {
        ids = cecs_sparse_set_get_unchecked(&wr->entity_associated_ids, (size_t)id, sizeof(cecs_associated_entity_ids));
    }

    return *CECS_COUNTED_SET_INCREMENT_OR_SET(cecs_entity_id, ids, &wr->entity_associated_ids_arena, (size_t)target.tag_id, &otherwise_associated_id);
}

bool cecs_world_relations_remove_associated_id(cecs_world_relations* wr, cecs_entity_id id, cecs_relation_target target, cecs_entity_id* out_associated_entity_id) {
    if (!cecs_sparse_set_contains(&wr->entity_associated_ids, (size_t)id)) {
        *out_associated_entity_id = 0;
        return false;
    }
    else {
        cecs_associated_entity_ids* ids = cecs_sparse_set_get_unchecked(&wr->entity_associated_ids, (size_t)id, sizeof(cecs_associated_entity_ids));
        return cecs_counted_set_remove_out(ids, (size_t)target.tag_id, out_associated_entity_id, sizeof(cecs_entity_id));
    }
}
