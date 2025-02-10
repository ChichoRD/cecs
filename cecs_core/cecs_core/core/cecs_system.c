#include <stdlib.h>
#include <memory.h>

#include "cecs_system.h"

cecs_entity_count cecs_world_system_iter(
    const cecs_world_system s,
    cecs_world* w,
    cecs_arena* iteration_arena,
    cecs_component_handles handles,
    cecs_system_predicate_data data,
    cecs_system_predicate* const predicate
) {
    cecs_entity_count count = 0;
    cecs_component_iterator it = cecs_component_iterator_create(s.descriptor, &w->components, iteration_arena);
    for (
        cecs_component_iterator_begin_iter(&it, iteration_arena);
        !cecs_component_iterator_done(&it);
        cecs_component_iterator_next(&it)
    ) {
        ++count;
        const cecs_entity_id entity = cecs_component_iterator_current(&it, handles);
        predicate(handles, entity, w, data);
    }
    cecs_component_iterator_end_iter(&it);
    return count;
}


cecs_system_predicates cecs_system_predicates_create(cecs_system_predicate** predicates, size_t predicate_count) {
    return (cecs_system_predicates) {
        .predicates = predicates,
        .predicate_count = predicate_count
    };
}

cecs_entity_count cecs_world_system_iter_all(
    const cecs_world_system s,
    cecs_world* w,
    cecs_arena* iteration_arena,
    cecs_component_handles handles,
    cecs_system_predicate_data data,
    const cecs_system_predicates predicates
) {
    cecs_entity_count count = 0;
    cecs_component_iterator it = cecs_component_iterator_create(s.descriptor, &w->components, iteration_arena);
    for (
        cecs_component_iterator_begin_iter(&it, iteration_arena);
        !cecs_component_iterator_done(&it);
        cecs_component_iterator_next(&it)
    ) {
        ++count;
        const cecs_entity_id entity = cecs_component_iterator_current(&it, handles);
        for (size_t i = 0; i < predicates.predicate_count; i++) {
            predicates.predicates[i](handles, entity, w, data);
        }
    }
    cecs_component_iterator_end_iter(&it);
    return count;
}

cecs_dynamic_world_system cecs_dynamic_world_system_create(void) {
    return (cecs_dynamic_world_system) {
        .components = cecs_dynamic_array_create(),
        .component_groups = cecs_dynamic_array_create(),
    };
}

cecs_dynamic_world_system cecs_dynamic_world_system_create_from(cecs_arena *a, const cecs_component_iteration_group groups[const], const size_t group_count) {
    cecs_dynamic_world_system d = (cecs_dynamic_world_system) {
        .components = cecs_dynamic_array_create_with_capacity(a, group_count * sizeof(cecs_component_id)),
        .component_groups = cecs_dynamic_array_create_with_capacity(a, group_count * sizeof(cecs_component_iteration_group)),
    };
    cecs_dynamic_world_system_add_range(&d, a, groups, group_count);
    return d;
}

cecs_component_iteration_group_range cecs_dynamic_world_system_add_range(
    cecs_dynamic_world_system* d,
    cecs_arena* a,
    const cecs_component_iteration_group groups[const],
    const size_t group_count
) {
    cecs_component_iteration_group *const added_groups = cecs_dynamic_array_add_range(
        &d->component_groups,
        a,
        groups,
        group_count,
        sizeof(cecs_component_iteration_group)
    );
    for (size_t i = 0; i < group_count; ++i) {
        const cecs_component_iteration_group added = added_groups[i];
        added_groups[i].components = cecs_dynamic_array_add_range(
            &d->components,
            a,
            added.components,
            added.component_count,
            sizeof(cecs_component_id)
        );
    }
    return (cecs_component_iteration_group_range) {
        0, cecs_dynamic_array_count_of_size(&d->component_groups, sizeof(cecs_component_iteration_group))
    };
}

cecs_component_iteration_group_range cecs_dynamic_world_system_set_or_extend_range(
    cecs_dynamic_world_system *d,
    cecs_arena *a,
    const size_t index,
    const cecs_component_iteration_group groups[const],
    const size_t group_count
) {
    const size_t current_group_count = cecs_dynamic_array_count_of_size(&d->component_groups, sizeof(cecs_component_iteration_group));
    assert(index <= group_count && "error: group index out of bounds");

    if (index + group_count > group_count) {
        const size_t extension = index + group_count - current_group_count;
        cecs_component_iteration_group *extended =
            cecs_dynamic_array_extend(&d->component_groups, a, extension, sizeof(cecs_component_iteration_group));
        for (size_t i = 0; i < extension; ++i) {
            extended[i].component_count = 0;
        }
    }

    for (size_t i = 0; i < group_count; ++i) {
        const cecs_component_iteration_group new_group = groups[i];
        cecs_component_iteration_group *previous_group =
            CECS_DYNAMIC_ARRAY_GET(cecs_component_iteration_group, &d->component_groups, index + i);

        if (new_group.component_count > previous_group->component_count) {
            previous_group->components = cecs_dynamic_array_add_range(
                &d->components,
                a,
                new_group.components,
                new_group.component_count,
                sizeof(cecs_component_id)
            );
            previous_group->component_count = new_group.component_count;
            previous_group->access = new_group.access;
            previous_group->search_mode = new_group.search_mode;
        } else {
            memcpy(
                previous_group->components,
                new_group.components,
                new_group.component_count * sizeof(cecs_component_id)
            );
            previous_group->component_count = new_group.component_count;
            previous_group->access = new_group.access;
            previous_group->search_mode = new_group.search_mode;
        }
    }

    return (cecs_component_iteration_group_range) { 0, index + group_count };
}

cecs_world_system cecs_world_system_from_dynamic(const cecs_dynamic_world_system* d) {
    return cecs_world_system_create((cecs_component_iterator_descriptor){
        .entity_range = { 0, PTRDIFF_MAX },
        .groups = cecs_dynamic_array_first(&d->component_groups),
        .group_count = cecs_dynamic_array_count_of_size(&d->component_groups, sizeof(cecs_component_iteration_group))
    });
}

cecs_world_system cecs_world_system_from_dynamic_range(const cecs_dynamic_world_system* d, const cecs_component_iteration_group_range r) {
    return cecs_world_system_create((cecs_component_iterator_descriptor){
        .entity_range = { 0, PTRDIFF_MAX },
        .groups = cecs_dynamic_array_get(&d->component_groups, r.start, sizeof(cecs_component_iteration_group)),
        .group_count = cecs_exclusive_range_length(r)
    });
}

cecs_dynamic_world_system cecs_dynamic_world_system_clone(const cecs_dynamic_world_system* d, cecs_arena* a) {
    return cecs_dynamic_world_system_create_from(
        a,
        cecs_dynamic_array_first(&d->component_groups),
        cecs_dynamic_array_count_of_size(&d->component_groups, sizeof(cecs_component_iteration_group))
    );
}
