#ifndef CECS_COMPONENT_REGISTRY_H
#define CECS_COMPONENT_REGISTRY_H

#include "registry/cecs_component_storage.h"

typedef struct cecs_component_registry_userdata {
    void *data;
} cecs_component_registry_userdata;

inline bool cecs_component_registry_userdata_is_empty(const cecs_component_registry_userdata userdata) {
    return userdata.data == NULL;
}
inline cecs_component_registry_userdata cecs_component_registry_userdata_create_unchecked(void *const data) {
    return (cecs_component_registry_userdata){
        .data = data
    };
}
inline cecs_component_registry_userdata cecs_component_registry_userdata_create_empty(void) {
    return cecs_component_registry_userdata_create_unchecked(NULL);
}
inline cecs_component_registry_userdata cecs_component_registry_userdata_create_with(void *const data) {
    cecs_assert_or_exit(
        data != NULL,
        "error: cecs_component_registry_userdata_create_with called with NULL data"
    );
    return cecs_component_registry_userdata_create_unchecked(data);
}


typedef struct cecs_component_registry {
    cecs_component_storage storage;
    cecs_component_registry_userdata userdata;
    size_t component_size;
} cecs_component_registry;

inline cecs_component_registry cecs_component_registry_create(
    const cecs_component_storage storage,
    const size_t component_size,
    void *const userdata
) {
    return (cecs_component_registry){
        .storage = storage,
        .component_size = component_size,
        .userdata = cecs_component_registry_userdata_create_unchecked(userdata),
    };
}

inline bool cecs_component_registry_has_userdata(const cecs_component_registry *const registry) {
    return !cecs_component_registry_userdata_is_empty(registry->userdata);
}
void *cecs_component_registry_set_userdata(cecs_component_registry *const registry, void *const userdata);
bool cecs_component_registry_unset_userdata(cecs_component_registry *const registry, void **const out_userdata);
void *cecs_component_registry_unset_userdata_expect(cecs_component_registry *const registry);

const void *cecs_component_registry_get_userdata(const cecs_component_registry *const registry);
void *cecs_component_registry_get_userdata_mut(cecs_component_registry *const registry);
void *cecs_component_registry_get_or_set_userdata(cecs_component_registry *const registry, void *const default_userdata);

#endif
