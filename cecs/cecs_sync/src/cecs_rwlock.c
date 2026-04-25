#include "cecs_rwlock.h"
#include <assert.h>
#include <limits.h>
#include <cecs_error.h>
#include <cecs_type_traits.h>
#include <relations/cecs_ordering.h>

static_assert(
    sizeof(UINT64_MAX) * CHAR_BIT == 64u,
    "error: expected UINT64_MAX to be 64 bits, but it is not, "
    "which means the implementation of cecs_rwlock may not work correctly because it relies on the assumption "
    "that a 64-bit unsigned integer can be used to represent the state of the lock "
    "with a certain number of bits for the reader count and a bit for the mutable lock"
);
// #define CECS_RWLOCK_VALUE_MAX ((1ull << CECS_RWLOCK_VALUE_TYPE_BITS) - 1ull)
#define CECS_RWLOCK_VALUE_MAX (UINT64_MAX >> (64ull - CECS_RWLOCK_VALUE_TYPE_BITS))
#define CECS_RWLOCK_VALUE_MUTABLE_LOCK_MASK (~((cecs_rwlock_value)(CECS_RWLOCK_VALUE_MAX >> 1ull)))
#define CECS_RWLOCK_VALUE_IMMUTABLE_ACQUIRE_FATAL_MAX (CECS_RWLOCK_VALUE_MUTABLE_LOCK_MASK | (CECS_RWLOCK_VALUE_MUTABLE_LOCK_MASK >> 1ull))


static inline cecs_rwlock_borrow cecs_rwlock_borrow_from_count(const cecs_rwlock_value count) {
    cecs_debugbreak_fail_unless(
        count != 0ull,
        "fatal error: attempted to create a cecs_rwlock_borrow with an invalid count of zero new shared references"
    );
    return (cecs_rwlock_borrow){ .new_shared_ref_count = count };
}
static inline cecs_rwlock_borrow_mut cecs_rwlock_borrow_mut_from_count(const cecs_rwlock_value count) {
    return (cecs_rwlock_borrow_mut){ .previous_ref_count = count };
}
extern inline bool cecs_rwlock_borrow_is_mutably_locked(const cecs_rwlock_borrow borrow) {
    return borrow.new_shared_ref_count == CECS_RWLOCK_VALUE_MUTABLE_LOCK_MASK;
}
extern inline bool cecs_rwlock_borrow_acquired(const cecs_rwlock_borrow borrow) {
    return borrow.new_shared_ref_count < CECS_RWLOCK_VALUE_MUTABLE_LOCK_MASK;
}
extern inline void cecs_rwlock_borrow_release(cecs_rwlock_borrow *borrow) {
    borrow->new_shared_ref_count = CECS_RWLOCK_VALUE_MUTABLE_LOCK_MASK;
}
extern inline bool cecs_rwlock_borrow_mut_is_immutably_locked(const cecs_rwlock_borrow_mut borrow) {
    return (borrow.previous_ref_count & (~CECS_RWLOCK_VALUE_MUTABLE_LOCK_MASK)) != 0;
}
extern inline bool cecs_rwlock_borrow_mut_is_mutably_locked(const cecs_rwlock_borrow_mut borrow) {
    return borrow.previous_ref_count == CECS_RWLOCK_VALUE_MUTABLE_LOCK_MASK;
}
extern inline bool cecs_rwlock_borrow_mut_acquired(const cecs_rwlock_borrow_mut borrow) {
    return borrow.previous_ref_count == 0ull;
}
extern inline void cecs_rwlock_borrow_mut_release(cecs_rwlock_borrow_mut *borrow) {
    borrow->previous_ref_count = CECS_RWLOCK_VALUE_IMMUTABLE_ACQUIRE_FATAL_MAX;
}


extern inline cecs_rwlock cecs_rwlock_create(void);
extern inline void cecs_rwlock_reset(cecs_rwlock *const lock);

static void cecs_rwlock_acquire_overflow_check(cecs_rwlock *const lock, const cecs_rwlock_value new_readers) {
    if (cecs_expect_not(new_readers == CECS_RWLOCK_VALUE_MUTABLE_LOCK_MASK)) {
        atomic_fetch_sub_explicit(&lock->state, 1ull, memory_order_release);
        cecs_debugbreak_fail_message(
            "fatal error: cecs_rwlock_acquire failed to acquire read lock because the maximum number of concurrent readers has been reached"
        );
    } else if (cecs_expect_not(new_readers >= CECS_RWLOCK_VALUE_IMMUTABLE_ACQUIRE_FATAL_MAX)) {
        cecs_debugbreak_fail_message(
            "fatal error: cecs_rwlock_acquire failed to acquire read lock because after the lock was mutably acquired, "
            "the maximum number of tries for the readers to acquire the lock has been reached"
        );
    }
}
cecs_rwlock_borrow cecs_rwlock_acquire(cecs_rwlock *const lock) {
    const cecs_rwlock_value new_readers = atomic_fetch_add_explicit(&lock->state, 1ull, memory_order_acquire) + 1ull;
    if ((new_readers & CECS_RWLOCK_VALUE_MUTABLE_LOCK_MASK) == 0ull) {
        return cecs_rwlock_borrow_from_count(new_readers);
    } else {
        cecs_rwlock_acquire_overflow_check(lock, new_readers);
        return cecs_rwlock_borrow_from_count(CECS_RWLOCK_VALUE_MUTABLE_LOCK_MASK);
    }
}
cecs_rwlock_borrow_mut cecs_rwlock_acquire_mut(cecs_rwlock *const lock) {
    cecs_rwlock_value expected = 0ull;
    if (atomic_compare_exchange_weak_explicit(&lock->state, &expected, CECS_RWLOCK_VALUE_MUTABLE_LOCK_MASK, memory_order_acquire, memory_order_relaxed)) {
        return cecs_rwlock_borrow_mut_from_count(0ull);
    } else {
        return cecs_rwlock_borrow_mut_from_count(expected);
    }
}

cecs_rwlock_borrow cecs_rwlock_acquire_or_exit(cecs_rwlock *const lock) {
    const cecs_rwlock_borrow borrow = cecs_rwlock_acquire(lock);
    if (cecs_expect_not(!cecs_rwlock_borrow_acquired(borrow))) {
        if (cecs_rwlock_borrow_is_mutably_locked(borrow)) {
            cecs_debugbreak_fail_message(
                "error: failed to acquire cecs_rwlock read lock, "
                "lock is mutably locked by another thread"
            );
        } else {
            cecs_debugbreak_fail_message(
                "error: failed to acquire cecs_rwlock read lock due to an unknown error"
            );
        }
    }
    return borrow;
}
cecs_rwlock_borrow_mut cecs_rwlock_acquire_mut_or_exit(cecs_rwlock *const lock) {
    const cecs_rwlock_borrow_mut borrow = cecs_rwlock_acquire_mut(lock);
    if (cecs_expect_not(!cecs_rwlock_borrow_mut_acquired(borrow))) {
        if (cecs_rwlock_borrow_mut_is_immutably_locked(borrow)) {
            cecs_debugbreak_fail_message(
                "error: failed to acquire cecs_rwlock write lock, "
                "lock is immutably locked by other threads"
            );
        } else if (cecs_rwlock_borrow_mut_is_mutably_locked(borrow)) {
            cecs_debugbreak_fail_message(
                "error: failed to acquire cecs_rwlock write lock, "
                "lock is mutably locked by another thread"
            );
        } else {
            cecs_debugbreak_fail_message(
                "error: failed to acquire cecs_rwlock write lock due to an unknown error"
            );
        }
    }
    return borrow;
}

void cecs_rwlock_release(cecs_rwlock *const lock, cecs_rwlock_borrow *const borrow) {
    cecs_debugbreak_fail_unless(
        cecs_rwlock_borrow_acquired(*borrow),
        "error: attempted to release a cecs_rwlock read lock that was not successfully acquired"
    );
    static_assert(
        CECS_IS_SAME_TYPE(cecs_rwlock_value, size_t),
        "static error: CECS_RWLOCK_VALUE_TYPE must be the same type as size_t for the atomic_fetch_sub_explicit call in cecs_rwlock_release to work correctly on MSVC"
    );
    const size_t sub = cecs_min(borrow->new_shared_ref_count, 1ull);
    const size_t previous_count = atomic_fetch_sub_explicit(&lock->state, sub, memory_order_release);
    cecs_rwlock_borrow_release(borrow);
    (void)previous_count;
}
void cecs_rwlock_release_mut(cecs_rwlock *const lock, cecs_rwlock_borrow_mut *const borrow) {
    cecs_debugbreak_fail_unless(
        cecs_rwlock_borrow_mut_acquired(*borrow),
        "error: attempted to release a cecs_rwlock write lock that was not successfully acquired"
    );
    static_assert(
        CECS_IS_SAME_TYPE(cecs_rwlock_value, size_t),
        "static error: CECS_RWLOCK_VALUE_TYPE must be the same type as size_t for the atomic_store_explicit call in cecs_rwlock_release_mut to work correctly on MSVC"
    );
    atomic_store_explicit(&lock->state, borrow->previous_ref_count, memory_order_release);
    cecs_rwlock_borrow_mut_release(borrow);
}
