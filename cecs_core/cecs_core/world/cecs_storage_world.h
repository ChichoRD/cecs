#ifndef CECS_STORAGE_WORLD_H
#define CECS_STORAGE_WORLD_H

#include "cecs_component_storage.h"
#include <cecs_core/sync/cecs_rwlock.h>

typedef enum cecs_storage_world_access_type {
    cecs_storage_world_access_type_none = 0,
    cecs_storage_world_access_type_immutable,
    cecs_storage_world_access_type_mutable,
} cecs_storage_world_access_type;
typedef uint8_t cecs_storage_world_access_value;

typedef struct cecs_storage_world_access {
    size_t shared_access_count;
    cecs_rwlock access_lock;
    cecs_storage_world_access_value last_access_type;
} cecs_storage_world_access;

// TODO: user and/or engine attachments, they would go here
typedef struct cecs_component_storage_world {
    cecs_component_storage storage;
} cecs_component_storage_world;

// TODO: consider removing the need for them to pass to us their system id on mutable access
typedef struct cecs_storage_world {
    cecs_component_storage_world storage;
    cecs_storage_world_access access;
} cecs_storage_world;

inline cecs_storage_world cecs_storage_world_create(const cecs_component_storage storage) {
    return (cecs_storage_world){
        .storage = (cecs_component_storage_world){
            .storage = storage,
        },
        .access = (cecs_storage_world_access){
            .access_lock = cecs_rwlock_create(),
            .shared_access_count = 0ull,
            .last_access_type = cecs_storage_world_access_type_none,
        },
    };
}
inline void cecs_storage_world_destroy(cecs_storage_world *const world, cecs_allocator *const allocator, const size_t component_size) {
    cecs_rwlock_reset(&world->access.access_lock);
    cecs_component_storage_destroy(&world->storage.storage, allocator, component_size);
}

inline const cecs_component_storage_world *cecs_storage_world_get_storage_unchecked(const cecs_storage_world *const world) {
    return &world->storage;
}
inline cecs_component_storage_world *cecs_storage_world_get_storage_mut_unchecked(cecs_storage_world *const world) {
    return &world->storage;
}

bool cecs_storage_world_acquire_storage_seq(cecs_storage_world *const world, const cecs_component_storage_world **const out_storage);
bool cecs_storage_world_acquire_storage_mut_seq(cecs_storage_world *const world, cecs_component_storage_world **const out_storage);

cecs_rwlock_borrow cecs_storage_world_acquire_storage(cecs_storage_world *const world, const cecs_component_storage_world **const out_storage);
cecs_rwlock_borrow_mut cecs_storage_world_acquire_storage_mut(cecs_storage_world *const world, cecs_component_storage_world **const out_storage);
cecs_rwlock_borrow cecs_storage_world_acquire_storage_or_exit(cecs_storage_world *const world, const cecs_component_storage_world **const out_storage);
cecs_rwlock_borrow_mut cecs_storage_world_acquire_storage_mut_or_exit(cecs_storage_world *const world, cecs_component_storage_world **const out_storage);


void cecs_storage_world_release_storage_seq(cecs_storage_world *const world);
void cecs_storage_world_release_storage_mut_seq(cecs_storage_world *const world);

void cecs_storage_world_release_storage(cecs_storage_world *const world, cecs_rwlock_borrow *const system_borrow);
void cecs_storage_world_release_storage_mut(cecs_storage_world *const world, cecs_rwlock_borrow_mut *const system_borrow);


#endif
