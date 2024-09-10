#ifndef SPARSE_SET_H
#define SPARSE_SET_H

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <intrin.h>
#include "tagged_union.h"
#include "../types/macro_utils.h"
#include "range.h"
#include "arena.h"
#include "list.h"
#include "displaced_set.h"


#define DEREFERENCE_EQUALS(type, element_ref, value) (*((type *)(element_ref)) == (type)(value))
#define ASSERT_DEREFERENCE_EQUALS(type, element_ref, value) \
    assert(DEREFERENCE_EQUALS(type, element_ref, value) && "Dereferenced element value does not match the provided value")
#define ASSERT_DEREFERENCE_EQUALS_SIZE_CASE(type, element_ref, value) \
    case sizeof(type): \
        ASSERT_DEREFERENCE_EQUALS(type, element_ref, value); \
        break

#define _ASSERT_DEREFERENCE_EQUALS_SIZE_CASE_SELECT(element_ref, value, type) \
    ASSERT_DEREFERENCE_EQUALS_SIZE_CASE(type, element_ref, value)
#define ASSERT_INTEGER_DEREFERENCE_EQUALS(element_size, element_ref, value) \
    switch (element_size) { \
        MAP_CONST2( \
            _ASSERT_DEREFERENCE_EQUALS_SIZE_CASE_SELECT, \
            element_ref, \
            value, \
            SEMICOLON, \
            uint8_t, uint16_t, uint32_t, uint64_t \
        ); \
        default: \
            assert(false && "Invalid element size for an integer element"); \
            break; \
    }
    

typedef OPTION_STRUCT(size_t, dense_index) dense_index;
typedef OPTION_STRUCT(void *, optional_element) optional_element;

typedef list any_elements;
typedef list integer_elements;
typedef TAGGED_UNION_STRUCT(sparse_set_elements, list, any_elements, list, integer_elements) sparse_set_elements;
typedef struct sparse_set {
    sparse_set_elements elements;
    displaced_set indices;
    list keys;
} sparse_set;

sparse_set sparse_set_create() {
    return (sparse_set) {
        .elements =
            TAGGED_UNION_CREATE(any_elements, sparse_set_elements, list_create()),
        .indices = displaced_set_create(),
        .keys = list_create(),
    };
}

sparse_set sparse_set_create_of_integers() {
    return (sparse_set) {
        .elements =
            TAGGED_UNION_CREATE(integer_elements, sparse_set_elements, list_create()),
        .indices = displaced_set_create(),
        .keys = list_create(),
    };
}

sparse_set sparse_set_create_with_capacity(arena *a, size_t element_capacity, size_t element_size) {
    return (sparse_set) {
        .elements =
            TAGGED_UNION_CREATE(any_elements, sparse_set_elements, list_create_with_capacity(a, element_capacity * element_size)),
        .indices = displaced_set_create_with_capacity(a, sizeof(dense_index) * element_capacity),
        .keys = list_create_with_capacity(a, sizeof(size_t) * element_capacity),
    };
}

sparse_set sparse_set_create_of_integers_with_capacity(arena *a, size_t element_capacity, size_t element_size) {
    return (sparse_set) {
        .elements =
            TAGGED_UNION_CREATE(integer_elements, sparse_set_elements, list_create_with_capacity(a, element_capacity * element_size)),
        .indices = displaced_set_create_with_capacity(a, sizeof(dense_index) * element_capacity),
        .keys = list_create(),
    };
}

inline void *sparse_set_data(const sparse_set *s) {
    return TAGGED_UNION_GET_UNCHECKED(any_elements, s->elements).elements;
}


inline size_t *sparse_set_keys(const sparse_set *s) {
    return s->keys.elements;
}

inline size_t sparse_set_count_of_size(const sparse_set *s, size_t element_size) {
    return list_count_of_size(&TAGGED_UNION_GET_UNCHECKED(any_elements, s->elements), element_size);
}

inline bool sparse_set_contains(const sparse_set *s, size_t key) {
    return displaced_set_contains_index(&s->indices, key)
        && OPTION_IS_SOME(dense_index, *DISPLACED_SET_GET(dense_index, &s->indices, key));
}

optional_element sparse_set_get(const sparse_set *s, size_t key, size_t element_size) {
    if (!displaced_set_contains_index(&s->indices, key)) {
        return OPTION_CREATE_NONE_STRUCT(optional_element);
    }

    dense_index index = *DISPLACED_SET_GET(dense_index, &s->indices, key);
    if (OPTION_IS_SOME(dense_index, index)) {
        return OPTION_CREATE_SOME_STRUCT(
            optional_element, 
            list_get(&TAGGED_UNION_GET_UNCHECKED(any_elements, s->elements), OPTION_GET(dense_index, index), element_size)
        );
    } else {
        return OPTION_CREATE_NONE_STRUCT(optional_element);
    }
}
#define SPARSE_SET_GET(type, sparse_set_ref, key) \
    sparse_set_get(sparse_set_ref, key, sizeof(type))

void *sparse_set_get_unchecked(const sparse_set *s, size_t key, size_t element_size) {
    return OPTION_GET(optional_element, sparse_set_get(s, key, element_size));
}
#define SPARSE_SET_GET_UNCHECKED(type, sparse_set_ref, key) \
    ((type *)sparse_set_get_unchecked(sparse_set_ref, key, sizeof(type)))

bool sparse_set_is_empty(const sparse_set *s) {
    return TAGGED_UNION_GET_UNCHECKED(any_elements, s->elements).count == 0;
}

void *sparse_set_set(sparse_set *s, arena *a, size_t key, void *element, size_t element_size) {
    if (TAGGED_UNION_IS(integer_elements, sparse_set_elements, s->elements)) {
        ASSERT_INTEGER_DEREFERENCE_EQUALS(element_size, element, key);
    }

    if (displaced_set_contains_index(&s->indices, key)) {
        dense_index *index = DISPLACED_SET_GET(dense_index, &s->indices, key);
        if (OPTION_IS_SOME(dense_index, *index)) {
            size_t element_index = OPTION_GET(dense_index, *index);
            if (TAGGED_UNION_IS(any_elements, sparse_set_elements, s->elements))
                LIST_SET(size_t, &s->keys, element_index, &key);
            return list_set(&TAGGED_UNION_GET_UNCHECKED(any_elements, s->elements), element_index, element, element_size);
        } else {
            *index = OPTION_CREATE_SOME_STRUCT(dense_index, list_count_of_size(&s->elements, element_size));
            if (TAGGED_UNION_IS(any_elements, sparse_set_elements, s->elements))
                LIST_ADD(dense_index, &s->keys, a, &key);
            return list_add(&TAGGED_UNION_GET_UNCHECKED(any_elements, s->elements), a, element, element_size);
        }
    } else {
        DISPLACED_SET_SET(dense_index, &s->indices, a, key, &OPTION_CREATE_SOME_STRUCT(
            dense_index,
            list_count_of_size(&TAGGED_UNION_GET_UNCHECKED(any_elements, s->elements), element_size)
        ));
        if (TAGGED_UNION_IS(any_elements, sparse_set_elements, s->elements))
            LIST_ADD(size_t, &s->keys, a, &key);
        return list_add(&TAGGED_UNION_GET_UNCHECKED(any_elements, s->elements), a, element, element_size);
    }
}
#define SPARSE_SET_SET(type, sparse_set_ref, arena_ref, key, element_ref) \
    ((type *)sparse_set_set(sparse_set_ref, arena_ref, key, element_ref, sizeof(type)))

bool sparse_set_remove(sparse_set *s, arena *a, size_t key, optional_element *out_removed_element, size_t element_size) {
    if (!displaced_set_contains_index(&s->indices, key)
        || sparse_set_is_empty(s)) {
        *out_removed_element = OPTION_CREATE_NONE_STRUCT(optional_element);
        return false;
    }
    
    dense_index *index = DISPLACED_SET_GET(dense_index, &s->indices, key);
    if (OPTION_IS_SOME(dense_index, *index)) {
        if (OPTION_IS_SOME(optional_element, *out_removed_element)) {
            memcpy(
                OPTION_GET(optional_element, *out_removed_element),
                list_last(&TAGGED_UNION_GET_UNCHECKED(any_elements, s->elements), element_size),
                element_size
            );
        }
        size_t removed_index = OPTION_GET(dense_index, *index);
        void *swapped_last = list_remove_swap_last(&TAGGED_UNION_GET_UNCHECKED(any_elements, s->elements), a, removed_index, element_size);
        if (TAGGED_UNION_IS(any_elements, sparse_set_elements, s->elements)) {
            DISPLACED_SET_SET(
                dense_index,
                &s->indices,
                a,
                *LIST_REMOVE_SWAP_LAST(size_t, &s->keys, a, removed_index),
                &OPTION_CREATE_SOME_STRUCT(dense_index, removed_index)
            );
        } else {
            DISPLACED_SET_SET(
                dense_index,
                &s->indices,
                a,
                *((size_t *)swapped_last),
                &OPTION_CREATE_SOME_STRUCT(dense_index, removed_index)
            );
        }
        *index = OPTION_CREATE_NONE_STRUCT(dense_index);
        return true;
    } else {
        *out_removed_element = OPTION_CREATE_NONE_STRUCT(optional_element);
        return false;
    }
}
#define SPARSE_SET_REMOVE(type, sparse_set_ref, arena_ref, key, out_removed_element_ref) \
    sparse_set_remove(sparse_set_ref, arena_ref, key, out_removed_element_ref, sizeof(type))

bool sparse_set_remove_unchecked(sparse_set *s, arena *a, size_t key, void *out_removed_element, size_t element_size) {
    return sparse_set_remove(s, a, key, &OPTION_CREATE_SOME_STRUCT(optional_element, out_removed_element), element_size);
}
#define SPARSE_SET_REMOVE_UNCHECKED(type, sparse_set_ref, arena_ref, key, out_removed_element_ref) \
    sparse_set_remove_unchecked(sparse_set_ref, arena_ref, key, out_removed_element_ref, sizeof(type))



#define PAGED_SPARSE_SET_PAGE_COUNT_LOG2 3
#define PAGED_SPARSE_SET_PAGE_COUNT (1 << PAGED_SPARSE_SET_PAGE_COUNT_LOG2)

#if (SIZE_MAX == 0xFFFF)
    #define PAGED_SPARSE_SET_KEY_BIT_COUNT_LOG2 4

#elif (SIZE_MAX == 0xFFFFFFFF)
    #define PAGED_SPARSE_SET_KEY_BIT_COUNT_LOG2 5

#elif (SIZE_MAX == 0xFFFFFFFFFFFFFFFF)
    #define PAGED_SPARSE_SET_KEY_BIT_COUNT_LOG2 6

#else
    #error TBD code SIZE_T_BITS

#endif
#define PAGED_SPARSE_SET_KEY_BIT_COUNT (1 << PAGED_SPARSE_SET_KEY_BIT_COUNT_LOG2)
static_assert(PAGED_SPARSE_SET_KEY_BIT_COUNT == (sizeof(size_t) * 8), "PAGED_SPARSE_SET_KEY_BIT_COUNT != sizeof(size_t) * 8");

#define PAGED_SPARSE_SET_PAGE_SIZE_LOG2 (PAGED_SPARSE_SET_KEY_BIT_COUNT_LOG2 - PAGED_SPARSE_SET_PAGE_COUNT_LOG2)
#define PAGED_SPARSE_SET_PAGE_SIZE (1 << PAGED_SPARSE_SET_PAGE_SIZE_LOG2)
typedef struct paged_sparse_set {
    sparse_set_elements elements;
    list keys;
    displaced_set indices[PAGED_SPARSE_SET_PAGE_COUNT];
} paged_sparse_set;

inline uint8_t paged_sparse_set_log2(size_t n) {
#if (SIZE_MAX == 0xFFFF)
    return PAGED_SPARSE_SET_KEY_BIT_COUNT - __lzcnt16((uint16_t)n);

#elif (SIZE_MAX == 0xFFFFFFFF)
    return PAGED_SPARSE_SET_KEY_BIT_COUNT - __lzcnt((uint32_t)n);

#elif (SIZE_MAX == 0xFFFFFFFFFFFFFFFF)
    return PAGED_SPARSE_SET_KEY_BIT_COUNT - __lzcnt64((uint64_t)n);

#else
    #error TBD code SIZE_T_BITS
    return 0;

#endif
}

inline uint8_t paged_sparse_set_page_index(size_t key) {
    return paged_sparse_set_log2(key) >> PAGED_SPARSE_SET_PAGE_SIZE_LOG2;
}

inline size_t paged_sparse_set_page_key(size_t key, uint8_t page_index) {
    return key >> (PAGED_SPARSE_SET_PAGE_SIZE * page_index);
}

paged_sparse_set paged_sparse_set_create() {
    paged_sparse_set set = (paged_sparse_set) {
        .elements =
            TAGGED_UNION_CREATE(any_elements, sparse_set_elements, list_create()),
        .keys = list_create(),
    };
    for (size_t i = 0; i < PAGED_SPARSE_SET_PAGE_COUNT; i++) {
        set.indices[i] = displaced_set_create();
    }
    return set;
}

paged_sparse_set paged_sparse_set_create_of_integers() {
    paged_sparse_set set = (paged_sparse_set) {
        .elements =
            TAGGED_UNION_CREATE(integer_elements, sparse_set_elements, list_create()),
        .keys = list_create(),
    };
    for (size_t i = 0; i < PAGED_SPARSE_SET_PAGE_COUNT; i++) {
        set.indices[i] = displaced_set_create();
    }
    return set;
}

paged_sparse_set paged_sparse_set_create_with_capacity(arena *a, size_t element_capacity, size_t element_size) {
    paged_sparse_set set = (paged_sparse_set) {
        .elements =
            TAGGED_UNION_CREATE(any_elements, sparse_set_elements, list_create_with_capacity(a, element_capacity * element_size)),
        .keys = list_create_with_capacity(a, sizeof(size_t) * element_capacity),
    };
    for (size_t i = 0; i < PAGED_SPARSE_SET_PAGE_COUNT; i++) {
        set.indices[i] = displaced_set_create_with_capacity(a, (sizeof(dense_index) * element_capacity) >> PAGED_SPARSE_SET_PAGE_COUNT_LOG2);
    }
    return set;
}

paged_sparse_set paged_sparse_set_create_of_integers_with_capacity(arena *a, size_t element_capacity, size_t element_size) {
    paged_sparse_set set = (paged_sparse_set) {
        .elements =
            TAGGED_UNION_CREATE(integer_elements, sparse_set_elements, list_create_with_capacity(a, element_capacity * element_size)),
        .keys = list_create(),
    };
    for (size_t i = 0; i < PAGED_SPARSE_SET_PAGE_COUNT; i++) {
        set.indices[i] = displaced_set_create_with_capacity(a, (sizeof(dense_index) * element_capacity) >> PAGED_SPARSE_SET_PAGE_COUNT_LOG2);
    }
    return set;
}

inline void *paged_sparse_set_data(const paged_sparse_set *s) {
    return TAGGED_UNION_GET_UNCHECKED(any_elements, s->elements).elements;
}

inline size_t *paged_sparse_set_keys(const paged_sparse_set *s) {
    return s->keys.elements;
}

inline size_t paged_sparse_set_count_of_size(const paged_sparse_set *s, size_t element_size) {
    return list_count_of_size(&TAGGED_UNION_GET_UNCHECKED(any_elements, s->elements), element_size);
}

inline bool paged_sparse_set_contains(const paged_sparse_set *s, size_t key) {
    uint8_t page_index = paged_sparse_set_page_index(key);
    displaced_set *indices = &s->indices[page_index];
    size_t page_key = paged_sparse_set_page_key(key, page_index);
    return displaced_set_contains_index(indices, page_key)
        && OPTION_IS_SOME(dense_index, *DISPLACED_SET_GET(dense_index, indices, page_key));
}

optional_element paged_sparse_set_get(const paged_sparse_set *s, size_t key, size_t element_size) {
    uint8_t page_index = paged_sparse_set_page_index(key);
    displaced_set *indices = &s->indices[page_index];
    size_t page_key = paged_sparse_set_page_key(key, page_index);
    if (!displaced_set_contains_index(indices, page_key)) {
        return OPTION_CREATE_NONE_STRUCT(optional_element);
    }

    dense_index index = *DISPLACED_SET_GET(dense_index, indices, page_key);
    if (OPTION_IS_SOME(dense_index, index)) {
        return OPTION_CREATE_SOME_STRUCT(
            optional_element, 
            list_get(&TAGGED_UNION_GET_UNCHECKED(any_elements, s->elements), OPTION_GET(dense_index, index), element_size)
        );
    } else {
        return OPTION_CREATE_NONE_STRUCT(optional_element);
    }
}
#define PAGED_SPARSE_SET_GET(type, paged_sparse_set_ref, key) \
    paged_sparse_set_get(paged_sparse_set_ref, key, sizeof(type))

void *paged_sparse_set_get_unchecked(const paged_sparse_set *s, size_t key, size_t element_size) {
    return OPTION_GET(optional_element, paged_sparse_set_get(s, key, element_size));
}
#define PAGED_SPARSE_SET_GET_UNCHECKED(type, paged_sparse_set_ref, key) \
    ((type *)paged_sparse_set_get_unchecked(paged_sparse_set_ref, key, sizeof(type)))

bool paged_sparse_set_is_empty(const paged_sparse_set *s) {
    return TAGGED_UNION_GET_UNCHECKED(any_elements, s->elements).count == 0;
}

void *paged_sparse_set_set(paged_sparse_set *s, arena *a, size_t key, void *element, size_t element_size) {
    uint8_t page_index = paged_sparse_set_page_index(key);
    displaced_set *indices = &s->indices[page_index];
    size_t page_key = paged_sparse_set_page_key(key, page_index);
    if (TAGGED_UNION_IS(integer_elements, sparse_set_elements, s->elements)) {
        ASSERT_INTEGER_DEREFERENCE_EQUALS(element_size, element, key);
    }

    if (displaced_set_contains_index(indices, page_key)) {
        dense_index *index = DISPLACED_SET_GET(dense_index, indices, page_key);
        if (OPTION_IS_SOME(dense_index, *index)) {
            size_t element_index = OPTION_GET(dense_index, *index);
            if (TAGGED_UNION_IS(any_elements, sparse_set_elements, s->elements))
                LIST_SET(size_t, &s->keys, element_index, &key);
            return list_set(&TAGGED_UNION_GET_UNCHECKED(any_elements, s->elements), element_index, element, element_size);
        } else {
            *index = OPTION_CREATE_SOME_STRUCT(dense_index, list_count_of_size(&s->elements, element_size));
            if (TAGGED_UNION_IS(any_elements, sparse_set_elements, s->elements))
                LIST_ADD(dense_index, &s->keys, a, &key);
            return list_add(&TAGGED_UNION_GET_UNCHECKED(any_elements, s->elements), a, element, element_size);
        }
    } else {
        DISPLACED_SET_SET(dense_index, indices, a, page_key, &OPTION_CREATE_SOME_STRUCT(
            dense_index,
            list_count_of_size(&TAGGED_UNION_GET_UNCHECKED(any_elements, s->elements), element_size)
        ));
        if (TAGGED_UNION_IS(any_elements, sparse_set_elements, s->elements))
            LIST_ADD(size_t, &s->keys, a, &key);
        return list_add(&TAGGED_UNION_GET_UNCHECKED(any_elements, s->elements), a, element, element_size);
    }
}
#define PAGED_SPARSE_SET_SET(type, paged_sparse_set_ref, arena_ref, key, element_ref) \
    ((type *)paged_sparse_set_set(paged_sparse_set_ref, arena_ref, key, element_ref, sizeof(type)))

bool paged_sparse_set_remove(paged_sparse_set *s, arena *a, size_t key, optional_element *out_removed_element, size_t element_size) {
    uint8_t page_index = paged_sparse_set_page_index(key);
    displaced_set *indices = &s->indices[page_index];
    size_t page_key = paged_sparse_set_page_key(key, page_index);
    if (!displaced_set_contains_index(indices, page_key)
        || paged_sparse_set_is_empty(s)) {
        *out_removed_element = OPTION_CREATE_NONE_STRUCT(optional_element);
        return false;
    }
    
    dense_index *index = DISPLACED_SET_GET(dense_index, indices, page_key);
    if (OPTION_IS_SOME(dense_index, *index)) {
        if (OPTION_IS_SOME(optional_element, *out_removed_element)) {
            memcpy(
                OPTION_GET(optional_element, *out_removed_element),
                list_last(&TAGGED_UNION_GET_UNCHECKED(any_elements, s->elements), element_size),
                element_size
            );
        }
        size_t removed_index = OPTION_GET(dense_index, *index);
        void *swapped_last = list_remove_swap_last(&TAGGED_UNION_GET_UNCHECKED(any_elements, s->elements), a, removed_index, element_size);
        if (TAGGED_UNION_IS(any_elements, sparse_set_elements, s->elements)) {
            DISPLACED_SET_SET(
                dense_index,
                indices,
                a,
                *LIST_REMOVE_SWAP_LAST(size_t, &s->keys, a, removed_index),
                &OPTION_CREATE_SOME_STRUCT(dense_index, removed_index)
            );
        } else {
            DISPLACED_SET_SET(
                dense_index,
                indices,
                a,
                *((size_t *)swapped_last),
                &OPTION_CREATE_SOME_STRUCT(dense_index, removed_index)
            );
        }
        *index = OPTION_CREATE_NONE_STRUCT(dense_index);
        return true;
    } else {
        *out_removed_element = OPTION_CREATE_NONE_STRUCT(optional_element);
        return false;
    }
}
#define PAGED_SPARSE_SET_REMOVE(type, paged_sparse_set_ref, arena_ref, key, out_removed_element_ref) \
    paged_sparse_set_remove(paged_sparse_set_ref, arena_ref, key, out_removed_element_ref, sizeof(type))

bool paged_sparse_set_remove_unchecked(paged_sparse_set *s, arena *a, size_t key, void *out_removed_element, size_t element_size) {
    return paged_sparse_set_remove(s, a, key, &OPTION_CREATE_SOME_STRUCT(optional_element, out_removed_element), element_size);
}
#define PAGED_SPARSE_SET_REMOVE_UNCHECKED(type, paged_sparse_set_ref, arena_ref, key, out_removed_element_ref) \
    paged_sparse_set_remove_unchecked(paged_sparse_set_ref, arena_ref, key, out_removed_element_ref, sizeof(type))


#endif