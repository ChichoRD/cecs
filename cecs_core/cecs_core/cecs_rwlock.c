#include "cecs_rwlock.h"
#include "cecs_error.h"
#include <cecs_math/relations/cecs_ordering.h>

static inline cecs_rwlock_guard cecs_rwlock_guard_from_count(const cecs_rwlock_value count) {
    return (cecs_rwlock_guard){ .new_reader_count = count };
}
static inline cecs_rwlock_guard_mut cecs_rwlock_guard_mut_from_count(const cecs_rwlock_value count) {
    return (cecs_rwlock_guard_mut){ .new_writer_count = count };
}
extern inline bool cecs_rwlock_guard_acquired(const cecs_rwlock_guard guard);
extern inline bool cecs_rwlock_guard_mut_acquired(const cecs_rwlock_guard_mut guard);


extern inline cecs_rwlock cecs_rwlock_create(void);
extern inline void cecs_rwlock_reset(cecs_rwlock *const lock);
cecs_rwlock_guard cecs_rwlock_acquire(cecs_rwlock *const lock) {
    cecs_rwlock_value expected = 0ull;
    if (atomic_compare_exchange_strong(&lock->counter, &expected, 1ull)) {
        atomic_flag_test_and_set(&lock->immutable);
        return cecs_rwlock_guard_from_count(1ull);
    } else if (atomic_flag_test_and_set(&lock->immutable)) {
        return cecs_rwlock_guard_from_count(atomic_fetch_add(&lock->counter, 1ull) + 1ull);
    } else {
        atomic_flag_clear(&lock->immutable);
        return cecs_rwlock_guard_from_count(0ull);
    }
}
cecs_rwlock_guard_mut cecs_rwlock_acquire_mut(cecs_rwlock *const lock) {
    cecs_rwlock_value expected = 0ull;
    const bool was_immutable = atomic_flag_test_and_set(&lock->immutable);
    atomic_flag_clear(&lock->immutable);
    if (atomic_compare_exchange_strong(&lock->counter, &expected, 1ull)) {
        return cecs_rwlock_guard_mut_from_count(1ull);
    } else if (was_immutable) {
        atomic_flag_test_and_set(&lock->immutable);
        return cecs_rwlock_guard_mut_from_count(0ull);
    } else {
        return cecs_rwlock_guard_mut_from_count(0ull);
    }
}
void cecs_rwlock_release(cecs_rwlock *const lock, cecs_rwlock_guard *const guard) {
    const size_t sub = cecs_min(guard->new_reader_count, 1ull);
    const size_t previous_count = atomic_fetch_sub(&lock->counter, sub);
    guard->new_reader_count = previous_count - sub;
}
void cecs_rwlock_release_mut(cecs_rwlock *const lock, cecs_rwlock_guard_mut *const guard) {
    if (atomic_compare_exchange_strong(&lock->counter, &guard->new_writer_count, 0ull)) {
        guard->new_writer_count = 0ull;
    } else if (cecs_rwlock_guard_mut_acquired(*guard)) {
        cecs_assert_unreachable(
            "fatal error: cecs_rwlock_release_mut failed to release the write lock because the lock state was modified externally"
        );
    }
}
