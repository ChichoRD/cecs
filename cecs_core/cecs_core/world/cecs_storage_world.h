#ifndef CECS_STORAGE_WORLD_H
#define CECS_STORAGE_WORLD_H

#include "cecs_component_storage.h"
#include "cecs_system.h"
#include <cecs_core/sync/cecs_rwlock.h>

typedef enum cecs_storage_world_access_type {
    cecs_storage_world_access_type_none = 0,
    cecs_storage_world_access_type_immutable,
    cecs_storage_world_access_type_mutable,
} cecs_storage_world_access_type;
typedef uint8_t cecs_storage_world_access_value;

typedef struct cecs_storage_world_access_system {
    cecs_system_id last_exclusive_system;
    size_t shared_access_count;
} cecs_storage_world_access_system;

typedef struct cecs_storage_world_access {
    cecs_rwlock access_lock;
    cecs_storage_world_access_system access_system;
    cecs_storage_world_access_value last_access_type;
} cecs_storage_world_access;
typedef struct cecs_storage_world {
    cecs_component_storage storage;
    cecs_storage_world_access access;
} cecs_storage_world;


inline const cecs_component_storage *cecs_storage_world_get_storage_unchecked(const cecs_storage_world *const world) {
    return &world->storage;
}
inline cecs_component_storage *cecs_storage_world_get_storage_mut_unchecked(cecs_storage_world *const world) {
    return &world->storage;
}

bool cecs_storage_world_acquire_storage_seq(cecs_storage_world *const world, const cecs_component_storage **const out_storage);
bool cecs_storage_world_acquire_storage_mut_seq(cecs_storage_world *const world, const cecs_system_id system_id, cecs_component_storage **const out_storage);

cecs_rwlock_borrow cecs_storage_world_acquire_storage(cecs_storage_world *const world, const cecs_component_storage **const out_storage);
cecs_rwlock_borrow_mut cecs_storage_world_acquire_storage_mut(cecs_storage_world *const world, const cecs_system_id system_id, cecs_component_storage **const out_storage);

void cecs_storage_world_release_storage_seq(cecs_storage_world *const world);
void cecs_storage_world_release_storage_mut_seq(cecs_storage_world *const world, const cecs_system_id system_id);

void cecs_storage_world_release_storage(cecs_storage_world *const world, cecs_rwlock_borrow *const system_borrow);
void cecs_storage_world_release_storage_mut(cecs_storage_world *const world, const cecs_system_id system_id, cecs_rwlock_borrow_mut *const system_borrow);


#endif
