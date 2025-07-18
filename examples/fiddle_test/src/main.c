// #include <cecs_math/cecs_math.h>
#include <cecs_core/containers/cecs_allocator.h>
#include <cecs_core/containers/cecs_dynarray.h>
#include <cecs_core/containers/cecs_sparse_set_v2.h>
#include <stdio.h>
void test_dynarray(cecs_allocator *allocator) {
    cecs_dynarray arr = cecs_dynarray_create_with_capacity(allocator, 10, sizeof(int));

#define ARRAY_SIZE 16
#define ARRAY_QUARTER (ARRAY_SIZE / 4)
#define ARRAY_HALF (ARRAY_SIZE / 2)
#define ARRAY_THREE_QUARTERS (ARRAY_SIZE * 3 / 4)
    for (int i = 0; i < ARRAY_SIZE; ++i) {
        int *value = cecs_dynarray_push(&arr, allocator, sizeof(int));
        *value = i;
        printf("Array count: %zu, capacity: %zu\n", arr.values_used, arr.values_capacity);
    }
    // mutate some in the middle
    for (size_t i = ARRAY_QUARTER; i < ARRAY_THREE_QUARTERS; ++i) {
        int *value = cecs_dynarray_get_mut(&arr, i, sizeof(int));
        *value += 10;
    }

    // print values
    for (size_t i = 0; i < arr.values_used; ++i) {
        const int *value = cecs_dynarray_get(&arr, i, sizeof(int));
        printf("Value at index %zu: %d\n", i, *value);
    }

    // statistics
    printf("Array used: %zu, capacity: %zu\n", arr.values_used, arr.values_capacity);
}

void test_sparse_set(cecs_allocator *allocator) {
    cecs_sparse_set set = cecs_sparse_set_create_with_capacity(allocator, 10, sizeof(int));

    for (size_t i = 0; i < 10; ++i) {
        int *value = cecs_sparse_set_insert_expect(&set, allocator, i, sizeof(int));
        *value = i * 2;
        printf("Inserted value %d at key %zu\n", *value, i);
    }

    // print values
    for (size_t i = 0; i < 10; ++i) {
        const int *value = cecs_sparse_set_get_value(&set, i, sizeof(int));
        printf("Value at key %zu: %d\n", i, *value);
    }

    // remove some values
    for (size_t i = 0; i < 5; ++i) {
        cecs_sparse_set_remove_expect(&set, allocator, i, sizeof(int));
        printf("Removed value at key %zu\n", i);
    }

    // print final size
    const size_t value_count = cecs_sparse_set_value_count(&set);
    printf("Sparse set size after removals: %zu\n", value_count);

    const size_t *keys = cecs_sparse_set_get_sparse_key_by_index(&set, cecs_dense_index_create_valid(0));
    const int *values = cecs_sparse_set_get_value_by_index(&set, cecs_dense_index_create_valid(0), sizeof(int));
    // print all pairs
    for (size_t i = 0; i < value_count; ++i) {
        printf("Key: %zu, Value: %d\n", keys[i], values[i]);
    }
}

int main(void) {
    cecs_allocator allocator = cecs_allocator_create_implicit_arena(1024, 4);

    // test_dynarray(&allocator);
    test_sparse_set(&allocator);

    // cleanup
    cecs_allocator_destroy(&allocator);
}