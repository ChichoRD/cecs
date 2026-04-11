#include "world/cecs_component_registry.h"
#include "cecs_component_registry.h"

#include <assert.h>

// #if sizeof(cecs_flatset_hash) <= sizeof(cecs_group_type_id)
// #define CECS_COMPONENT_REGISTRY_GROUP_ID_TYPE cecs_group_type_id

// #else
// #define CECS_COMPONENT_REGISTRY_GROUP_ID_TYPE cecs_flatset_hash

// #endif

#define CECS_COMPONENT_REGISTRY_GROUP_ID_TYPE cecs_flatset_hash
static_assert(
    sizeof(cecs_group_type_id) <= sizeof(cecs_flatset_hash),
    "static error: cecs_group_type_id must fit within cecs_flatset_hash to be used as cecs_component_registry_group_id"
);

#ifndef CECS_COMPONENT_REGISTRY_GROUP_ID_TYPE
static_assert(false, "static error: CECS_COMPONENT_REGISTRY_GROUP_ID_TYPE must be defined");
#endif
typedef CECS_COMPONENT_REGISTRY_GROUP_ID_TYPE cecs_component_registry_group_id;
typedef struct cecs_component_registry_group_type {
    cecs_component_registry_group_id id;
} cecs_component_registry_group_type;


extern inline bool cecs_component_registry_userdata_is_empty(const cecs_component_registry_userdata userdata);
extern inline cecs_component_registry_userdata cecs_component_registry_userdata_create_unchecked(void *const data);
extern inline cecs_component_registry_userdata cecs_component_registry_userdata_create_empty(void);
extern inline cecs_component_registry_userdata cecs_component_registry_userdata_create_with(void *const data);


extern inline cecs_component_registry_groups cecs_component_registry_groups_create(void);
cecs_component_registry_groups cecs_component_registry_groups_create_with_capacity(cecs_allocator *const allocator, const size_t capacity) {
    // HACK: enusre non-zero capacity, else call cecs_flatset_create(void)
    cecs_debugbreak_fail_unless(
        capacity > 0ull,
        "error: cecs_component_registry_groups_create_with_capacity called with zero capacity"
    );
    return (cecs_component_registry_groups){
        .group_types = cecs_flatset_create_with_capacity(
            allocator,
            ((capacity - 1ull) >> CECS_FLATBUCKET8_MAX_COUNT_LOG2) + 1ull,
            sizeof(cecs_component_registry_group_type)
        ),
    };
}


extern inline cecs_component_registry cecs_component_registry_create(
    const cecs_component_storage storage,
    const cecs_component_registry_groups groups,
    const size_t component_size,
    void *const userdata
);

extern inline bool cecs_component_registry_has_userdata(const cecs_component_registry *const registry);
void *cecs_component_registry_set_userdata(cecs_component_registry *const registry, void *const userdata) {
    cecs_debugbreak_fail_unless(
        cecs_component_registry_userdata_is_empty(registry->userdata),
        "error: cecs_component_registry_set_userdata called on registry that already has userdata set"
    );
    registry->userdata = cecs_component_registry_userdata_create_with(userdata);
    return registry->userdata.data;
}
bool cecs_component_registry_unset_userdata(cecs_component_registry *const registry, void **const out_userdata) {
    if (cecs_component_registry_userdata_is_empty(registry->userdata)) {
        *out_userdata = NULL;
        return false;
    } else {
        *out_userdata = registry->userdata.data;
        registry->userdata = cecs_component_registry_userdata_create_empty();
        return true;
    }
}
void *cecs_component_registry_unset_userdata_expect(cecs_component_registry *const registry) {
    void *userdata;
    const bool unset_success = cecs_component_registry_unset_userdata(registry, &userdata);
    cecs_debugbreak_fail_unless(
        unset_success,
        "error: cecs_component_registry_unset_userdata_expect called on registry with no userdata set"
    );
    return userdata;
}

const void *cecs_component_registry_get_userdata(const cecs_component_registry *const registry) {
    cecs_debugbreak_fail_unless(
        cecs_component_registry_has_userdata(registry),
        "error: cecs_component_registry_get_userdata called on registry with no userdata set"
    );
    return registry->userdata.data;
}
void *cecs_component_registry_get_userdata_mut(cecs_component_registry *const registry) {
    cecs_debugbreak_fail_unless(
        cecs_component_registry_has_userdata(registry),
        "error: cecs_component_registry_get_userdata_mut called on registry with no userdata set"
    );
    return registry->userdata.data;
}
void *cecs_component_registry_get_or_set_userdata(cecs_component_registry *const registry, void *const default_userdata) {
    if (cecs_component_registry_has_userdata(registry)) {
        return registry->userdata.data;
    } else {
        registry->userdata = cecs_component_registry_userdata_create_with(default_userdata);
        return default_userdata;
    }
}

void cecs_component_registry_insert_group(cecs_component_registry *const registry, cecs_allocator *const allocator, const cecs_group_type group) {
    cecs_unimplemented_fail("unimplemented error: cecs_component_registry_insert_group is not yet implemented");
    // static_assert(false, "// TODO: implement cecs_component_registry_insert_group");
    cecs_component_registry_group_type group_type = {
        .id = (cecs_component_registry_group_id)group.id,
    };
    cecs_component_registry_group_type *inserted_group = (cecs_component_registry_group_type *)cecs_flatset_insert_expect(
        &registry->groups.group_types,
        allocator,
        group_type.id,
        sizeof(cecs_component_registry_group_type),
        0
    );
    (void)inserted_group;
}
