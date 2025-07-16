// #include <cecs_math/cecs_math.h>
#include <cecs_core/containers/cecs_allocator.h>
#include <cecs_core/containers/cecs_dynarray.h>
#include <stdio.h>

int main(void) {
    cecs_allocator allocator = cecs_allocator_create_implicit_arena(1024, 4);
    cecs_dynarray arr = cecs_dynarray_create_with_capacity(&allocator, 10, sizeof(int));

    #define ARRAY_SIZE 16
    #define ARRAY_QUARTER (ARRAY_SIZE / 4)
    #define ARRAY_HALF (ARRAY_SIZE / 2)
    #define ARRAY_THREE_QUARTERS (ARRAY_SIZE * 3 / 4)
    for (int i = 0; i < ARRAY_SIZE; ++i) {
        int *value = cecs_dynarray_push(&arr, &allocator, sizeof(int));
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

    // cleanup
    cecs_allocator_destroy(&allocator);
}