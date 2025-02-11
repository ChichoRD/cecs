#include <assert.h>
#include <memory.h>
#include <stddef.h>

#include "cecs_component.h"

cecs_world_components_checksum cecs_world_components_checksum_hash(cecs_world_components_checksum current) {
    const uint32_t fnv32_prime = 0x01000193;
    const uint32_t fnv32_basis = 0x811C9DC5;
    return (fnv32_basis ^ current) * fnv32_prime;
}

cecs_world_components cecs_world_components_create(size_t component_type_capacity) {
    return (cecs_world_components) {
        .storages_arena = cecs_arena_create_with_capacity(
            component_type_capacity * sizeof(cecs_sized_component_storage)
        ),
        .components_arena = cecs_arena_create(),
        .component_storages = cecs_paged_sparse_set_create(),
        .component_storages_attachments = cecs_paged_sparse_set_create(),
        .checksum = 0,
        .discard = cecs_discard_create()
    };
}
void cecs_world_components_free(cecs_world_components* wc) {
    wc->component_storages = (cecs_paged_sparse_set){ 0 };
    wc->component_storages_attachments = (cecs_paged_sparse_set){ 0 };
    cecs_arena_free(&wc->components_arena);
    cecs_arena_free(&wc->storages_arena);
    wc->discard = (cecs_component_discard){ 0 };
}

// FIXME: messed up with const qualifications
cecs_optional_component_storage cecs_world_components_get_component_storage(cecs_world_components* wc, const cecs_component_id component_id) {
    return CECS_OPTION_MAP_REFERENCE_STRUCT(
        cecs_optional_element,
        CECS_PAGED_SPARSE_SET_GET(cecs_sized_component_storage, &wc->component_storages, (size_t)component_id),
        cecs_optional_component_storage
    );
}

cecs_sized_component_storage *cecs_world_components_get_component_storage_expect(cecs_world_components* wc, const cecs_component_id component_id) {
    return CECS_OPTION_GET(cecs_optional_component_storage, cecs_world_components_get_component_storage(wc, component_id));
}

bool cecs_world_components_has_storage(const cecs_world_components* wc, cecs_component_id component_id) {
    return cecs_paged_sparse_set_contains(&wc->component_storages, (size_t)component_id);
}

static cecs_sized_component_storage *cecs_world_components_get_or_set_component_storage(
    cecs_world_components *wc,
    const cecs_component_id component_id,
    const cecs_component_storage_descriptor storage_descriptor,
    const size_t size
) {
    cecs_optional_component_storage optional_storage = cecs_world_components_get_component_storage(wc, component_id);
    if (CECS_OPTION_IS_SOME(cecs_optional_component_storage, optional_storage)) {
        cecs_sized_component_storage *storage = CECS_OPTION_GET_UNCHECKED(cecs_optional_component_storage, optional_storage);
        assert(
            ((storage->component_size == size) || (!storage_descriptor.is_size_known && size == 0))
            && "error: component size mismatch, or size known when it should not be"
        );
        return storage;
    } else {
        cecs_sized_component_storage new_storage = cecs_component_storage_descriptor_build(storage_descriptor, wc, size);
        return CECS_PAGED_SPARSE_SET_SET(
            cecs_sized_component_storage,
            &wc->component_storages,
            &wc->storages_arena,
            (size_t)component_id,
            &new_storage
        );
    }
}

cecs_sized_component_storage cecs_component_storage_descriptor_build(cecs_component_storage_descriptor descriptor, cecs_world_components* wc, size_t component_size) {
    if (component_size == 0 && descriptor.is_size_known) {
        return (cecs_sized_component_storage){
            .storage = cecs_component_storage_create_unit(&wc->components_arena),
            .component_size = 0
        };
    } else if (!descriptor.is_size_known) {
        assert(component_size == 0 && "component_size must be zero if is_size_known is false");
    }

    if (CECS_OPTION_IS_SOME(cecs_indirect_component_id, descriptor.indirect_component_id)) {
        const cecs_component_id other_id = CECS_OPTION_GET(cecs_indirect_component_id, descriptor.indirect_component_id);

        cecs_optional_component_storage optional_storage = cecs_world_components_get_component_storage(wc, other_id);
        if (CECS_OPTION_IS_NONE(cecs_optional_component_storage, optional_storage)) {
            assert(
                false
                && "fatal error: indirect referenced component must be set before setting a relation with it as component.\n"
                "It is necessary to know at least the size of the component before setting a relation with it."
            );
            exit(EXIT_FAILURE);
        }

        cecs_sized_component_storage *other_storage = CECS_OPTION_GET_UNCHECKED(cecs_optional_component_storage, optional_storage);
        return (cecs_sized_component_storage){
            .storage = cecs_component_storage_create_indirect(
                &wc->components_arena,
                &other_storage->storage,
                other_storage->component_size
            ),
            .component_size = component_size
        };
    } else {
        switch (descriptor.config.storage_type) {
        case cecs_component_config_storage_dense_set:
        case cecs_component_config_storage_flatmap:
        // TODO: create storage types
        case cecs_component_config_storage_sparse_array:{
            return (cecs_sized_component_storage){
                .storage = cecs_component_storage_create_sparse(&wc->components_arena, descriptor.capacity, component_size),
                .component_size = component_size
            };
        }
        default: {
            assert(false && "unreachable: invalid component storage type");
            exit(EXIT_FAILURE);
            return (cecs_sized_component_storage){0};
        }
        }
    }
}


extern inline cecs_world_components_checksum cecs_world_components_checksum_add(cecs_world_components_checksum current, cecs_component_id component_id);
extern inline cecs_world_components_checksum cecs_world_components_checksum_remove(cecs_world_components_checksum current, cecs_component_id component_id);

cecs_optional_component cecs_world_components_set_component(
    cecs_world_components* wc,
    cecs_entity_id entity_id,
    cecs_component_id component_id,
    void* component,
    size_t size,
    cecs_component_storage_descriptor additional_storage_descriptor
) {
    cecs_discard_ensure(&wc->discard, &wc->components_arena, size);
    wc->checksum = cecs_world_components_checksum_add(wc->checksum, component_id);

    cecs_sized_component_storage *storage = cecs_world_components_get_or_set_component_storage(
        wc,
        component_id,
        additional_storage_descriptor,
        size
    );

    return cecs_component_storage_set(
        &storage->storage,
        &wc->components_arena,
        entity_id,
        component,
        size
    );
}

cecs_optional_component_array cecs_world_components_set_component_array(
    cecs_world_components* wc,
    cecs_entity_id entity_id,
    cecs_component_id component_id,
    void* components,
    size_t count,
    size_t size,
    cecs_component_storage_descriptor additional_storage_descriptor
) {
    cecs_discard_ensure(&wc->discard, &wc->components_arena, size);
    wc->checksum = cecs_world_components_checksum_add(wc->checksum, component_id);

    cecs_sized_component_storage *storage = cecs_world_components_get_or_set_component_storage(
        wc,
        component_id,
        additional_storage_descriptor,
        size
    );

    return cecs_component_storage_set_array(
        &storage->storage,
        &wc->components_arena,
        entity_id,
        components,
        count,
        size
    );
}

cecs_optional_component_array cecs_world_components_set_component_copy_array(
    cecs_world_components *wc,
    cecs_entity_id entity_id,
    cecs_component_id component_id,
    void *component_single_src,
    size_t count,
    size_t size,
    cecs_component_storage_descriptor additional_storage_descriptor
) {
    cecs_discard_ensure(&wc->discard, &wc->components_arena, size);
    wc->checksum = cecs_world_components_checksum_add(wc->checksum, component_id);

    cecs_sized_component_storage *storage = cecs_world_components_get_or_set_component_storage(
        wc,
        component_id,
        additional_storage_descriptor,
        size
    );

    return cecs_component_storage_set_copy_array(
        &storage->storage,
        &wc->components_arena,
        entity_id,
        component_single_src,
        count,
        size
    );
}

bool cecs_world_components_has_component(const cecs_world_components* wc, cecs_entity_id entity_id, cecs_component_id component_id) {
    // TODO: deciding if casting away const when I know I do NOT mut is a good idea
    cecs_optional_component_storage storage = cecs_world_components_get_component_storage((cecs_world_components *)wc, component_id);

    return CECS_OPTION_IS_SOME(cecs_optional_component_storage, storage)
        && cecs_component_storage_has(&CECS_OPTION_GET_UNCHECKED(cecs_optional_component_storage, storage)->storage, entity_id);
}

cecs_optional_component cecs_world_components_get_component(cecs_world_components* wc, const cecs_entity_id entity_id, const cecs_component_id component_id) {
    cecs_optional_component_storage storage = cecs_world_components_get_component_storage(wc, component_id);
    if (CECS_OPTION_IS_NONE(cecs_optional_component_storage, storage)) {
        return CECS_OPTION_CREATE_NONE_STRUCT(cecs_optional_component);
    }
    
    cecs_sized_component_storage *sized_storage = CECS_OPTION_GET_UNCHECKED(cecs_optional_component_storage, storage);
    if (cecs_component_storage_has(&sized_storage->storage, entity_id)) {
        return cecs_component_storage_get(
            &sized_storage->storage,
            entity_id,
            sized_storage->component_size
        );
    } else {
        return CECS_OPTION_CREATE_NONE_STRUCT(cecs_optional_component);
    }
}

size_t cecs_world_components_get_component_array(
    cecs_world_components* wc,
    const cecs_entity_id entity_id,
    const cecs_component_id component_id,
    void** out_components,
    const size_t count
) {
    cecs_optional_component_storage storage = cecs_world_components_get_component_storage(wc, component_id);
    if (!CECS_OPTION_IS_SOME(cecs_optional_component_storage, storage)) {
        *out_components = NULL;
        return 0;
    }

    cecs_sized_component_storage *sized_storage = CECS_OPTION_GET_UNCHECKED(cecs_optional_component_storage, storage);
    return cecs_component_storage_get_array(
        &sized_storage->storage,
        entity_id,
        out_components,
        count,
        sized_storage->component_size
    );
    
}

bool cecs_world_components_remove_component(cecs_world_components* wc, cecs_entity_id entity_id, cecs_component_id component_id, void* out_removed_component) {
    wc->checksum = cecs_world_components_checksum_remove(wc->checksum, component_id);
    cecs_optional_component_storage storage = cecs_world_components_get_component_storage(wc, component_id);

    if (CECS_OPTION_IS_NONE(cecs_optional_component_storage, storage)) {
        assert(out_removed_component == NULL && "out_removed_component must be NULL if component size is unknown");
        return false;
    } else {
        cecs_sized_component_storage *sized_storage = CECS_OPTION_GET_UNCHECKED(cecs_optional_component_storage, storage);
        return cecs_component_storage_remove(
            &sized_storage->storage,
            &wc->components_arena,
            entity_id,
            out_removed_component,
            sized_storage->component_size
        );
    }
}

size_t cecs_world_components_remove_component_array(
    cecs_world_components *wc,
    cecs_entity_id entity_id,
    cecs_component_id component_id,
    void *out_removed_components,
    size_t count
) {
    wc->checksum = cecs_world_components_checksum_remove(wc->checksum, component_id);
    cecs_optional_component_storage storage = cecs_world_components_get_component_storage(wc, component_id);

    if (CECS_OPTION_IS_NONE(cecs_optional_component_storage, storage)) {
        assert(out_removed_components == NULL && "out_removed_components must be NULL if component size is unknown");
        return 0;
    } else {
        cecs_sized_component_storage *sized_storage = CECS_OPTION_GET_UNCHECKED(cecs_optional_component_storage, storage);
        return cecs_component_storage_remove_array(
            &sized_storage->storage,
            &wc->components_arena,
            entity_id,
            out_removed_components,
            count,
            sized_storage->component_size
        );
    }
}

const cecs_component_storage_attachments *cecs_world_components_set_component_storage_attachments(
    cecs_world_components *wc,
    cecs_component_id component_id,
    void *attachments,
    size_t size,
    cecs_component_storage_attachment_usage_flags flags
) {
    assert(attachments != NULL && "error: attachments must not be NULL");
    assert(size > 0 && "error: attachments size must be greater than zero");
    assert(
        flags != cecs_component_storage_attachment_usage_none
        && "error: attachments must be flagged for usage"
    );

    cecs_discard_ensure(&wc->discard, &wc->components_arena, size);
    cecs_optional_element stored_attachments =
        CECS_PAGED_SPARSE_SET_GET(cecs_component_storage_attachments, &wc->component_storages_attachments, (size_t)component_id);
    if (CECS_OPTION_IS_NONE(cecs_optional_element, stored_attachments)) {
        cecs_component_storage_attachments new_attachments = {
            .user_attachments = cecs_arena_alloc(&wc->storages_arena, size),
            .attachments_size = size,
            .flags = flags
        };
        memcpy(new_attachments.user_attachments, attachments, size);
        return CECS_PAGED_SPARSE_SET_SET(
            cecs_component_storage_attachments,
            &wc->component_storages_attachments,
            &wc->storages_arena,
            (size_t)component_id,
            &new_attachments
        );
    } else {
        cecs_component_storage_attachments *current_attachments =
            CECS_OPTION_GET_UNCHECKED(cecs_optional_element, stored_attachments);
        if (current_attachments->attachments_size < size) {
            current_attachments->user_attachments = cecs_arena_realloc(
                &wc->storages_arena,
                current_attachments->user_attachments,
                current_attachments->attachments_size,
                size
            );
        }
        current_attachments->attachments_size = size;
        current_attachments->flags = flags;
        memcpy(current_attachments->user_attachments, attachments, size);
        return current_attachments;
    }
}

bool cecs_world_components_has_component_storage_attachments(const cecs_world_components *wc, const cecs_component_id component_id) {
    // TODO: const correctness for this one (true const use)
    return CECS_OPTION_IS_SOME(
        cecs_optional_element,
        CECS_PAGED_SPARSE_SET_GET(cecs_component_storage_attachments, &wc->component_storages_attachments, (size_t)component_id)
    );
}

const cecs_component_storage_attachments *cecs_world_components_get_component_storage_attachments_expect(
    cecs_world_components *wc,
    const cecs_component_id component_id
) {
    cecs_optional_element stored_attachments =
        CECS_PAGED_SPARSE_SET_GET(cecs_component_storage_attachments, &wc->component_storages_attachments, (size_t)component_id);
    CECS_OPTION_IS_SOME_ASSERT(cecs_optional_element, stored_attachments);
    const cecs_component_storage_attachments *attachments = CECS_OPTION_GET_UNCHECKED(cecs_optional_element, stored_attachments);
    if (attachments->user_attachments == NULL) {
        assert(false && "unreachable: attachments must not be NULL");
        exit(EXIT_FAILURE);
    }
    if (attachments->attachments_size == 0) {
        assert(false && "unreachable: attachments size must be greater than zero");
        exit(EXIT_FAILURE);
    }
    if (attachments->flags == cecs_component_storage_attachment_usage_none) {
        assert(false && "unreachable: attachments must be flagged for usage");
        exit(EXIT_FAILURE);
    }
    return attachments;
}

const cecs_component_storage_attachments *cecs_world_components_get_or_set_component_storage_attachments(
    cecs_world_components *wc,
    cecs_component_id component_id,
    void *attachments,
    size_t size,
    cecs_component_storage_attachment_usage_flags flags
) {
    cecs_optional_element stored_attachments =
        CECS_PAGED_SPARSE_SET_GET(cecs_component_storage_attachments, &wc->component_storages_attachments, (size_t)component_id);

    if (CECS_OPTION_IS_NONE(cecs_optional_element, stored_attachments)) {
        assert(attachments != NULL && "error: attachments must not be NULL");
        assert(size > 0 && "error: attachments size must be greater than zero");

        cecs_discard_ensure(&wc->discard, &wc->components_arena, size);

        cecs_component_storage_attachments new_attachments = {
            .user_attachments = cecs_arena_alloc(&wc->storages_arena, size),
            .attachments_size = size,
            .flags = flags
        };
        memcpy(new_attachments.user_attachments, attachments, size);
        return CECS_PAGED_SPARSE_SET_SET(
            cecs_component_storage_attachments,
            &wc->component_storages_attachments,
            &wc->storages_arena,
            (size_t)component_id,
            &new_attachments
        );
    } else {
        cecs_component_storage_attachments *attachments = CECS_OPTION_GET_UNCHECKED(cecs_optional_element, stored_attachments);
        if (attachments->user_attachments == NULL) {
            assert(false && "unreachable: attachments must not be NULL");
            exit(EXIT_FAILURE);
        }
        if (attachments->attachments_size == 0) {
            assert(false && "unreachable: attachments size must be greater than zero");
            exit(EXIT_FAILURE);
        }
        if (attachments->flags == cecs_component_storage_attachment_usage_none) {
            assert(false && "unreachable: attachments must be flagged for usage");
            exit(EXIT_FAILURE);
        }
        return attachments;
    }
}

bool cecs_world_components_remove_component_storage_attachments(
    cecs_world_components *wc,
    cecs_component_id component_id,
    cecs_component_storage_attachments *out_removed_attachments
) {
    return CECS_PAGED_SPARSE_SET_REMOVE(
        cecs_component_storage_attachments,
        &wc->component_storages_attachments,
        &wc->storages_arena,
        (size_t)component_id,
        out_removed_attachments
    );
}

cecs_world_components_iterator cecs_world_components_iterator_create(const cecs_world_components* components) {
    return (cecs_world_components_iterator) {
        .components = components,
            .storage_raw_index = 0
    };
}

bool cecs_world_components_iterator_done(const cecs_world_components_iterator* it) {
    return it->storage_raw_index >= cecs_world_components_get_component_storage_count(it->components);
}

size_t cecs_world_components_iterator_next(cecs_world_components_iterator* it) {
    return ++it->storage_raw_index;
}

cecs_associated_component_storage cecs_world_components_iterator_current(cecs_world_components_iterator* it) {
    // TODO: solve this const borrowing
    cecs_sized_component_storage *current =
        ((cecs_sized_component_storage *)cecs_paged_sparse_set_values_mut(&it->components->component_storages)) + it->storage_raw_index;
    const cecs_component_id component_id =
        (cecs_component_id)cecs_paged_sparse_set_keys(&it->components->component_storages)[it->storage_raw_index];

    return (cecs_associated_component_storage) {
        .storage = current,
        .component_id = component_id
    };
}

cecs_world_components_entity_iterator cecs_world_components_entity_iterator_create(const cecs_world_components* components, cecs_entity_id entity_id) {
    return (cecs_world_components_entity_iterator) {
        .it = cecs_world_components_iterator_create(components),
            .entity_id = entity_id
    };
}

bool cecs_world_components_entity_iterator_done(const cecs_world_components_entity_iterator* it) {
    return cecs_world_components_iterator_done(&it->it);
}

static inline bool cecs_world_components_iterator_current_contains_entity(const cecs_world_components_entity_iterator* it) {
    assert(!cecs_world_components_iterator_done(&it->it) && "error: iterator is done");

    const cecs_component_storage *current = &((cecs_sized_component_storage *)(
        cecs_paged_sparse_set_values(&it->it.components->component_storages)
    ))[it->it.storage_raw_index].storage;
    return cecs_component_storage_has(
        current,
        it->entity_id
    );
}

size_t cecs_world_components_entity_iterator_next(cecs_world_components_entity_iterator* it) {
    do {
        cecs_world_components_iterator_next(&it->it);
    } while (
        !cecs_world_components_iterator_done(&it->it)
        && !cecs_world_components_iterator_current_contains_entity(it)
    );

    return it->it.storage_raw_index;
}

cecs_associated_component_storage cecs_world_components_entity_iterator_current(cecs_world_components_entity_iterator* it) {
    return cecs_world_components_iterator_current(&it->it);
}
