#include "cecs_group_storage.h"
#include <cecs_dynarray.h>

static_assert(
    sizeof(cecs_component_type_id) <= sizeof(cecs_flatset_hash),
    "static error: cecs_component_type_id must fit within cecs_flatset_hash"
);
typedef struct cecs_group_component_type {
    cecs_flatset_hash component_type_id;
    cecs_component_storage_value storage_type;
}  cecs_group_component_type;

cecs_group_type cecs_group_storage_register_group(
    cecs_group_storage *const storage,
    cecs_allocator *const allocator,
    const cecs_group_storage_type storage_type,
    const cecs_component_type *const group_components,
    const size_t group_components_count
) {
    const size_t group_index = cecs_dynarray_count(&storage->descriptors);
    cecs_assert_or_exit(
        group_index <= CECS_GROUP_TYPE_ID_TYPE_MAX,
        "fatal error: exceeded maximum number of group types supported by cecs_group_storage_register_group"
    );
    const cecs_group_type group = (cecs_group_type){
        .id = (cecs_group_type_id)group_index,
        .storage_type = storage_type,
    };
    cecs_group_descriptor *const descriptor = (cecs_group_descriptor *)cecs_dynarray_push(
        &storage->descriptors,
        allocator,
        sizeof(cecs_group_descriptor)
    );
    *descriptor = (cecs_group_descriptor){
        .component_types = cecs_flatset_create_with_capacity(
            allocator,
            group_components_count >> CECS_FLATBUCKET8_MAX_COUNT_LOG2,
            sizeof(cecs_group_component_type)
        ),
        .group = (cecs_group){
            .sparse_set = cecs_sparse_set_group_create()
        }
    };
    for (size_t i = 0; i < group_components_count; ++i) {
        cecs_group_component_type *const component = (cecs_group_component_type *)cecs_flatset_insert_expect(
            &descriptor->component_types,
            allocator,
            (cecs_flatset_hash)group_components[i].id,
            sizeof(cecs_group_component_type),
            0
        );
        *component = (cecs_group_component_type){
            .component_type_id = (cecs_flatset_hash)group_components[i].id,
            .storage_type = group_components[i].storage_type,
        };
    }
    return group;
}
