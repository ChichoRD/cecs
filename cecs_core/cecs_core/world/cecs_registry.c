#include "cecs_registry.h"

extern inline cecs_registry cecs_registry_create(const cecs_component_registry registry);

extern inline const cecs_component_registry *cecs_registry_get_unchecked(const cecs_registry *const registry);
extern inline cecs_component_registry *cecs_registry_get_mut_unchecked(cecs_registry *const registry);

static inline const cecs_component_registry *cecs_registry_acquire_seq_unchecked(cecs_registry *const registry) {
    registry->access.last_access_type = cecs_registry_access_type_immutable;
    ++registry->access.shared_access_count;
    return cecs_registry_get_unchecked(registry);
}
static inline cecs_component_registry *cecs_registry_acquire_mut_seq_unchecked(cecs_registry *const registry) {
    registry->access.last_access_type = cecs_registry_access_type_mutable;
    return cecs_registry_get_mut_unchecked(registry);
}

bool cecs_registry_acquire_seq(cecs_registry *const registry, const cecs_component_registry **const out_registry) {
    if (registry->access.last_access_type == cecs_registry_access_type_mutable) {
        *out_registry = NULL;
        return false;
    } else {
        *out_registry = cecs_registry_acquire_seq_unchecked(registry);
        return true;
    }
}
bool cecs_registry_acquire_mut_seq(cecs_registry *const registry, cecs_component_registry **const out_registry) {
    if (registry->access.last_access_type != cecs_registry_access_type_none) {
        *out_registry = NULL;
        return false;
    } else {
        cecs_assert_or_exit(
            registry->access.shared_access_count == 0ull,
            "fatal error: cecs_registry_acquire_mut_seq called when there were active shared accesses"
        );
        *out_registry = cecs_registry_acquire_mut_seq_unchecked(registry);
        return true;
    }
}

cecs_rwlock_borrow cecs_registry_acquire(cecs_registry *const registry, const cecs_component_registry **const out_registry) {
    const cecs_rwlock_borrow borrow = cecs_rwlock_acquire(&registry->access.access_lock);
    if (cecs_rwlock_borrow_acquired(borrow)) {
        *out_registry = cecs_registry_acquire_seq_unchecked(registry);
    } else {
        *out_registry = NULL;
    }
    return borrow;
}
cecs_rwlock_borrow_mut cecs_registry_acquire_mut(cecs_registry *const registry, cecs_component_registry **const out_registry) {
    const cecs_rwlock_borrow_mut borrow = cecs_rwlock_acquire_mut(&registry->access.access_lock);
    if (cecs_rwlock_borrow_mut_acquired(borrow)) {
        *out_registry = cecs_registry_acquire_mut_seq_unchecked(registry);
    } else {
        *out_registry = NULL;
    }
    return borrow;
}
cecs_rwlock_borrow cecs_registry_acquire_or_exit(cecs_registry *const registry, const cecs_component_registry **const out_registry) {
    const cecs_rwlock_borrow borrow = cecs_rwlock_acquire_or_exit(&registry->access.access_lock);
    *out_registry = cecs_registry_acquire_seq_unchecked(registry);
    return borrow;
}
cecs_rwlock_borrow_mut cecs_registry_acquire_mut_or_exit(cecs_registry *const registry, cecs_component_registry **const out_registry) {
    const cecs_rwlock_borrow_mut borrow = cecs_rwlock_acquire_mut_or_exit(&registry->access.access_lock);
    *out_registry = cecs_registry_acquire_mut_seq_unchecked(registry);
    return borrow;
}

void cecs_registry_release_seq(cecs_registry *const registry) {
    cecs_assert_or_exit(
        registry->access.shared_access_count > 0ull,
        "error: cecs_registry_release_seq called when there were no active shared accesses"
    );
    cecs_assert_or_exit(
        registry->access.last_access_type == cecs_registry_access_type_immutable,
        "error: cecs_registry_release_seq called when the last access type was not immutable"
    );
    --registry->access.shared_access_count;
    if (registry->access.shared_access_count == 0ull) {
        registry->access.last_access_type = cecs_registry_access_type_none;
    }
}
void cecs_registry_release_mut_seq(cecs_registry *const registry) {
    cecs_assert_or_exit(
        registry->access.last_access_type == cecs_registry_access_type_mutable,
        "error: cecs_registry_release_mut_seq called when the last access type was not mutable"
    );
    registry->access.last_access_type = cecs_registry_access_type_none;
}
void cecs_registry_release(cecs_registry *const registry, cecs_rwlock_borrow *const system_borrow) {
    cecs_rwlock_release(&registry->access.access_lock, system_borrow);
    if (system_borrow->new_shared_ref_count == 0ull) {
        cecs_registry_release_seq(registry);
    }
}
void cecs_registry_release_mut(cecs_registry *const registry, cecs_rwlock_borrow_mut *const system_borrow){
    cecs_rwlock_release_mut(&registry->access.access_lock, system_borrow);
    if (system_borrow->previous_ref_count == 0ull) {
        cecs_registry_release_mut_seq(registry);
    }
}
