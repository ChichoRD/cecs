#include <memory.h>
#include "cecs_world.h"

cecs_world cecs_world_create(size_t entity_capacity, size_t component_type_capacity, size_t resource_capacity) {
    cecs_world w;
    w.entities = cecs_world_entities_create(entity_capacity);
    w.components = cecs_world_components_create(component_type_capacity);
    w.relations = cecs_world_relations_create(entity_capacity);

    const size_t resource_default_size = sizeof(intptr_t) * 4;
    w.resources = cecs_world_resources_create(resource_capacity, resource_default_size);
    return w;
}

void *cecs_world_get_component(cecs_world* w, const cecs_entity_id entity_id, const cecs_component_id component_id) {
    return cecs_world_components_get_component_expect(&w->components, entity_id, component_id);
}

bool cecs_world_try_get_component(cecs_world* w, const cecs_entity_id entity_id, const cecs_component_id component_id, void** out_component) {
    cecs_optional_component component = cecs_world_components_get_component(&w->components, entity_id, component_id);
    if (CECS_OPTION_IS_SOME(cecs_optional_component, component)) {
        *out_component = CECS_OPTION_GET(cecs_optional_component, component);
        return true;
    } else {
        *out_component = NULL;
        return false;
    }
}

size_t cecs_world_get_component_array(cecs_world *w, const cecs_entity_id_range range, const cecs_component_id component_id, void **out_components) {
    return cecs_world_components_get_component_array(
        &w->components,
        (cecs_entity_id)range.start,
        component_id,
        out_components,
        cecs_exclusive_range_length(range)
    );
}

cecs_entity_flags cecs_world_get_entity_flags(const cecs_world* w, const cecs_entity_id entity_id) {
    assert(cecs_world_enities_has_entity(&w->entities, entity_id) && "entity with given ID does not exist");
    cecs_entity_flags *flags = NULL;
    if (CECS_WORLD_TRY_GET_COMPONENT(cecs_entity_flags, (cecs_world *)w, entity_id, &flags)) {
        return *flags;
    } else {
        return cecs_entity_flags_default();
    }
}

void* cecs_world_set_component(cecs_world* w, cecs_entity_id id, cecs_component_id component_id, void* component, size_t size) {
    assert(cecs_world_enities_has_entity(&w->entities, id) && "entity with given ID does not exist");
    assert(
        !cecs_world_get_entity_flags(w, id).is_inmutable
        && "entity with given ID is inmutable and its components may not be set"
    );

    return cecs_world_components_set_component_expect(
        &w->components,
        id,
        component_id,
        component,
        size,
        (cecs_component_storage_descriptor) {
            .capacity = 1,
            .is_size_known = true,
            .indirect_component_id = { 0 }
        }
    );
}

void *cecs_world_set_component_array(cecs_world *w, cecs_entity_id_range range, cecs_component_id component_id, void *components, size_t size) {
    for (cecs_entity_id e = (cecs_entity_id)range.start; e < (cecs_entity_id)range.end; ++e) {
        assert(cecs_world_enities_has_entity(&w->entities, e) && "error: entity with given ID does not exist");
        assert(
            !cecs_world_get_entity_flags(w, e).is_inmutable
            && "error: entity with given ID is inmutable and its components may not be set"
        );
    }

    return cecs_world_components_set_component_array_unchecked(
        &w->components,
        (cecs_entity_id)range.start,
        component_id,
        components,
        cecs_exclusive_range_length(range),
        size,
        (cecs_component_storage_descriptor) {
            .capacity = 1,
            .is_size_known = true,
            .indirect_component_id = { 0 }
        }
    );
}

void *cecs_world_set_component_copy_array(cecs_world *w, cecs_entity_id_range range, cecs_component_id component_id, void *component_single_src, size_t size) {
    for (cecs_entity_id e = (cecs_entity_id)range.start; e < (cecs_entity_id)range.end; ++e) {
        assert(cecs_world_enities_has_entity(&w->entities, e) && "error: entity with given ID does not exist");
        assert(
            !cecs_world_get_entity_flags(w, e).is_inmutable
            && "error: entity with given ID is inmutable and its components may not be set"
        );
    }

    return cecs_world_components_set_component_copy_array_unchecked(
        &w->components,
        (cecs_entity_id)range.start,
        component_id,
        component_single_src,
        cecs_exclusive_range_length(range),
        size,
        (cecs_component_storage_descriptor) {
            .capacity = 1,
            .is_size_known = true,
            .indirect_component_id = { 0 }
        }
    );
}

bool cecs_world_remove_component(cecs_world* w, cecs_entity_id id, cecs_component_id component_id, void* out_removed_component) {
    assert(cecs_world_enities_has_entity(&w->entities, id) && "entity with given ID does not exist");
    assert(
        !cecs_world_get_entity_flags(w, id).is_inmutable
        && "entity with given ID is inmutable and components may not be removed from it"
    );

    return cecs_world_components_remove_component(&w->components, id, component_id, out_removed_component);
}

size_t cecs_world_remove_component_array(cecs_world *w, cecs_entity_id_range range, cecs_component_id component_id, void *out_removed_components) {
    for (cecs_entity_id e = (cecs_entity_id)range.start; e < (cecs_entity_id)range.end; ++e) {
        assert(cecs_world_enities_has_entity(&w->entities, e) && "entity with given ID does not exist");
        assert(
            !cecs_world_get_entity_flags(w, e).is_inmutable
            && "entity with given ID is inmutable and components may not be removed from it"
        );
    }

    return cecs_world_components_remove_component_array(
        &w->components,
        (cecs_entity_id)range.start,
        component_id,
        out_removed_components,
        cecs_exclusive_range_length(range)
    );
}

void *cecs_world_set_component_storage_attachments(cecs_world *w, cecs_component_id component_id, void *attachments, size_t size) {
    return cecs_world_components_set_component_storage_attachments(
        &w->components,
        component_id,
        attachments,
        size,
        cecs_component_storage_attachment_usage_user
    )->user_attachments;
}

bool cecs_world_has_component_storage_attachments(const cecs_world *w, cecs_component_id component_id) {
    return cecs_world_components_has_component_storage_attachments(&w->components, component_id);
}

void *cecs_world_get_component_storage_attachments(cecs_world *w, const cecs_component_id component_id) {
    return cecs_world_components_get_component_storage_attachments_expect(
        &w->components,
        component_id
    )->user_attachments;
}

void *cecs_world_get_or_set_component_storage_attachments(cecs_world *w, cecs_component_id component_id, void *default_attachments, size_t size) {
    return cecs_world_components_get_or_set_component_storage_attachments(
        &w->components,
        component_id,
        default_attachments,
        size,
        cecs_component_storage_attachment_usage_user
    )->user_attachments;
}

bool cecs_world_remove_component_storage_attachments(cecs_world *w, cecs_component_id component_id, void *out_removed_attachments) {
    cecs_component_storage_attachments removed_attachments;
    if (!cecs_world_components_remove_component_storage_attachments(
        &w->components,
        component_id,
        &removed_attachments
    )) {
        return false;
    }
    
    if (out_removed_attachments != NULL) {
        memcpy(out_removed_attachments, removed_attachments.user_attachments, removed_attachments.attachments_size);
    }
    return true;
}

cecs_entity_flags* cecs_world_get_or_set_default_entity_flags(cecs_world* w, cecs_entity_id id) {
    assert(cecs_world_enities_has_entity(&w->entities, id) && "entity with given ID does not exist");
    cecs_entity_flags* flags = NULL;
    if (CECS_WORLD_TRY_GET_COMPONENT(cecs_entity_flags, w, id, &flags)) {
        return flags;
    }
    else {
        return cecs_world_set_entity_flags(w, id, cecs_entity_flags_default());
    }
}

bool cecs_world_has_tag(const cecs_world* w, cecs_entity_id id, cecs_tag_id tag_id) {
    if (!cecs_world_enities_has_entity(&w->entities, id))
        return false;

    return cecs_world_components_has_component(&w->components, id, tag_id);
}

cecs_tag_id cecs_world_add_tag(cecs_world* w, cecs_entity_id id, cecs_tag_id tag_id) {
    assert(cecs_world_enities_has_entity(&w->entities, id) && "entity with given ID does not exist");
    assert(
        !cecs_world_get_entity_flags(w, id).is_inmutable
        && "entity with given ID is inmutable and tags may not be added to it"
    );

    cecs_world_components_set_component(
        &w->components,
        id,
        tag_id,
        NULL,
        0,
        (cecs_component_storage_descriptor) {
            .capacity = 1,
            .is_size_known = true,
            .indirect_component_id = { 0 }
        }
    );
    return tag_id;
}

cecs_tag_id cecs_world_add_tag_array(cecs_world *w, cecs_entity_id_range range, cecs_tag_id tag_id) {
    for (cecs_entity_id e = (cecs_entity_id)range.start; e < (cecs_entity_id)range.end; ++e) {
        assert(cecs_world_enities_has_entity(&w->entities, e) && "error: entity with given ID does not exist");
        assert(
            !cecs_world_get_entity_flags(w, e).is_inmutable
            && "error: entity with given ID is inmutable and tags may not be added to it"
        );
    }

    cecs_world_components_set_component_copy_array_unchecked(
        &w->components,
        (cecs_entity_id)range.start,
        tag_id,
        NULL,
        cecs_exclusive_range_length(range),
        0,
        (cecs_component_storage_descriptor) {
            .capacity = 1,
            .is_size_known = true,
            .indirect_component_id = { 0 }
        }
    );
    return tag_id;
}

cecs_tag_id cecs_world_remove_tag(cecs_world* w, cecs_entity_id id, cecs_tag_id tag_id) {
    assert(cecs_world_enities_has_entity(&w->entities, id) && "entity with given ID does not exist");
    assert(
        !cecs_world_get_entity_flags(w, id).is_inmutable
        && "entity with given ID is inmutable and tags may not be removed from it"
    );

    cecs_world_components_remove_component(&w->components, id, tag_id, &(cecs_optional_component){0});
    return tag_id;
}

size_t cecs_world_remove_tag_array(cecs_world *w, cecs_entity_id_range range, cecs_tag_id tag_id) {
    for (cecs_entity_id e = (cecs_entity_id)range.start; e < (cecs_entity_id)range.end; ++e) {
        assert(cecs_world_enities_has_entity(&w->entities, e) && "error: entity with given ID does not exist");
        assert(
            !cecs_world_get_entity_flags(w, e).is_inmutable
            && "error: entity with given ID is inmutable and tags may not be removed from it"
        );
    }

    return cecs_world_components_remove_component_array(
        &w->components,
        (cecs_entity_id)range.start,
        tag_id,
        NULL,
        cecs_exclusive_range_length(range)
    );
}

void *cecs_world_use_component_discard(cecs_world *w, size_t size)  {
    return cecs_discard_use(&w->components.discard, &w->components.components_arena, size);
}

void *cecs_world_use_resource_discard(cecs_world *w, size_t size) {
    return cecs_discard_use(&w->resources.discard, &w->resources.resources_arena, size);
}

cecs_entity_id cecs_world_add_entity(cecs_world* w) {
    cecs_entity_id e = cecs_world_entities_add_entity(&w->entities);
#if CECS_WORLD_FLAG_ALL_ENTITIES
    cecs_world_set_entity_flags(w, e, cecs_entity_flags_default());
#endif
    return e;
}

cecs_entity_id cecs_world_clear_entity(cecs_world* w, cecs_entity_id entity_id) {
    assert(
        !cecs_world_get_entity_flags(w, entity_id).is_inmutable
        && "entity with given ID is inmutable and cannot be cleared"
    );

    for (
        cecs_world_components_entity_iterator it = cecs_world_components_entity_iterator_create(&w->components, entity_id);
        !cecs_world_components_entity_iterator_done(&it);
        cecs_world_components_entity_iterator_next(&it)
        ) {
        cecs_associated_component_storage storage = cecs_world_components_entity_iterator_current(&it);
        cecs_component_storage_remove(
            &storage.storage->storage,
            &w->components.components_arena,
            entity_id,
            cecs_world_use_component_discard(w, storage.storage->component_size),
            storage.storage->component_size
        );
    }
    return entity_id;
}

cecs_entity_id cecs_world_copy_entity_onto(cecs_world* w, cecs_entity_id destination, cecs_entity_id source) {
    assert(
        !cecs_world_get_entity_flags(w, destination).is_inmutable
        && "entity with given ID is inmutable and components may not be copied onto it"
    );

    for (
        cecs_world_components_entity_iterator it = cecs_world_components_entity_iterator_create(&w->components, source);
        !cecs_world_components_entity_iterator_done(&it);
        cecs_world_components_entity_iterator_next(&it)
    ) {
        cecs_associated_component_storage storage = cecs_world_components_entity_iterator_current(&it);
        cecs_component_storage_set(
            &storage.storage->storage,
            &w->components.components_arena,
            destination,
            cecs_component_storage_info(&storage.storage->storage).is_unit_type_storage
            ? NULL
            : CECS_OPTION_GET(cecs_optional_component, cecs_component_storage_get(
                &storage.storage->storage,
                source,
                storage.storage->component_size
            )),
            storage.storage->component_size
        );
    }

    return destination;
}

cecs_entity_id cecs_world_copy_entity(cecs_world* w, cecs_entity_id destination, cecs_entity_id source) {
    cecs_world_clear_entity(w, destination);
    return cecs_world_copy_entity_onto(w, destination, source);
}

cecs_entity_id cecs_world_add_entity_copy(cecs_world* w, cecs_entity_id source) {
    return cecs_world_copy_entity_onto(w, cecs_world_add_entity(w), source);
}

cecs_entity_id_range cecs_world_add_entity_range(cecs_world *w, size_t count) {
    cecs_entity_id_range range = cecs_world_entities_add_entity_range(&w->entities, count);
#if CECS_WORLD_FLAG_ALL_ENTITIES
    for (cecs_entity_id e = (cecs_entity_id)range.start; e < (cecs_entity_id)range.end; ++e) {
        cecs_world_set_entity_flags(w, e, cecs_entity_flags_default());
    }
#endif
    return range;
}

size_t cecs_world_clear_entity_range(cecs_world *w, cecs_entity_id_range range, cecs_entity_id representative) {
    assert(
        !cecs_world_get_entity_flags(w, representative).is_inmutable
        && "error: entity with given ID is inmutable and cannot be cleared"
    );

    size_t count = 0;
    for (
        cecs_world_components_entity_iterator it = cecs_world_components_entity_iterator_create(&w->components, representative);
        !cecs_world_components_entity_iterator_done(&it);
        cecs_world_components_entity_iterator_next(&it)
    ) {
        cecs_associated_component_storage storage = cecs_world_components_entity_iterator_current(&it);
        cecs_component_storage_remove_array(
            &storage.storage->storage,
            &w->components.components_arena,
            range.start,
            cecs_world_use_component_discard(w, storage.storage->component_size),
            cecs_exclusive_range_length(range),
            storage.storage->component_size
        );
        ++count;
    }
    return count;
}

cecs_entity_id_range cecs_world_remove_entity_range(cecs_world *w, cecs_entity_id_range range) {
    for (cecs_entity_id e = (cecs_entity_id)range.start; e < (cecs_entity_id)range.end; ++e) {
        assert(
            !cecs_world_get_entity_flags(w, e).is_permanent
            && "entity with given ID is permanent and cannot be removed"
        );
    }
    
    CECS_WORLD_SET_COMPONENT_COPY_ARRAY(
        cecs_entity_flags,
        w,
        range,
        &CECS_ENTITY_FLAGS_DEFAULT
    );
    cecs_world_clear_entity_range(w, range, range.start);
    return cecs_world_entities_remove_entity_range(&w->entities, range);
}

size_t cecs_world_copy_entity_range_onto(cecs_world *w, cecs_entity_id_range destination, cecs_entity_id_range source, cecs_entity_id representative) {
    assert(
        !cecs_world_get_entity_flags(w, representative).is_inmutable
        && "error: entity with given ID is inmutable and components may not be copied onto it"
    );

    size_t destination_count = cecs_exclusive_range_length(destination);
    size_t source_count = cecs_exclusive_range_length(source);
    assert(
        destination_count >= source_count
        && "error: destination range must be larger than source range"
    );

    size_t component_types_count = 0;
    for (
        cecs_world_components_entity_iterator it = cecs_world_components_entity_iterator_create(&w->components, representative);
        !cecs_world_components_entity_iterator_done(&it);
        cecs_world_components_entity_iterator_next(&it)
    ) {
        cecs_associated_component_storage storage = cecs_world_components_entity_iterator_current(&it);
        const void *source_components;
        const cecs_storage_info storage_info = cecs_component_storage_info(&storage.storage->storage);
        if (storage_info.is_unit_type_storage) {
            source_components = NULL;
        } else if (CECS_UNION_IS(cecs_indirect_component_storage, cecs_component_storage_union, storage.storage->storage.storage)) {
            assert(storage_info.guarantees_contiguity && "fatal error: indirect component storage must guarantee contiguity");
            const cecs_indirect_component_storage *indirect_storage =
                &CECS_UNION_GET_UNCHECKED(cecs_indirect_component_storage, storage.storage->storage.storage);
            source_components = cecs_sentinel_set_get_range_inbounds(
                &indirect_storage->component_indices,
                cecs_inclusive_range_from_exclusive(source.range),
                storage.storage->component_size
            );
        } else if (storage_info.guarantees_contiguity) {
            void *contiguous_source_components;
            size_t copy_source_count = cecs_component_storage_get_array(
                &storage.storage->storage,
                source.start,
                &contiguous_source_components,
                source_count,
                storage.storage->component_size
            );

            assert(
                copy_source_count == source_count
                && "error: source range does not contain enough components to copy"
            );
            source_components = contiguous_source_components;
        } else {
            // FIXME: implement!
            assert(false && "unimplemented");
            exit(EXIT_FAILURE);
        }

        cecs_component_storage_set_array(
            &storage.storage->storage,
            &w->components.components_arena,
            destination.start,
            source_components,
            source_count,
            storage.storage->component_size
        );

        size_t copied_count = source_count;
        while ((copied_count << 1) < destination_count) {
            cecs_component_storage_set_array(
                &storage.storage->storage,
                &w->components.components_arena,
                destination.start + copied_count,
                source_components,
                source_count,
                storage.storage->component_size
            );
            copied_count <<= 1;
        }

        assert(
            copied_count >= destination_count / 2
            && "error: copied components should be at least half of destination components"
        );
        assert(
            copied_count <= destination_count
            && "error: copied components should be at most the same as destination components"
        );
        
        size_t remaining_count = destination_count - copied_count;
        cecs_component_storage_set_array(
            &storage.storage->storage,
            &w->components.components_arena,
            destination.start + copied_count,
            source_components,
            remaining_count,
            storage.storage->component_size
        );
        copied_count += remaining_count;

        assert(
            copied_count == destination_count
            && "error: destination range does not contain enough components to copy"
        );
        ++component_types_count;
    }
    return component_types_count;
}

size_t cecs_world_copy_entity_range(
    cecs_world *w,
    cecs_entity_id_range destination,
    cecs_entity_id_range source,
    cecs_entity_id clear_representative,
    cecs_entity_id copy_representative
) {
    cecs_world_clear_entity_range(w, destination, clear_representative);
    return cecs_world_copy_entity_range_onto(w, destination, source, copy_representative);
}

void* cecs_world_copy_entity_onto_and_grab(cecs_world* w, cecs_entity_id destination, cecs_entity_id source, cecs_component_id grab_component_id) {
    assert(
        !cecs_world_get_entity_flags(w, destination).is_inmutable
        && "entity with given ID is inmutable and components may not be copied onto it"
    );

    void* grabbed = NULL;
    for (
        cecs_world_components_entity_iterator it = cecs_world_components_entity_iterator_create(&w->components, source);
        !cecs_world_components_entity_iterator_done(&it);
        cecs_world_components_entity_iterator_next(&it)
        ) {
        cecs_associated_component_storage storage = cecs_world_components_entity_iterator_current(&it);
        cecs_optional_component copied_component = cecs_component_storage_set(
            &storage.storage->storage,
            &w->components.components_arena,
            destination,
            cecs_component_storage_info(&storage.storage->storage).is_unit_type_storage
            ? NULL
            : CECS_OPTION_GET(cecs_optional_component, cecs_component_storage_get(
                &storage.storage->storage,
                source,
                storage.storage->component_size
            )),
            storage.storage->component_size
        );

        if (CECS_OPTION_IS_SOME(cecs_optional_component, copied_component) && grab_component_id == storage.component_id) {
            grabbed = CECS_OPTION_GET(cecs_optional_component, copied_component);
        }
    }

    return grabbed;
}

void* cecs_world_copy_entity_and_grab(cecs_world* w, cecs_entity_id destination, cecs_entity_id source, cecs_component_id grab_component_id) {
    cecs_world_clear_entity(w, destination);
    return cecs_world_copy_entity_onto_and_grab(w, destination, source, grab_component_id);
}

static cecs_target_holder_id cecs_world_get_or_add_relation_target(cecs_world *w, cecs_entity_id source, cecs_relation_target target, bool *out_created_new) {
    *out_created_new = false;
    cecs_target_holder_id holder;
    if (!cecs_world_relations_get_target(&w->relations, source, target, &holder)) {
        holder = cecs_world_add_entity(w);
        bool added = cecs_world_relations_add_target_holder(&w->relations, source, target, holder);
        assert(added && "fatal error: failed to add relation target holder");
        *out_created_new = true;
    }
    return holder;
}

void* cecs_world_set_component_relation(cecs_world* w, cecs_entity_id id, cecs_component_id component_id, void* component, size_t size, cecs_tag_id tag_id) {
    assert(cecs_world_enities_has_entity(&w->entities, id) && "entity with given ID does not exist");
    
    bool created_new;
    cecs_target_holder_id holder = cecs_world_get_or_add_relation_target(w, id, (cecs_relation_target) {tag_id}, &created_new);
    
    if (created_new) {
        cecs_world_set_component(
            w,
            holder,
            CECS_TYPE_ID(CECS_COMPONENT(cecs_relation_entity_reference)),
            &(cecs_relation_entity_reference){id},
            sizeof(cecs_relation_entity_reference)
        );
        *cecs_world_get_or_set_default_entity_flags(w, id) = (cecs_entity_flags){ .is_permanent = true };
    }

    void* component_source =
#if CECS_WORLD_UNIQUE_RELATION_COMPONENTS
        cecs_world_set_component(
            w,
            id,
            component_id,
            component,
            size
        );
#else
        component;
#endif

    void *holder_component = cecs_world_set_component(
        w,
        holder,
        component_id,
        component_source,
        size
    );
    void *indirect_component = cecs_world_components_set_component_expect(
        &w->components,
        id,
        cecs_relation_id_create(cecs_relation_id_descriptor_create_tag(component_id, tag_id)),
        &holder,
        sizeof(cecs_entity_id),
        (cecs_component_storage_descriptor) {
            .capacity = 1,
            .indirect_component_id = CECS_OPTION_CREATE_SOME(cecs_indirect_component_id, component_id),
            .is_size_known = true,
            .config = CECS_COMPONENT_CONFIG_DEFAULT
        }
    );
    assert(indirect_component == holder_component && "error: stored indirect component does not match holder component");
    return indirect_component;
}

void* cecs_world_get_component_relation(cecs_world* w, const cecs_entity_id id, const cecs_component_id component_id, const cecs_tag_id tag_id) {
    assert(cecs_world_enities_has_entity(&w->entities, id) && "entity with given ID does not exist");
    return cecs_world_get_component(w, id, cecs_relation_id_create(cecs_relation_id_descriptor_create_tag(component_id, tag_id)));
}

bool cecs_world_remove_component_relation(cecs_world* w, cecs_entity_id id, cecs_component_id component_id, void* out_removed_component, cecs_tag_id tag_id) {
    assert(cecs_world_enities_has_entity(&w->entities, id) && "entity with given ID does not exist");
    cecs_target_holder_id holder;
    if (!cecs_world_relations_get_target(&w->relations, id, (cecs_relation_target) { tag_id }, &holder)) {
        return false;
    }

    return cecs_world_remove_component(
        w,
        id,
        cecs_relation_id_create(cecs_relation_id_descriptor_create_tag(component_id, tag_id)),
        out_removed_component
    )
#if CECS_WORLD_UNIQUE_RELATION_COMPONENTS
        && cecs_world_remove_component(w, id, component_id, out_removed_component)
#endif
        && cecs_world_remove_component(
            w,
            holder,
            component_id,
            out_removed_component
        );
}

cecs_tag_id cecs_world_add_tag_relation(cecs_world* w, cecs_entity_id id, cecs_tag_id tag, cecs_tag_id target_tag_id) {
    assert(cecs_world_enities_has_entity(&w->entities, id) && "entity with given ID does not exist");
    
    bool created_new;
    cecs_target_holder_id holder =
        cecs_world_get_or_add_relation_target(w, id, (cecs_relation_target){ target_tag_id }, &created_new);
    if (created_new) {
        cecs_world_set_component(
            w,
            holder,
            CECS_TYPE_ID(CECS_COMPONENT(cecs_relation_entity_reference)),
            &(cecs_relation_entity_reference){id},
            sizeof(cecs_relation_entity_reference)
        );
        *cecs_world_get_or_set_default_entity_flags(w, id) = (cecs_entity_flags){ .is_permanent = true };
    }

    cecs_tag_id tag_source =
#if CECS_WORLD_UNIQUE_RELATION_COMPONENTS
        cecs_world_add_tag(
            w,
            id,
            tag
        );
#else
        tag;
#endif

    cecs_world_add_tag(
        w,
        holder,
        tag_source
    );
    cecs_tag_id tag_id = cecs_world_add_tag(
        w,
        id,
        cecs_relation_id_create(
            cecs_relation_id_descriptor_create_tag(tag, target_tag_id)
        )
    );

    return tag_id;
}

bool cecs_world_remove_tag_relation(cecs_world* w, cecs_entity_id id, cecs_tag_id tag, cecs_tag_id target_tag_id) {
    assert(cecs_world_enities_has_entity(&w->entities, id) && "entity with given ID does not exist");
    cecs_target_holder_id holder;
    if (!cecs_world_relations_get_target(&w->relations, id, (cecs_relation_target) { target_tag_id }, &holder)) {
        return false;
    }

    return cecs_world_remove_tag(
        w,
        id,
        cecs_relation_id_create(cecs_relation_id_descriptor_create_tag(tag, target_tag_id))
    )
#if CECS_WORLD_UNIQUE_RELATION_COMPONENTS
        && cecs_world_remove_tag(w, id, tag)
#endif
        && cecs_world_remove_tag(
            w,
            holder,
            tag
        );
}

cecs_relation_targets_iterator cecs_world_get_associated_ids(cecs_world* w, cecs_entity_id id) {
    assert(cecs_world_enities_has_entity(&w->entities, id) && "entity with given ID does not exist");

    cecs_relation_targets_iterator it;
    bool has_relations = cecs_world_relations_get_targets(&w->relations, id, &it);
    assert(has_relations && "error: entity with given ID has no associated entities");

    return it;
}

cecs_resource_handle cecs_world_set_resource(cecs_world* w, cecs_resource_id id, void* resource, size_t size) {
    return cecs_world_resources_set_resource(&w->resources, id, resource, size);
}

cecs_resource_handle cecs_world_get_resource(const cecs_world* w, cecs_resource_id id) {
    return cecs_world_resources_get_resource(&w->resources, id);
}

bool cecs_world_remove_resource(cecs_world* w, cecs_resource_id id) {
    return cecs_world_resources_remove_resource(&w->resources, id);
}

bool cecs_world_remove_resource_out(cecs_world* w, cecs_resource_id id, cecs_resource_handle out_resource, size_t size) {
    return cecs_world_resources_remove_resource_out(&w->resources, id, out_resource, size);
}

cecs_entity_id cecs_world_remove_entity(cecs_world* w, cecs_entity_id entity_id) {
    assert(
        !cecs_world_get_entity_flags(w, entity_id).is_permanent
        && "entity with given ID is permanent and cannot be removed"
    );

    cecs_world_set_entity_flags(w, entity_id, cecs_entity_flags_default());
    cecs_world_clear_entity(w, entity_id);
    return cecs_world_entities_remove_entity(&w->entities, entity_id);
}

void cecs_world_free(cecs_world* w) {
    cecs_world_entities_free(&w->entities);
    cecs_world_components_free(&w->components);
    cecs_world_relations_free(&w->relations);
    cecs_world_resources_free(&w->resources);
    w->resources = (cecs_world_resources){ 0 };
    w->relations = (cecs_world_relations){ 0 };
    w->components = (cecs_world_components){ 0 };
    w->entities = (cecs_world_entities){ 0 };
}
