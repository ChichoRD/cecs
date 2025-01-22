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
    cecs_any_elements,
    cecs_any_elements,
    cecs_integer_elements,
    cecs_integer_elements
) cecs_sparse_set_elements;

typedef cecs_dynamic_array cecs_sparse_set_index_to_key;
typedef cecs_displaced_set cecs_sparse_set_key_to_index;

typedef struct cecs_sparse_set_base {
    cecs_sparse_set_elements values;
    cecs_sparse_set_index_to_key index_to_key;
} cecs_sparse_set_base;

static inline bool cecs_sparse_set_base_is_of_integers(const cecs_sparse_set_base *s) {
    return CECS_UNION_IS(cecs_integer_elements, cecs_sparse_set_elements, s->values);
}
static inline void *cecs_sparse_set_base_values(cecs_sparse_set_base *s) {
    return CECS_UNION_GET_UNCHECKED(cecs_any_elements, s->values).elements;
}
static inline size_t *cecs_sparse_set_base_keys(cecs_sparse_set_base *s) {
    return s->index_to_key.elements;
}

size_t cecs_sparse_set_base_key_unchecked(const cecs_sparse_set_base *s, size_t index);
size_t cecs_sparse_set_base_count_of_size(const cecs_sparse_set_base *s, size_t element_size);
bool cecs_sparse_set_base_is_empty(const cecs_sparse_set_base *s);


typedef struct cecs_sparse_set {
    cecs_sparse_set_base base;
    cecs_sparse_set_key_to_index key_to_index;
} cecs_sparse_set;

cecs_sparse_set cecs_sparse_set_create(void);
cecs_sparse_set cecs_sparse_set_create_of_integers(void);
cecs_sparse_set cecs_sparse_set_create_with_capacity(cecs_arena *a, size_t element_capacity, size_t element_size);
cecs_sparse_set cecs_sparse_set_create_of_integers_with_capacity(cecs_arena *a, size_t element_capacity, size_t element_size);

static inline void *cecs_sparse_set_values(cecs_sparse_set *s) {
    return cecs_sparse_set_base_values(&s->base);
}
static inline size_t *cecs_sparse_set_keys(const cecs_sparse_set *s) {
    return cecs_sparse_set_base_keys((cecs_sparse_set_base *)&s->base);
}
static inline size_t cecs_sparse_set_key_unchecked(const cecs_sparse_set *s, size_t index) {
    return cecs_sparse_set_base_key_unchecked(&s->base, index);
}
static inline size_t cecs_sparse_set_count_of_size(const cecs_sparse_set *s, size_t element_size) {
    return cecs_sparse_set_base_count_of_size(&s->base, element_size);
}

cecs_dense_index cecs_sparse_set_index(const cecs_sparse_set *s, size_t key);
static inline size_t cecs_sparse_set_index_unchecked(const cecs_sparse_set *s, size_t key) {
    return CECS_OPTION_GET(cecs_dense_index, cecs_sparse_set_index(s, key));
}

static inline bool cecs_sparse_set_contains(const cecs_sparse_set *s, size_t key) {
    return cecs_displaced_set_contains_index(&s->key_to_index, key)
        && CECS_OPTION_IS_SOME(cecs_dense_index, cecs_sparse_set_index(s, key));
}
static inline bool cecs_sparse_set_is_empty(const cecs_sparse_set *s) {
    return cecs_sparse_set_base_is_empty(&s->base);
}

void cecs_sparse_set_clear(cecs_sparse_set *s);

cecs_optional_element cecs_sparse_set_get(cecs_sparse_set *s, size_t key, size_t element_size);
#define CECS_SPARSE_SET_GET(type, sparse_set_ref, key) \
    cecs_sparse_set_get(sparse_set_ref, key, sizeof(type))

void *cecs_sparse_set_get_unchecked(cecs_sparse_set *s, size_t key, size_t element_size);
#define CECS_SPARSE_SET_GET_UNCHECKED(type, sparse_set_ref, key) \
    ((type *)cecs_sparse_set_get_unchecked(sparse_set_ref, key, sizeof(type)))


void *cecs_sparse_set_set(cecs_sparse_set *s, cecs_arena *a, size_t key, void *element, size_t element_size);
#define CECS_SPARSE_SET_SET(type, sparse_set_ref, arena_ref, key, element_ref) \
    ((type *)cecs_sparse_set_set(sparse_set_ref, arena_ref, key, element_ref, sizeof(type)))

void *cecs_sparse_set_set_range(cecs_sparse_set *s, cecs_arena *a, size_t key, void *elements, size_t count, size_t element_size);
#define CECS_SPARSE_SET_SET_RANGE(type, sparse_set_ref, arena_ref, key, elements_ref, count) \
    ((type *)cecs_sparse_set_set_range(sparse_set_ref, arena_ref, key, elements_ref, count, sizeof(type)))

bool cecs_sparse_set_remove_range(
    cecs_sparse_set* s,
    cecs_arena* a,
    size_t key,
    void* out_removed_elements,
    size_t count,
    size_t element_size
);
#define CECS_SPARSE_SET_REMOVE_RANGE(type, sparse_set_ref, arena_ref, key, out_removed_elements_ref, count) \
    cecs_sparse_set_remove_range(sparse_set_ref, arena_ref, key, out_removed_elements_ref, count, sizeof(type))

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
extern uint_fast8_t cecs_log2(size_t n);

#define CECS_PAGED_SPARSE_SET_PAGE_SIZE_LOG2 (CECS_PAGED_SPARSE_SET_KEY_BIT_COUNT_LOG2 - CECS_CECS_PAGED_SPARSE_SET_PAGE_COUNT_LOG2)
#define CECS_PAGED_SPARSE_SET_PAGE_SIZE (1 << CECS_PAGED_SPARSE_SET_PAGE_SIZE_LOG2)
typedef struct cecs_paged_sparse_set {
    cecs_sparse_set_base base;
    cecs_sparse_set_key_to_index keys_to_indices[CECS_PAGED_SPARSE_SET_PAGE_COUNT];
} cecs_paged_sparse_set;

cecs_paged_sparse_set cecs_paged_sparse_set_create(void);
cecs_paged_sparse_set cecs_paged_sparse_set_create_of_integers(void);
cecs_paged_sparse_set cecs_paged_sparse_set_create_with_capacity(cecs_arena *a, size_t element_capacity, size_t element_size);
cecs_paged_sparse_set cecs_paged_sparse_set_create_of_integers_with_capacity(cecs_arena *a, size_t element_capacity, size_t element_size);

static inline void *cecs_paged_sparse_set_data(cecs_paged_sparse_set *s) {
    return cecs_sparse_set_base_values(&s->base);
}
static inline size_t *cecs_paged_sparse_set_keys(cecs_paged_sparse_set *s) {
    return cecs_sparse_set_base_keys(&s->base);
}
static inline size_t cecs_paged_sparse_set_count_of_size(const cecs_paged_sparse_set *s, size_t element_size) {
    return cecs_sparse_set_base_count_of_size(&s->base, element_size);
}


bool cecs_paged_sparse_set_contains(const cecs_paged_sparse_set *s, size_t key);
static inline bool cecs_paged_sparse_set_is_empty(const cecs_paged_sparse_set *s) {
    return cecs_sparse_set_base_is_empty(&s->base);
}

cecs_optional_element cecs_paged_sparse_set_get(cecs_paged_sparse_set *s, size_t key, size_t element_size);
#define CECS_PAGED_SPARSE_SET_GET(type, paged_sparse_set_ref, key) \
    cecs_paged_sparse_set_get(paged_sparse_set_ref, key, sizeof(type))

void *cecs_paged_sparse_set_get_unchecked(cecs_paged_sparse_set *s, size_t key, size_t element_size);
#define CECS_PAGED_SPARSE_SET_GET_UNCHECKED(type, paged_sparse_set_ref, key) \
    ((type *)cecs_paged_sparse_set_get_unchecked(paged_sparse_set_ref, key, sizeof(type)))


void *cecs_paged_sparse_set_set(cecs_paged_sparse_set *s, cecs_arena *a, size_t key, void *element, size_t element_size);
#define CECS_PAGED_SPARSE_SET_SET(type, paged_sparse_set_ref, arena_ref, key, element_ref) \
    ((type *)cecs_paged_sparse_set_set(paged_sparse_set_ref, arena_ref, key, element_ref, sizeof(type)))

bool cecs_paged_sparse_set_remove(cecs_paged_sparse_set *s, cecs_arena *a, size_t key, void *out_removed_element, size_t element_size);
#define CECS_PAGED_SPARSE_SET_REMOVE(type, paged_sparse_set_ref, arena_ref, key, out_removed_element_ref) \
    cecs_paged_sparse_set_remove(paged_sparse_set_ref, arena_ref, key, out_removed_element_ref, sizeof(type))


#endif