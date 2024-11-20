#include "component.h"

world_components_checksum world_components_checksum_hash(world_components_checksum current) {
    const uint32_t fnv32_prime = 0x01000193;
    const uint32_t fnv32_basis = 0x811C9DC5;
    return (fnv32_basis ^ current) * fnv32_prime;
}

inline world_components_checksum world_components_checksum_add(world_components_checksum current, component_id component_id) {
    return world_components_checksum_hash(current + (world_components_checksum)component_id);
}

world_components world_components_create(size_t component_type_capacity) {
    return (world_components) {
        .storages_arena = cecs_arena_create_with_capacity(
            (sizeof(component_storage) + sizeof(optional_component_size)) * component_type_capacity
        ),
            .components_arena = cecs_arena_create(),
            .component_storages = cecs_paged_sparse_set_create(),
            .component_sizes = cecs_paged_sparse_set_create(),
            .checksum = 0,
            .discard = (component_discard){ 0 }
    };
}

void world_components_free(world_components* wc) {
    cecs_arena_free(&wc->components_arena);
    cecs_arena_free(&wc->storages_arena);
    wc->component_storages = (cecs_paged_sparse_set){ 0 };
    wc->component_sizes = (cecs_paged_sparse_set){ 0 };
    wc->discard = (component_discard){ 0 };
}

size_t world_components_get_component_storage_count(const world_components* wc) {
    return cecs_paged_sparse_set_count_of_size(&wc->component_storages, sizeof(component_storage));
}

optional_component_size world_components_get_component_size(const world_components* wc, component_id component_id) {
    return CECS_OPTION_MAP_REFERENCE_STRUCT(
        cecs_optional_element,
        CECS_PAGED_SPARSE_SET_GET(size_t, &wc->component_sizes, (size_t)component_id),
        optional_component_size
    );
}

size_t world_components_get_component_size_unchecked(const world_components* wc, component_id component_id) {
    return *CECS_OPTION_GET(optional_component_size, world_components_get_component_size(wc, component_id));
}

optional_component_storage world_components_get_component_storage(const world_components* wc, component_id component_id) {
    return CECS_OPTION_MAP_REFERENCE_STRUCT(
        cecs_optional_element,
        CECS_PAGED_SPARSE_SET_GET(component_storage, &wc->component_storages, (size_t)component_id),
        optional_component_storage
    );
}

component_storage* world_components_get_component_storage_unchecked(const world_components* wc, component_id component_id) {
    return CECS_OPTION_GET(optional_component_storage, world_components_get_component_storage(wc, component_id));
}

bool world_components_has_storage(const world_components* wc, component_id component_id) {
    return cecs_paged_sparse_set_contains(&wc->component_storages, (size_t)component_id);
}

component_storage component_storage_descriptor_build(component_storage_descriptor descriptor, world_components* wc, size_t component_size) {
    if (component_size == 0 && descriptor.is_size_known) {
        return component_storage_create_unit(&wc->components_arena);
    }

    if (!descriptor.is_size_known)
        assert(component_size == 0 && "component_size must be zero if is_size_known is false");

    if (CECS_OPTION_IS_SOME(indirect_component_id, descriptor.indirect_component_id)) {
        component_id other_id = CECS_OPTION_GET(indirect_component_id, descriptor.indirect_component_id);
        if (world_components_has_storage(wc, other_id)) {
            return component_storage_create_indirect(
                &wc->components_arena,
                world_components_get_component_storage_unchecked(wc, other_id)
            );
        }
        else {
            component_storage other_storage = component_storage_descriptor_build((component_storage_descriptor) {
                .is_size_known = false,
                    .capacity = descriptor.capacity,
                    .indirect_component_id = CECS_OPTION_CREATE_NONE(indirect_component_id)
            }, wc, 0);
            return component_storage_create_indirect(
                &wc->components_arena,
                CECS_PAGED_SPARSE_SET_SET(component_storage, &wc->component_storages, &wc->storages_arena, (size_t)other_id, &other_storage)
            );
        }
    }
    else {
        return component_storage_create_sparse(&wc->components_arena, descriptor.capacity, component_size);
    }
}

optional_component world_components_set_component(world_components* wc, entity_id entity_id, component_id component_id, void* component, size_t size, component_storage_descriptor additional_storage_descriptor) {
    if (size > wc->discard.size) {
        wc->discard.handle = cecs_arena_realloc(&wc->components_arena, wc->discard.handle, wc->discard.size, size);
        wc->discard.size = size;
    }
    wc->checksum = world_components_checksum_add(wc->checksum, component_id);
    optional_component_size stored_size = world_components_get_component_size(wc, component_id);
    if (CECS_OPTION_IS_SOME(optional_component_size, stored_size)) {
        assert((*CECS_OPTION_GET(optional_component_size, stored_size) == size) && "Component size mismatch");
    }
    else {
        stored_size = CECS_OPTION_CREATE_SOME_STRUCT(
            optional_component_size,
            CECS_PAGED_SPARSE_SET_SET(size_t, &wc->component_sizes, &wc->storages_arena, (size_t)component_id, &size)
        );
    }

    optional_component_storage storage = world_components_get_component_storage(wc, component_id);
    if (CECS_OPTION_IS_SOME(optional_component_storage, storage)) {
        return component_storage_set(
            CECS_OPTION_GET(optional_component_storage, storage),
            &wc->components_arena,
            entity_id,
            component,
            size
        );
    }
    else {
        component_storage new_storage = component_storage_descriptor_build(additional_storage_descriptor, wc, size);
        return component_storage_set(
            CECS_PAGED_SPARSE_SET_SET(component_storage, &wc->component_storages, &wc->storages_arena, (size_t)component_id, &new_storage),
            &wc->components_arena,
            entity_id,
            component,
            size
        );
    }
}

void* world_components_set_component_unchecked(world_components* wc, entity_id entity_id, component_id component_id, void* component, size_t size, component_storage_descriptor additional_storage_descriptor) {
    return CECS_OPTION_GET(optional_component, world_components_set_component(wc, entity_id, component_id, component, size, additional_storage_descriptor));
}

bool world_components_has_component(const world_components* wc, entity_id entity_id, component_id component_id) {
    optional_component_size stored_size = world_components_get_component_size(wc, component_id);
    optional_component_storage storage = world_components_get_component_storage(wc, component_id);

    return CECS_OPTION_IS_SOME(optional_component_size, stored_size)
        && component_storage_has(CECS_OPTION_GET(optional_component_storage, storage), entity_id);
}

optional_component world_components_get_component(const world_components* wc, entity_id entity_id, component_id component_id) {
    optional_component_size stored_size = world_components_get_component_size(wc, component_id);
    optional_component_storage storage = world_components_get_component_storage(wc, component_id);
    if (!CECS_OPTION_IS_SOME(optional_component_size, stored_size)) {
        return CECS_OPTION_CREATE_NONE_STRUCT(optional_component);
    }
    else if (component_storage_has(CECS_OPTION_GET(optional_component_storage, storage), entity_id)) {
        return component_storage_get(
            CECS_OPTION_GET(optional_component_storage, storage),
            entity_id,
            *CECS_OPTION_GET(optional_component_size, stored_size)
        );
    }
    else {
        return CECS_OPTION_CREATE_NONE_STRUCT(optional_component);
    }
}

void* world_components_get_component_unchecked(const world_components* wc, entity_id entity_id, component_id component_id) {
    return CECS_OPTION_GET(optional_component, world_components_get_component(wc, entity_id, component_id));
}

bool world_components_remove_component(world_components* wc, entity_id entity_id, component_id component_id, void* out_removed_component) {
    wc->checksum = world_components_checksum_remove(wc->checksum, component_id);
    optional_component_size stored_size = world_components_get_component_size(wc, component_id);
    if (CECS_OPTION_IS_NONE(optional_component_size, stored_size)) {
        assert(out_removed_component == NULL && "out_removed_component must be NULL if component size is unknown");
        return false;
    }
    else {
        return component_storage_remove(
            CECS_OPTION_GET(optional_component_storage, world_components_get_component_storage(wc, component_id)),
            &wc->components_arena,
            entity_id,
            out_removed_component,
            *CECS_OPTION_GET(optional_component_size, stored_size)
        );
    }
}

world_components_iterator world_components_iterator_create(const world_components* components) {
    return (world_components_iterator) {
        .components = components,
            .storage_raw_index = 0
    };
}

bool world_components_iterator_done(const world_components_iterator* it) {
    return it->storage_raw_index >= world_components_get_component_storage_count(it->components);
}

size_t world_components_iterator_next(world_components_iterator* it) {
    return ++it->storage_raw_index;
}

sized_component_storage world_components_iterator_current(const world_components_iterator* it) {
    return (sized_component_storage) {
        .storage = ((component_storage*)cecs_paged_sparse_set_data(&it->components->component_storages)) + it->storage_raw_index,
            .component_size = ((size_t*)cecs_paged_sparse_set_data(&it->components->component_sizes))[it->storage_raw_index],
            .component_id = ((size_t*)cecs_paged_sparse_set_keys(&it->components->component_storages))[it->storage_raw_index]
    };
}

world_components_entity_iterator world_components_entity_iterator_create(const world_components* components, entity_id entity_id) {
    return (world_components_entity_iterator) {
        .it = world_components_iterator_create(components),
            .entity_id = entity_id
    };
}

bool world_components_entity_iterator_done(const world_components_entity_iterator* it) {
    return world_components_iterator_done(&it->it);
}

size_t world_components_entity_iterator_next(world_components_entity_iterator* it) {
    do {
        world_components_iterator_next(&it->it);
    } while (
        !world_components_iterator_done(&it->it)
        && !component_storage_has(
            ((component_storage*)cecs_paged_sparse_set_data(&it->it.components->component_storages)) + it->it.storage_raw_index,
            it->entity_id
        )
        );

    return it->it.storage_raw_index;
}

sized_component_storage world_components_entity_iterator_current(const world_components_entity_iterator* it) {
    return world_components_iterator_current(&it->it);
}
