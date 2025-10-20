#ifndef CECS_RWLOCK_H
#define CECS_RWLOCK_H

#include <assert.h>
#include <stdbool.h>
#include <stdatomic.h>


#ifndef CECS_RWLOCK_VALUE_TYPE
#define CECS_RWLOCK_VALUE_TYPE_DEFAULT uint32_t
#define CECS_RWLOCK_VALUE_TYPE CECS_RWLOCK_VALUE_TYPE_DEFAULT

#define CECS_RWLOCK_VALUE_TYPE_BITS_LOG2 5ull
#endif

#ifndef CECS_RWLOCK_VALUE_TYPE_BITS_LOG2 
static_assert(
    false,
    "error: CECS_RWLOCK_VALUE_TYPE_BITS_LOG2 must be defined to the base-2 logarithm of the number of bits in the CECS_RWLOCK_VALUE_TYPE"
);
#endif
#define CECS_RWLOCK_VALUE_TYPE_BITS (1 << CECS_RWLOCK_VALUE_TYPE_BITS_LOG2)


typedef CECS_RWLOCK_VALUE_TYPE cecs_rwlock_value;
static_assert(
    sizeof(cecs_rwlock_value) * 8 == CECS_RWLOCK_VALUE_TYPE_BITS,
    "error: CECS_RWLOCK_VALUE_TYPE_BITS does not match the size of CECS_RWLOCK_VALUE_TYPE"
);
typedef struct cecs_rwlock_borrow {
    cecs_rwlock_value new_shared_ref_count;
} cecs_rwlock_borrow;
typedef struct cecs_rwlock_borrow_mut {
    cecs_rwlock_value previous_ref_count;
} cecs_rwlock_borrow_mut;
bool cecs_rwlock_borrow_is_mutably_locked(const cecs_rwlock_borrow borrow);
bool cecs_rwlock_borrow_acquired(const cecs_rwlock_borrow borrow);
bool cecs_rwlock_borrow_mut_is_immutably_locked(const cecs_rwlock_borrow_mut borrow);
bool cecs_rwlock_borrow_mut_is_mutably_locked(const cecs_rwlock_borrow_mut borrow);
bool cecs_rwlock_borrow_mut_acquired(const cecs_rwlock_borrow_mut borrow);


// TODO: maybe move to sync/ directory
// source: https://docs.rs/atomic_refcell/latest/atomic_refcell/
typedef struct cecs_rwlock {
    _Atomic cecs_rwlock_value state;
} cecs_rwlock;
inline void cecs_rwlock_reset(cecs_rwlock *const lock) {
    atomic_store(&lock->state, 0ull);
}
inline cecs_rwlock cecs_rwlock_create(void) {
    cecs_rwlock lock;
    cecs_rwlock_reset(&lock);
    return lock;
}
cecs_rwlock_borrow cecs_rwlock_acquire(cecs_rwlock *const lock);
cecs_rwlock_borrow_mut cecs_rwlock_acquire_mut(cecs_rwlock *const lock);

cecs_rwlock_borrow cecs_rwlock_acquire_or_exit(cecs_rwlock *const lock);
cecs_rwlock_borrow_mut cecs_rwlock_acquire_mut_or_exit(cecs_rwlock *const lock);

void cecs_rwlock_release(cecs_rwlock *const lock, cecs_rwlock_borrow *const borrow);
void cecs_rwlock_release_mut(cecs_rwlock *const lock, cecs_rwlock_borrow_mut *const borrow);

#endif
