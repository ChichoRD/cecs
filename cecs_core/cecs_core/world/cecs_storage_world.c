#include "cecs_storage_world.h"

extern inline const cecs_component_storage *cecs_storage_world_get_storage_unchecked(const cecs_storage_world *const world);
extern inline cecs_component_storage *cecs_storage_world_get_storage_mut_unchecked(cecs_storage_world *const world);

static inline const cecs_component_storage *cecs_storage_world_acquire_storage_seq_unchecked(cecs_storage_world *const world) {
    world->access.last_access_type = cecs_storage_world_access_type_immutable;
    ++world->access.access_system.shared_access_count;
    return cecs_storage_world_get_storage_unchecked(world);
}
static inline cecs_component_storage *cecs_storage_world_acquire_storage_mut_seq_unchecked(cecs_storage_world *const world, const cecs_system_id system_id) {
    world->access.last_access_type = cecs_storage_world_access_type_mutable;
    world->access.access_system.last_exclusive_system = system_id;
    return cecs_storage_world_get_storage_mut_unchecked(world);
}

bool cecs_storage_world_acquire_storage_seq(cecs_storage_world *const world, const cecs_component_storage **const out_storage) {
    if (world->access.last_access_type == cecs_storage_world_access_type_mutable) {
        *out_storage = NULL;
        return false;
    } else {
        *out_storage = cecs_storage_world_acquire_storage_seq_unchecked(world);
        return true;
    }
}
bool cecs_storage_world_acquire_storage_mut_seq(cecs_storage_world *const world, const cecs_system_id system_id, cecs_component_storage **const out_storage) {
    if (world->access.last_access_type != cecs_storage_world_access_type_none) {
        *out_storage = NULL;
        return false;
    } else {
        cecs_assert_or_exit(
            world->access.access_system.shared_access_count == 0ull,
            "fatal error: cecs_storage_world_acquire_storage_mut_seq called when there were active shared accesses"
        );
        *out_storage = cecs_storage_world_acquire_storage_mut_seq_unchecked(world, system_id);
        return true;
    }
}

cecs_rwlock_guard cecs_storage_world_acquire_storage(cecs_storage_world *const world, const cecs_component_storage **const out_storage) {
    const cecs_rwlock_guard guard = cecs_rwlock_acquire(&world->access.access_lock);
    if (cecs_rwlock_guard_acquired(guard)) {
        *out_storage = cecs_storage_world_acquire_storage_seq_unchecked(world);
    } else {
        *out_storage = NULL;
    }
    return guard;
}
cecs_rwlock_guard_mut cecs_storage_world_acquire_storage_mut(cecs_storage_world *const world, const cecs_system_id system_id, cecs_component_storage **const out_storage) {
    const cecs_rwlock_guard_mut guard = cecs_rwlock_acquire_mut(&world->access.access_lock);
    if (cecs_rwlock_guard_mut_acquired(guard)) {
        *out_storage = cecs_storage_world_acquire_storage_mut_seq_unchecked(world, system_id);
    } else {
        *out_storage = NULL;
    }
    return guard;
}

void cecs_storage_world_release_storage_seq(cecs_storage_world *const world) {
    cecs_assert_or_exit(
        world->access.access_system.shared_access_count > 0ull,
        "error: cecs_storage_world_release_storage_seq called when there were no active shared accesses"
    );
    cecs_assert_or_exit(
        world->access.last_access_type == cecs_storage_world_access_type_immutable,
        "error: cecs_storage_world_release_storage_seq called when the last access type was not immutable"
    );
    --world->access.access_system.shared_access_count;
    if (world->access.access_system.shared_access_count == 0ull) {
        world->access.last_access_type = cecs_storage_world_access_type_none;
    }
}
void cecs_storage_world_release_storage_mut_seq(cecs_storage_world *const world, const cecs_system_id system_id) {
    cecs_assert_or_exit(
        world->access.last_access_type == cecs_storage_world_access_type_mutable,
        "error: cecs_storage_world_release_storage_mut_seq called when the last access type was not mutable"
    );
    cecs_assert_or_exit(
        world->access.access_system.last_exclusive_system.value == system_id.value,
        "error: cecs_storage_world_release_storage_mut_seq called with a system id that does not match the last exclusive system id"
    );
    world->access.last_access_type = cecs_storage_world_access_type_none;
}
void cecs_storage_world_release_storage(cecs_storage_world *const world, cecs_rwlock_guard *const system_guard) {
    cecs_rwlock_release(&world->access.access_lock, system_guard);
    if (system_guard->new_reader_count == 0ull) {
        cecs_storage_world_release_storage_seq(world);
    }
}
void cecs_storage_world_release_storage_mut(cecs_storage_world *const world, const cecs_system_id system_id, cecs_rwlock_guard_mut *const system_guard){
    cecs_assert_or_exit(
        world->access.access_system.last_exclusive_system.value == system_id.value,
        "error: cecs_storage_world_release_storage_mut called with a system id that does not match the last exclusive system id"
    );
    cecs_rwlock_release_mut(&world->access.access_lock, system_guard);
    if (system_guard->new_writer_count == 0ull) {
        cecs_storage_world_release_storage_mut_seq(world, system_id);
    }
}
