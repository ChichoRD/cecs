#include <memory.h>

#include "cecs_sparse_set.h"

const cecs_sparse_set_index cecs_sparse_set_index_invalid = { CECS_SPARSE_SET_INDEX_INVALID_VALUE };
extern inline bool cecs_sparse_set_index_check(const cecs_sparse_set_index index);
extern inline size_t cecs_sparse_set_index_look(const cecs_sparse_set_index index);

[[maybe_unused]]
static inline cecs_any_elements *cecs_sparse_set_base_values_array_any_unchecked(cecs_sparse_set_base *s) {
    return &CECS_UNION_GET_UNCHECKED(cecs_any_elements, s->values);
}
[[maybe_unused]]
static inline cecs_any_elements *cecs_sparse_set_base_values_array_any(cecs_sparse_set_base *s) {
    CECS_UNION_IS_ASSERT(cecs_any_elements, cecs_sparse_set_elements, s->values);
    return cecs_sparse_set_base_values_array_any_unchecked(s);
}
[[maybe_unused]]
static inline cecs_integer_elements *cecs_sparse_set_base_values_array_int(cecs_sparse_set_base *s) {
    CECS_UNION_IS_ASSERT(cecs_integer_elements, cecs_sparse_set_elements, s->values);
    return &CECS_UNION_GET_UNCHECKED(cecs_integer_elements, s->values);
}

size_t cecs_sparse_set_base_key_unchecked(const cecs_sparse_set_base *s, size_t index) {
    if (cecs_sparse_set_base_is_of_integers(s)) {
        return *CECS_DYNAMIC_ARRAY_GET(size_t, cecs_sparse_set_base_values_array_any_unchecked((cecs_sparse_set_base *)s), index);
    } else {
        return *CECS_DYNAMIC_ARRAY_GET(size_t, &s->index_to_key, index);
    }
}
size_t cecs_sparse_set_base_count_of_size(const cecs_sparse_set_base *s, size_t element_size) {
    return cecs_dynamic_array_count_of_size(cecs_sparse_set_base_values_array_any_unchecked((cecs_sparse_set_base *)s), element_size);
}
bool cecs_sparse_set_base_is_empty(const cecs_sparse_set_base *s){
    return cecs_sparse_set_base_values_array_any_unchecked((cecs_sparse_set_base *)s)->count == 0;
}

cecs_sparse_set cecs_sparse_set_create(void)
{
    return (cecs_sparse_set) {
        .base = (cecs_sparse_set_base) {
            .values =
                CECS_UNION_CREATE(cecs_any_elements, cecs_sparse_set_elements, cecs_dynamic_array_create()),
            .index_to_key = cecs_dynamic_array_create(),
        },
        .key_to_index = cecs_displaced_set_create(),
    };
}
cecs_sparse_set cecs_sparse_set_create_of_integers(void) {
    return (cecs_sparse_set) {
        .base = (cecs_sparse_set_base) {
            .values =
                CECS_UNION_CREATE(cecs_integer_elements, cecs_sparse_set_elements, cecs_dynamic_array_create()),
            .index_to_key = cecs_dynamic_array_create(),
        },
        .key_to_index = cecs_displaced_set_create(),
    };
}
cecs_sparse_set cecs_sparse_set_create_with_capacity(cecs_arena* a, size_t element_capacity, size_t element_size) {
    return (cecs_sparse_set) {
        .base = (cecs_sparse_set_base) {
            .values =
                CECS_UNION_CREATE(cecs_any_elements, cecs_sparse_set_elements, cecs_dynamic_array_create_with_capacity(a, element_capacity * element_size)),
            .index_to_key = cecs_dynamic_array_create_with_capacity(a, sizeof(size_t) * element_capacity),
        },
        .key_to_index = cecs_displaced_set_create_with_capacity(a, sizeof(cecs_sparse_set_index) * element_capacity),
    };
}
cecs_sparse_set cecs_sparse_set_create_of_integers_with_capacity(cecs_arena* a, size_t element_capacity, size_t element_size) {
    return (cecs_sparse_set) {
        .base = (cecs_sparse_set_base) {
            .values =
                CECS_UNION_CREATE(cecs_any_elements, cecs_sparse_set_elements, cecs_dynamic_array_create_with_capacity(a, element_capacity * element_size)),
            .index_to_key = cecs_dynamic_array_create(),
        },
        .key_to_index = cecs_displaced_set_create_with_capacity(a, sizeof(cecs_sparse_set_index) * element_capacity),
    };
}

static inline cecs_sparse_set_index *cecs_sparse_set_get_index_ptr(cecs_sparse_set_key_to_index *k, size_t key) {
    return CECS_DISPLACED_SET_GET(cecs_sparse_set_index, k, key);
}
cecs_sparse_set_index cecs_sparse_set_get_index(const cecs_sparse_set *s, size_t key) {
    return *cecs_sparse_set_get_index_ptr((cecs_sparse_set_key_to_index *)&s->key_to_index, key);
}

void cecs_sparse_set_clear(cecs_sparse_set *s) {
    cecs_dynamic_array_clear(cecs_sparse_set_base_values_array_any_unchecked(&s->base));
    cecs_dynamic_array_clear(&s->base.index_to_key);
    cecs_displaced_set_clear(&s->key_to_index);
}

static cecs_sparse_set_index cecs_sparse_set_add_key(
    cecs_sparse_set_base *s,
    cecs_sparse_set_key_to_index *k,
    cecs_arena *a,
    size_t key,
    size_t size
) {
    cecs_sparse_set_index *index;
    if (!cecs_displaced_set_contains_index(k, key)) {
        cecs_displaced_set_expand(k, a, key, sizeof(cecs_sparse_set_index), CECS_SPARSE_SET_INDEX_INVALID_VALUE_INT);
        index = cecs_displaced_set_set(k, a, key, (cecs_sparse_set_index *)&cecs_sparse_set_index_invalid, sizeof(cecs_sparse_set_index));
    } else {
        index = cecs_sparse_set_get_index_ptr(k, key);
    }
    
    if (cecs_sparse_set_index_check(*index)) {
        return cecs_sparse_set_index_invalid;
    } else {
        const size_t values_count = cecs_sparse_set_base_count_of_size(s, size);
        if (cecs_sparse_set_base_is_of_integers(s)) {
            assert(s->index_to_key.count == 0 && "fatal error: sparse set integer mode mismatch");
        } else {
            assert(
                values_count == cecs_dynamic_array_count_of_size(&s->index_to_key, sizeof(size_t))
                && "fatal error: values - index_to_key count mismatch"
            );
            cecs_dynamic_array_add(&s->index_to_key, a, &key, sizeof(size_t));
        }

        cecs_dynamic_array_append_empty(cecs_sparse_set_base_values_array_any_unchecked(s), a, 1, size);
        *index = (cecs_sparse_set_index){ values_count };
        return *index;
    }
}

[[maybe_unused]]
static size_t cecs_sparse_set_add_key_expect(cecs_sparse_set *s, cecs_arena *a, size_t key, size_t size) {
    return cecs_sparse_set_index_look(cecs_sparse_set_add_key(&s->base, &s->key_to_index, a, key, size));
}

cecs_optional_element cecs_sparse_set_get(cecs_sparse_set *s, size_t key, size_t element_size) {
    if (!cecs_displaced_set_contains_index(&s->key_to_index, key)) {
        return CECS_OPTION_CREATE_NONE_STRUCT(cecs_optional_element);
    }

    cecs_sparse_set_index index = cecs_sparse_set_get_index(s, key);
    if (cecs_sparse_set_index_check(index)) {
        return CECS_OPTION_CREATE_SOME_STRUCT(
            cecs_optional_element,
            cecs_dynamic_array_get(
                cecs_sparse_set_base_values_array_any_unchecked(&s->base),
                cecs_sparse_set_index_look(index),
                element_size
            )
        );
    } else {
        return CECS_OPTION_CREATE_NONE_STRUCT(cecs_optional_element);
    }
}

void* cecs_sparse_set_get_unchecked(cecs_sparse_set* s, size_t key, size_t element_size) {
    return CECS_OPTION_GET(cecs_optional_element, cecs_sparse_set_get(s, key, element_size));
}

void* cecs_sparse_set_set(cecs_sparse_set* s, cecs_arena* a, size_t key, void* element, size_t element_size) {
    if (cecs_sparse_set_base_is_of_integers(&s->base)) {
        CECS_ASSERT_INTEGER_DEREFERENCE_EQUALS(element_size, element, key);
    }

    cecs_sparse_set_index added = cecs_sparse_set_add_key(&s->base, &s->key_to_index, a, key, element_size);
    if (!cecs_sparse_set_index_check(added)) {
        size_t index = cecs_sparse_set_get_index_expect(s, key);
        return cecs_dynamic_array_set(cecs_sparse_set_base_values_array_any_unchecked(&s->base), index, element, element_size);
    } else {
        return cecs_dynamic_array_set(
            cecs_sparse_set_base_values_array_any_unchecked(&s->base),
            cecs_sparse_set_index_look(added),
            element,
            element_size
        );
    }
}

void *cecs_sparse_set_set_range(cecs_sparse_set *s, cecs_arena *a, size_t key, void *elements, size_t count, size_t element_size) {
    if (cecs_sparse_set_base_is_of_integers(&s->base)) {
        CECS_ASSERT_INTEGER_DEREFERENCE_EQUALS(element_size, elements, key);
    }

    cecs_sparse_set_index added = cecs_sparse_set_add_key(&s->base, &s->key_to_index, a, key, element_size);
    if (!cecs_sparse_set_index_check(added)) {
        size_t index = cecs_sparse_set_get_index_expect(s, key);
        if (!cecs_sparse_set_base_is_of_integers(&s->base)) {
            cecs_dynamic_array_set_copy_range(
                &s->base.index_to_key,
                index,
                &key,
                count,
                sizeof(size_t)
            );
        }

        return cecs_dynamic_array_set_range(
            cecs_sparse_set_base_values_array_any_unchecked(&s->base),
            index,
            elements,
            count,
            element_size
        );
    } else if (count == 1) {
        return cecs_dynamic_array_set(
            cecs_sparse_set_base_values_array_any_unchecked(&s->base),
            cecs_sparse_set_index_look(added),
            elements,
            element_size
        );
    } else {
        size_t index = cecs_sparse_set_index_look(added);
        if (!cecs_sparse_set_base_is_of_integers(&s->base)) {
            cecs_dynamic_array_append_empty(&s->base.index_to_key, a, count - 1, sizeof(size_t));
            cecs_dynamic_array_set_copy_range(&s->base.index_to_key, index + 1,  &key, count - 1, sizeof(size_t));
        }

        cecs_any_elements *values = cecs_sparse_set_base_values_array_any_unchecked(&s->base);
        cecs_dynamic_array_append_empty(values, a, count - 1, element_size);
        return cecs_dynamic_array_set_range(values, index, elements, count, element_size);
    }
}

static bool cecs_sparse_set_remove_key(
    cecs_sparse_set_base *s,
    cecs_sparse_set_key_to_index *k,
    cecs_arena *a,
    size_t key,
    size_t *out_removed_index,
    size_t **out_invalidated_key
) {
    (void)a;
    if (!cecs_displaced_set_contains_index(k, key)) {
        return false;
    }

    cecs_sparse_set_index *index = cecs_sparse_set_get_index_ptr(k, key);
    if (!cecs_sparse_set_index_check(*index)) {
        return false;
    } else {
        *out_removed_index = cecs_sparse_set_index_look(*index);
        if (cecs_sparse_set_base_is_of_integers(s)) {
            assert(s->index_to_key.count == 0 && "fatal error: sparse set integer mode mismatch");
            *out_invalidated_key = NULL;
        } else {
            *out_invalidated_key = cecs_dynamic_array_get(&s->index_to_key, *out_removed_index, sizeof(size_t));
        }

        *index = cecs_sparse_set_index_invalid;
        return true;
    }
}

[[maybe_unused]]
static void cecs_sparse_set_remove_key_unchecked(
    cecs_sparse_set *s, cecs_arena *a, size_t key, size_t *out_removed_index, size_t **out_invalidated_key
) {
    assert(
        cecs_sparse_set_remove_key(&s->base, &s->key_to_index, a, key, out_removed_index, out_invalidated_key)
        && "error: key must be present in order to be removed from sparse set"
    );
}

bool cecs_sparse_set_remove(cecs_sparse_set* s, cecs_arena* a, size_t key, void* out_removed_element, size_t element_size) {
    assert(out_removed_element != NULL && "error: out_removed_element must not be NULL");

    size_t removed_index;
    size_t *invalidated_key;
    if (cecs_sparse_set_remove_key(&s->base, &s->key_to_index, a, key, &removed_index, &invalidated_key)) {
        bool integer = cecs_sparse_set_base_is_of_integers(&s->base);
        assert(
            ((invalidated_key == NULL) == integer)
            && "fatal error: invalidated keys - integer mode mismatch"
        );

        size_t last_value_index = cecs_sparse_set_count_of_size(s, element_size) - 1;
        size_t last_value_key = cecs_sparse_set_key_unchecked(s, last_value_index);

        typedef cecs_any_elements *elements;
        const elements values = cecs_sparse_set_base_values_array_any_unchecked(&s->base);
        
        memcpy(
            out_removed_element,
            cecs_dynamic_array_get(values, removed_index, element_size),
            element_size
        );
        cecs_dynamic_array_remove_swap_last(values, a, removed_index, element_size);

        if (!integer) {
            *invalidated_key = last_value_key;
            cecs_dynamic_array_remove(&s->base.index_to_key, a, last_value_index, sizeof(size_t));
        }

        if (removed_index != last_value_index) {
            *cecs_sparse_set_get_index_ptr(&s->key_to_index, last_value_key) = (cecs_sparse_set_index){ removed_index };
        }
        return true;
    } else {
        return false;
    }
}

bool cecs_sparse_set_remove_range(
    cecs_sparse_set* s,
    cecs_arena* a,
    size_t key,
    void* out_removed_elements,
    size_t count,
    size_t element_size
) {
    assert(out_removed_elements != NULL && "error: out_removed_element must not be NULL");

    size_t removed_index;
    size_t *invalid_key;
    if (cecs_sparse_set_remove_key(&s->base, &s->key_to_index, a, key, &removed_index, &invalid_key)) {
        bool integer = cecs_sparse_set_base_is_of_integers(&s->base);
        assert(
            ((invalid_key == NULL) == integer)
            && "fatal error: invalidated keys - integer mode mismatch"
        );

        // size_t last_value_index = cecs_sparse_set_count_of_size(s, element_size) - 1;
        // size_t last_value_key = cecs_sparse_set_key_unchecked(s, last_value_index);

        typedef cecs_any_elements *elements;
        const elements values = cecs_sparse_set_base_values_array_any_unchecked(&s->base);
        const int invalidated_key_count = (int)cecs_dynamic_array_count_of_size(values, element_size) - (int)removed_index - (int)count;
        assert(invalidated_key_count >= 0 && "error: tried to remove more elements than exist in the sparse set");
        memcpy(
            out_removed_elements,
            cecs_dynamic_array_get_range(values, removed_index, count, element_size),
            element_size
        );
        cecs_dynamic_array_remove_range(values, a, removed_index, count, element_size);

        const size_t *invalidated_keys;
        if (!integer) {
            cecs_dynamic_array_remove_range(&s->base.index_to_key, a, removed_index, count, sizeof(size_t));
            invalidated_keys = cecs_dynamic_array_get_range(&s->base.index_to_key, removed_index, count, sizeof(size_t));
        } else {
            invalidated_keys = cecs_dynamic_array_get_range(values, removed_index, count, sizeof(size_t));
        }

        size_t last_key = ~0;
        for (int i = 0; i < invalidated_key_count; i++) {
            size_t invalidated_key = invalidated_keys[i];
            if (invalidated_key != last_key) {
                *cecs_sparse_set_get_index_ptr(&s->key_to_index, invalidated_key) = (cecs_sparse_set_index){ removed_index + i };
            }
            last_key = invalidated_key;
        }
        return true;
    } else {
        return false;
    }
}

static inline uint_fast8_t cecs_paged_sparse_set_log2(size_t n) {
#if (SIZE_MAX == 0xFFFF)
    return (uint_fast8_t)(CECS_PAGED_SPARSE_SET_KEY_BIT_COUNT - __lzcnt16((uint16_t)n));

#elif (SIZE_MAX == 0xFFFFFFFF)
    return (uint_fast8_t)(CECS_PAGED_SPARSE_SET_KEY_BIT_COUNT - __lzcnt((uint32_t)n));

#elif (SIZE_MAX == 0xFFFFFFFFFFFFFFFF)
    return (uint_fast8_t)(CECS_PAGED_SPARSE_SET_KEY_BIT_COUNT - __lzcnt64((uint64_t)n));

#else
    #error TBD code SIZE_T_BITS
    return 0;

#endif
}

uint_fast8_t cecs_log2(size_t n) {
    return cecs_paged_sparse_set_log2(n);
}

static inline uint_fast8_t cecs_paged_sparse_set_page_index(size_t key, uint_fast8_t *out_key_log2) {
    *out_key_log2 = cecs_paged_sparse_set_log2(key);
    return *out_key_log2 >> CECS_PAGED_SPARSE_SET_PAGE_SIZE_LOG2;
}

static inline size_t cecs_paged_sparse_set_page_key(size_t key, uint_fast8_t page_index, uint_fast8_t key_log2) {
    const size_t mask_size = CECS_PAGED_SPARSE_SET_PAGE_SIZE * (size_t)page_index;
    const size_t upper_mask = (SIZE_MAX << mask_size);

    const size_t upper_key = key & upper_mask;
    const size_t lower_key = key & ~upper_mask;

    static const uint_fast8_t key_log_mask = (1 << CECS_PAGED_SPARSE_SET_PAGE_SIZE_LOG2) - 1;
    const uint_fast8_t upper_key_low_bit_count = (key_log2 & key_log_mask) + 1;
    // static_assert(false);
    const size_t page_key = (upper_key >> mask_size) | ((lower_key) << upper_key_low_bit_count);
    return page_key;
}

cecs_paged_sparse_set cecs_paged_sparse_set_create(void) {
    cecs_paged_sparse_set set = (cecs_paged_sparse_set){
        .base = (cecs_sparse_set_base){
            .values = CECS_UNION_CREATE(cecs_any_elements, cecs_sparse_set_elements, cecs_dynamic_array_create()),
            .index_to_key = cecs_dynamic_array_create(),
        },
    };
    for (size_t i = 0; i < CECS_PAGED_SPARSE_SET_PAGE_COUNT; i++) {
        set.keys_to_indices[i] = cecs_displaced_set_create();
    }
    return set;
}
cecs_paged_sparse_set cecs_paged_sparse_set_create_of_integers(void) {
    cecs_paged_sparse_set set = (cecs_paged_sparse_set){
        .base = (cecs_sparse_set_base){
            .values = CECS_UNION_CREATE(cecs_integer_elements, cecs_sparse_set_elements, cecs_dynamic_array_create()),
            .index_to_key = cecs_dynamic_array_create(),
        },
    };
    for (size_t i = 0; i < CECS_PAGED_SPARSE_SET_PAGE_COUNT; i++) {
        set.keys_to_indices[i] = cecs_displaced_set_create();
    }
    return set;
}
cecs_paged_sparse_set cecs_paged_sparse_set_create_with_capacity(cecs_arena* a, size_t element_capacity, size_t element_size) {
    cecs_paged_sparse_set set = (cecs_paged_sparse_set){
        .base = (cecs_sparse_set_base){
            .values = CECS_UNION_CREATE(cecs_any_elements, cecs_sparse_set_elements, cecs_dynamic_array_create_with_capacity(a, element_capacity * element_size)),
            .index_to_key = cecs_dynamic_array_create_with_capacity(a, sizeof(size_t) * element_capacity),
        },
    };
    for (size_t i = 0; i < CECS_PAGED_SPARSE_SET_PAGE_COUNT; i++) {
        set.keys_to_indices[i] = cecs_displaced_set_create_with_capacity(
            a,
            (sizeof(cecs_sparse_set_index) * element_capacity) >> CECS_CECS_PAGED_SPARSE_SET_PAGE_COUNT_LOG2
        );
    }
    return set;
}
cecs_paged_sparse_set cecs_paged_sparse_set_create_of_integers_with_capacity(cecs_arena* a, size_t element_capacity, size_t element_size) {
    cecs_paged_sparse_set set = (cecs_paged_sparse_set){
        .base = (cecs_sparse_set_base){
            .values = CECS_UNION_CREATE(cecs_integer_elements, cecs_sparse_set_elements, cecs_dynamic_array_create_with_capacity(a, element_capacity * element_size)),
            .index_to_key = cecs_dynamic_array_create(),
        },
    };
    for (size_t i = 0; i < CECS_PAGED_SPARSE_SET_PAGE_COUNT; i++) {
        set.keys_to_indices[i] = cecs_displaced_set_create_with_capacity(
            a,
            (sizeof(cecs_sparse_set_index) * element_capacity) >> CECS_CECS_PAGED_SPARSE_SET_PAGE_COUNT_LOG2
        );
    }
    return set;
}

static cecs_sparse_set_key_to_index *cecs_paged_sparse_set_key_to_index(cecs_paged_sparse_set *s, size_t key, size_t *out_page_key) {
    uint_fast8_t key_log2;
    uint_fast8_t page_index = cecs_paged_sparse_set_page_index(key, &key_log2);
    
    *out_page_key = cecs_paged_sparse_set_page_key(key, page_index, key_log2);
    return &s->keys_to_indices[page_index];
}

bool cecs_paged_sparse_set_contains(const cecs_paged_sparse_set *s, size_t key) {
    size_t page_key;
    const cecs_sparse_set_key_to_index *key_to_index = cecs_paged_sparse_set_key_to_index((cecs_paged_sparse_set *)s, key, &page_key);
    return cecs_displaced_set_contains_index(key_to_index, page_key)
        && cecs_sparse_set_index_check(*CECS_DISPLACED_SET_GET(cecs_sparse_set_index, key_to_index, page_key));
}

cecs_optional_element cecs_paged_sparse_set_get(cecs_paged_sparse_set *s, size_t key, size_t element_size) {
    size_t page_key;
    const cecs_displaced_set* key_to_index = cecs_paged_sparse_set_key_to_index(s, key, &page_key);
    if (!cecs_displaced_set_contains_index(key_to_index, page_key)) {
        return CECS_OPTION_CREATE_NONE_STRUCT(cecs_optional_element);
    }

    cecs_sparse_set_index index = *CECS_DISPLACED_SET_GET(cecs_sparse_set_index, key_to_index, page_key);
    if (cecs_sparse_set_index_check(index)) {
        return CECS_OPTION_CREATE_SOME_STRUCT(
            cecs_optional_element,
            cecs_dynamic_array_get(
                cecs_sparse_set_base_values_array_any_unchecked(&s->base),
                cecs_sparse_set_index_look(index),
                element_size
            )
        );
    } else {
        return CECS_OPTION_CREATE_NONE_STRUCT(cecs_optional_element);
    }
}

void* cecs_paged_sparse_set_get_unchecked(cecs_paged_sparse_set* s, size_t key, size_t element_size) {
    return CECS_OPTION_GET(cecs_optional_element, cecs_paged_sparse_set_get(s, key, element_size));
}

void* cecs_paged_sparse_set_set(cecs_paged_sparse_set* s, cecs_arena* a, size_t key, void* element, size_t element_size) {
    size_t page_key;
    cecs_sparse_set_key_to_index *key_to_index = cecs_paged_sparse_set_key_to_index(s, key, &page_key);

    bool integer = cecs_sparse_set_base_is_of_integers(&s->base);
    if (integer) {
        CECS_ASSERT_INTEGER_DEREFERENCE_EQUALS(element_size, element, key);
    }

    cecs_sparse_set_index added = cecs_sparse_set_add_key(&s->base, key_to_index, a, page_key, element_size);
    if (!cecs_sparse_set_index_check(added)) {
        size_t index = cecs_sparse_set_index_look(*cecs_sparse_set_get_index_ptr(key_to_index, page_key));
        return cecs_dynamic_array_set(cecs_sparse_set_base_values_array_any_unchecked(&s->base), index, element, element_size);
    } else {
        if (!integer) {
            *CECS_DYNAMIC_ARRAY_LAST(size_t, &s->base.index_to_key) = key;
        }

        return cecs_dynamic_array_set(
            cecs_sparse_set_base_values_array_any_unchecked(&s->base),
            cecs_sparse_set_index_look(added),
            element,
            element_size
        );
    }
}

bool cecs_paged_sparse_set_remove(cecs_paged_sparse_set* s, cecs_arena* a, size_t key, void* out_removed_element, size_t element_size) {
    assert(out_removed_element != NULL && "error: out_removed_element must not be NULL");

    size_t page_key;
    cecs_sparse_set_key_to_index *key_to_index = cecs_paged_sparse_set_key_to_index(s, key, &page_key);
    if (!cecs_displaced_set_contains_index(key_to_index, page_key)
        || cecs_paged_sparse_set_is_empty(s)
    ) {
        memset(out_removed_element, 0, element_size);
        return false;
    }

    size_t removed_index;
    size_t *invalidated_key;
    if (cecs_sparse_set_remove_key(&s->base, key_to_index, a, page_key, &removed_index, &invalidated_key)) {
        bool integer = cecs_sparse_set_base_is_of_integers(&s->base);
        assert(
            ((invalidated_key == NULL) == integer)
            && "fatal error: invalidated keys - integer mode mismatch"
        );

        size_t last_value_index = cecs_paged_sparse_set_count_of_size(s, element_size) - 1;
        size_t last_value_key = cecs_sparse_set_base_key_unchecked(&s->base, last_value_index);

        typedef cecs_any_elements *elements;
        const elements values = cecs_sparse_set_base_values_array_any_unchecked(&s->base);
        memcpy(
            out_removed_element,
            cecs_dynamic_array_get(values, removed_index, element_size),
            element_size
        );
        cecs_dynamic_array_remove_swap_last(values, a, removed_index, element_size);

        if (!integer) {
            *invalidated_key = last_value_key;
            cecs_dynamic_array_remove(&s->base.index_to_key, a, last_value_index, sizeof(size_t));
        }

        if (removed_index != last_value_index) {
            size_t last_page_key;
            cecs_sparse_set_key_to_index *last_key_to_index = cecs_paged_sparse_set_key_to_index(s, last_value_key, &last_page_key);
            *cecs_sparse_set_get_index_ptr(last_key_to_index, last_page_key) = (cecs_sparse_set_index){ removed_index };
        }
        return true;
    } else {
        return false;
    }
}
