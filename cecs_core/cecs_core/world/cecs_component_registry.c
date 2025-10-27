#include "world/cecs_component_registry.h"
#include "cecs_component_registry.h"

extern inline bool cecs_component_registry_userdata_is_empty(const cecs_component_registry_userdata userdata);
extern inline cecs_component_registry_userdata cecs_component_registry_userdata_create_unchecked(void *const data);
extern inline cecs_component_registry_userdata cecs_component_registry_userdata_create_empty(void);
extern inline cecs_component_registry_userdata cecs_component_registry_userdata_create_with(void *const data);

extern inline cecs_component_registry cecs_component_registry_create(
    const cecs_component_storage storage,
    const size_t component_size,
    void *const userdata
);

extern inline bool cecs_component_registry_has_userdata(const cecs_component_registry *const registry);
void *cecs_component_registry_set_userdata(cecs_component_registry *const registry, void *const userdata) {
    cecs_assert_or_exit(
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
    cecs_assert_or_exit(
        unset_success,
        "error: cecs_component_registry_unset_userdata_expect called on registry with no userdata set"
    );
    return userdata;
}

const void *cecs_component_registry_get_userdata(const cecs_component_registry *const registry) {
    cecs_assert_or_exit(
        cecs_component_registry_has_userdata(registry),
        "error: cecs_component_registry_get_userdata called on registry with no userdata set"
    );
    return registry->userdata.data;
}
void *cecs_component_registry_get_userdata_mut(cecs_component_registry *const registry) {
    cecs_assert_or_exit(
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
