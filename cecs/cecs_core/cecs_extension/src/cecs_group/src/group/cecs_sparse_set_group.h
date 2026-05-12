#ifndef CECS_SPARSE_SET_GROUP_H
#define CECS_SPARSE_SET_GROUP_H

#include <cecs_range.h>
#include <world/cecs_entity.h>

typedef cecs_exclusive_range cecs_component_range;
typedef struct cecs_sparse_set_group {
    cecs_component_range free_grouped_range;
    cecs_component_range free_ungrouped_range;
} cecs_sparse_set_group;

static inline cecs_sparse_set_group cecs_sparse_set_group_create(void) {
    return (cecs_sparse_set_group){
        .free_grouped_range = cecs_exclusive_range_from((cecs_range){0, 0}),
        .free_ungrouped_range = cecs_exclusive_range_from((cecs_range){0, 0}),
    };
}

typedef struct cecs_sparse_set_group_insert_result {
    cecs_component_range shifted_range;
    size_t shift_length;
} cecs_sparse_set_group_insert_result;

// FIXME: forward declaration means we could probably structure files in different hierarchy to avoid it
struct cecs_component_registry;
cecs_sparse_set_group_insert_result cecs_sparse_set_group_insert(
    cecs_sparse_set_group *const group,
    struct cecs_component_registry **const registries,
    const size_t count,
    const size_t storage_index,
    const cecs_entity entity,
    const size_t component_size
);
void cecs_sparse_set_group_push_ungrouped(
    cecs_sparse_set_group *const group,
    const struct cecs_component_registry *const registry,
    const cecs_entity entity
);

#endif
