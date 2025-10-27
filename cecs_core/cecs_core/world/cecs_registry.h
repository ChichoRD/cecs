#ifndef CECS_REGISTRY_H
#define CECS_REGISTRY_H

#include "cecs_component_registry.h"
#include "registry/cecs_component_storage.h"
#include <cecs_core/sync/cecs_rwlock.h>

typedef enum cecs_registry_access_type {
    cecs_registry_access_type_none = 0,
    cecs_registry_access_type_immutable,
    cecs_registry_access_type_mutable,
} cecs_registry_access_type;
typedef uint8_t cecs_registry_access_value;

typedef struct cecs_registry_access {
    size_t shared_access_count;
    cecs_rwlock access_lock;
    cecs_registry_access_value last_access_type;
} cecs_registry_access;

typedef struct cecs_registry {
    cecs_component_registry registry;
    cecs_registry_access access;
} cecs_registry;

inline cecs_registry cecs_registry_create(const cecs_component_registry registry) {
    return (cecs_registry){
        .registry = registry,
        .access = (cecs_registry_access){
            .access_lock = cecs_rwlock_create(),
            .shared_access_count = 0ull,
            .last_access_type = cecs_registry_access_type_none,
        },
    };
}
inline void cecs_registry_destroy(cecs_registry *const registry, cecs_allocator *const allocator, const size_t component_size) {
    cecs_rwlock_reset(&registry->access.access_lock);
    cecs_component_storage_destroy(&registry->registry.storage, allocator, component_size);
}

inline const cecs_component_registry *cecs_registry_get_unchecked(const cecs_registry *const registry) {
    return &registry->registry;
}
inline cecs_component_registry *cecs_registry_get_mut_unchecked(cecs_registry *const registry) {
    return &registry->registry;
}

bool cecs_registry_acquire_seq(cecs_registry *const registry, const cecs_component_registry **const out_registry);
bool cecs_registry_acquire_mut_seq(cecs_registry *const registry, cecs_component_registry **const out_registry);

cecs_rwlock_borrow cecs_registry_acquire(cecs_registry *const registry, const cecs_component_registry **const out_registry);
cecs_rwlock_borrow_mut cecs_registry_acquire_mut(cecs_registry *const registry, cecs_component_registry **const out_registry);
cecs_rwlock_borrow cecs_registry_acquire_or_exit(cecs_registry *const registry, const cecs_component_registry **const out_registry);
cecs_rwlock_borrow_mut cecs_registry_acquire_mut_or_exit(cecs_registry *const registry, cecs_component_registry **const out_registry);


void cecs_registry_release_seq(cecs_registry *const registry);
void cecs_registry_release_mut_seq(cecs_registry *const registry);

void cecs_registry_release(cecs_registry *const registry, cecs_rwlock_borrow *const system_borrow);
void cecs_registry_release_mut(cecs_registry *const registry, cecs_rwlock_borrow_mut *const system_borrow);


#endif
