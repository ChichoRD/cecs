#ifndef CECS_SPARSE_SET_H
#define CECS_SPARSE_SET_H

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <intrin.h>
#include "../types/cecs_macro_utils.h"
#include "cecs_union.h"
#include "cecs_arena.h"
#include "cecs_dynamic_array.h"
#include "cecs_displaced_set.h"


#define CECS_DEREFERENCE_EQUALS(type, element_ref, value) (*((type *)(element_ref)) == (type)(value))
#define CECS_ASSERT_DEREFERENCE_EQUALS(type, element_ref, value) \
    assert(CECS_DEREFERENCE_EQUALS(type, element_ref, value) && "Dereferenced element value does not match the provided value")
#define ASSERT_DEREFERENCE_EQUALS_SIZE_CASE(type, element_ref, value) \
    case sizeof(type): \
        CECS_ASSERT_DEREFERENCE_EQUALS(type, element_ref, value); \
        break

#define _CECS_ASSERT_DEREFERENCE_EQUALS_SIZE_CASE_SELECT(element_ref, value, type) \
    ASSERT_DEREFERENCE_EQUALS_SIZE_CASE(type, element_ref, value)
#define CECS_ASSERT_INTEGER_DEREFERENCE_EQUALS(element_size, element_ref, value) \
    switch (element_size) { \
        CECS_MAP_CONST2( \
            _CECS_ASSERT_DEREFERENCE_EQUALS_SIZE_CASE_SELECT, \
            element_ref, \
            value, \
            CECS_SEMICOLON, \
            uint8_t, uint16_t, uint32_t, uint64_t \
        ); \
        default: \
            assert(false && "Invalid element size for an integer element"); \
            break; \
    }
    

typedef CECS_OPTION_STRUCT(size_t, cecs_dense_index) cecs_dense_index;
typedef CECS_OPTION_STRUCT(void *, cecs_optional_element) cecs_optional_element;

typedef cecs_dynamic_array cecs_any_elements;
typedef cecs_dynamic_array cecs_integer_elements;
typedef CECS_UNION_STRUCT(
    cecs_sparse_set_elements,
    cecs_dynamic_array,
    cecs_any_elements,
    cecs_dynamic_array,
    cecs_integer_elements
) cecs_sparse_set_elements;

typedef struct cecs_sparse_set {
    cecs_sparse_set_elements elements;
    cecs_displaced_set indices;
    cecs_dynamic_array keys;
} cecs_sparse_set;

cecs_sparse_set cecs_sparse_set_create(void);

cecs_sparse_set cecs_sparse_set_create_of_integers(void);

cecs_sparse_set cecs_sparse_set_create_with_capacity(cecs_arena *a, size_t element_capacity, size_t element_size);

cecs_sparse_set cecs_sparse_set_create_of_integers_with_capacity(cecs_arena *a, size_t element_capacity, size_t element_size);

inline void *cecs_sparse_set_data(const cecs_sparse_set *s) {
    return CECS_UNION_GET_UNCHECKED(cecs_any_elements, s->elements).elements;
}


inline size_t *cecs_sparse_set_keys(const cecs_sparse_set *s) {
    return s->keys.elements;
}

inline size_t cecs_sparse_set_count_of_size(const cecs_sparse_set *s, size_t element_size) {
    return cecs_dynamic_array_count_of_size(&CECS_UNION_GET_UNCHECKED(cecs_any_elements, s->elements), element_size);
}

inline bool cecs_sparse_set_contains(const cecs_sparse_set *s, size_t key) {
    return cecs_displaced_set_contains_index(&s->indices, key)
        && CECS_OPTION_IS_SOME(cecs_dense_index, *CECS_DISPLACED_SET_GET(cecs_dense_index, &s->indices, key));
}

cecs_optional_element cecs_sparse_set_get(const cecs_sparse_set *s, size_t key, size_t element_size);
#define CECS_SPARSE_SET_GET(type, sparse_set_ref, key) \
    cecs_sparse_set_get(sparse_set_ref, key, sizeof(type))

void *cecs_sparse_set_get_unchecked(const cecs_sparse_set *s, size_t key, size_t element_size);
#define CECS_SPARSE_SET_GET_UNCHECKED(type, sparse_set_ref, key) \
    ((type *)cecs_sparse_set_get_unchecked(sparse_set_ref, key, sizeof(type)))

bool cecs_sparse_set_is_empty(const cecs_sparse_set *s);

void *cecs_sparse_set_set(cecs_sparse_set *s, cecs_arena *a, size_t key, void *element, size_t element_size);
#define CECS_SPARSE_SET_SET(type, sparse_set_ref, arena_ref, key, element_ref) \
    ((type *)cecs_sparse_set_set(sparse_set_ref, arena_ref, key, element_ref, sizeof(type)))

bool cecs_sparse_set_remove(cecs_sparse_set *s, cecs_arena *a, size_t key, void *out_removed_element, size_t element_size);
#define CECS_SPARSE_SET_REMOVE(type, sparse_set_ref, arena_ref, key, out_removed_element_ref) \
    cecs_sparse_set_remove(sparse_set_ref, arena_ref, key, out_removed_element_ref, sizeof(type))


#define CECS_CECS_PAGED_SPARSE_SET_PAGE_COUNT_LOG2 3
#define CECS_PAGED_SPARSE_SET_PAGE_COUNT (1 << CECS_CECS_PAGED_SPARSE_SET_PAGE_COUNT_LOG2)

#if (SIZE_MAX == 0xFFFF)
    #define CECS_PAGED_SPARSE_SET_KEY_BIT_COUNT_LOG2 4

#elif (SIZE_MAX == 0xFFFFFFFF)
    #define CECS_PAGED_SPARSE_SET_KEY_BIT_COUNT_LOG2 5

#elif (SIZE_MAX == 0xFFFFFFFFFFFFFFFF)
    #define CECS_PAGED_SPARSE_SET_KEY_BIT_COUNT_LOG2 6

#else
    #error TBD code SIZE_T_BITS

#endif
#define CECS_PAGED_SPARSE_SET_KEY_BIT_COUNT (1 << CECS_PAGED_SPARSE_SET_KEY_BIT_COUNT_LOG2)
static_assert(CECS_PAGED_SPARSE_SET_KEY_BIT_COUNT == (sizeof(size_t) * 8), "CECS_PAGED_SPARSE_SET_KEY_BIT_COUNT != sizeof(size_t) * 8");

#define CECS_PAGED_SPARSE_SET_PAGE_SIZE_LOG2 (CECS_PAGED_SPARSE_SET_KEY_BIT_COUNT_LOG2 - CECS_CECS_PAGED_SPARSE_SET_PAGE_COUNT_LOG2)
#define CECS_PAGED_SPARSE_SET_PAGE_SIZE (1 << CECS_PAGED_SPARSE_SET_PAGE_SIZE_LOG2)
typedef struct cecs_paged_sparse_set {
    cecs_sparse_set_elements elements;
    cecs_dynamic_array keys;
    cecs_displaced_set indices[CECS_PAGED_SPARSE_SET_PAGE_COUNT];
} cecs_paged_sparse_set;

inline uint8_t cecs_paged_sparse_set_log2(size_t n) {
#if (SIZE_MAX == 0xFFFF)
    return (uint8_t)(CECS_PAGED_SPARSE_SET_KEY_BIT_COUNT - __lzcnt16((uint16_t)n));

#elif (SIZE_MAX == 0xFFFFFFFF)
    return (uint8_t)(CECS_PAGED_SPARSE_SET_KEY_BIT_COUNT - __lzcnt((uint32_t)n));

#elif (SIZE_MAX == 0xFFFFFFFFFFFFFFFF)
    return (uint8_t)(CECS_PAGED_SPARSE_SET_KEY_BIT_COUNT - __lzcnt64((uint64_t)n));

#else
    #error TBD code SIZE_T_BITS
    return 0;

#endif
}

inline uint8_t cecs_paged_sparse_set_page_index(size_t key) {
    return cecs_paged_sparse_set_log2(key) >> CECS_PAGED_SPARSE_SET_PAGE_SIZE_LOG2;
}

inline size_t cecs_paged_sparse_set_page_key(size_t key, uint8_t page_index) {
    const size_t mask_size = CECS_PAGED_SPARSE_SET_PAGE_SIZE * (size_t)page_index;
    return (key >> mask_size) + (key & (((size_t)1 << mask_size) - 1));
}

cecs_paged_sparse_set cecs_paged_sparse_set_create(void);

cecs_paged_sparse_set cecs_paged_sparse_set_create_of_integers(void);

cecs_paged_sparse_set cecs_paged_sparse_set_create_with_capacity(cecs_arena *a, size_t element_capacity, size_t element_size);

cecs_paged_sparse_set cecs_paged_sparse_set_create_of_integers_with_capacity(cecs_arena *a, size_t element_capacity, size_t element_size);

inline void *cecs_paged_sparse_set_data(const cecs_paged_sparse_set *s) {
    return CECS_UNION_GET_UNCHECKED(cecs_any_elements, s->elements).elements;
}

inline size_t *cecs_paged_sparse_set_keys(const cecs_paged_sparse_set *s) {
    return s->keys.elements;
}

inline size_t cecs_paged_sparse_set_count_of_size(const cecs_paged_sparse_set *s, size_t element_size) {
    return cecs_dynamic_array_count_of_size(&CECS_UNION_GET_UNCHECKED(cecs_any_elements, s->elements), element_size);
}

inline bool cecs_paged_sparse_set_contains(const cecs_paged_sparse_set *s, size_t key) {
    uint8_t page_index = cecs_paged_sparse_set_page_index(key);
    const cecs_displaced_set *indices = &s->indices[page_index];
    size_t page_key = cecs_paged_sparse_set_page_key(key, page_index);
    return cecs_displaced_set_contains_index(indices, page_key)
        && CECS_OPTION_IS_SOME(cecs_dense_index, *CECS_DISPLACED_SET_GET(cecs_dense_index, indices, page_key));
}

cecs_optional_element cecs_paged_sparse_set_get(const cecs_paged_sparse_set *s, size_t key, size_t element_size);
#define CECS_PAGED_SPARSE_SET_GET(type, paged_sparse_set_ref, key) \
    cecs_paged_sparse_set_get(paged_sparse_set_ref, key, sizeof(type))

void *cecs_paged_sparse_set_get_unchecked(const cecs_paged_sparse_set *s, size_t key, size_t element_size);
#define CECS_PAGED_SPARSE_SET_GET_UNCHECKED(type, paged_sparse_set_ref, key) \
    ((type *)cecs_paged_sparse_set_get_unchecked(paged_sparse_set_ref, key, sizeof(type)))

bool cecs_paged_sparse_set_is_empty(const cecs_paged_sparse_set *s);

void *cecs_paged_sparse_set_set(cecs_paged_sparse_set *s, cecs_arena *a, size_t key, void *element, size_t element_size);
#define CECS_PAGED_SPARSE_SET_SET(type, paged_sparse_set_ref, arena_ref, key, element_ref) \
    ((type *)cecs_paged_sparse_set_set(paged_sparse_set_ref, arena_ref, key, element_ref, sizeof(type)))

bool cecs_paged_sparse_set_remove(cecs_paged_sparse_set *s, cecs_arena *a, size_t key, void *out_removed_element, size_t element_size);
#define CECS_PAGED_SPARSE_SET_REMOVE(type, paged_sparse_set_ref, arena_ref, key, out_removed_element_ref) \
    cecs_paged_sparse_set_remove(paged_sparse_set_ref, arena_ref, key, out_removed_element_ref, sizeof(type))


#endif