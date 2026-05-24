#include "cecs_sparse_set_storage.h"

#ifndef CECS_SPARSE_SET_STORAGE_PAGE_SIZE_LOG2
#define CECS_SPARSE_SET_STORAGE_PAGE_SIZE_LOG2_DEFAULT 4ull
#define CECS_SPARSE_SET_STORAGE_PAGE_SIZE_LOG2 CECS_SPARSE_SET_STORAGE_PAGE_SIZE_LOG2_DEFAULT

#endif
#define CECS_SPARSE_SET_STORAGE_PAGE_SIZE (1ull << CECS_SPARSE_SET_STORAGE_PAGE_SIZE_LOG2)

static inline size_t cecs_sparse_set_storage_key_page(const size_t key) {
    return key >> CECS_SPARSE_SET_STORAGE_PAGE_SIZE_LOG2;
}
extern inline size_t cecs_sparse_set_storage_map_offset(const cecs_sparse_set_storage *storage) {
    return storage->skipped_key_pages << CECS_SPARSE_SET_STORAGE_PAGE_SIZE_LOG2;
}
extern inline size_t cecs_sparse_set_storage_map_key(const cecs_sparse_set_storage *storage, const size_t key) {
    const size_t offset = cecs_sparse_set_storage_map_offset(storage);
    cecs_debugbreak_fail_unless(
        key >= offset,
        "error: cecs_sparse_set_storage_map_key called with key in skipped range"
    );
    return key - offset;
}

extern inline cecs_sparse_set_storage cecs_sparse_set_storage_create(void);
extern inline cecs_sparse_set_storage cecs_sparse_set_storage_create_with_capacity(cecs_allocator *allocator, const size_t capacity, const size_t value_size);

extern inline void cecs_sparse_set_storage_clear(cecs_sparse_set_storage *storage);
extern inline void cecs_sparse_set_storage_destroy(cecs_sparse_set_storage *storage, cecs_allocator *allocator, const size_t value_size);

const void *cecs_sparse_set_storage_get(const cecs_sparse_set_storage *storage, const size_t key, const size_t value_size) {
    const size_t mapped_key = cecs_sparse_set_storage_map_key(storage, key);
    return cecs_sparse_set_get_value(&storage->set, mapped_key, value_size);
}
void *cecs_sparse_set_storage_get_mut(cecs_sparse_set_storage *storage, const size_t key, const size_t value_size) {
    const size_t mapped_key = cecs_sparse_set_storage_map_key(storage, key);
    return cecs_sparse_set_get_value_mut(&storage->set, mapped_key, value_size);
}


extern size_t *cecs_sparse_set_get_sparse_key_by_index_mut(cecs_sparse_set *set, const cecs_dense_index index);
static void cecs_sparse_set_storage_ensure_key_page(cecs_sparse_set_storage *storage, cecs_allocator *allocator, const size_t key_page) {
    cecs_debugbreak_fail_unless(
        key_page < storage->skipped_key_pages,
        "error: cecs_sparse_set_storage_ensure_key_page called with key_page less or equal than skipped_key_pages"
    );
    const size_t needed_pages = storage->skipped_key_pages - key_page;
    const size_t needed_keys = needed_pages << CECS_SPARSE_SET_STORAGE_PAGE_SIZE_LOG2;

    cecs_sparse_set_upsize_sparse_range_exact(&storage->set, allocator, needed_keys + cecs_sparse_set_sparse_range_size(&storage->set));
    static_assert(
        sizeof(cecs_sparse_set_usize) <= sizeof(size_t),
        "static error: cecs_sparse_set_storage_ensure_key_page assumes that cecs_sparse_set_usize can represent all possible key indices"
    );
    for (size_t i = 0; i < cecs_sparse_set_value_count(&storage->set); ++i) {
        const cecs_dense_index index = cecs_dense_index_create_unchecked((cecs_sparse_set_usize)i);
        
        size_t *sparse_key = cecs_sparse_set_get_sparse_key_by_index_mut(&storage->set, index);
        cecs_dense_index *invalid_index = cecs_sparse_set_get_index_mut(&storage->set, *sparse_key);
        *invalid_index = cecs_dense_index_create_invalid();

        *sparse_key += needed_keys;
        cecs_dense_index *new_index = cecs_sparse_set_get_index_mut(&storage->set, *sparse_key);
        *new_index = index;
    }
    storage->skipped_key_pages = key_page;
}
void *cecs_sparse_set_storage_get_or_insert(cecs_sparse_set_storage *storage, cecs_allocator *allocator, const size_t key, const size_t value_size) {
    const size_t key_page = cecs_sparse_set_storage_key_page(key);
    if (storage->skipped_key_pages == SIZE_MAX) {
        storage->skipped_key_pages = key_page;
    }
    if (key_page < storage->skipped_key_pages) {
        cecs_sparse_set_storage_ensure_key_page(storage, allocator, key_page);
        const size_t mapped_key = cecs_sparse_set_storage_map_key(storage, key);
        return cecs_sparse_set_insert_expect(&storage->set, allocator, mapped_key, value_size);
    } else {
        const size_t mapped_key = cecs_sparse_set_storage_map_key(storage, key);
        return cecs_sparse_set_get_or_insert(&storage->set, allocator, mapped_key, value_size);
    }
}
void *cecs_sparse_set_storage_insert_expect(cecs_sparse_set_storage *storage, cecs_allocator *allocator, const size_t key, const size_t value_size) {
    const size_t key_page = cecs_sparse_set_storage_key_page(key);
    if (storage->skipped_key_pages == SIZE_MAX) {
        storage->skipped_key_pages = key_page;
    }
    if (key_page < storage->skipped_key_pages) {
        cecs_sparse_set_storage_ensure_key_page(storage, allocator, key_page);
        const size_t mapped_key = cecs_sparse_set_storage_map_key(storage, key);
        return cecs_sparse_set_insert_expect(&storage->set, allocator, mapped_key, value_size);
    } else {
        const size_t mapped_key = cecs_sparse_set_storage_map_key(storage, key);
        return cecs_sparse_set_insert_expect(&storage->set, allocator, mapped_key, value_size);
    }
}

bool cecs_sparse_set_storage_remove(cecs_sparse_set_storage *storage, const size_t key, const size_t value_size) {
    const size_t key_offset = cecs_sparse_set_storage_map_offset(storage);
    if (key < key_offset) {
        return false;
    } else {
        const size_t mapped_key = key - key_offset;
        return cecs_sparse_set_remove(&storage->set, mapped_key, value_size);
    }
}
void cecs_sparse_set_storage_remove_expect(cecs_sparse_set_storage *storage, const size_t key, const size_t value_size) {
    const size_t mapped_key = cecs_sparse_set_storage_map_key(storage, key);
    cecs_sparse_set_remove_expect(&storage->set, mapped_key, value_size);
}

bool cecs_sparse_set_storage_contains(const cecs_sparse_set_storage *storage, const size_t key) {
    const size_t key_offset = cecs_sparse_set_storage_map_offset(storage);
    if (key < key_offset) {
        return false;
    } else {
        const size_t mapped_key = key - key_offset;
        return cecs_sparse_set_contains(&storage->set, mapped_key);
    }
}

extern inline size_t cecs_sparse_set_storage_count(const cecs_sparse_set_storage *const storage);