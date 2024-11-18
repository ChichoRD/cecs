#include "component_iterator.h"

bool component_iterator_descriptor_copy_component_bitsets(const world_components* world_components, components_type_info components_type_info, hibitset bitsets_destination[]) {
    bool intersection_empty = false;
    for (size_t i = 0; i < components_type_info.component_count; i++) {
        if (!world_components_has_storage(world_components, components_type_info.component_ids[i])) {
            intersection_empty = true;
            bitsets_destination[i] = hibitset_empty();
        }
        else {
            component_storage* storage = world_components_get_component_storage_unchecked(
                world_components,
                components_type_info.component_ids[i]
            );
            bitsets_destination[i] = storage->entity_bitset;
        }
    }
    return !intersection_empty;
}

size_t component_iterator_descriptor_append_sized_component_ids(const world_components* world_components, arena* iterator_temporary_arena, components_type_info components_type_info, list* sized_component_ids) {
    size_t sized_count = 0;
    for (size_t i = 0; i < components_type_info.component_count; i++) {
        if (
            world_components_has_storage(world_components, components_type_info.component_ids[i])
            && !component_storage_info(world_components_get_component_storage_unchecked(
                world_components,
                components_type_info.component_ids[i]
            )).is_unit_type_storage
            ) {
            LIST_ADD(component_id, sized_component_ids, iterator_temporary_arena, &components_type_info.component_ids[i]);
            ++sized_count;
        }
    }

    return sized_count;
}

hibitset component_iterator_descriptor_get_search_groups_bitset(const world_components* world_components, arena* iterator_temporary_arena, components_search_groups search_groups, size_t max_component_count) {
    hibitset* bitsets = calloc(max_component_count, sizeof(hibitset));
    hibitset entities_bitset = hibitset_create(iterator_temporary_arena);
    bool has_empty_intersection = false;
    for (size_t i = 0; i < search_groups.group_count; i++) {
        components_type_info info = TAGGED_UNION_GET_UNCHECKED(COMPONENTS_ALL_ID, search_groups.groups[i]);
        assert(max_component_count >= info.component_count && "component count is larger than max component count");

        bool component_bitsets_empty = !component_iterator_descriptor_copy_component_bitsets(
            world_components,
            info,
            bitsets
        );
        bool entities_bitset_empty = exclusive_range_is_empty(hibitset_bit_range(&entities_bitset));

        TAGGED_UNION_MATCH(search_groups.groups[i]) {
            case TAGGED_UNION_VARIANT(COMPONENTS_ALL_ID, components_search_group): {
                if (has_empty_intersection)
                    break;

                if (component_bitsets_empty) {
                    hibitset_unset_all(&entities_bitset);
                }
                else if (entities_bitset_empty) {
                    entities_bitset = info.component_count == 1
                        ? bitsets[0]
                        : hibitset_intersection(bitsets, info.component_count, iterator_temporary_arena);
                }
                else {
                    hibitset_intersect(&entities_bitset, bitsets, info.component_count, iterator_temporary_arena);
                }

                has_empty_intersection |= exclusive_range_is_empty(hibitset_bit_range(&entities_bitset));
                break;
            }

            case TAGGED_UNION_VARIANT(COMPONENTS_ANY_ID, components_search_group): {
                if (component_bitsets_empty)
                    break;

                if (entities_bitset_empty) {
                    entities_bitset = info.component_count == 1
                        ? bitsets[0]
                        : hibitset_union(bitsets, info.component_count, iterator_temporary_arena);
                }
                else {
                    hibitset_join(&entities_bitset, bitsets, info.component_count, iterator_temporary_arena);
                }
                break;
            }

            case TAGGED_UNION_VARIANT(COMPONENTS_NONE_ID, components_search_group): {
                if (component_bitsets_empty || entities_bitset_empty) {
                    break;
                }
                hibitset_subtract(&entities_bitset, bitsets, info.component_count, iterator_temporary_arena);
                break;
            }

            case TAGGED_UNION_VARIANT(COMPONENTS_OR_ALL_ID, components_search_group): {
                if (component_bitsets_empty)
                    break;

                if (entities_bitset_empty) {
                    entities_bitset = info.component_count == 1
                        ? bitsets[0]
                        : hibitset_intersection(bitsets, info.component_count, iterator_temporary_arena);
                }
                else {
                    hibitset intersection = info.component_count == 1
                        ? bitsets[0]
                        : hibitset_intersection(bitsets, info.component_count, iterator_temporary_arena);
                    hibitset_join(&entities_bitset, &intersection, 1, iterator_temporary_arena);
                }
                break;
            }

            case TAGGED_UNION_VARIANT(COMPONENTS_AND_ANY_ID, components_search_group): {
                if (has_empty_intersection)
                    break;

                if (component_bitsets_empty) {
                    hibitset_unset_all(&entities_bitset);
                }
                else if (entities_bitset_empty) {
                    entities_bitset = info.component_count == 1
                        ? bitsets[0]
                        : hibitset_union(bitsets, info.component_count, iterator_temporary_arena);
                }
                else {
                    hibitset union_ = info.component_count == 1
                        ? bitsets[0]
                        : hibitset_union(bitsets, info.component_count, iterator_temporary_arena);
                    hibitset_intersect(&entities_bitset, &union_, 1, iterator_temporary_arena);
                }

                has_empty_intersection |= exclusive_range_is_empty(hibitset_bit_range(&entities_bitset));
                break;
            }

            default: {
                assert(false && "unreachable: invalid component search group variant");
                exit(EXIT_FAILURE);
            }
        }
    }
    free(bitsets);

    return entities_bitset;
}

component_iterator_descriptor component_iterator_descriptor_create(const world_components* world_components, arena* iterator_temporary_arena, components_search_groups search_groups) {
    list sized_component_ids = LIST_CREATE_WITH_CAPACITY(component_id, iterator_temporary_arena, search_groups.group_count);
    size_t sized_component_count = 0;
    size_t component_count = 0;
    size_t max_component_count = 0;
    for (size_t i = 0; i < search_groups.group_count; i++) {
        components_type_info info = TAGGED_UNION_GET_UNCHECKED(COMPONENTS_ALL_ID, search_groups.groups[i]);
        component_count += info.component_count;
        max_component_count = max(max_component_count, info.component_count);

        if (!TAGGED_UNION_IS(COMPONENTS_NONE_ID, components_search_group, search_groups.groups[i])) {
            sized_component_count += component_iterator_descriptor_append_sized_component_ids(
                world_components,
                iterator_temporary_arena,
                info,
                &sized_component_ids
            );
        }
    }

    hibitset entities_bitset = component_iterator_descriptor_get_search_groups_bitset(
        world_components,
        iterator_temporary_arena,
        search_groups,
        max_component_count
    );
    component_iterator_descriptor descriptor = {
        .checksum = world_components->checksum,
        .world_components = world_components,
        .entities_bitset = entities_bitset,
        .components_type_info = (struct components_type_info){
            .component_ids = sized_component_ids.elements,
            .component_count = sized_component_count
    }
    };

    return descriptor;
}

raw_iteration_handle_reference component_iterator_descriptor_allocate_handle(const component_iterator_descriptor descriptor) {
    return malloc(component_iteration_handle_size(descriptor.components_type_info));
}

raw_iteration_handle_reference component_iterator_descriptor_allocate_handle_in(arena* a, const component_iterator_descriptor descriptor) {
    return arena_alloc(a, component_iteration_handle_size(descriptor.components_type_info));
}

component_iterator component_iterator_create(component_iterator_descriptor descriptor) {
    assert(descriptor.checksum == descriptor.world_components->checksum
        && "Component iterator descriptor is invalid or outdated, please create a new one");
    hibitset_iterator iterator = hibitset_iterator_create_owned_at_first(descriptor.entities_bitset);
    if (!hibitset_iterator_current_is_set(&iterator)) {
        hibitset_iterator_next_set(&iterator);
    }
    return (component_iterator) {
        .descriptor = descriptor,
            .entities_iterator = iterator,
            .entity_range = hibitset_bit_range(&descriptor.entities_bitset)
    };
}

component_iterator component_iterator_create_ranged(component_iterator_descriptor descriptor, entity_id_range range) {
    assert(descriptor.checksum == descriptor.world_components->checksum
        && "Component iterator descriptor is invalid or outdated, please create a new one");
    hibitset_iterator iterator = hibitset_iterator_create_owned_at_first(descriptor.entities_bitset);
    if (!hibitset_iterator_current_is_set(&iterator)) {
        hibitset_iterator_next_set(&iterator);
    }
    return (component_iterator) {
        .descriptor = descriptor,
            .entities_iterator = iterator,
            .entity_range = range
    };
}

bool component_iterator_done(const component_iterator* it) {
    return !exclusive_range_contains(it->entity_range, it->entities_iterator.current_bit_index)
        || hibitset_iterator_done(&it->entities_iterator);
}

entity_id component_iterator_current(const component_iterator* it, raw_iteration_handle_reference out_iteration_handle) {
    entity_id* handle = (entity_id*)out_iteration_handle;
    *handle = it->entities_iterator.current_bit_index;

    for (size_t i = 0; i < it->descriptor.component_count; i++) {
        component_storage* storage = world_components_get_component_storage_unchecked(
            it->descriptor.world_components,
            it->descriptor.component_ids[i]
        );

        ((raw_component_reference*)(handle + 1))[i] = component_storage_get_or_null(
            storage,
            it->entities_iterator.current_bit_index,
            world_components_get_component_size_unchecked(it->descriptor.world_components, it->descriptor.component_ids[i])
        );
    }
    return it->entities_iterator.current_bit_index;
}

size_t component_iterator_next(component_iterator* it) {
    return hibitset_iterator_next_set(&it->entities_iterator);
}
