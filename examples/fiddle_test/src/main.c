// #include <cecs_math/cecs_math.h>
#include <cecs_core/containers/cecs_allocator.h>
#include <cecs_core/containers/cecs_dynarray.h>
#include <cecs_core/containers/cecs_sparse_set_v2.h>
#include <cecs_core/containers/cecs_flatset.h>
#include <stdio.h>
#include <stddef.h>

void test_dynarray(cecs_allocator *allocator) {
    printf("=== Testing Dynamic Array ===\n");
    
    // Test 1: Creation and basic operations
    printf("\n--- Test 1: Creation and basic push operations ---\n");
    cecs_dynarray arr = cecs_dynarray_create_with_capacity(allocator, 4, sizeof(int));
    printf("Initial count: %zu, capacity: %zu\n", cecs_dynarray_count(&arr), cecs_dynarray_capacity(&arr));

    // Test push operations with reallocation
    for (int i = 0; i < 16; ++i) {
        int *value = cecs_dynarray_push(&arr, allocator, sizeof(int));
        *value = i * 10;
        printf("Pushed %d: count=%zu, capacity=%zu\n", *value, cecs_dynarray_count(&arr), cecs_dynarray_capacity(&arr));
    }

    // Test 2: Access operations
    printf("\n--- Test 2: Access operations ---\n");
    printf("First element: %d\n", *(const int*)cecs_dynarray_first(&arr));
    printf("Last element: %d\n", *(const int*)cecs_dynarray_last(&arr, sizeof(int)));
    
    for (size_t i = 0; i < cecs_dynarray_count(&arr); i += 3) {
        const int *value = cecs_dynarray_get(&arr, i, sizeof(int));
        printf("Element at index %zu: %d\n", i, *value);
    }

    // Test 3: Mutation operations
    printf("\n--- Test 3: Mutation operations ---\n");
    for (size_t i = 4; i < 12; i += 2) {
        int *value = cecs_dynarray_get_mut(&arr, i, sizeof(int));
        *value += 1000;
        printf("Modified index %zu to: %d\n", i, *value);
    }

    // Test 4: Insert operations
    printf("\n--- Test 4: Insert operations ---\n");
    int *inserted = cecs_dynarray_insert(&arr, allocator, 5, sizeof(int));
    *inserted = 9999;
    printf("Inserted 9999 at index 5, new count: %zu\n", cecs_dynarray_count(&arr));

    int new_values[] = {-1, -2, -3};
    cecs_dynarray_insert_many_cpy(&arr, allocator, 0, new_values, 3, sizeof(int));
    printf("Inserted 3 values at beginning, new count: %zu\n", cecs_dynarray_count(&arr));

    // Test 5: Push many operations
    printf("\n--- Test 5: Push many operations ---\n");
    int *many_values = cecs_dynarray_push_many(&arr, allocator, 5, sizeof(int));
    for (size_t i = 0; i < 5; ++i) {
        many_values[i] = 2000 + (int)i;
    }
    printf("Pushed 5 more values, new count: %zu\n", cecs_dynarray_count(&arr));

    // Test 6: Removal operations
    printf("\n--- Test 6: Removal operations ---\n");
    printf("Before removal: count=%zu\n", cecs_dynarray_count(&arr));
    
    cecs_dynarray_remove(&arr, allocator, 10, sizeof(int));
    printf("Removed index 10, new count: %zu\n", cecs_dynarray_count(&arr));
    
    cecs_dynarray_remove_many(&arr, allocator, 5, 3, sizeof(int));
    printf("Removed 3 elements starting at index 5, new count: %zu\n", cecs_dynarray_count(&arr));
    
    cecs_dynarray_swap_last_pop(&arr, allocator, 2, sizeof(int));
    printf("Swap-last-pop index 2, new count: %zu\n", cecs_dynarray_count(&arr));

    // Test 7: Final state verification
    printf("\n--- Test 7: Final state verification ---\n");
    printf("Final array state - count: %zu, capacity: %zu\n", cecs_dynarray_count(&arr), cecs_dynarray_capacity(&arr));
    for (size_t i = 0; i < cecs_dynarray_count(&arr); ++i) {
        const int *value = cecs_dynarray_get(&arr, i, sizeof(int));
        printf("Final[%zu]: %d\n", i, *value);
    }

    // Test 8: Clear and cleanup
    printf("\n--- Test 8: Clear and cleanup ---\n");
    cecs_dynarray_clear(&arr);
    printf("After clear: count=%zu, capacity=%zu\n", cecs_dynarray_count(&arr), cecs_dynarray_capacity(&arr));
    
    printf("=== Dynamic Array tests completed ===\n");
}

void test_sparse_set(cecs_allocator *allocator) {
    printf("\n=== Testing Sparse Set ===\n");
    
    // Test 1: Creation and basic insertions
    printf("\n--- Test 1: Creation and basic insertions ---\n");
    cecs_sparse_set set = cecs_sparse_set_create_with_capacity(allocator, 8, sizeof(int));
    printf("Initial capacity: %u, count: %u\n", cecs_sparse_set_value_capacity(&set), cecs_sparse_set_value_count(&set));

    // Insert values with non-sequential keys to test sparse behavior
    size_t test_keys[] = {1, 5, 10, 15, 3, 7, 20, 25, 100, 50};
    for (size_t i = 0; i < 10; ++i) {
        int *value = cecs_sparse_set_insert_expect(&set, allocator, test_keys[i], sizeof(int));
        *value = (int)test_keys[i] * 100;
        printf("Inserted key=%zu, value=%d (count: %u)\n", test_keys[i], *value, cecs_sparse_set_value_count(&set));
    }

    // Test 2: Lookup operations
    printf("\n--- Test 2: Lookup operations ---\n");
    for (size_t i = 0; i < 5; ++i) {
        if (cecs_sparse_set_contains(&set, test_keys[i])) {
            const int *value = cecs_sparse_set_get_value(&set, test_keys[i], sizeof(int));
            printf("Found key=%zu, value=%d\n", test_keys[i], *value);
        } else {
            printf("Key=%zu not found (unexpected)\n", test_keys[i]);
        }
    }

    // Test failed lookups
    size_t missing_keys[] = {2, 4, 6, 99, 101};
    for (size_t i = 0; i < 5; ++i) {
        if (cecs_sparse_set_contains(&set, missing_keys[i])) {
            printf("Key=%zu found (unexpected)\n", missing_keys[i]);
        } else {
            printf("Key=%zu not found (expected)\n", missing_keys[i]);
        }
    }

    // Test 3: Index-based access
    printf("\n--- Test 3: Index-based access ---\n");
    const size_t value_count = cecs_sparse_set_value_count(&set);
    const size_t *keys = cecs_sparse_set_get_sparse_key_by_index(&set, cecs_dense_index_create_valid(0));
    const int *values = cecs_sparse_set_get_value_by_index(&set, cecs_dense_index_create_valid(0), sizeof(int));
    
    printf("All key-value pairs by index:\n");
    for (size_t i = 0; i < value_count; ++i) {
        printf("  Index %zu: key=%zu, value=%d\n", i, keys[i], values[i]);
    }

    // Test 4: Mutation operations
    printf("\n--- Test 4: Mutation operations ---\n");
    for (size_t i = 0; i < 3; ++i) {
        int *value = cecs_sparse_set_get_value_mut(&set, test_keys[i], sizeof(int));
        *value += 5000;
        printf("Modified key=%zu to value=%d\n", test_keys[i], *value);
    }

    // Test 5: Removal operations
    printf("\n--- Test 5: Removal operations ---\n");
    printf("Before removals: count=%u\n", cecs_sparse_set_value_count(&set));
    
    for (size_t i = 0; i < 4; ++i) {
        if (cecs_sparse_set_remove(&set, allocator, test_keys[i], sizeof(int))) {
            printf("Removed key=%zu (count: %u)\n", test_keys[i], cecs_sparse_set_value_count(&set));
        } else {
            printf("Failed to remove key=%zu\n", test_keys[i]);
        }
    }

    // Test failed removals
    for (size_t i = 0; i < 2; ++i) {
        if (cecs_sparse_set_remove(&set, allocator, missing_keys[i], sizeof(int))) {
            printf("Removed key=%zu (unexpected)\n", missing_keys[i]);
        } else {
            printf("Failed to remove key=%zu (expected)\n", missing_keys[i]);
        }
    }

    // Test 6: Final state verification
    printf("\n--- Test 6: Final state verification ---\n");
    printf("Final sparse set state - count: %u, capacity: %u\n", 
           cecs_sparse_set_value_count(&set), cecs_sparse_set_value_capacity(&set));
    
    const size_t final_count = cecs_sparse_set_value_count(&set);
    const size_t *final_keys = cecs_sparse_set_get_sparse_key_by_index(&set, cecs_dense_index_create_valid(0));
    const int *final_values = cecs_sparse_set_get_value_by_index(&set, cecs_dense_index_create_valid(0), sizeof(int));
    
    printf("Remaining elements:\n");
    for (size_t i = 0; i < final_count; ++i) {
        printf("  Key: %zu, Value: %d\n", final_keys[i], final_values[i]);
    }

    // Test 7: Re-insertion after removal
    printf("\n--- Test 7: Re-insertion after removal ---\n");
    int *reinserted = cecs_sparse_set_insert_expect(&set, allocator, test_keys[0], sizeof(int));
    *reinserted = 77777;
    printf("Re-inserted key=%zu with value=%d (count: %u)\n", test_keys[0], *reinserted, cecs_sparse_set_value_count(&set));

    printf("=== Sparse Set tests completed ===\n");
}

void test_flatset(cecs_allocator *allocator) {
    printf("\n=== Testing Flatset ===\n");
    
    typedef struct pair {
        size_t hash;
        int value;
    } pair;
    
    // Test 1: Creation and initial insertions
    printf("\n--- Test 1: Creation and initial insertions ---\n");
    cecs_flatset set = cecs_flatset_create_with_capacity(allocator, 4, sizeof(pair)); // 4 buckets * 8 = 32 capacity
    printf("Initial capacity: %zu, count: %zu, buckets: %zu\n", 
           cecs_flatset_capacity(&set), cecs_flatset_count(&set), cecs_flatset_bucket_count(&set));
    
    // Insert values to test basic functionality (25 elements = ~78% load factor, close to 80% threshold)
    printf("Inserting elements close to 80%% load factor threshold...\n");
    for (size_t i = 1; i <= 25; ++i) {
        pair *inserted = cecs_flatset_insert_expect(&set, allocator, i, sizeof(pair), offsetof(pair, hash), sizeof(pair));
        inserted->hash = i;
        inserted->value = (int)i * 100;
        printf("Inserted hash=%zu, value=%d (count: %zu, capacity: %zu, load: %.1f%%)\n", 
               i, inserted->value, cecs_flatset_count(&set), cecs_flatset_capacity(&set),
               (double)cecs_flatset_count(&set) / cecs_flatset_capacity(&set) * 100.0);
    }
    
    // Test 2: Trigger upward reallocation (insert one more to exceed 80%)
    printf("\n--- Test 2: Trigger upward reallocation ---\n");
    printf("Current state before triggering reallocation: count=%zu, capacity=%zu, buckets=%zu\n",
           cecs_flatset_count(&set), cecs_flatset_capacity(&set), cecs_flatset_bucket_count(&set));
    
    pair *trigger_resize = cecs_flatset_insert_expect(&set, allocator, 26, sizeof(pair), offsetof(pair, hash), sizeof(pair));
    trigger_resize->hash = 26;
    trigger_resize->value = 2600;
    printf("After inserting element 26: count=%zu, capacity=%zu, buckets=%zu (should have doubled)\n",
           cecs_flatset_count(&set), cecs_flatset_capacity(&set), cecs_flatset_bucket_count(&set));

    // Test 3: Insert more elements to fill up some space
    printf("\n--- Test 3: Insert additional elements ---\n");
    for (size_t i = 27; i <= 35; ++i) {
        pair *inserted = cecs_flatset_insert_expect(&set, allocator, i, sizeof(pair), offsetof(pair, hash), sizeof(pair));
        inserted->hash = i;
        inserted->value = (int)i * 100;
    }
    printf("After inserting more elements: count=%zu, capacity=%zu\n",
           cecs_flatset_count(&set), cecs_flatset_capacity(&set));

    // Test 4: Find operations
    printf("\n--- Test 4: Find operations ---\n");
    for (size_t i = 1; i <= 5; ++i) {
        const void *found_value;
        if (cecs_flatset_find(&set, i, sizeof(pair), offsetof(pair, hash), sizeof(pair), &found_value)) {
            const pair *found_pair = (const pair *)found_value;
            printf("Found hash=%zu: value=%d\n", found_pair->hash, found_pair->value);
        } else {
            printf("Hash=%zu not found (unexpected)\n", i);
        }
    }

    // Test unsuccessful finds
    size_t missing_hashes[] = {99, 404, 777};
    for (size_t i = 0; i < 3; ++i) {
        const void *found_value;
        if (cecs_flatset_find(&set, missing_hashes[i], sizeof(pair), offsetof(pair, hash), sizeof(pair), &found_value)) {
            printf("Hash=%zu found (unexpected)\n", missing_hashes[i]);
        } else {
            printf("Hash=%zu not found (expected)\n", missing_hashes[i]);
        }
    }

    // Test 5: Find or insert operations
    printf("\n--- Test 5: Find or insert operations ---\n");
    // Test with existing values
    for (size_t i = 30; i <= 32; ++i) {
        pair *found = cecs_flatset_find_or_insert(&set, allocator, i, sizeof(pair), offsetof(pair, hash), sizeof(pair));
        printf("Find or insert hash=%zu: found value=%d\n", i, found->value);
    }
    
    // Test with new values
    pair new_pairs[] = {{100, 10000}, {200, 20000}, {300, 30000}};
    for (size_t i = 0; i < 3; ++i) {
        pair *inserted = cecs_flatset_find_or_insert(&set, allocator, new_pairs[i].hash, sizeof(pair), offsetof(pair, hash), sizeof(pair));
        *inserted = new_pairs[i];
        printf("Find or insert hash=%zu: inserted value=%d (count: %zu)\n", 
               new_pairs[i].hash, new_pairs[i].value, cecs_flatset_count(&set));
    }

    // Test 6: Remove elements to trigger downward reallocation
    printf("\n--- Test 6: Remove elements to trigger downward reallocation ---\n");
    printf("Current state before mass removal: count=%zu, capacity=%zu, buckets=%zu\n",
           cecs_flatset_count(&set), cecs_flatset_capacity(&set), cecs_flatset_bucket_count(&set));
    
    // Calculate 20% threshold
    size_t capacity = cecs_flatset_capacity(&set);
    size_t threshold_20_percent = capacity / 5; // 20% of capacity
    size_t current_count = cecs_flatset_count(&set);
    size_t elements_to_remove = current_count - threshold_20_percent + 1; // Remove enough to go below 20%
    
    printf("Need to remove %zu elements to go below 20%% threshold (%zu elements)\n", 
           elements_to_remove, threshold_20_percent);
    
    // Remove elements systematically
    for (size_t i = 1; i <= elements_to_remove && i <= 35; ++i) {
        if (cecs_flatset_find_remove(&set, allocator, i, sizeof(pair), offsetof(pair, hash), sizeof(pair))) {
            printf("Removed hash=%zu (count: %zu, capacity: %zu, load: %.1f%%)\n", 
                   i, cecs_flatset_count(&set), cecs_flatset_capacity(&set),
                   (double)cecs_flatset_count(&set) / cecs_flatset_capacity(&set) * 100.0);
        }
    }
    
    printf("After mass removal: count=%zu, capacity=%zu, buckets=%zu (should have shrunk)\n",
           cecs_flatset_count(&set), cecs_flatset_capacity(&set), cecs_flatset_bucket_count(&set));

    // Test 7: Verify remaining elements
    printf("\n--- Test 7: Verify remaining elements ---\n");
    printf("Checking remaining elements...\n");
    
    // Check elements that should still be there
    for (size_t i = elements_to_remove + 1; i <= 35; ++i) {
        const void *found_value;
        if (cecs_flatset_find(&set, i, sizeof(pair), offsetof(pair, hash), sizeof(pair), &found_value)) {
            const pair *found_pair = (const pair *)found_value;
            printf("Remaining: hash=%zu, value=%d\n", found_pair->hash, found_pair->value);
        }
    }
    
    // Check the new pairs we added
    for (size_t i = 0; i < 3; ++i) {
        const void *found_value;
        if (cecs_flatset_find(&set, new_pairs[i].hash, sizeof(pair), offsetof(pair, hash), sizeof(pair), &found_value)) {
            const pair *found_pair = (const pair *)found_value;
            printf("Remaining new pair: hash=%zu, value=%d\n", found_pair->hash, found_pair->value);
        }
    }

    // Test 8: Test removal of non-existent elements
    printf("\n--- Test 8: Test removal of non-existent elements ---\n");
    for (size_t i = 0; i < 3; ++i) {
        if (cecs_flatset_find_remove(&set, allocator, missing_hashes[i], sizeof(pair), offsetof(pair, hash), sizeof(pair))) {
            printf("Removed hash=%zu (unexpected)\n", missing_hashes[i]);
        } else {
            printf("Failed to remove hash=%zu (expected)\n", missing_hashes[i]);
        }
    }

    // Test 9: Final state verification
    printf("\n--- Test 9: Final state verification ---\n");
    printf("Final flatset state: count=%zu, capacity=%zu, buckets=%zu, load=%.1f%%\n", 
           cecs_flatset_count(&set), cecs_flatset_capacity(&set), cecs_flatset_bucket_count(&set),
           (double)cecs_flatset_count(&set) / cecs_flatset_capacity(&set) * 100.0);

    // Clean up
    cecs_flatset_destroy(&set, allocator, sizeof(pair));
    printf("=== Flatset tests completed ===\n");
}

int main(void) {
    cecs_allocator allocator = cecs_allocator_create_implicit_arena(8192, 8); // Increased size for comprehensive tests

    test_dynarray(&allocator);
    test_sparse_set(&allocator);
    test_flatset(&allocator);

    // cleanup
    cecs_allocator_destroy(&allocator);
    return 0;
}