#include <stdlib.h>

#include "cecs_system.h"

cecs_entity_count cecs_world_system_iter(const cecs_world_system s, cecs_world* w, cecs_arena* iteration_arena, cecs_system_predicate_data data, cecs_system_predicate* predicate) {
    cecs_entity_count count = 0;
    cecs_component_iterator_descriptor descriptor = cecs_component_iterator_descriptor_create(&w->components, iteration_arena, s.search_groups);
    raw_iteration_handle_reference handle = cecs_component_iterator_descriptor_allocate_handle(descriptor);
    for (
        cecs_component_iterator it = cecs_component_iterator_create(descriptor);
        !cecs_component_iterator_done(&it);
        cecs_component_iterator_next(&it)
        ) {
        ++count;
        cecs_component_iterator_current(&it, handle);
        predicate(handle, w, data);
    }
    free(handle);
    return count;
}

cecs_entity_count cecs_world_system_iter_range(const cecs_world_system s, cecs_world* w, cecs_arena* iteration_arena, cecs_entity_id_range range, cecs_system_predicate_data data, cecs_system_predicate* predicate) {
    cecs_entity_count count = 0;
    cecs_component_iterator_descriptor descriptor = cecs_component_iterator_descriptor_create(&w->components, iteration_arena, s.search_groups);
    raw_iteration_handle_reference handle = cecs_component_iterator_descriptor_allocate_handle(descriptor);
    for (
        cecs_component_iterator it = cecs_component_iterator_create_ranged(descriptor, range);
        !cecs_component_iterator_done(&it);
        cecs_component_iterator_next(&it)
        ) {
        ++count;
        cecs_component_iterator_current(&it, handle);
        predicate(handle, w, data);
    }
    free(handle);
    return count;
}

cecs_system_predicates cecs_system_predicates_create(cecs_system_predicate** predicates, size_t predicate_count) {
    return (cecs_system_predicates) {
        .predicates = predicates,
            .predicate_count = predicate_count
    };
}

cecs_entity_count cecs_world_system_iter_all(const cecs_world_system s, cecs_world* w, cecs_arena* iteration_arena, cecs_system_predicate_data data, cecs_system_predicates predicates) {
    cecs_entity_count count = 0;
    cecs_component_iterator_descriptor descriptor = cecs_component_iterator_descriptor_create(&w->components, iteration_arena, s.search_groups);
    raw_iteration_handle_reference handle = cecs_component_iterator_descriptor_allocate_handle(descriptor);
    for (
        cecs_component_iterator it = cecs_component_iterator_create(descriptor);
        !cecs_component_iterator_done(&it);
        cecs_component_iterator_next(&it)
        ) {
        ++count;
        cecs_component_iterator_current(&it, handle);
        for (size_t i = 0; i < predicates.predicate_count; i++) {
            predicates.predicates[i](handle, w, data);
        }
    }
    free(handle);
    return count;
}

cecs_entity_count cecs_world_system_iter_range_all(const cecs_world_system s, cecs_world* w, cecs_arena* iteration_arena, cecs_entity_id_range range, cecs_system_predicate_data data, cecs_system_predicates predicates) {
    cecs_entity_count count = 0;
    cecs_component_iterator_descriptor descriptor = cecs_component_iterator_descriptor_create(&w->components, iteration_arena, s.search_groups);
    raw_iteration_handle_reference handle = cecs_component_iterator_descriptor_allocate_handle(descriptor);
    for (
        cecs_component_iterator it = cecs_component_iterator_create_ranged(descriptor, range);
        !cecs_component_iterator_done(&it);
        cecs_component_iterator_next(&it)
        ) {
        ++count;
        cecs_component_iterator_current(&it, handle);
        for (size_t i = 0; i < predicates.predicate_count; i++) {
            predicates.predicates[i](handle, w, data);
        }
    }
    free(handle);
    return count;
}

cecs_dynamic_world_system cecs_dynamic_world_system_create(void) {
    return (cecs_dynamic_world_system) {
        .component_ids = cecs_dynamic_array_create(),
            .components_search_groups = cecs_dynamic_array_create(),
    };
}

cecs_dynamic_world_system cecs_dynamic_world_system_create_from(cecs_components_search_groups s, cecs_arena* a) {
    cecs_dynamic_world_system d = cecs_dynamic_world_system_create();
    for (size_t i = 0; i < s.group_count; ++i) {
        cecs_components_search_group group = s.groups[i];
        cecs_components_type_info info = CECS_UNION_GET_UNCHECKED(CECS_COMPONENTS_ALL_ID, group);
        CECS_UNION_GET_UNCHECKED(CECS_COMPONENTS_ALL_ID, group).component_ids =
            CECS_DYNAMIC_ARRAY_ADD_RANGE(cecs_component_id, &d.component_ids, a, info.component_ids, info.component_count);
        CECS_DYNAMIC_ARRAY_ADD(cecs_components_search_group, &d.components_search_groups, a, &group);
    }
    return d;
}

cecs_components_search_group_range cecs_dynamic_world_system_add(cecs_dynamic_world_system* d, cecs_arena* a, cecs_components_search_group s) {
    cecs_components_type_info info = CECS_UNION_GET_UNCHECKED(CECS_COMPONENTS_ALL_ID, s);
    CECS_UNION_GET_UNCHECKED(CECS_COMPONENTS_ALL_ID, s).component_ids =
        CECS_DYNAMIC_ARRAY_ADD_RANGE(cecs_component_id, &d->component_ids, a, info.component_ids, info.component_count);

    CECS_DYNAMIC_ARRAY_ADD(cecs_components_search_group, &d->components_search_groups, a, &s);
    return (cecs_components_search_group_range) { 0, CECS_DYNAMIC_ARRAY_COUNT(cecs_components_search_group, &d->components_search_groups) };
}

cecs_components_search_group_range cecs_dynamic_world_system_add_range(cecs_dynamic_world_system* d, cecs_arena* a, cecs_components_search_groups s) {
    for (size_t i = 0; i < s.group_count; ++i) {
        cecs_components_search_group group = s.groups[i];
        cecs_components_type_info info = CECS_UNION_GET_UNCHECKED(CECS_COMPONENTS_ALL_ID, group);
        CECS_UNION_GET_UNCHECKED(CECS_COMPONENTS_ALL_ID, group).component_ids =
            CECS_DYNAMIC_ARRAY_ADD_RANGE(cecs_component_id, &d->component_ids, a, info.component_ids, info.component_count);
        CECS_DYNAMIC_ARRAY_ADD(cecs_components_search_group, &d->components_search_groups, a, &group);
    }
    return (cecs_components_search_group_range) { 0, CECS_DYNAMIC_ARRAY_COUNT(cecs_components_search_group, &d->components_search_groups) };
}

cecs_components_search_group_range cecs_dynamic_world_system_set(cecs_dynamic_world_system* d, cecs_arena* a, cecs_components_search_group s, size_t index) {
    assert(index <= CECS_DYNAMIC_ARRAY_COUNT(cecs_components_search_group, &d->components_search_groups) && "index out of bounds");
    if (index == CECS_DYNAMIC_ARRAY_COUNT(cecs_components_search_group, &d->components_search_groups)) {
        cecs_dynamic_world_system_add(d, a, s);
        return (cecs_components_search_group_range) { 0, index + 1 };
    }

    cecs_components_type_info info = CECS_UNION_GET_UNCHECKED(CECS_COMPONENTS_ALL_ID, s);
    cecs_components_type_info previous_info = CECS_UNION_GET_UNCHECKED(
        CECS_COMPONENTS_ALL_ID,
        *CECS_DYNAMIC_ARRAY_GET(cecs_components_search_group, &d->components_search_groups, index)
    );
    if (info.component_count <= previous_info.component_count) {
        size_t existing_index = previous_info.component_ids - (cecs_component_id*)d->component_ids.values;
        if (CECS_DYNAMIC_ARRAY_GET(cecs_component_id, &d->component_ids, existing_index) != previous_info.component_ids) {
            assert(false && "unreachable: failed component ids index computation");
            exit(EXIT_FAILURE);
        }

        CECS_UNION_GET_UNCHECKED(CECS_COMPONENTS_ALL_ID, s).component_ids = CECS_DYNAMIC_ARRAY_SET_RANGE(
            cecs_component_id,
            &d->component_ids,
            existing_index,
            info.component_ids,
            info.component_count
        );
    }
    else {
        CECS_UNION_GET_UNCHECKED(CECS_COMPONENTS_ALL_ID, s).component_ids = CECS_DYNAMIC_ARRAY_ADD_RANGE(
            cecs_component_id,
            &d->component_ids,
            a,
            info.component_ids,
            info.component_count
        );
    }

    CECS_DYNAMIC_ARRAY_SET(cecs_components_search_group, &d->components_search_groups, index, &s);
    return (cecs_components_search_group_range) { 0, index + 1 };
}

cecs_components_search_group_range cecs_dynamic_world_system_set_range(cecs_dynamic_world_system* d, cecs_arena* a, cecs_components_search_groups s, size_t index) {
    assert(index <= CECS_DYNAMIC_ARRAY_COUNT(cecs_components_search_group, &d->components_search_groups) && "index out of bounds");
    if (index + s.group_count >= CECS_DYNAMIC_ARRAY_COUNT(cecs_components_search_group, &d->components_search_groups) - 1) {
        size_t missing = index + s.group_count - CECS_DYNAMIC_ARRAY_COUNT(cecs_components_search_group, &d->components_search_groups) - 1;
        CECS_DYNAMIC_ARRAY_APPEND_EMPTY(cecs_components_search_group, &d->components_search_groups, a, missing);
    }

    for (size_t i = 0; i < s.group_count; ++i) {
        cecs_components_search_group group = s.groups[i];
        cecs_components_type_info info = CECS_UNION_GET_UNCHECKED(CECS_COMPONENTS_ALL_ID, group);
        cecs_components_type_info previous_info = CECS_UNION_GET_UNCHECKED(
            CECS_COMPONENTS_ALL_ID,
            *CECS_DYNAMIC_ARRAY_GET(cecs_components_search_group, &d->components_search_groups, index)
        );
        if (info.component_count <= previous_info.component_count) {
            size_t index = previous_info.component_ids - (cecs_component_id*)d->component_ids.values;
            if (CECS_DYNAMIC_ARRAY_GET(cecs_component_id, &d->component_ids, index) != previous_info.component_ids) {
                assert(false && "unreachable: failed component ids index computation");
                exit(EXIT_FAILURE);
            }

            CECS_UNION_GET_UNCHECKED(CECS_COMPONENTS_ALL_ID, group).component_ids = CECS_DYNAMIC_ARRAY_SET_RANGE(
                cecs_component_id,
                &d->component_ids,
                (previous_info.component_ids - (cecs_component_id*)d->component_ids.values),
                info.component_ids,
                info.component_count
            );
        }
        else {
            CECS_UNION_GET_UNCHECKED(CECS_COMPONENTS_ALL_ID, group).component_ids = CECS_DYNAMIC_ARRAY_ADD_RANGE(
                cecs_component_id,
                &d->component_ids,
                a,
                info.component_ids,
                info.component_count
            );
        }
        CECS_DYNAMIC_ARRAY_SET(cecs_components_search_group, &d->components_search_groups, index + i, &group);
    }

    return (cecs_components_search_group_range) { 0, index + s.group_count };
}

cecs_world_system cecs_dynamic_world_system_get(const cecs_dynamic_world_system* d) {
    return cecs_world_system_create(cecs_components_search_groups_create(
        d->components_search_groups.values,
        CECS_DYNAMIC_ARRAY_COUNT(cecs_components_search_group, &d->components_search_groups)
    ));
}

cecs_world_system cecs_dynamic_world_system_get_range(const cecs_dynamic_world_system* d, cecs_components_search_group_range r) {
    return cecs_world_system_create(cecs_components_search_groups_create(
        (cecs_components_search_group*)d->components_search_groups.values + r.start,
        cecs_exclusive_range_length(r)
    ));
}

cecs_dynamic_world_system cecs_dynamic_world_system_clone(const cecs_dynamic_world_system* d, cecs_arena* a) {
    return cecs_dynamic_world_system_create_from(cecs_dynamic_world_system_get(d).search_groups, a);
}
