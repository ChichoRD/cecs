#include "sparse_set.h"

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

sparse_set sparse_set_create_with_capacity(cecs_arena* a, size_t element_capacity, size_t element_size) {
    return (sparse_set) {
        .elements =
            TAGGED_UNION_CREATE(any_elements, sparse_set_elements, list_create_with_capacity(a, element_capacity * element_size)),
            .indices = displaced_set_create_with_capacity(a, sizeof(dense_index) * element_capacity),
            .keys = list_create_with_capacity(a, sizeof(size_t) * element_capacity),
    };
}

sparse_set sparse_set_create_of_integers_with_capacity(cecs_arena* a, size_t element_capacity, size_t element_size) {
    return (sparse_set) {
        .elements =
            TAGGED_UNION_CREATE(integer_elements, sparse_set_elements, list_create_with_capacity(a, element_capacity * element_size)),
            .indices = displaced_set_create_with_capacity(a, sizeof(dense_index) * element_capacity),
            .keys = list_create(),
    };
}

optional_element sparse_set_get(const sparse_set* s, size_t key, size_t element_size) {
    if (!displaced_set_contains_index(&s->indices, key)) {
        return OPTION_CREATE_NONE_STRUCT(optional_element);
    }

    dense_index index = *DISPLACED_SET_GET(dense_index, &s->indices, key);
    if (OPTION_IS_SOME(dense_index, index)) {
        return OPTION_CREATE_SOME_STRUCT(
            optional_element,
            list_get(&TAGGED_UNION_GET_UNCHECKED(any_elements, s->elements), OPTION_GET(dense_index, index), element_size)
        );
    }
    else {
        return OPTION_CREATE_NONE_STRUCT(optional_element);
    }
}

void* sparse_set_get_unchecked(const sparse_set* s, size_t key, size_t element_size) {
    return OPTION_GET(optional_element, sparse_set_get(s, key, element_size));
}

bool sparse_set_is_empty(const sparse_set* s) {
    return TAGGED_UNION_GET_UNCHECKED(any_elements, s->elements).count == 0;
}

void* sparse_set_set(sparse_set* s, cecs_arena* a, size_t key, void* element, size_t element_size) {
    if (TAGGED_UNION_IS(integer_elements, sparse_set_elements, s->elements)) {
        ASSERT_INTEGER_DEREFERENCE_EQUALS(element_size, element, key);
    }

    if (displaced_set_contains_index(&s->indices, key)) {
        dense_index* index = DISPLACED_SET_GET(dense_index, &s->indices, key);
        if (OPTION_IS_SOME(dense_index, *index)) {
            size_t element_index = OPTION_GET(dense_index, *index);
            if (TAGGED_UNION_IS(any_elements, sparse_set_elements, s->elements))
                LIST_SET(size_t, &s->keys, element_index, &key);
            return list_set(&TAGGED_UNION_GET_UNCHECKED(any_elements, s->elements), element_index, element, element_size);
        }
        else {
            // TODO: watch over (implicit first member cast)
            *index = OPTION_CREATE_SOME_STRUCT(dense_index, list_count_of_size(
                &TAGGED_UNION_GET_UNCHECKED(any_elements, s->elements),
                element_size
            ));
            if (TAGGED_UNION_IS(any_elements, sparse_set_elements, s->elements))
                LIST_ADD(dense_index, &s->keys, a, &key);
            return list_add(&TAGGED_UNION_GET_UNCHECKED(any_elements, s->elements), a, element, element_size);
        }
    }
    else {
        DISPLACED_SET_SET(dense_index, &s->indices, a, key, &OPTION_CREATE_SOME_STRUCT(
            dense_index,
            list_count_of_size(&TAGGED_UNION_GET_UNCHECKED(any_elements, s->elements), element_size)
        ));
        if (TAGGED_UNION_IS(any_elements, sparse_set_elements, s->elements))
            LIST_ADD(size_t, &s->keys, a, &key);
        return list_add(&TAGGED_UNION_GET_UNCHECKED(any_elements, s->elements), a, element, element_size);
    }
}

bool sparse_set_remove(sparse_set* s, cecs_arena* a, size_t key, void* out_removed_element, size_t element_size) {
    if (!displaced_set_contains_index(&s->indices, key)
        || sparse_set_is_empty(s)) {
        memset(out_removed_element, 0, element_size);
        return false;
    }

    dense_index* index = DISPLACED_SET_GET(dense_index, &s->indices, key);
    if (OPTION_IS_SOME(dense_index, *index)) {
        size_t removed_index = OPTION_GET(dense_index, *index);
        memcpy(
            out_removed_element,
            list_get(&TAGGED_UNION_GET_UNCHECKED(any_elements, s->elements), removed_index, element_size),
            element_size
        );
        assert(*(size_t*)out_removed_element != 0);

        size_t last_element_index = list_count_of_size(&TAGGED_UNION_GET_UNCHECKED(any_elements, s->elements), element_size) - 1;
        void* swapped_last = list_remove_swap_last(&TAGGED_UNION_GET_UNCHECKED(any_elements, s->elements), a, removed_index, element_size);
        {
            if (TAGGED_UNION_IS(any_elements, sparse_set_elements, s->elements)) {
                *DISPLACED_SET_GET(
                    dense_index,
                    &s->indices,
                    *LIST_REMOVE_SWAP_LAST(size_t, &s->keys, a, removed_index)
                ) = OPTION_CREATE_SOME_STRUCT(dense_index, removed_index);
            }
            else {
                *DISPLACED_SET_GET(
                    dense_index,
                    &s->indices,
                    *((size_t*)swapped_last)
                ) = OPTION_CREATE_SOME_STRUCT(dense_index, removed_index);
            }
        }
        *index = OPTION_CREATE_NONE_STRUCT(dense_index);
        return true;
    }
    else {
        memset(out_removed_element, 0, element_size);
        return false;
    }
}

paged_sparse_set paged_sparse_set_create() {
    paged_sparse_set set = (paged_sparse_set){
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
    paged_sparse_set set = (paged_sparse_set){
        .elements =
        TAGGED_UNION_CREATE(integer_elements, sparse_set_elements, list_create()),
        .keys = list_create(),
    };
    for (size_t i = 0; i < PAGED_SPARSE_SET_PAGE_COUNT; i++) {
        set.indices[i] = displaced_set_create();
    }
    return set;
}

paged_sparse_set paged_sparse_set_create_with_capacity(cecs_arena* a, size_t element_capacity, size_t element_size) {
    paged_sparse_set set = (paged_sparse_set){
        .elements =
        TAGGED_UNION_CREATE(any_elements, sparse_set_elements, list_create_with_capacity(a, element_capacity * element_size)),
        .keys = list_create_with_capacity(a, sizeof(size_t) * element_capacity),
    };
    for (size_t i = 0; i < PAGED_SPARSE_SET_PAGE_COUNT; i++) {
        set.indices[i] = displaced_set_create_with_capacity(a, (sizeof(dense_index) * element_capacity) >> PAGED_SPARSE_SET_PAGE_COUNT_LOG2);
    }
    return set;
}

paged_sparse_set paged_sparse_set_create_of_integers_with_capacity(cecs_arena* a, size_t element_capacity, size_t element_size) {
    paged_sparse_set set = (paged_sparse_set){
        .elements =
        TAGGED_UNION_CREATE(integer_elements, sparse_set_elements, list_create_with_capacity(a, element_capacity * element_size)),
        .keys = list_create(),
    };
    for (size_t i = 0; i < PAGED_SPARSE_SET_PAGE_COUNT; i++) {
        set.indices[i] = displaced_set_create_with_capacity(a, (sizeof(dense_index) * element_capacity) >> PAGED_SPARSE_SET_PAGE_COUNT_LOG2);
    }
    return set;
}

optional_element paged_sparse_set_get(const paged_sparse_set* s, size_t key, size_t element_size) {
    uint8_t page_index = paged_sparse_set_page_index(key);
    const displaced_set* indices = &s->indices[page_index];
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
    }
    else {
        return OPTION_CREATE_NONE_STRUCT(optional_element);
    }
}

void* paged_sparse_set_get_unchecked(const paged_sparse_set* s, size_t key, size_t element_size) {
    return OPTION_GET(optional_element, paged_sparse_set_get(s, key, element_size));
}

bool paged_sparse_set_is_empty(const paged_sparse_set* s) {
    return TAGGED_UNION_GET_UNCHECKED(any_elements, s->elements).count == 0;
}

void* paged_sparse_set_set(paged_sparse_set* s, cecs_arena* a, size_t key, void* element, size_t element_size) {
    uint8_t page_index = paged_sparse_set_page_index(key);
    displaced_set* indices = &s->indices[page_index];
    size_t page_key = paged_sparse_set_page_key(key, page_index);
    if (TAGGED_UNION_IS(integer_elements, sparse_set_elements, s->elements)) {
        ASSERT_INTEGER_DEREFERENCE_EQUALS(element_size, element, key);
    }

    if (displaced_set_contains_index(indices, page_key)) {
        dense_index* index = DISPLACED_SET_GET(dense_index, indices, page_key);
        if (OPTION_IS_SOME(dense_index, *index)) {
            size_t element_index = OPTION_GET(dense_index, *index);
            if (TAGGED_UNION_IS(any_elements, sparse_set_elements, s->elements))
                LIST_SET(size_t, &s->keys, element_index, &key);
            return list_set(&TAGGED_UNION_GET_UNCHECKED(any_elements, s->elements), element_index, element, element_size);
        }
        else {
            *index = OPTION_CREATE_SOME_STRUCT(dense_index, list_count_of_size(
                &TAGGED_UNION_GET_UNCHECKED(any_elements, s->elements),
                element_size
            ));
            if (TAGGED_UNION_IS(any_elements, sparse_set_elements, s->elements))
                LIST_ADD(dense_index, &s->keys, a, &key);
            return list_add(&TAGGED_UNION_GET_UNCHECKED(any_elements, s->elements), a, element, element_size);
        }
    }
    else {
        DISPLACED_SET_SET(dense_index, indices, a, page_key, &OPTION_CREATE_SOME_STRUCT(
            dense_index,
            list_count_of_size(&TAGGED_UNION_GET_UNCHECKED(any_elements, s->elements), element_size)
        ));
        if (TAGGED_UNION_IS(any_elements, sparse_set_elements, s->elements))
            LIST_ADD(size_t, &s->keys, a, &key);
        return list_add(&TAGGED_UNION_GET_UNCHECKED(any_elements, s->elements), a, element, element_size);
    }
}

bool paged_sparse_set_remove(paged_sparse_set* s, cecs_arena* a, size_t key, void* out_removed_element, size_t element_size) {
    uint8_t page_index = paged_sparse_set_page_index(key);
    displaced_set* indices = &s->indices[page_index];
    size_t page_key = paged_sparse_set_page_key(key, page_index);
    if (!displaced_set_contains_index(indices, page_key)
        || paged_sparse_set_is_empty(s)) {
        memset(out_removed_element, 0, element_size);
        return false;
    }

    dense_index* index = DISPLACED_SET_GET(dense_index, indices, page_key);
    if (OPTION_IS_SOME(dense_index, *index)) {
        size_t removed_index = OPTION_GET(dense_index, *index);
        memcpy(
            out_removed_element,
            list_get(&TAGGED_UNION_GET_UNCHECKED(any_elements, s->elements), removed_index, element_size),
            element_size
        );
        void* swapped_last = list_remove_swap_last(&TAGGED_UNION_GET_UNCHECKED(any_elements, s->elements), a, removed_index, element_size);
        if (TAGGED_UNION_IS(any_elements, sparse_set_elements, s->elements)) {
            DISPLACED_SET_SET(
                dense_index,
                indices,
                a,
                *LIST_REMOVE_SWAP_LAST(size_t, &s->keys, a, removed_index),
                &OPTION_CREATE_SOME_STRUCT(dense_index, removed_index)
            );
        }
        else {
            DISPLACED_SET_SET(
                dense_index,
                indices,
                a,
                *((size_t*)swapped_last),
                &OPTION_CREATE_SOME_STRUCT(dense_index, removed_index)
            );
        }
        *index = OPTION_CREATE_NONE_STRUCT(dense_index);
        return true;
    }
    else {
        memset(out_removed_element, 0, element_size);
        return false;
    }
}
