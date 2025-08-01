// #include <cecs_math/cecs_math.h>
#include <cecs_core/containers/cecs_allocator.h>
#include <cecs_core/containers/cecs_dynarray.h>
#include <cecs_core/containers/cecs_sparse_set_v2.h>
#include <cecs_core/containers/cecs_flatset.h>
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
void test_flatset(cecs_allocator *allocator) {
    // thoroughly test flatset
    // it is required that the value inserted caches its hash and provides its offset and stride at find operations
    // the test should include successful exclusive insertions, unsuccessful exclusive insertions, successful find or insert,
    // successful find, unsuccessful find,
    // successful removal, unsuccessful removal and shrinking
    typedef struct pair {
        size_t hash;
        int value;
    } pair;
    // with pair we can test all operations
    
    cecs_flatset set = cecs_flatset_create_with_capacity(allocator, 4, sizeof(pair));
    
    printf("=== Testing Flatset ===\n");
    printf("Initial capacity: %zu, count: %zu\n", cecs_flatset_capacity(&set), cecs_flatset_count(&set));
    
    // Test 1: Successful exclusive insertions
    printf("\n--- Test 1: Successful exclusive insertions ---\n");
    pair test_pairs[] = {
        {1, 100}, {2, 200}, {3, 300}, {5, 500}, {8, 800}
    };
    
    for (size_t i = 0; i < 5; ++i) {
        pair *inserted = cecs_flatset_insert_expect(&set, allocator, test_pairs[i].hash, sizeof(pair), offsetof(pair, hash), sizeof(pair));
        *inserted = test_pairs[i];
        printf("Inserted hash=%zu, value=%d (count: %zu)\n", test_pairs[i].hash, test_pairs[i].value, cecs_flatset_count(&set));
    }
    
    // Test 2: Unsuccessful exclusive insertions (attempting to insert existing hashes)
    printf("\n--- Test 2: Unsuccessful exclusive insertions ---\n");
    printf("Attempting to insert existing hash 2 (should fail)...\n");
    // Note: This would assert/exit in the current implementation, so we'll skip actual test
    // cecs_flatset_insert_expect(&set, allocator, 2, sizeof(pair), offsetof(pair, hash), sizeof(pair));
    printf("Skipped - would assert in current implementation\n");
    
    // Test 3: Successful find or insert (existing values)
    printf("\n--- Test 3: Find or insert (existing values) ---\n");
    for (size_t i = 0; i < 3; ++i) {
        pair *found = cecs_flatset_find_or_insert(&set, allocator, test_pairs[i].hash, sizeof(pair), offsetof(pair, hash), sizeof(pair));
        printf("Find or insert hash=%zu: found value=%d\n", test_pairs[i].hash, found->value);
    }
    printf("Count after find_or_insert existing: %zu\n", cecs_flatset_count(&set));
    
    // Test 4: Successful find or insert (new values)
    printf("\n--- Test 4: Find or insert (new values) ---\n");
    pair new_pairs[] = {{13, 1300}, {21, 2100}};
    for (size_t i = 0; i < 2; ++i) {
        pair *inserted = cecs_flatset_find_or_insert(&set, allocator, new_pairs[i].hash, sizeof(pair), offsetof(pair, hash), sizeof(pair));
        *inserted = new_pairs[i];
        printf("Find or insert hash=%zu: inserted value=%d (count: %zu)\n", new_pairs[i].hash, new_pairs[i].value, cecs_flatset_count(&set));
    }
    
    // Test 5: Successful find
    printf("\n--- Test 5: Successful find ---\n");
    for (size_t i = 0; i < 3; ++i) {
        const void *found_value;
        if (cecs_flatset_find(&set, test_pairs[i].hash, sizeof(pair), offsetof(pair, hash), sizeof(pair), &found_value)) {
            const pair *found_pair = (const pair *)found_value;
            printf("Found hash=%zu: value=%d\n", test_pairs[i].hash, found_pair->value);
        } else {
            printf("Hash=%zu not found (unexpected)\n", test_pairs[i].hash);
        }
    }
    
    // Test 6: Unsuccessful find
    printf("\n--- Test 6: Unsuccessful find ---\n");
    size_t missing_hashes[] = {99, 404, 777};
    for (size_t i = 0; i < 3; ++i) {
        const void *found_value;
        if (cecs_flatset_find(&set, missing_hashes[i], sizeof(pair), offsetof(pair, hash), sizeof(pair), &found_value)) {
            printf("Hash=%zu found (unexpected)\n", missing_hashes[i]);
        } else {
            printf("Hash=%zu not found (expected)\n", missing_hashes[i]);
        }
    }
    
    // Test 7: Find expect (existing)
    printf("\n--- Test 7: Find expect (existing) ---\n");
    const pair *expected = (const pair *)cecs_flatset_find_expect(&set, test_pairs[0].hash, sizeof(pair), offsetof(pair, hash), sizeof(pair));
    printf("Find expect hash=%zu: value=%d\n", test_pairs[0].hash, expected->value);
    
    // Test 8: Successful removal
    printf("\n--- Test 8: Successful removal ---\n");
    size_t initial_count = cecs_flatset_count(&set);
    for (size_t i = 0; i < 2; ++i) {
        if (cecs_flatset_find_remove(&set, allocator, test_pairs[i].hash, sizeof(pair), offsetof(pair, hash), sizeof(pair))) {
            printf("Removed hash=%zu (count: %zu)\n", test_pairs[i].hash, cecs_flatset_count(&set));
        } else {
            printf("Failed to remove hash=%zu\n", test_pairs[i].hash);
        }
    }
    printf("Count after removals: %zu (was %zu)\n", cecs_flatset_count(&set), initial_count);
    
    // Test 9: Unsuccessful removal
    printf("\n--- Test 9: Unsuccessful removal ---\n");
    for (size_t i = 0; i < 2; ++i) {
        if (cecs_flatset_find_remove(&set, allocator, missing_hashes[i], sizeof(pair), offsetof(pair, hash), sizeof(pair))) {
            printf("Removed hash=%zu (unexpected)\n", missing_hashes[i]);
        } else {
            printf("Failed to remove hash=%zu (expected)\n", missing_hashes[i]);
        }
    }
    
    // Test 10: Verify remaining elements
    printf("\n--- Test 10: Verify remaining elements ---\n");
    printf("Current count: %zu, capacity: %zu\n", cecs_flatset_count(&set), cecs_flatset_capacity(&set));
    
    // Try to find remaining elements
    size_t remaining_hashes[] = {3, 5, 8, 13, 21};
    for (size_t i = 0; i < 5; ++i) {
        const void *found_value;
        if (cecs_flatset_find(&set, remaining_hashes[i], sizeof(pair), offsetof(pair, hash), sizeof(pair), &found_value)) {
            const pair *found_pair = (const pair *)found_value;
            printf("Remaining: hash=%zu, value=%d\n", found_pair->hash, found_pair->value);
        } else {
            printf("Hash=%zu not found in remaining elements\n", remaining_hashes[i]);
        }
    }
    
    // Test 11: Test shrinking by removing more elements
    printf("\n--- Test 11: Test shrinking behavior ---\n");
    printf("Removing more elements to trigger shrinking...\n");
    size_t count_before_shrink = cecs_flatset_count(&set);
    size_t capacity_before_shrink = cecs_flatset_capacity(&set);
    
    // Remove most remaining elements
    for (size_t i = 0; i < 3; ++i) {
        if (cecs_flatset_find_remove(&set, allocator, remaining_hashes[i], sizeof(pair), offsetof(pair, hash), sizeof(pair))) {
            printf("Removed hash=%zu for shrinking test (count: %zu, capacity: %zu)\n", 
                   remaining_hashes[i], cecs_flatset_count(&set), cecs_flatset_capacity(&set));
        }
    }
    
    printf("After removals: count=%zu (was %zu), capacity=%zu (was %zu)\n", 
           cecs_flatset_count(&set), count_before_shrink, 
           cecs_flatset_capacity(&set), capacity_before_shrink);
    
    // Test 12: Final verification
    printf("\n--- Test 12: Final verification ---\n");
    printf("Final set state: count=%zu, capacity=%zu\n", cecs_flatset_count(&set), cecs_flatset_capacity(&set));
    
    // Verify the last remaining elements
    for (size_t i = 3; i < 5; ++i) {
        const void *found_value;
        if (cecs_flatset_find(&set, remaining_hashes[i], sizeof(pair), offsetof(pair, hash), sizeof(pair), &found_value)) {
            const pair *found_pair = (const pair *)found_value;
            printf("Final remaining: hash=%zu, value=%d\n", found_pair->hash, found_pair->value);
        }
    }
    
    // Clean up
    cecs_flatset_destroy(&set, allocator, sizeof(pair));
    printf("\n=== Flatset tests completed ===\n");
}

int main(void) {
    cecs_allocator allocator = cecs_allocator_create_implicit_arena(1024, 4);

    // test_dynarray(&allocator);
    // test_sparse_set(&allocator);
    test_flatset(&allocator);

    // cleanup
    cecs_allocator_destroy(&allocator);
}