
#include <stdlib.h>

#include "cecs_component_iterator.h"


static bool cecs_component_iterator_copy_component_bitset_or_empty(
    const cecs_world_components* world_components,
    const cecs_component_id id,
    cecs_hibitset *destination_bitset
) {
    if (!cecs_world_components_has_storage(world_components, id)) {
        *destination_bitset = cecs_hibitset_empty();
        return false;
    } else {
        *destination_bitset = cecs_world_components_get_component_storage_expect(
            world_components,
            id
        )->storage.entity_bitset;
        return true;
    }
}

static cecs_hibitset cecs_component_iterator_join_iteration_groups(
    const cecs_world_components* world_components,
    cecs_arena* iterator_temporary_arena,
    const cecs_component_iteration_group groups[],
    const size_t group_count,
    const size_t total_component_count
) {
    cecs_hibitset *source_bitsets = cecs_arena_alloc(iterator_temporary_arena, total_component_count * sizeof(cecs_hibitset));
    cecs_hibitset result_bitset = cecs_hibitset_create(iterator_temporary_arena);

    bool has_empty_intersection = false;
    for (size_t i = 0; i < group_count; i++) {
        const cecs_component_iteration_group group = groups[i];
        assert(total_component_count >= group.component_count && "group component count is larger than total component count");

        bool any_source_set_empty = false;
        for (size_t j = 0; j < group.component_count; j++) {
            any_source_set_empty |= !cecs_component_iterator_copy_component_bitset_or_empty(
                world_components,
                group.components[j],
                &source_bitsets[j]
            );
        }
        const bool result_set_empty = cecs_exclusive_range_is_empty(cecs_hibitset_bit_range(&result_bitset));
        
        switch (group.search_mode) {
        case cecs_component_group_search_all: {
            if (has_empty_intersection)
                break;

            if (any_source_set_empty) {
                cecs_hibitset_unset_all(&result_bitset);
            } else if (result_set_empty) {
                result_bitset = group.component_count == 1
                    ? source_bitsets[0]
                    : cecs_hibitset_intersection(source_bitsets, group.component_count, iterator_temporary_arena);
            } else {
                cecs_hibitset_intersect(&result_bitset, source_bitsets, group.component_count, iterator_temporary_arena);
            }

            has_empty_intersection |= cecs_exclusive_range_is_empty(cecs_hibitset_bit_range(&result_bitset));
            break;
        }

        case cecs_component_group_search_any: {
            if (any_source_set_empty)
                break;

            if (result_set_empty) {
                result_bitset = group.component_count == 1
                    ? source_bitsets[0]
                    : cecs_hibitset_union(source_bitsets, group.component_count, iterator_temporary_arena);
            } else {
                cecs_hibitset_join(&result_bitset, source_bitsets, group.component_count, iterator_temporary_arena);
            }
            break;
        }

        case cecs_component_group_search_none: {
            if (any_source_set_empty || result_set_empty) {
                break;
            }
            cecs_hibitset_subtract(&result_bitset, source_bitsets, group.component_count, iterator_temporary_arena);
            break;
        }

        case cecs_component_group_search_or_all: {
            if (any_source_set_empty)
                break;

            if (result_set_empty) {
                result_bitset = group.component_count == 1
                    ? source_bitsets[0]
                    : cecs_hibitset_intersection(source_bitsets, group.component_count, iterator_temporary_arena);
            } else {
                cecs_hibitset intersection = group.component_count == 1
                    ? source_bitsets[0]
                    : cecs_hibitset_intersection(source_bitsets, group.component_count, iterator_temporary_arena);
                cecs_hibitset_join(&result_bitset, &intersection, 1, iterator_temporary_arena);
            }
            break;
        }

        case cecs_component_group_search_and_any: {
            if (has_empty_intersection)
                break;

            if (any_source_set_empty) {
                cecs_hibitset_unset_all(&result_bitset);
            } else if (result_set_empty) {
                result_bitset = group.component_count == 1
                    ? source_bitsets[0]
                    : cecs_hibitset_union(source_bitsets, group.component_count, iterator_temporary_arena);
            } else {
                cecs_hibitset union_ = group.component_count == 1
                    ? source_bitsets[0]
                    : cecs_hibitset_union(source_bitsets, group.component_count, iterator_temporary_arena);
                cecs_hibitset_intersect(&result_bitset, &union_, 1, iterator_temporary_arena);
            }

            has_empty_intersection |= cecs_exclusive_range_is_empty(cecs_hibitset_bit_range(&result_bitset));
            break;
        }

        default: {
            assert(false && "unreachable: invalid component search group variant");
            exit(EXIT_FAILURE);
        }
        }
    }
    return result_bitset;
}

static cecs_component_iterator_descriptor cecs_component_iterator_descriptor_filter_sized(
    const cecs_world_components* world_components,
    cecs_arena *iterator_temporary_arena,
    cecs_component_iterator_descriptor *in_out_reordered_source,
    size_t *out_component_count,
    size_t *out_sized_component_count
) {
    cecs_component_iterator_descriptor descriptor = {
        .entity_range = in_out_reordered_source->entity_range,
        .groups = cecs_arena_alloc(iterator_temporary_arena, in_out_reordered_source->group_count * sizeof(cecs_component_iteration_group)),
        .group_count = in_out_reordered_source->group_count,
    };
    
    size_t component_count = 0;
    size_t sized_component_count = 0;
    for (size_t i = 0; i < descriptor.group_count; i++) {
        descriptor.groups[i] = in_out_reordered_source->groups[i];
        
        const size_t group_component_count = in_out_reordered_source->groups[i].component_count;
        size_t first_tag_index = group_component_count;
        for (size_t j = 0; j < group_component_count; j++) {
            if (cecs_world_components_has_storage(world_components, descriptor.groups[i].components[j])) {
                ++component_count;
                const size_t component_size = cecs_world_components_get_component_storage_expect(
                    world_components,
                    descriptor.groups[i].components[j]
                )->component_size;

                if (component_size == 0 && first_tag_index == group_component_count) {
                    first_tag_index = j;
                } else if (component_size != 0) {
                    ++sized_component_count;
                    if (first_tag_index < group_component_count) {
                        const cecs_component_id temp = descriptor.groups[i].components[first_tag_index];
                        descriptor.groups[i].components[first_tag_index] = descriptor.groups[i].components[j];
                        descriptor.groups[i].components[j] = temp;
                        first_tag_index = j;
                    }
                }
                // c c t c t c t c
            }
        }
        descriptor.groups[i].component_count = first_tag_index;
    }
    *out_component_count = component_count;
    *out_sized_component_count = sized_component_count;
    return descriptor;
}

cecs_component_iterator cecs_component_iterator_create(
    const cecs_component_iterator_descriptor descriptor, 
    cecs_world_components *world_components,
    cecs_arena *iterator_temporary_arena
) {
    size_t component_count;
    size_t sized_component_count;
    cecs_component_iterator_descriptor descriptor_filtered = cecs_component_iterator_descriptor_filter_sized(
        world_components,
        iterator_temporary_arena,
        &descriptor,
        &component_count,
        &sized_component_count
    );

    cecs_hibitset set = cecs_component_iterator_join_iteration_groups(
        world_components,
        iterator_temporary_arena,
        descriptor.groups,
        descriptor.group_count,
        component_count
    );
    return (cecs_component_iterator) {
        .descriptor = { .groupped = descriptor_filtered },
        .entities_iterator = cecs_hibitset_iterator_create_owned_at(set, descriptor.entity_range.start),
        .creation_checksum = world_components->checksum,
        .world_components = world_components,
        .component_count = sized_component_count,
        .flags = cecs_component_iterator_status_none
    };
}

bool cecs_component_iterator_can_begin_iter(const cecs_component_iterator *it) {
    return it->creation_checksum == it->world_components->checksum;
}

static size_t cecs_component_iteration_group_ensure_access_and_collect(
    const cecs_component_iteration_group group,
    cecs_world_components *world_components,
    cecs_component_id components_destination[]
) {
    const cecs_component_access group_access = group.access;
    cecs_component_storage_status_flags access_flags;
    switch (group_access) {
    case cecs_component_access_inmmutable:
        access_flags = cecs_component_storage_status_reading;
        break;
    case cecs_component_access_mutable:
        access_flags =
            cecs_component_storage_status_reading | cecs_component_storage_status_writing | cecs_component_storage_status_dirty;
        break;
    case cecs_component_access_ignore:
        access_flags = cecs_component_storage_status_none;
        break;
    default: {
        assert(false && "unreachable: invalid component access mode");
        exit(EXIT_FAILURE);
    }
    }
    size_t count = 0;
    for (size_t i = 0; i < group.component_count; i++) {
        if (cecs_world_components_has_storage(world_components, group.components[i])) {  
            cecs_component_storage *storage = &cecs_world_components_get_component_storage_expect(
                world_components,
                group.components[i]
            )->storage;
    
            if (storage->status & cecs_component_storage_status_writing) {
                if (group_access != cecs_component_access_ignore) {
                    assert(false && "error: user requested reference access on a component that is being written to");
                    exit(EXIT_FAILURE);
                }
            } else if (storage->status & cecs_component_storage_status_reading) {
                if (group_access == cecs_component_access_mutable) {
                    assert(false && "error: user requested mutable access on a component that is being read from");
                    exit(EXIT_FAILURE);
                }
            }
    
            storage->status |= access_flags;
            components_destination[count] = group.components[i];
            ++count;
        }
    }
    return count;
}

cecs_entity_id cecs_component_iterator_begin_iter(cecs_component_iterator *it, cecs_arena *iterator_temporary_arena) {
    assert(
        cecs_component_iterator_can_begin_iter(it)
        && "error: component iterator is not in a state where it can begin iteration"
    );
    it->flags |= cecs_component_iterator_status_iter_checked;

    const cecs_component_iterator_descriptor_flat flat_descriptor = {
        .entity_range = it->descriptor.groupped.entity_range,
        .components = cecs_arena_alloc(iterator_temporary_arena, it->component_count * sizeof(cecs_component_id))
    };
    size_t component_count = 0;
    for (size_t i = 0; i < it->descriptor.groupped.group_count; i++) {
        const size_t collected = cecs_component_iteration_group_ensure_access_and_collect(
            it->descriptor.groupped.groups[i],
            it->world_components,
            flat_descriptor.components + component_count
        );
        component_count += collected;
    }
    assert(component_count == it->component_count && "fatal error: component count mismatch");
    it->descriptor.flattened = flat_descriptor;

    if (!cecs_hibitset_iterator_done(&it->entities_iterator)
        && !cecs_hibitset_iterator_current_is_set(&it->entities_iterator)) {
        cecs_hibitset_iterator_next_set(&it->entities_iterator);
    }
    return it->entities_iterator.current_bit_index;
}

cecs_component_iterator cecs_component_iterator_create_and_begin_iter(
    cecs_component_iterator_descriptor descriptor,
    cecs_world_components *world_components,
    cecs_arena *iterator_temporary_arena
) {
    cecs_component_iterator it = cecs_component_iterator_create(descriptor, world_components, iterator_temporary_arena);
    cecs_component_iterator_begin_iter(&it, iterator_temporary_arena);
    return it;
}

bool cecs_component_iterator_done(const cecs_component_iterator* it) {
    return !cecs_exclusive_range_contains(it->descriptor.flattened.entity_range, it->entities_iterator.current_bit_index)
        || cecs_hibitset_iterator_done(&it->entities_iterator);
}

cecs_entity_id cecs_component_iterator_current(const cecs_component_iterator* it, void *out_component_handles[]) {
    for (size_t i = 0; i < it->component_count; i++) {
        cecs_sized_component_storage* storage = cecs_world_components_get_component_storage_expect(
            it->world_components,
            it->descriptor.flattened.components[i]
        );
        out_component_handles[i] = cecs_component_storage_get_or_null(
            &storage->storage,
            it->entities_iterator.current_bit_index,
            storage->component_size
        );
    }
    return it->entities_iterator.current_bit_index;
}

size_t cecs_component_iterator_next(cecs_component_iterator* it) {
    return cecs_hibitset_iterator_next_set(&it->entities_iterator);
}

void cecs_component_iterator_end_iter(cecs_component_iterator *it) {
    assert(
        (it->flags & cecs_component_iterator_status_iter_checked)
        && "error: component iterator is not in a state where it can end iteration"
    );
    it->flags &= ~cecs_component_iterator_status_iter_checked;

    for (size_t i = 0; i < it->component_count; i++) {
        cecs_component_storage *storage = &cecs_world_components_get_component_storage_expect(
            it->world_components,
            it->descriptor.flattened.components[i]
        )->storage;
        storage->status &= ~(
            cecs_component_storage_status_reading | cecs_component_storage_status_writing
        );
    }
}
