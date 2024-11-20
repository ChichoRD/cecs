#include "cecs_sparse_set.h"

cecs_sparse_set cecs_sparse_set_create(void) {
    return (cecs_sparse_set) {
        .elements =
            CECS_UNION_CREATE(cecs_any_elements, cecs_sparse_set_elements, cecs_dynamic_array_create()),
            .indices = cecs_displaced_set_create(),
            .keys = cecs_dynamic_array_create(),
    };
}

cecs_sparse_set cecs_sparse_set_create_of_integers(void) {
    return (cecs_sparse_set) {
        .elements =
            CECS_UNION_CREATE(cecs_integer_elements, cecs_sparse_set_elements, cecs_dynamic_array_create()),
            .indices = cecs_displaced_set_create(),
            .keys = cecs_dynamic_array_create(),
    };
}

cecs_sparse_set cecs_sparse_set_create_with_capacity(cecs_arena* a, size_t element_capacity, size_t element_size) {
    return (cecs_sparse_set) {
        .elements =
            CECS_UNION_CREATE(cecs_any_elements, cecs_sparse_set_elements, cecs_dynamic_array_create_with_capacity(a, element_capacity * element_size)),
            .indices = cecs_displaced_set_create_with_capacity(a, sizeof(cecs_dense_index) * element_capacity),
            .keys = cecs_dynamic_array_create_with_capacity(a, sizeof(size_t) * element_capacity),
    };
}

cecs_sparse_set cecs_sparse_set_create_of_integers_with_capacity(cecs_arena* a, size_t element_capacity, size_t element_size) {
    return (cecs_sparse_set) {
        .elements =
            CECS_UNION_CREATE(cecs_integer_elements, cecs_sparse_set_elements, cecs_dynamic_array_create_with_capacity(a, element_capacity * element_size)),
            .indices = cecs_displaced_set_create_with_capacity(a, sizeof(cecs_dense_index) * element_capacity),
            .keys = cecs_dynamic_array_create(),
    };
}

cecs_optional_element cecs_sparse_set_get(const cecs_sparse_set* s, size_t key, size_t element_size) {
    if (!cecs_displaced_set_contains_index(&s->indices, key)) {
        return CECS_OPTION_CREATE_NONE_STRUCT(cecs_optional_element);
    }

    cecs_dense_index index = *CECS_DISPLACED_SET_GET(cecs_dense_index, &s->indices, key);
    if (CECS_OPTION_IS_SOME(cecs_dense_index, index)) {
        return CECS_OPTION_CREATE_SOME_STRUCT(
            cecs_optional_element,
            cecs_dynamic_array_get(&CECS_UNION_GET_UNCHECKED(cecs_any_elements, s->elements), CECS_OPTION_GET(cecs_dense_index, index), element_size)
        );
    }
    else {
        return CECS_OPTION_CREATE_NONE_STRUCT(cecs_optional_element);
    }
}

void* cecs_sparse_set_get_unchecked(const cecs_sparse_set* s, size_t key, size_t element_size) {
    return CECS_OPTION_GET(cecs_optional_element, cecs_sparse_set_get(s, key, element_size));
}

bool cecs_sparse_set_is_empty(const cecs_sparse_set* s) {
    return CECS_UNION_GET_UNCHECKED(cecs_any_elements, s->elements).count == 0;
}

void* cecs_sparse_set_set(cecs_sparse_set* s, cecs_arena* a, size_t key, void* element, size_t element_size) {
    if (CECS_UNION_IS(cecs_integer_elements, cecs_sparse_set_elements, s->elements)) {
        CECS_ASSERT_INTEGER_DEREFERENCE_EQUALS(element_size, element, key);
    }

    if (cecs_displaced_set_contains_index(&s->indices, key)) {
        cecs_dense_index* index = CECS_DISPLACED_SET_GET(cecs_dense_index, &s->indices, key);
        if (CECS_OPTION_IS_SOME(cecs_dense_index, *index)) {
            size_t element_index = CECS_OPTION_GET(cecs_dense_index, *index);
            if (CECS_UNION_IS(cecs_any_elements, cecs_sparse_set_elements, s->elements))
                CECS_DYNAMIC_ARRAY_SET(size_t, &s->keys, element_index, &key);
            return cecs_dynamic_array_set(&CECS_UNION_GET_UNCHECKED(cecs_any_elements, s->elements), element_index, element, element_size);
        }
        else {
            // TODO: watch over (implicit first member cast)
            *index = CECS_OPTION_CREATE_SOME_STRUCT(cecs_dense_index, cecs_dynamic_array_count_of_size(
                &CECS_UNION_GET_UNCHECKED(cecs_any_elements, s->elements),
                element_size
            ));
            if (CECS_UNION_IS(cecs_any_elements, cecs_sparse_set_elements, s->elements))
                CECS_DYNAMIC_ARRAY_ADD(cecs_dense_index, &s->keys, a, &key);
            return cecs_dynamic_array_add(&CECS_UNION_GET_UNCHECKED(cecs_any_elements, s->elements), a, element, element_size);
        }
    }
    else {
        CECS_DISPLACED_SET_SET(cecs_dense_index, &s->indices, a, key, &CECS_OPTION_CREATE_SOME_STRUCT(
            cecs_dense_index,
            cecs_dynamic_array_count_of_size(&CECS_UNION_GET_UNCHECKED(cecs_any_elements, s->elements), element_size)
        ));
        if (CECS_UNION_IS(cecs_any_elements, cecs_sparse_set_elements, s->elements))
            CECS_DYNAMIC_ARRAY_ADD(size_t, &s->keys, a, &key);
        return cecs_dynamic_array_add(&CECS_UNION_GET_UNCHECKED(cecs_any_elements, s->elements), a, element, element_size);
    }
}

bool cecs_sparse_set_remove(cecs_sparse_set* s, cecs_arena* a, size_t key, void* out_removed_element, size_t element_size) {
    if (!cecs_displaced_set_contains_index(&s->indices, key)
        || cecs_sparse_set_is_empty(s)) {
        memset(out_removed_element, 0, element_size);
        return false;
    }

    cecs_dense_index* index = CECS_DISPLACED_SET_GET(cecs_dense_index, &s->indices, key);
    if (CECS_OPTION_IS_SOME(cecs_dense_index, *index)) {
        size_t removed_index = CECS_OPTION_GET(cecs_dense_index, *index);
        memcpy(
            out_removed_element,
            cecs_dynamic_array_get(&CECS_UNION_GET_UNCHECKED(cecs_any_elements, s->elements), removed_index, element_size),
            element_size
        );
        assert(*(size_t*)out_removed_element != 0);

        size_t last_element_index = cecs_dynamic_array_count_of_size(&CECS_UNION_GET_UNCHECKED(cecs_any_elements, s->elements), element_size) - 1;
        void* swapped_last = cecs_dynamic_array_remove_swap_last(&CECS_UNION_GET_UNCHECKED(cecs_any_elements, s->elements), a, removed_index, element_size);
        {
            if (CECS_UNION_IS(cecs_any_elements, cecs_sparse_set_elements, s->elements)) {
                *CECS_DISPLACED_SET_GET(
                    cecs_dense_index,
                    &s->indices,
                    *CECS_DYNAMIC_ARRAY_REMOVE_SWAP_LAST(size_t, &s->keys, a, removed_index)
                ) = CECS_OPTION_CREATE_SOME_STRUCT(cecs_dense_index, removed_index);
            }
            else {
                *CECS_DISPLACED_SET_GET(
                    cecs_dense_index,
                    &s->indices,
                    *((size_t*)swapped_last)
                ) = CECS_OPTION_CREATE_SOME_STRUCT(cecs_dense_index, removed_index);
            }
        }
        *index = CECS_OPTION_CREATE_NONE_STRUCT(cecs_dense_index);
        return true;
    }
    else {
        memset(out_removed_element, 0, element_size);
        return false;
    }
}

cecs_paged_sparse_set cecs_paged_sparse_set_create(void) {
    cecs_paged_sparse_set set = (cecs_paged_sparse_set){
        .elements =
        CECS_UNION_CREATE(cecs_any_elements, cecs_sparse_set_elements, cecs_dynamic_array_create()),
        .keys = cecs_dynamic_array_create(),
    };
    for (size_t i = 0; i < CECS_PAGED_SPARSE_SET_PAGE_COUNT; i++) {
        set.indices[i] = cecs_displaced_set_create();
    }
    return set;
}

cecs_paged_sparse_set cecs_paged_sparse_set_create_of_integers(void) {
    cecs_paged_sparse_set set = (cecs_paged_sparse_set){
        .elements =
        CECS_UNION_CREATE(cecs_integer_elements, cecs_sparse_set_elements, cecs_dynamic_array_create()),
        .keys = cecs_dynamic_array_create(),
    };
    for (size_t i = 0; i < CECS_PAGED_SPARSE_SET_PAGE_COUNT; i++) {
        set.indices[i] = cecs_displaced_set_create();
    }
    return set;
}

cecs_paged_sparse_set cecs_paged_sparse_set_create_with_capacity(cecs_arena* a, size_t element_capacity, size_t element_size) {
    cecs_paged_sparse_set set = (cecs_paged_sparse_set){
        .elements =
        CECS_UNION_CREATE(cecs_any_elements, cecs_sparse_set_elements, cecs_dynamic_array_create_with_capacity(a, element_capacity * element_size)),
        .keys = cecs_dynamic_array_create_with_capacity(a, sizeof(size_t) * element_capacity),
    };
    for (size_t i = 0; i < CECS_PAGED_SPARSE_SET_PAGE_COUNT; i++) {
        set.indices[i] = cecs_displaced_set_create_with_capacity(a, (sizeof(cecs_dense_index) * element_capacity) >> CECS_CECS_PAGED_SPARSE_SET_PAGE_COUNT_LOG2);
    }
    return set;
}

cecs_paged_sparse_set cecs_paged_sparse_set_create_of_integers_with_capacity(cecs_arena* a, size_t element_capacity, size_t element_size) {
    cecs_paged_sparse_set set = (cecs_paged_sparse_set){
        .elements =
        CECS_UNION_CREATE(cecs_integer_elements, cecs_sparse_set_elements, cecs_dynamic_array_create_with_capacity(a, element_capacity * element_size)),
        .keys = cecs_dynamic_array_create(),
    };
    for (size_t i = 0; i < CECS_PAGED_SPARSE_SET_PAGE_COUNT; i++) {
        set.indices[i] = cecs_displaced_set_create_with_capacity(a, (sizeof(cecs_dense_index) * element_capacity) >> CECS_CECS_PAGED_SPARSE_SET_PAGE_COUNT_LOG2);
    }
    return set;
}

cecs_optional_element cecs_paged_sparse_set_get(const cecs_paged_sparse_set* s, size_t key, size_t element_size) {
    uint8_t page_index = cecs_paged_sparse_set_page_index(key);
    const cecs_displaced_set* indices = &s->indices[page_index];
    size_t page_key = cecs_paged_sparse_set_page_key(key, page_index);
    if (!cecs_displaced_set_contains_index(indices, page_key)) {
        return CECS_OPTION_CREATE_NONE_STRUCT(cecs_optional_element);
    }

    cecs_dense_index index = *CECS_DISPLACED_SET_GET(cecs_dense_index, indices, page_key);
    if (CECS_OPTION_IS_SOME(cecs_dense_index, index)) {
        return CECS_OPTION_CREATE_SOME_STRUCT(
            cecs_optional_element,
            cecs_dynamic_array_get(&CECS_UNION_GET_UNCHECKED(cecs_any_elements, s->elements), CECS_OPTION_GET(cecs_dense_index, index), element_size)
        );
    }
    else {
        return CECS_OPTION_CREATE_NONE_STRUCT(cecs_optional_element);
    }
}

void* cecs_paged_sparse_set_get_unchecked(const cecs_paged_sparse_set* s, size_t key, size_t element_size) {
    return CECS_OPTION_GET(cecs_optional_element, cecs_paged_sparse_set_get(s, key, element_size));
}

bool cecs_paged_sparse_set_is_empty(const cecs_paged_sparse_set* s) {
    return CECS_UNION_GET_UNCHECKED(cecs_any_elements, s->elements).count == 0;
}

void* cecs_paged_sparse_set_set(cecs_paged_sparse_set* s, cecs_arena* a, size_t key, void* element, size_t element_size) {
    uint8_t page_index = cecs_paged_sparse_set_page_index(key);
    cecs_displaced_set* indices = &s->indices[page_index];
    size_t page_key = cecs_paged_sparse_set_page_key(key, page_index);
    if (CECS_UNION_IS(cecs_integer_elements, cecs_sparse_set_elements, s->elements)) {
        CECS_ASSERT_INTEGER_DEREFERENCE_EQUALS(element_size, element, key);
    }

    if (cecs_displaced_set_contains_index(indices, page_key)) {
        cecs_dense_index* index = CECS_DISPLACED_SET_GET(cecs_dense_index, indices, page_key);
        if (CECS_OPTION_IS_SOME(cecs_dense_index, *index)) {
            size_t element_index = CECS_OPTION_GET(cecs_dense_index, *index);
            if (CECS_UNION_IS(cecs_any_elements, cecs_sparse_set_elements, s->elements))
                CECS_DYNAMIC_ARRAY_SET(size_t, &s->keys, element_index, &key);
            return cecs_dynamic_array_set(&CECS_UNION_GET_UNCHECKED(cecs_any_elements, s->elements), element_index, element, element_size);
        }
        else {
            *index = CECS_OPTION_CREATE_SOME_STRUCT(cecs_dense_index, cecs_dynamic_array_count_of_size(
                &CECS_UNION_GET_UNCHECKED(cecs_any_elements, s->elements),
                element_size
            ));
            if (CECS_UNION_IS(cecs_any_elements, cecs_sparse_set_elements, s->elements))
                CECS_DYNAMIC_ARRAY_ADD(cecs_dense_index, &s->keys, a, &key);
            return cecs_dynamic_array_add(&CECS_UNION_GET_UNCHECKED(cecs_any_elements, s->elements), a, element, element_size);
        }
    }
    else {
        CECS_DISPLACED_SET_SET(cecs_dense_index, indices, a, page_key, &CECS_OPTION_CREATE_SOME_STRUCT(
            cecs_dense_index,
            cecs_dynamic_array_count_of_size(&CECS_UNION_GET_UNCHECKED(cecs_any_elements, s->elements), element_size)
        ));
        if (CECS_UNION_IS(cecs_any_elements, cecs_sparse_set_elements, s->elements))
            CECS_DYNAMIC_ARRAY_ADD(size_t, &s->keys, a, &key);
        return cecs_dynamic_array_add(&CECS_UNION_GET_UNCHECKED(cecs_any_elements, s->elements), a, element, element_size);
    }
}

bool cecs_paged_sparse_set_remove(cecs_paged_sparse_set* s, cecs_arena* a, size_t key, void* out_removed_element, size_t element_size) {
    uint8_t page_index = cecs_paged_sparse_set_page_index(key);
    cecs_displaced_set* indices = &s->indices[page_index];
    size_t page_key = cecs_paged_sparse_set_page_key(key, page_index);
    if (!cecs_displaced_set_contains_index(indices, page_key)
        || cecs_paged_sparse_set_is_empty(s)) {
        memset(out_removed_element, 0, element_size);
        return false;
    }

    cecs_dense_index* index = CECS_DISPLACED_SET_GET(cecs_dense_index, indices, page_key);
    if (CECS_OPTION_IS_SOME(cecs_dense_index, *index)) {
        size_t removed_index = CECS_OPTION_GET(cecs_dense_index, *index);
        memcpy(
            out_removed_element,
            cecs_dynamic_array_get(&CECS_UNION_GET_UNCHECKED(cecs_any_elements, s->elements), removed_index, element_size),
            element_size
        );
        void* swapped_last = cecs_dynamic_array_remove_swap_last(&CECS_UNION_GET_UNCHECKED(cecs_any_elements, s->elements), a, removed_index, element_size);
        if (CECS_UNION_IS(cecs_any_elements, cecs_sparse_set_elements, s->elements)) {
            CECS_DISPLACED_SET_SET(
                cecs_dense_index,
                indices,
                a,
                *CECS_DYNAMIC_ARRAY_REMOVE_SWAP_LAST(size_t, &s->keys, a, removed_index),
                &CECS_OPTION_CREATE_SOME_STRUCT(cecs_dense_index, removed_index)
            );
        }
        else {
            CECS_DISPLACED_SET_SET(
                cecs_dense_index,
                indices,
                a,
                *((size_t*)swapped_last),
                &CECS_OPTION_CREATE_SOME_STRUCT(cecs_dense_index, removed_index)
            );
        }
        *index = CECS_OPTION_CREATE_NONE_STRUCT(cecs_dense_index);
        return true;
    }
    else {
        memset(out_removed_element, 0, element_size);
        return false;
    }
}
