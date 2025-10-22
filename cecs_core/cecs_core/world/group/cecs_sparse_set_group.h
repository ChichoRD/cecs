#ifndef CECS_SPARSE_SET_GROUP_H
#define CECS_SPARSE_SET_GROUP_H

#include <cecs_core/container/cecs_range.h>
#include <cecs_core/world/cecs_entity.h>
#include <cecs_core/world/cecs_storage_world.h>

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

void *cecs_sparse_set_group_insert(
    cecs_sparse_set_group *const group,
    cecs_component_storage_world **const storages,
    const size_t count,
    const size_t storage_index,
    const cecs_entity entity,
    const size_t component_size
);

#endif
