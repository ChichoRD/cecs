#include "cecs_storage_world.h"

extern inline cecs_storage_world cecs_storage_world_create(const cecs_component_storage storage);

extern inline const cecs_component_storage_world *cecs_storage_world_get_storage_unchecked(const cecs_storage_world *const world);
extern inline cecs_component_storage_world *cecs_storage_world_get_storage_mut_unchecked(cecs_storage_world *const world);

static inline const cecs_component_storage_world *cecs_storage_world_acquire_storage_seq_unchecked(cecs_storage_world *const world) {
    world->access.last_access_type = cecs_storage_world_access_type_immutable;
    ++world->access.shared_access_count;
    return cecs_storage_world_get_storage_unchecked(world);
}
static inline cecs_component_storage_world *cecs_storage_world_acquire_storage_mut_seq_unchecked(cecs_storage_world *const world) {
    world->access.last_access_type = cecs_storage_world_access_type_mutable;
    return cecs_storage_world_get_storage_mut_unchecked(world);
}

bool cecs_storage_world_acquire_storage_seq(cecs_storage_world *const world, const cecs_component_storage_world **const out_storage) {
    if (world->access.last_access_type == cecs_storage_world_access_type_mutable) {
        *out_storage = NULL;
        return false;
    } else {
        *out_storage = cecs_storage_world_acquire_storage_seq_unchecked(world);
        return true;
    }
}
bool cecs_storage_world_acquire_storage_mut_seq(cecs_storage_world *const world, cecs_component_storage_world **const out_storage) {
    if (world->access.last_access_type != cecs_storage_world_access_type_none) {
        *out_storage = NULL;
        return false;
    } else {
        cecs_assert_or_exit(
            world->access.shared_access_count == 0ull,
            "fatal error: cecs_storage_world_acquire_storage_mut_seq called when there were active shared accesses"
        );
        *out_storage = cecs_storage_world_acquire_storage_mut_seq_unchecked(world);
        return true;
    }
}

cecs_rwlock_borrow cecs_storage_world_acquire_storage(cecs_storage_world *const world, const cecs_component_storage_world **const out_storage) {
    const cecs_rwlock_borrow borrow = cecs_rwlock_acquire(&world->access.access_lock);
    if (cecs_rwlock_borrow_acquired(borrow)) {
        *out_storage = cecs_storage_world_acquire_storage_seq_unchecked(world);
    } else {
        *out_storage = NULL;
    }
    return borrow;
}
cecs_rwlock_borrow_mut cecs_storage_world_acquire_storage_mut(cecs_storage_world *const world, cecs_component_storage_world **const out_storage) {
    const cecs_rwlock_borrow_mut borrow = cecs_rwlock_acquire_mut(&world->access.access_lock);
    if (cecs_rwlock_borrow_mut_acquired(borrow)) {
        *out_storage = cecs_storage_world_acquire_storage_mut_seq_unchecked(world);
    } else {
        *out_storage = NULL;
    }
    return borrow;
}
cecs_rwlock_borrow cecs_storage_world_acquire_storage_or_exit(cecs_storage_world *const world, const cecs_component_storage_world **const out_storage) {
    const cecs_rwlock_borrow borrow = cecs_rwlock_acquire_or_exit(&world->access.access_lock);
    *out_storage = cecs_storage_world_acquire_storage_seq_unchecked(world);
    return borrow;
}
cecs_rwlock_borrow_mut cecs_storage_world_acquire_storage_mut_or_exit(cecs_storage_world *const world, cecs_component_storage_world **const out_storage) {
    const cecs_rwlock_borrow_mut borrow = cecs_rwlock_acquire_mut_or_exit(&world->access.access_lock);
    *out_storage = cecs_storage_world_acquire_storage_mut_seq_unchecked(world);
    return borrow;
}

void cecs_storage_world_release_storage_seq(cecs_storage_world *const world) {
    cecs_assert_or_exit(
        world->access.shared_access_count > 0ull,
        "error: cecs_storage_world_release_storage_seq called when there were no active shared accesses"
    );
    cecs_assert_or_exit(
        world->access.last_access_type == cecs_storage_world_access_type_immutable,
        "error: cecs_storage_world_release_storage_seq called when the last access type was not immutable"
    );
    --world->access.shared_access_count;
    if (world->access.shared_access_count == 0ull) {
        world->access.last_access_type = cecs_storage_world_access_type_none;
    }
}
void cecs_storage_world_release_storage_mut_seq(cecs_storage_world *const world) {
    cecs_assert_or_exit(
        world->access.last_access_type == cecs_storage_world_access_type_mutable,
        "error: cecs_storage_world_release_storage_mut_seq called when the last access type was not mutable"
    );
    world->access.last_access_type = cecs_storage_world_access_type_none;
}
void cecs_storage_world_release_storage(cecs_storage_world *const world, cecs_rwlock_borrow *const system_borrow) {
    cecs_rwlock_release(&world->access.access_lock, system_borrow);
    if (system_borrow->new_shared_ref_count == 0ull) {
        cecs_storage_world_release_storage_seq(world);
    }
}
void cecs_storage_world_release_storage_mut(cecs_storage_world *const world, cecs_rwlock_borrow_mut *const system_borrow){
    cecs_rwlock_release_mut(&world->access.access_lock, system_borrow);
    if (system_borrow->previous_ref_count == 0ull) {
        cecs_storage_world_release_storage_mut_seq(world);
    }
}
