#include "cecs_component_storage.h"

extern inline cecs_component_storage cecs_component_storage_create_from(const cecs_internal_component_storage storage, const cecs_component_storage_type type) {
    return (cecs_component_storage){
        .storage = storage,
        .type = type
    };
}

extern inline const cecs_sparse_set_storage *cecs_component_storage_sparse_set(const cecs_component_storage *const storage);
extern inline cecs_sparse_set_storage *cecs_component_storage_sparse_set_mut(cecs_component_storage *const storage);

cecs_component_storage cecs_component_storage_create_sparse_set(cecs_allocator *const allocator, const size_t component_capacity, const size_t component_size) {
    return cecs_component_storage_create_from(
        (cecs_internal_component_storage){
            .sparse_set = cecs_sparse_set_storage_create_with_capacity(allocator, component_capacity, component_size)
        },
        cecs_component_storage_type_sparse_set
    );
}

#define CECS_COMPONENT_STORAGE_TYPE_MAX cecs_component_storage_type_sparse_set
void cecs_component_storage_clear(cecs_component_storage *const storage, cecs_allocator *const allocator, const size_t component_size) {
    (void)allocator;
    (void)component_size;
    cecs_expect_not(storage->type == cecs_component_storage_type_none);
    cecs_expect_not(storage->type > CECS_COMPONENT_STORAGE_TYPE_MAX);
    switch (storage->type) {
    case cecs_component_storage_type_none:
        cecs_debugbreak_fail_message(
            "fatal error: attempted to clear cecs_component_storage of type 'none'"
        );
    case cecs_component_storage_type_sparse_set:
        cecs_sparse_set_storage_clear(
            &storage->storage.sparse_set
        );
        break;
    default:
        cecs_debugbreak_fail_message(
            "fatal error: attempted to clear cecs_component_storage of unsupported type"
        );
    }
}
void cecs_component_storage_destroy(cecs_component_storage *const storage, cecs_allocator *const allocator, const size_t component_size) {
    cecs_expect_not(storage->type == cecs_component_storage_type_none);
    cecs_expect_not(storage->type > CECS_COMPONENT_STORAGE_TYPE_MAX);
    switch (storage->type) {
    case cecs_component_storage_type_none:
        cecs_debugbreak_fail_message(
            "fatal error: attempted to destroy cecs_component_storage of type 'none'"
        );
    case cecs_component_storage_type_sparse_set:
        cecs_sparse_set_storage_destroy(
            &storage->storage.sparse_set,
            allocator,
            component_size
        );
        break;
    default:
        cecs_debugbreak_fail_message(
            "fatal error: attempted to destroy cecs_component_storage of unsupported type"
        );
    }
}

const void *cecs_component_storage_get(const cecs_component_storage *const storage, const size_t key, const size_t component_size) {
    cecs_expect_not(storage->type == cecs_component_storage_type_none);
    cecs_expect_not(storage->type > CECS_COMPONENT_STORAGE_TYPE_MAX);
    switch (storage->type) {
    case cecs_component_storage_type_none:
        cecs_debugbreak_fail_message(
            "fatal error: attempted to get from cecs_component_storage of type 'none'"
        );
    case cecs_component_storage_type_sparse_set:
        return cecs_sparse_set_storage_get(
            &storage->storage.sparse_set,
            key,
            component_size
        );
    default:
        cecs_debugbreak_fail_message(
            "fatal error: attempted to get from cecs_component_storage of unsupported type"
        );
    }
}
void *cecs_component_storage_get_mut(cecs_component_storage *const storage, const size_t key, const size_t component_size) {
    cecs_expect_not(storage->type == cecs_component_storage_type_none);
    cecs_expect_not(storage->type > CECS_COMPONENT_STORAGE_TYPE_MAX);
    switch (storage->type) {
    case cecs_component_storage_type_none:
        cecs_debugbreak_fail_message(
            "fatal error: attempted to get_mut from cecs_component_storage of type 'none'"
        );
    case cecs_component_storage_type_sparse_set:
        return cecs_sparse_set_storage_get_mut(
            &storage->storage.sparse_set,
            key,
            component_size
        );
    default:
        cecs_debugbreak_fail_message(
            "fatal error: attempted to get_mut from cecs_component_storage of unsupported type"
        );
    }
}

void *cecs_component_storage_get_or_insert(cecs_component_storage *const storage, cecs_allocator *const allocator, const size_t key, const size_t component_size) {
    cecs_expect_not(storage->type == cecs_component_storage_type_none);
    cecs_expect_not(storage->type > CECS_COMPONENT_STORAGE_TYPE_MAX);
    switch (storage->type) {
    case cecs_component_storage_type_none:
        cecs_debugbreak_fail_message(
            "fatal error: attempted to get_or_insert from cecs_component_storage of type 'none'"
        );
    case cecs_component_storage_type_sparse_set:
        return cecs_sparse_set_storage_get_or_insert(
            &storage->storage.sparse_set,
            allocator,
            key,
            component_size
        );
    default:
        cecs_debugbreak_fail_message(
            "fatal error: attempted to get_or_insert from cecs_component_storage of unsupported type"
        );
    }
}
void *cecs_component_storage_insert_expect(cecs_component_storage *const storage, cecs_allocator *const allocator, const size_t key, const size_t component_size) {
    cecs_expect_not(storage->type == cecs_component_storage_type_none);
    cecs_expect_not(storage->type > CECS_COMPONENT_STORAGE_TYPE_MAX);
    switch (storage->type) {
    case cecs_component_storage_type_none:
        cecs_debugbreak_fail_message(
            "fatal error: attempted to insert_expect into cecs_component_storage of type 'none'"
        );
    case cecs_component_storage_type_sparse_set:
        return cecs_sparse_set_storage_insert_expect(
            &storage->storage.sparse_set,
            allocator,
            key,
            component_size
        );
    default:
        cecs_debugbreak_fail_message(
            "fatal error: attempted to insert_expect into cecs_component_storage of unsupported type"
        );
    }
}

bool cecs_component_storage_remove(cecs_component_storage *const storage, cecs_allocator *const allocator, const size_t key, const size_t component_size) {
    cecs_expect_not(storage->type == cecs_component_storage_type_none);
    cecs_expect_not(storage->type > CECS_COMPONENT_STORAGE_TYPE_MAX);
    switch (storage->type) {
    case cecs_component_storage_type_none:
        cecs_debugbreak_fail_message(
            "fatal error: attempted to remove from cecs_component_storage of type 'none'"
        );
    case cecs_component_storage_type_sparse_set:
        (void)allocator;
        return cecs_sparse_set_storage_remove(
            &storage->storage.sparse_set,
            key,
            component_size
        );
    default:
        cecs_debugbreak_fail_message(
            "fatal error: attempted to remove from cecs_component_storage of unsupported type"
        );
    }
}
void cecs_component_storage_remove_expect(cecs_component_storage *const storage, cecs_allocator *const allocator, const size_t key, const size_t component_size) {
    cecs_expect_not(storage->type == cecs_component_storage_type_none);
    cecs_expect_not(storage->type > CECS_COMPONENT_STORAGE_TYPE_MAX);
    switch (storage->type) {
    case cecs_component_storage_type_none:
        cecs_debugbreak_fail_message(
            "fatal error: attempted to remove_expect from cecs_component_storage of type 'none'"
        );
    case cecs_component_storage_type_sparse_set:
        (void)allocator;
        cecs_sparse_set_storage_remove_expect(
            &storage->storage.sparse_set,
            key,
            component_size
        );
        break;
    default:
        cecs_debugbreak_fail_message(
            "fatal error: attempted to remove_expect from cecs_component_storage of unsupported type"
        );
    }
}

bool cecs_component_storage_contains(const cecs_component_storage *const storage, const size_t key, const size_t component_size) {
    (void)component_size;
    cecs_expect_not(storage->type == cecs_component_storage_type_none);
    cecs_expect_not(storage->type > CECS_COMPONENT_STORAGE_TYPE_MAX);
    switch (storage->type) {
    case cecs_component_storage_type_none:
        cecs_debugbreak_fail_message(
            "fatal error: attempted to check contains on cecs_component_storage of type 'none'"
        );
    case cecs_component_storage_type_sparse_set:
        return cecs_sparse_set_storage_contains(
            &storage->storage.sparse_set,
            key
        );
    default:
        cecs_debugbreak_fail_message(
            "fatal error: attempted to check contains on cecs_component_storage of unsupported type"
        );
    }
}

size_t cecs_component_storage_count(const cecs_component_storage *const storage) {
    cecs_expect_not(storage->type == cecs_component_storage_type_none);
    cecs_expect_not(storage->type > CECS_COMPONENT_STORAGE_TYPE_MAX);
    // const cecs_component_storage_type type = storage->type;
    switch (storage->type) {
    case cecs_component_storage_type_none:
        cecs_debugbreak_fail_message(
            "fatal error: attempted to count cecs_component_storage of type 'none'"
        );
    case cecs_component_storage_type_sparse_set:
        return cecs_sparse_set_storage_count(
            &storage->storage.sparse_set
        );
    default:
        cecs_debugbreak_fail_message(
            "fatal error: attempted to count cecs_component_storage of unsupported type"
        );
    }
}
