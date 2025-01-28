#include "cecs_relation.h"

cecs_world_relations cecs_world_relations_create(size_t entity_capacity) {
    const size_t initial_capacity = (entity_capacity >> 4) + 1;
    cecs_world_relations wr;
    wr.associations_arena = cecs_arena_create_with_capacity(sizeof(cecs_entity_associated_holders) * initial_capacity);
    wr.associations = (cecs_entity_associations){
        .entity_to_target_holders = cecs_flatmap_create(),
    };
    return wr;
}

void cecs_world_relations_free(cecs_world_relations* wr) {
    cecs_arena_free(&wr->associations_arena);
    wr->associations = (cecs_entity_associations){ 0 };
}


bool cecs_world_relations_add_target_holder(
    cecs_world_relations *wr,
    const cecs_entity_id source,
    const cecs_relation_target target,
    const cecs_target_holder_id holder
) {
    const cecs_entity_associated_holders default_holders = { cecs_sparse_set_create() };
    cecs_entity_associated_holders *holders = cecs_flatmap_get_or_add(
        &wr->associations.entity_to_target_holders,
        &wr->associations_arena,
        (cecs_flatmap_hash)source,
        &default_holders,
        sizeof(cecs_entity_associated_holders)
    );

    if (cecs_sparse_set_contains(&holders->target_to_holder, (size_t)target.tag_id)) {
        return false;
    } else {
        cecs_target_holder_info holder_info = { holder };
        cecs_sparse_set_set(
            &holders->target_to_holder,
            &wr->associations_arena,
            (size_t)target.tag_id,
            &holder_info,
            sizeof(cecs_target_holder_info)
        );
        return true;
    }
}

cecs_target_holder_id cecs_world_relations_add_target_expect(cecs_world_relations *wr, const cecs_entity_id source, const cecs_relation_target target) {
    const cecs_entity_associated_holders default_holders = { cecs_sparse_set_create() };
    cecs_entity_associated_holders *holders = cecs_flatmap_get_or_add(
        &wr->associations.entity_to_target_holders,
        &wr->associations_arena,
        (cecs_flatmap_hash)source,
        &default_holders,
        sizeof(cecs_entity_associated_holders)
    );

    cecs_target_holder_info *holder =
        cecs_sparse_set_get_expect(&holders->target_to_holder, (size_t)target.tag_id, sizeof(cecs_target_holder_info));
    return holder->holder;
}

static bool cecs_world_relations_has_holder_for_target(
    const cecs_world_relations *wr,
    const cecs_entity_id source,
    const cecs_relation_target target,
    cecs_entity_associated_holders **out_holders
) {
    cecs_entity_associated_holders *holders;
    if (
        !cecs_flatmap_get(
            &wr->associations.entity_to_target_holders,
            (cecs_flatmap_hash)source,
            (void **)holders,
            sizeof(cecs_entity_associated_holders)
        )
    ) {
        *out_holders = NULL;
        return false;
    }

    *out_holders = holders;
    return cecs_sparse_set_contains(&holders->target_to_holder, (size_t)target.tag_id);
}

bool cecs_world_relations_get_target(cecs_world_relations *wr, const cecs_entity_id source, const cecs_relation_target target, cecs_target_holder_id *out_target_holder) {
    cecs_entity_associated_holders *holders;
    if (!cecs_world_relations_has_holder_for_target(wr, source, target, &holders)) {
        return false;
    }

    cecs_target_holder_info *holder = cecs_sparse_set_get_expect(&holders->target_to_holder, (size_t)target.tag_id, sizeof(cecs_target_holder_info));
    *out_target_holder = holder->holder;
    return true;
}

bool cecs_world_relations_remove_target(cecs_world_relations *wr, const cecs_entity_id source, const cecs_relation_target target, cecs_target_holder_id *out_target_holder) {
    cecs_entity_associated_holders *holders;
    if (!cecs_world_relations_has_holder_for_target(wr, source, target, &holders)) {
        return false;
    }

    cecs_target_holder_info holder;
    if (
        !cecs_sparse_set_remove(
            &holders->target_to_holder,
            &wr->associations_arena,
            (size_t)target.tag_id,
            &holder,
            sizeof(cecs_target_holder_info)
        )
    ) {
        return false;
    }

    *out_target_holder = holder.holder;
    if (cecs_sparse_set_is_empty(&holders->target_to_holder)) {
        cecs_entity_associated_holders out;
        cecs_flatmap_remove(
            &wr->associations.entity_to_target_holders,
            &wr->associations_arena,
            (cecs_flatmap_hash)source,
            &out,
            sizeof(cecs_entity_associated_holders)
        );
    }

    return true;
}

bool cecs_world_relations_get_targets(cecs_world_relations *wr, const cecs_entity_id source, cecs_relation_targets_iterator *out_iterator) {
    cecs_entity_associated_holders *holders;
    if (
        !cecs_flatmap_get(
            &wr->associations.entity_to_target_holders,
            (cecs_flatmap_hash)source,
            (void **)&holders,
            sizeof(cecs_entity_associated_holders)
        )
    ) {
        return false;
    }

    *out_iterator = cecs_sparse_set_iterator_create_at_index(&holders->target_to_holder, 0);
    return true;
}