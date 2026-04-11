#include "cecs_sparse_set_group.h"
#include "../cecs_component_registry.h"

#include <string.h>

// extern inline cecs_sparse_set_group cecs_sparse_set_group_create(void);

static void cecs_sparse_set_group_insert_within(
    cecs_sparse_set_group *group,
    cecs_component_registry **const registries,
    const size_t count,
    const size_t entity_index,
    const size_t component_size
) {
    cecs_assert_or_exit(
        !cecs_exclusive_range_is_empty(group->free_grouped_range),
        "fatal error: cecs_sparse_set_group_insert_available called when there were no available grouped slots"
    );
    for (size_t i = 0; i < count; ++i) {
        cecs_component_registry *const registry = registries[i];
        cecs_sparse_set_storage *const sparse_set_storage = cecs_component_storage_sparse_set_mut(&registry->storage);
        cecs_sparse_set *const set = &sparse_set_storage->set;
        const size_t set_capacity = cecs_sparse_set_value_capacity(set);
        const size_t set_count = cecs_sparse_set_value_count(set);
        if (set_capacity <= set_count) {
            cecs_assert_or_exit(set_capacity == set_count, "fatal error: capacity is strictly less than count, that behaviour is illegal");
            cecs_unimplemented_fail(
                "unimplemented error: cecs_sparse_set_group_insert_available called when sparse set is at full capacity"
            );
        } else {
            const size_t insert_dense_index = group->free_grouped_range.range.start;
            const size_t insert_key = cecs_sparse_set_storage_map_key(sparse_set_storage, entity_index);
            cecs_dense_index *const insert_index = cecs_sparse_set_get_index_mut(set, insert_key);

            // Swap the dense values
            if (insert_dense_index != insert_index->value) {
                // Get the key of the value currently at the insert index
                extern size_t *cecs_sparse_set_get_sparse_key_by_index_mut(cecs_sparse_set *set, const cecs_dense_index index);
                size_t *const swapped_key_ptr = cecs_sparse_set_get_sparse_key_by_index_mut(set, cecs_dense_index_create_valid(insert_dense_index));
                const size_t swapped_key = *swapped_key_ptr;

                // Update the index of the swapped key to point to the new dense index
                cecs_dense_index *const swapped_index = cecs_sparse_set_get_index_mut(set, swapped_key);
                swapped_index->value = insert_index->value;

                // Swap the sparse-to-dense mapping
                *swapped_key_ptr = insert_key;

                // Swap the dense values
                void *const insert_value_ptr = cecs_sparse_set_get_value_mut(set, insert_key, component_size);
                void *const swapped_value_ptr = cecs_sparse_set_get_value_mut(set, swapped_key, component_size);
                uint8_t *const temp_buffer =
                    (uint8_t *)cecs_sparse_set_get_value_by_index_mut(set, cecs_dense_index_create_valid(0), component_size)
                    + (component_size * set_count);
                memcpy(temp_buffer, insert_value_ptr, component_size);
                memcpy(insert_value_ptr, swapped_value_ptr, component_size);
                memcpy(swapped_value_ptr, temp_buffer, component_size);
            }
        }
    }
    // Update the grouped range
    ++group->free_grouped_range.range.start;
}

// TODO: DEPRECATED! distinguish when to use this one
size_t cecs_sparse_set_group_reserve_grouped_range_owning(
    cecs_sparse_set_group *group,
    cecs_component_registry **const registries,
    const size_t count,
    const size_t component_size
) {
    cecs_assert_or_exit(
        cecs_exclusive_range_is_empty(group->free_grouped_range),
        "fatal error: cecs_sparse_set_group_reserve_grouped_range called when there were available grouped slots"
    );
    const size_t midpoint = (group->free_ungrouped_range.range.start + group->free_ungrouped_range.range.end + 1u) >> 1u;
    const cecs_component_range take_range = cecs_exclusive_range_from(
        (cecs_range){group->free_ungrouped_range.range.start, midpoint}
    );
    const size_t length = cecs_exclusive_range_length(take_range);
    for (size_t i = 0; i < count; ++i) {
        cecs_component_registry *const registry = registries[i];
        cecs_sparse_set_storage *const sparse_set_storage = cecs_component_storage_sparse_set_mut(&registry->storage);
        cecs_sparse_set *const set = &sparse_set_storage->set;
        const size_t set_capacity = cecs_sparse_set_value_capacity(set);
        const size_t set_count = cecs_sparse_set_value_count(set);
        if (set_capacity < set_count + length) {
            cecs_unimplemented_fail(
                "unimplemented error: cecs_sparse_set_group_reserve_grouped_range called when sparse set does not have enough capacity"
            );
        } else {
            /* TODO: this is implemented as if we only support full owning groups,
            * so it needs to be updated to support mutually exclusive groups as well, and respect their boundaries.
            * Still, for an owning group, we could memcpy the entire range of values at once instead of moving them one by one.
            */

            // Move the dense values
            for (size_t j = 0; j < length; ++j) {
                const size_t from_index = take_range.range.start + j;
                const size_t to_index = group->free_grouped_range.range.end + j;

                // Get the key of the value at from_index
                extern size_t *cecs_sparse_set_get_sparse_key_by_index_mut(cecs_sparse_set *set, const cecs_dense_index index);
                size_t *const moved_key_ptr = cecs_sparse_set_get_sparse_key_by_index_mut(set, cecs_dense_index_create_valid(from_index));
                const size_t moved_key = *moved_key_ptr;

                // Update the index of the moved key to point to the new dense index
                cecs_dense_index *const moved_index = cecs_sparse_set_get_index_mut(set, moved_key);
                moved_index->value = to_index;

                // Update the sparse-to-dense mapping
                *moved_key_ptr = moved_key; // stays the same

                // Swap the dense values
                void *const from_value_ptr = cecs_sparse_set_get_value_mut(set, moved_key, component_size);
                void *const to_value_ptr = cecs_sparse_set_get_value_by_index_mut(set, cecs_dense_index_create_valid(to_index), component_size);
                uint8_t *const temp_buffer =
                    (uint8_t *)cecs_sparse_set_get_value_by_index_mut(set, cecs_dense_index_create_valid(0), component_size)
                    + (component_size * set_count);
                memcpy(temp_buffer, from_value_ptr, component_size);
                memcpy(from_value_ptr, to_value_ptr, component_size);
                memcpy(to_value_ptr, temp_buffer, component_size);
            }
        }
    }
    // Update the ranges
    group->free_grouped_range.range.end += length;
    group->free_ungrouped_range.range.start += length;
    return length;
}

// TODO: be able to indicate reserve requirement
static cecs_sparse_set_group_insert_result cecs_sparse_set_group_reserve_grouped_range(
    cecs_sparse_set_group *group,
    cecs_component_registry **const registries,
    const size_t count,
    const size_t component_size
) {
    cecs_assert_or_exit(
        cecs_exclusive_range_is_empty(group->free_grouped_range),
        "fatal error: cecs_sparse_set_group_reserve_grouped_range called when there were available grouped slots"
    );
    const size_t midpoint = (group->free_ungrouped_range.range.start + group->free_ungrouped_range.range.end + 1u) >> 1u;
    const cecs_component_range take_range = cecs_exclusive_range_from(
        (cecs_range){group->free_ungrouped_range.range.start, midpoint}
    );
    const size_t length = cecs_exclusive_range_length(take_range);
    const size_t insertion_start = group->free_grouped_range.range.end;
    // TODO: introduce this optimization back later
    // if (take_range.range.start == group->free_grouped_range.range.end) {
    //     group->free_grouped_range = take_range;
    //     group->free_ungrouped_range = cecs_exclusive_range_from(
    //         (cecs_range){midpoint, group->free_ungrouped_range.range.end}
    //     );
    //     return 0u;
    // }

    const cecs_dense_index take_start = cecs_dense_index_create_valid(take_range.range.end);
    const cecs_dense_index take_end = cecs_dense_index_create_valid(take_start.value + length);
    for (size_t i = 0; i < count; ++i) {
        cecs_component_registry *const registry = registries[i];
        cecs_sparse_set_storage *const sparse_set_storage = cecs_component_storage_sparse_set_mut(&registry->storage);
        cecs_sparse_set *const set = &sparse_set_storage->set;
        const size_t set_capacity = cecs_sparse_set_value_capacity(set);
        const size_t set_count = cecs_sparse_set_value_count(set);
        if (set_capacity < set_count + length) {
            cecs_unimplemented_fail(
                "unimplemented error: cecs_sparse_set_group_reserve_grouped_range called when sparse set does not have enough capacity"
            );
        } else {
            cecs_array *const values_array = &set->values.values.array;
            cecs_array *const keys_array = &set->values.sparse_from_dense.array;
            const size_t initial_size = cecs_array_count(values_array);
            const size_t temp_size = initial_size + length;
            cecs_assert_or_exit(initial_size == cecs_array_count(keys_array), "fatal error: values and keys arrays are out of sync");
            cecs_assert_or_exit(length <= initial_size, "fatal error: length to move is larger than the array size");

            void *const value_insert = cecs_array_insert_many(values_array, insertion_start, length, component_size);
            size_t *const key_insert = cecs_array_insert_many(keys_array, insertion_start, length, sizeof(size_t));

            extern size_t *cecs_sparse_set_get_sparse_key_by_index_mut(cecs_sparse_set *set, const cecs_dense_index index);
            uint8_t *const value_take_start = cecs_sparse_set_get_value_by_index_mut(set, take_start, component_size);
            size_t *const key_take_start = cecs_sparse_set_get_sparse_key_by_index_mut(set, take_start);
            memmove(value_insert, value_take_start, component_size * length);
            memmove(key_insert, key_take_start, sizeof(size_t) * length);

            if (take_end.value < temp_size) {
                const size_t move_count = temp_size - take_end.value;
                memmove(
                    value_take_start,
                    cecs_sparse_set_get_value_by_index_mut(set, take_end, component_size),
                    component_size * move_count
                );
                memmove(
                    key_take_start,
                    cecs_sparse_set_get_sparse_key_by_index_mut(set, take_end),
                    sizeof(size_t) * move_count
                );
            }

            cecs_array_truncate(values_array, initial_size);
            cecs_array_truncate(keys_array, initial_size);
            
            for (size_t j = 0; j < length; ++j) {
                const size_t moved_key = key_insert[j];
                *cecs_sparse_set_get_index_mut(set, moved_key) = cecs_dense_index_create_valid(insertion_start + j);
            }
            for (size_t j = insertion_start + length; j < take_start.value; ++j) {
                const size_t moved_key = *cecs_sparse_set_get_sparse_key_by_index(set, cecs_dense_index_create_valid(j));
                cecs_dense_index *const index = cecs_sparse_set_get_index_mut(set, moved_key);
                *index = cecs_dense_index_create_valid(index->value + length);
            }
        }
    }
    // Update the ranges
    group->free_grouped_range.range.end += length;
    group->free_ungrouped_range.range.start += length;
    return (cecs_sparse_set_group_insert_result){
        .shifted_range = cecs_exclusive_range_from((cecs_range){insertion_start, take_range.range.start}),
        .shift_length = length,
    };
}

cecs_sparse_set_group_insert_result cecs_sparse_set_group_insert(
    cecs_sparse_set_group *group,
    cecs_component_registry **const registries,
    const size_t count,
    const size_t storage_index,
    const cecs_entity entity,
    const size_t component_size
) {
    const cecs_component_registry *const registry = registries[storage_index];
    const size_t entity_index = cecs_entity_index(entity);
    const cecs_sparse_set_storage *const sparse_set_storage = cecs_component_storage_sparse_set(&registry->storage);
    const size_t insert_key = cecs_sparse_set_storage_map_key(sparse_set_storage, entity_index);

    const bool in_grouped_range = cecs_exclusive_range_contains(group->free_grouped_range, insert_key);
    const bool in_ungrouped_range = cecs_exclusive_range_contains(group->free_ungrouped_range, insert_key);
    cecs_assert_or_exit(
        in_grouped_range || in_ungrouped_range,
        "fatal error: cecs_sparse_set_group_insert called with an entity that is not in either the grouped or ungrouped free ranges"
    );
    if (group->free_grouped_range.range.end == 0) {
        const size_t ungrouped_midpoint = (group->free_ungrouped_range.range.start + group->free_ungrouped_range.range.end + 1u) >> 1u;
        group->free_grouped_range = cecs_exclusive_range_from(
            (cecs_range){group->free_ungrouped_range.range.start, ungrouped_midpoint}
        );
        group->free_ungrouped_range = cecs_exclusive_range_from(
            (cecs_range){ungrouped_midpoint, group->free_ungrouped_range.range.end}
        );
    }

    cecs_sparse_set_group_insert_result result = {0};
    if (cecs_exclusive_range_is_empty(group->free_grouped_range)) {
        result = cecs_sparse_set_group_reserve_grouped_range(
            group,
            registries,
            count,
            component_size
        );
    }
    cecs_sparse_set_group_insert_within(
        group,
        registries,
        count,
        entity_index,
        component_size
    );
    return result;
}

void cecs_sparse_set_group_push_ungrouped(
    cecs_sparse_set_group *const group,
    const cecs_component_registry *const registry,
    const cecs_entity entity
) {
    const size_t entity_index = cecs_entity_index(entity);
    const cecs_sparse_set_storage *const sparse_set_storage = cecs_component_storage_sparse_set(&registry->storage);
    const size_t insert_key = cecs_sparse_set_storage_map_key(sparse_set_storage, entity_index);
    const cecs_dense_index *const insert_index = cecs_sparse_set_get_index(&sparse_set_storage->set, insert_key);
    cecs_assert_or_exit(
        insert_index->value == cecs_sparse_set_value_count(&sparse_set_storage->set) - 1,
        "fatal error: cecs_sparse_set_group_push_ungrouped called for a component that was not the last to be added to a storage"
    );
    if (cecs_exclusive_range_is_empty(group->free_ungrouped_range)) {
        group->free_ungrouped_range = cecs_exclusive_range_singleton(insert_index->value);
    } else {
        ++group->free_ungrouped_range.range.end;
    }
}
