#ifndef CECS_SPARSE_SET_STORAGE_H
#define CECS_SPARSE_SET_STORAGE_H

#include <cecs_sparse_set.h>

typedef struct cecs_sparse_set_storage {
    cecs_sparse_set set;
    size_t skipped_key_pages;
} cecs_sparse_set_storage;

size_t cecs_sparse_set_storage_map_offset(const cecs_sparse_set_storage *storage);
size_t cecs_sparse_set_storage_map_key(const cecs_sparse_set_storage *storage, const size_t key);

inline cecs_sparse_set_storage cecs_sparse_set_storage_create(void) {
    return (cecs_sparse_set_storage){
        .set = cecs_sparse_set_create(),
        .skipped_key_pages = SIZE_MAX,
    };
}
inline cecs_sparse_set_storage cecs_sparse_set_storage_create_with_capacity(cecs_allocator *allocator, const size_t capacity, const size_t value_size) {
    return (cecs_sparse_set_storage){
        .set = cecs_sparse_set_create_with_capacity(allocator, capacity, value_size),
        .skipped_key_pages = SIZE_MAX,
    };
}

inline void cecs_sparse_set_storage_clear(cecs_sparse_set_storage *storage) {
    storage->skipped_key_pages = SIZE_MAX;
    cecs_sparse_set_clear(&storage->set);
}
inline void cecs_sparse_set_storage_destroy(cecs_sparse_set_storage *storage, cecs_allocator *allocator, const size_t value_size) {
    storage->skipped_key_pages = SIZE_MAX;
    cecs_sparse_set_destroy(&storage->set, allocator, value_size);
}

const void *cecs_sparse_set_storage_get(const cecs_sparse_set_storage *storage, const size_t key, const size_t value_size);
void *cecs_sparse_set_storage_get_mut(cecs_sparse_set_storage *storage, const size_t key, const size_t value_size);

void *cecs_sparse_set_storage_get_or_insert(cecs_sparse_set_storage *storage, cecs_allocator *allocator, const size_t key, const size_t value_size);
void *cecs_sparse_set_storage_insert_expect(cecs_sparse_set_storage *storage, cecs_allocator *allocator, const size_t key, const size_t value_size);

bool cecs_sparse_set_storage_remove(cecs_sparse_set_storage *const storage, const size_t key, const size_t value_size);
void cecs_sparse_set_storage_remove_expect(cecs_sparse_set_storage *const storage, const size_t key, const size_t value_size);

bool cecs_sparse_set_storage_contains(const cecs_sparse_set_storage *storage, const size_t key);

#endif
