#include <assert.h>
#include <memory.h>
#include "cecs_component.h"

cecs_world_components_checksum cecs_world_components_checksum_hash(cecs_world_components_checksum current) {
    const uint32_t fnv32_prime = 0x01000193;
    const uint32_t fnv32_basis = 0x811C9DC5;
    return (fnv32_basis ^ current) * fnv32_prime;
}

cecs_world_components cecs_world_components_create(size_t component_type_capacity) {
    return (cecs_world_components) {
        .storages_arena = cecs_arena_create_with_capacity(
            (sizeof(cecs_component_storage) + sizeof(cecs_optional_component_size)) * component_type_capacity
        ),
            .components_arena = cecs_arena_create(),
            .component_storages = cecs_paged_sparse_set_create(),
            .component_storages_attachments = cecs_paged_sparse_set_create(),
            .component_sizes = cecs_paged_sparse_set_create(),
            .checksum = 0,
            .discard = (cecs_component_discard){ 0 }
    };
}

void cecs_world_components_free(cecs_world_components* wc) {
    cecs_arena_free(&wc->components_arena);
    cecs_arena_free(&wc->storages_arena);
    wc->component_storages = (cecs_paged_sparse_set){ 0 };
    wc->component_storages_attachments = (cecs_paged_sparse_set){ 0 };
    wc->component_sizes = (cecs_paged_sparse_set){ 0 };
    wc->discard = (cecs_component_discard){ 0 };
}

cecs_optional_component_size cecs_world_components_get_component_size(const cecs_world_components* wc, cecs_component_id component_id) {
    return CECS_OPTION_MAP_REFERENCE_STRUCT(
        cecs_optional_element,
        CECS_PAGED_SPARSE_SET_GET(size_t, &wc->component_sizes, (size_t)component_id),
        cecs_optional_component_size
    );
}

static size_t cecs_world_components_get_or_set_component_size(cecs_world_components* wc, cecs_component_id component_id, size_t size) {
    cecs_optional_component_size stored_size = cecs_world_components_get_component_size(wc, component_id);
    if (CECS_OPTION_IS_SOME(cecs_optional_component_size, stored_size)) {
        assert((*CECS_OPTION_GET(cecs_optional_component_size, stored_size) == size) && "error: component size mismatch");
    } else {
        stored_size = CECS_OPTION_CREATE_SOME_STRUCT(
            cecs_optional_component_size,
            CECS_PAGED_SPARSE_SET_SET(size_t, &wc->component_sizes, &wc->storages_arena, (size_t)component_id, &size)
        );
    }
    return *CECS_OPTION_GET(cecs_optional_component_size, stored_size);
}

cecs_optional_component_storage cecs_world_components_get_component_storage(const cecs_world_components* wc, cecs_component_id component_id) {
    return CECS_OPTION_MAP_REFERENCE_STRUCT(
        cecs_optional_element,
        CECS_PAGED_SPARSE_SET_GET(cecs_component_storage, &wc->component_storages, (size_t)component_id),
        cecs_optional_component_storage
    );
}

cecs_component_storage* cecs_world_components_get_component_storage_unchecked(const cecs_world_components* wc, cecs_component_id component_id) {
    return CECS_OPTION_GET(cecs_optional_component_storage, cecs_world_components_get_component_storage(wc, component_id));
}

bool cecs_world_components_has_storage(const cecs_world_components* wc, cecs_component_id component_id) {
    return cecs_paged_sparse_set_contains(&wc->component_storages, (size_t)component_id);
}

cecs_component_storage cecs_component_storage_descriptor_build(cecs_component_storage_descriptor descriptor, cecs_world_components* wc, size_t component_size) {
    if (component_size == 0 && descriptor.is_size_known) {
        return cecs_component_storage_create_unit(&wc->components_arena);
    }

    if (!descriptor.is_size_known)
        assert(component_size == 0 && "component_size must be zero if is_size_known is false");

    if (CECS_OPTION_IS_SOME(cecs_indirect_component_id, descriptor.indirect_component_id)) {
        cecs_component_id other_id = CECS_OPTION_GET(cecs_indirect_component_id, descriptor.indirect_component_id);
        if (cecs_world_components_has_storage(wc, other_id)) {
            return cecs_component_storage_create_indirect(
                &wc->components_arena
            );
        } else {
            // TODO: keep an eye on indirect storages, and allow customization
            cecs_component_storage other_storage = cecs_component_storage_descriptor_build((cecs_component_storage_descriptor) {
                .is_size_known = false,
                .capacity = descriptor.capacity,
                .indirect_component_id = CECS_OPTION_CREATE_NONE(cecs_indirect_component_id)
            }, wc, 0);
            return cecs_component_storage_create_indirect(
                &wc->components_arena
            );
        }
    } else {
        return cecs_component_storage_create_sparse(&wc->components_arena, descriptor.capacity, component_size);
    }
}

static cecs_component_storage *cecs_world_components_get_or_set_component_storage(
    cecs_world_components *wc,
    cecs_component_id component_id,
    cecs_component_storage_descriptor storage_descriptor,
    size_t size
) {
    cecs_optional_component_storage optional_storage = cecs_world_components_get_component_storage(wc, component_id);
    if (CECS_OPTION_IS_SOME(cecs_optional_component_storage, optional_storage)) {
        return CECS_OPTION_GET_UNCHECKED(cecs_optional_component_storage, optional_storage);
    } else {
        cecs_component_storage new_storage = cecs_component_storage_descriptor_build(storage_descriptor, wc, size);
        return CECS_PAGED_SPARSE_SET_SET(
            cecs_component_storage,
            &wc->component_storages,
            &wc->storages_arena,
            (size_t)component_id,
            &new_storage
        );
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
    if (size > wc->discard.size) {
        wc->discard.handle = cecs_arena_realloc(&wc->components_arena, wc->discard.handle, wc->discard.size, size);
        wc->discard.size = size;
    }
    wc->checksum = cecs_world_components_checksum_add(wc->checksum, component_id);
    size_t component_size = cecs_world_components_get_or_set_component_size(wc, component_id, size);
    cecs_component_storage *storage = cecs_world_components_get_or_set_component_storage(
        wc,
        component_id,
        additional_storage_descriptor,
        component_size
    );

    return cecs_component_storage_set(
        storage,
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
    if (size > wc->discard.size) {
        wc->discard.handle = cecs_arena_realloc(&wc->components_arena, wc->discard.handle, wc->discard.size, size);
        wc->discard.size = size;
    }
    wc->checksum = cecs_world_components_checksum_add(wc->checksum, component_id);
    size_t component_size = cecs_world_components_get_or_set_component_size(wc, component_id, size);
    cecs_component_storage *storage = cecs_world_components_get_or_set_component_storage(
        wc,
        component_id,
        additional_storage_descriptor,
        component_size
    );

    return cecs_component_storage_set_array(
        storage,
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
    if (size > wc->discard.size) {
        wc->discard.handle = cecs_arena_realloc(&wc->components_arena, wc->discard.handle, wc->discard.size, size);
        wc->discard.size = size;
    }
    wc->checksum = cecs_world_components_checksum_add(wc->checksum, component_id);
    size_t component_size = cecs_world_components_get_or_set_component_size(wc, component_id, size);
    cecs_component_storage *storage = cecs_world_components_get_or_set_component_storage(
        wc,
        component_id,
        additional_storage_descriptor,
        component_size
    );

    return cecs_component_storage_set_copy_array(
        storage,
        &wc->components_arena,
        entity_id,
        component_single_src,
        count,
        size
    );
}

bool cecs_world_components_has_component(const cecs_world_components* wc, cecs_entity_id entity_id, cecs_component_id component_id) {
    cecs_optional_component_size stored_size = cecs_world_components_get_component_size(wc, component_id);
    cecs_optional_component_storage storage = cecs_world_components_get_component_storage(wc, component_id);

    return CECS_OPTION_IS_SOME(cecs_optional_component_size, stored_size)
        && cecs_component_storage_has(CECS_OPTION_GET(cecs_optional_component_storage, storage), entity_id);
}

cecs_optional_component cecs_world_components_get_component(const cecs_world_components* wc, cecs_entity_id entity_id, cecs_component_id component_id) {
    cecs_optional_component_size stored_size = cecs_world_components_get_component_size(wc, component_id);
    cecs_optional_component_storage storage = cecs_world_components_get_component_storage(wc, component_id);
    if (!CECS_OPTION_IS_SOME(cecs_optional_component_size, stored_size)) {
        return CECS_OPTION_CREATE_NONE_STRUCT(cecs_optional_component);
    } else if (cecs_component_storage_has(CECS_OPTION_GET(cecs_optional_component_storage, storage), entity_id)) {
        return cecs_component_storage_get(
            CECS_OPTION_GET(cecs_optional_component_storage, storage),
            entity_id,
            *CECS_OPTION_GET(cecs_optional_component_size, stored_size)
        );
    } else {
        return CECS_OPTION_CREATE_NONE_STRUCT(cecs_optional_component);
    }
}

size_t cecs_world_components_get_component_array(
    const cecs_world_components* wc,
    cecs_entity_id entity_id,
    cecs_component_id component_id,
    void** out_components,
    size_t count
) {
    cecs_optional_component_size stored_size = cecs_world_components_get_component_size(wc, component_id);
    cecs_optional_component_storage storage = cecs_world_components_get_component_storage(wc, component_id);
    if (!CECS_OPTION_IS_SOME(cecs_optional_component_size, stored_size)) {
        *out_components = NULL;
        return 0;
    } else {
        return cecs_component_storage_get_array(
            CECS_OPTION_GET(cecs_optional_component_storage, storage),
            entity_id,
            out_components,
            count,
            *CECS_OPTION_GET(cecs_optional_component_size, stored_size)
        );
    }
}

bool cecs_world_components_remove_component(cecs_world_components* wc, cecs_entity_id entity_id, cecs_component_id component_id, void* out_removed_component) {
    wc->checksum = cecs_world_components_checksum_remove(wc->checksum, component_id);
    cecs_optional_component_size stored_size = cecs_world_components_get_component_size(wc, component_id);
    if (CECS_OPTION_IS_NONE(cecs_optional_component_size, stored_size)) {
        assert(out_removed_component == NULL && "out_removed_component must be NULL if component size is unknown");
        return false;
    }
    else {
        return cecs_component_storage_remove(
            CECS_OPTION_GET(cecs_optional_component_storage, cecs_world_components_get_component_storage(wc, component_id)),
            &wc->components_arena,
            entity_id,
            out_removed_component,
            *CECS_OPTION_GET(cecs_optional_component_size, stored_size)
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
    cecs_optional_component_size stored_size = cecs_world_components_get_component_size(wc, component_id);
    if (CECS_OPTION_IS_NONE(cecs_optional_component_size, stored_size)) {
        assert(out_removed_components == NULL && "out_removed_components must be NULL if component size is unknown");
        return 0;
    } else {
        return cecs_component_storage_remove_array(
            CECS_OPTION_GET(cecs_optional_component_storage, cecs_world_components_get_component_storage(wc, component_id)),
            &wc->components_arena,
            entity_id,
            out_removed_components,
            count,
            *CECS_OPTION_GET(cecs_optional_component_size, stored_size)
        );
    }
}

void *cecs_world_components_set_component_storage_attachments(
    cecs_world_components *wc,
    cecs_component_id component_id,
    void *attachments,
    size_t size
) {
    assert(attachments != NULL && "error: attachments must not be NULL");
    assert(size > 0 && "error: attachments size must be greater than zero");

    if (size > wc->discard.size) {
        wc->discard.handle = cecs_arena_realloc(&wc->components_arena, wc->discard.handle, wc->discard.size, size);
        wc->discard.size = size;
    }
    cecs_optional_element stored_attachments =
        CECS_PAGED_SPARSE_SET_GET(cecs_component_storage_attachments, &wc->component_storages_attachments, (size_t)component_id);
    if (CECS_OPTION_IS_NONE(cecs_optional_element, stored_attachments)) {
        cecs_component_storage_attachments new_attachments = {
            .user_attachments = cecs_arena_alloc(&wc->storages_arena, size),
            .attachments_size = size,
        };
        memcpy(new_attachments.user_attachments, attachments, size);
        return CECS_PAGED_SPARSE_SET_SET(
            cecs_component_storage_attachments,
            &wc->component_storages_attachments,
            &wc->storages_arena,
            (size_t)component_id,
            &new_attachments
        )->user_attachments;
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
        memcpy(current_attachments->user_attachments, attachments, size);
        return current_attachments->user_attachments;
    }
}

void *cecs_world_components_get_component_storage_attachments_unchecked(const cecs_world_components *wc, cecs_component_id component_id) {
    cecs_optional_element stored_attachments =
        CECS_PAGED_SPARSE_SET_GET(cecs_component_storage_attachments, &wc->component_storages_attachments, (size_t)component_id);
    CECS_OPTION_IS_SOME_ASSERT(cecs_optional_element, stored_attachments);
    cecs_component_storage_attachments *attachments = CECS_OPTION_GET_UNCHECKED(cecs_optional_element, stored_attachments);
    if (attachments->user_attachments == NULL) {
        assert(false && "unreachable: attachments must not be NULL");
        exit(EXIT_FAILURE);
    }
    if (attachments->attachments_size == 0) {
        assert(false && "unreachable: attachments size must be greater than zero");
        exit(EXIT_FAILURE);
    }
    return attachments->user_attachments;
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

cecs_sized_component_storage cecs_world_components_iterator_current(const cecs_world_components_iterator* it) {
    return (cecs_sized_component_storage) {
        .storage = ((cecs_component_storage*)cecs_paged_sparse_set_data(&it->components->component_storages)) + it->storage_raw_index,
            .component_size = ((size_t*)cecs_paged_sparse_set_data(&it->components->component_sizes))[it->storage_raw_index],
            .component_id = ((size_t*)cecs_paged_sparse_set_keys(&it->components->component_storages))[it->storage_raw_index]
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

size_t cecs_world_components_entity_iterator_next(cecs_world_components_entity_iterator* it) {
    do {
        cecs_world_components_iterator_next(&it->it);
    } while (
        !cecs_world_components_iterator_done(&it->it)
        && !cecs_component_storage_has(
            ((cecs_component_storage*)cecs_paged_sparse_set_data(&it->it.components->component_storages)) + it->it.storage_raw_index,
            it->entity_id
        )
        );

    return it->it.storage_raw_index;
}

cecs_sized_component_storage cecs_world_components_entity_iterator_current(const cecs_world_components_entity_iterator* it) {
    return cecs_world_components_iterator_current(&it->it);
}
