
#include <stdlib.h>

#include "cecs_component_iterator.h"

bool cecs_component_iterator_descriptor_copy_component_bitsets(const cecs_world_components* world_components, cecs_components_type_info components_type_info, cecs_hibitset bitsets_destination[]) {
    bool intersection_empty = false;
    for (size_t i = 0; i < components_type_info.component_count; i++) {
        if (!cecs_world_components_has_storage(world_components, components_type_info.component_ids[i])) {
            intersection_empty = true;
            bitsets_destination[i] = cecs_hibitset_empty();
        }
        else {
            cecs_component_storage* storage = &cecs_world_components_get_component_storage_expect(
                world_components,
                components_type_info.component_ids[i]
            )->storage;
            bitsets_destination[i] = storage->entity_bitset;
        }
    }
    return !intersection_empty;
}

size_t cecs_component_iterator_descriptor_append_sized_component_ids(const cecs_world_components* world_components, cecs_arena* iterator_temporary_arena, cecs_components_type_info components_type_info, cecs_dynamic_array* sized_component_ids) {
    size_t sized_count = 0;
    for (size_t i = 0; i < components_type_info.component_count; i++) {
        if (
            cecs_world_components_has_storage(world_components, components_type_info.component_ids[i])
            && !cecs_component_storage_info(&cecs_world_components_get_component_storage_expect(
                world_components,
                components_type_info.component_ids[i]
            )->storage).is_unit_type_storage
            ) {
            CECS_DYNAMIC_ARRAY_ADD(cecs_component_id, sized_component_ids, iterator_temporary_arena, &components_type_info.component_ids[i]);
            ++sized_count;
        }
    }

    return sized_count;
}

cecs_hibitset cecs_component_iterator_descriptor_get_search_groups_bitset(const cecs_world_components* world_components, cecs_arena* iterator_temporary_arena, cecs_components_search_groups search_groups, size_t max_component_count) {
    cecs_hibitset* bitsets = calloc(max_component_count, sizeof(cecs_hibitset));
    cecs_hibitset entities_bitset = cecs_hibitset_create(iterator_temporary_arena);
    bool has_empty_intersection = false;
    for (size_t i = 0; i < search_groups.group_count; i++) {
        cecs_components_type_info info = CECS_UNION_GET_UNCHECKED(CECS_COMPONENTS_ALL_ID, search_groups.groups[i]);
        assert(max_component_count >= info.component_count && "component count is larger than max component count");

        bool component_bitsets_empty = !cecs_component_iterator_descriptor_copy_component_bitsets(
            world_components,
            info,
            bitsets
        );
        bool entities_bitset_empty = cecs_exclusive_range_is_empty(cecs_hibitset_bit_range(&entities_bitset));

        CECS_UNION_MATCH(search_groups.groups[i]) {
            case CECS_UNION_VARIANT(CECS_COMPONENTS_ALL_ID, cecs_components_search_group): {
                if (has_empty_intersection)
                    break;

                if (component_bitsets_empty) {
                    cecs_hibitset_unset_all(&entities_bitset);
                }
                else if (entities_bitset_empty) {
                    entities_bitset = info.component_count == 1
                        ? bitsets[0]
                        : cecs_hibitset_intersection(bitsets, info.component_count, iterator_temporary_arena);
                }
                else {
                    cecs_hibitset_intersect(&entities_bitset, bitsets, info.component_count, iterator_temporary_arena);
                }

                has_empty_intersection |= cecs_exclusive_range_is_empty(cecs_hibitset_bit_range(&entities_bitset));
                break;
            }

            case CECS_UNION_VARIANT(CECS_COMPONENTS_ANY_ID, cecs_components_search_group): {
                if (component_bitsets_empty)
                    break;

                if (entities_bitset_empty) {
                    entities_bitset = info.component_count == 1
                        ? bitsets[0]
                        : cecs_hibitset_union(bitsets, info.component_count, iterator_temporary_arena);
                }
                else {
                    cecs_hibitset_join(&entities_bitset, bitsets, info.component_count, iterator_temporary_arena);
                }
                break;
            }

            case CECS_UNION_VARIANT(CECS_COMPONENTS_NONE_ID, cecs_components_search_group): {
                if (component_bitsets_empty || entities_bitset_empty) {
                    break;
                }
                cecs_hibitset_subtract(&entities_bitset, bitsets, info.component_count, iterator_temporary_arena);
                break;
            }

            case CECS_UNION_VARIANT(CECS_COMPONENTS_OR_ALL_ID, cecs_components_search_group): {
                if (component_bitsets_empty)
                    break;

                if (entities_bitset_empty) {
                    entities_bitset = info.component_count == 1
                        ? bitsets[0]
                        : cecs_hibitset_intersection(bitsets, info.component_count, iterator_temporary_arena);
                }
                else {
                    cecs_hibitset intersection = info.component_count == 1
                        ? bitsets[0]
                        : cecs_hibitset_intersection(bitsets, info.component_count, iterator_temporary_arena);
                    cecs_hibitset_join(&entities_bitset, &intersection, 1, iterator_temporary_arena);
                }
                break;
            }

            case CECS_UNION_VARIANT(CECS_COMPONENTS_AND_ANY_ID, cecs_components_search_group): {
                if (has_empty_intersection)
                    break;

                if (component_bitsets_empty) {
                    cecs_hibitset_unset_all(&entities_bitset);
                }
                else if (entities_bitset_empty) {
                    entities_bitset = info.component_count == 1
                        ? bitsets[0]
                        : cecs_hibitset_union(bitsets, info.component_count, iterator_temporary_arena);
                }
                else {
                    cecs_hibitset union_ = info.component_count == 1
                        ? bitsets[0]
                        : cecs_hibitset_union(bitsets, info.component_count, iterator_temporary_arena);
                    cecs_hibitset_intersect(&entities_bitset, &union_, 1, iterator_temporary_arena);
                }

                has_empty_intersection |= cecs_exclusive_range_is_empty(cecs_hibitset_bit_range(&entities_bitset));
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

cecs_component_iterator_descriptor cecs_component_iterator_descriptor_create(const cecs_world_components* world_components, cecs_arena* iterator_temporary_arena, cecs_components_search_groups search_groups) {
    cecs_dynamic_array sized_component_ids = CECS_DYNAMIC_ARRAY_CREATE_WITH_CAPACITY(cecs_component_id, iterator_temporary_arena, search_groups.group_count);
    size_t sized_component_count = 0;
    size_t component_count = 0;
    (void)component_count;
    size_t max_component_count = 0;
    for (size_t i = 0; i < search_groups.group_count; i++) {
        cecs_components_type_info info = CECS_UNION_GET_UNCHECKED(CECS_COMPONENTS_ALL_ID, search_groups.groups[i]);
        component_count += info.component_count;
        max_component_count = max(max_component_count, info.component_count);

        if (!CECS_UNION_IS(CECS_COMPONENTS_NONE_ID, cecs_components_search_group, search_groups.groups[i])) {
            sized_component_count += cecs_component_iterator_descriptor_append_sized_component_ids(
                world_components,
                iterator_temporary_arena,
                info,
                &sized_component_ids
            );
        }
    }

    cecs_hibitset entities_bitset = cecs_component_iterator_descriptor_get_search_groups_bitset(
        world_components,
        iterator_temporary_arena,
        search_groups,
        max_component_count
    );
    cecs_component_iterator_descriptor descriptor = {
        .checksum = world_components->checksum,
        .world_components = world_components,
        .entities_bitset = entities_bitset,
        .components_type_info = (struct cecs_components_type_info){
            .component_ids = sized_component_ids.elements,
            .component_count = sized_component_count
    }
    };

    return descriptor;
}

raw_iteration_handle_reference cecs_component_iterator_descriptor_allocate_handle(const cecs_component_iterator_descriptor descriptor) {
    return malloc(cecs_component_iteration_handle_size(descriptor.components_type_info));
}

raw_iteration_handle_reference cecs_component_iterator_descriptor_allocate_handle_in(cecs_arena* a, const cecs_component_iterator_descriptor descriptor) {
    return cecs_arena_alloc(a, cecs_component_iteration_handle_size(descriptor.components_type_info));
}

cecs_component_iterator cecs_component_iterator_create(cecs_component_iterator_descriptor descriptor) {
    assert(descriptor.checksum == descriptor.world_components->checksum
        && "Component iterator descriptor is invalid or outdated, please create a new one");
    cecs_hibitset_iterator iterator = cecs_hibitset_iterator_create_owned_at_first(descriptor.entities_bitset);
    if (!cecs_hibitset_iterator_current_is_set(&iterator)) {
        cecs_hibitset_iterator_next_set(&iterator);
    }
    return (cecs_component_iterator) {
        .descriptor = descriptor,
            .entities_iterator = iterator,
            .entity_range = cecs_hibitset_bit_range(&descriptor.entities_bitset)
    };
}

cecs_component_iterator cecs_component_iterator_create_ranged(cecs_component_iterator_descriptor descriptor, cecs_entity_id_range range) {
    assert(descriptor.checksum == descriptor.world_components->checksum
        && "Component iterator descriptor is invalid or outdated, please create a new one");
    cecs_hibitset_iterator iterator = cecs_hibitset_iterator_create_owned_at_first(descriptor.entities_bitset);
    if (!cecs_hibitset_iterator_current_is_set(&iterator)) {
        cecs_hibitset_iterator_next_set(&iterator);
    }
    return (cecs_component_iterator) {
        .descriptor = descriptor,
            .entities_iterator = iterator,
            .entity_range = range
    };
}

bool cecs_component_iterator_done(const cecs_component_iterator* it) {
    return !cecs_exclusive_range_contains(it->entity_range, it->entities_iterator.current_bit_index)
        || cecs_hibitset_iterator_done(&it->entities_iterator);
}

cecs_entity_id cecs_component_iterator_current(const cecs_component_iterator* it, raw_iteration_handle_reference out_iteration_handle) {
    cecs_entity_id* handle = (cecs_entity_id*)out_iteration_handle;
    *handle = it->entities_iterator.current_bit_index;

    for (size_t i = 0; i < it->descriptor.component_count; i++) {
        cecs_sized_component_storage* storage = cecs_world_components_get_component_storage_expect(
            it->descriptor.world_components,
            it->descriptor.component_ids[i]
        );

        ((cecs_raw_component_reference*)(handle + 1))[i] = cecs_component_storage_get_or_null(
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
