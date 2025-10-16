// #include <cecs_math/cecs_math.h>
#include <cecs_core/cecs_allocator.h>
#include <cecs_core/containers/cecs_dynarray.h>
#include <cecs_core/containers/cecs_sparse_set.h>
#include <cecs_core/containers/cecs_flatset.h>
#include <cecs_core/containers/cecs_flatmap.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <immintrin.h>  // For SIMD intrinsics
#include <float.h>
#include <math.h>


void test_dynarray(cecs_allocator *allocator) {
    printf("=== Testing Dynamic Array ===\n");
    
    // Test 1: Creation and basic operations
    printf("\n--- Test 1: Creation and basic push operations ---\n");
    cecs_dynarray arr = cecs_dynarray_create_with_capacity(allocator, 4, sizeof(int));
    if (cecs_dynarray_count(&arr) != 0 || cecs_dynarray_capacity(&arr) != 4) {
        fprintf(stderr, "ERROR: Initial dynarray state incorrect\n");
        assert(false && "Initial dynarray state incorrect");
        exit(EXIT_FAILURE);
    }
    printf("Initial count: %zu, capacity: %zu\n", cecs_dynarray_count(&arr), cecs_dynarray_capacity(&arr));

    // Test push operations with reallocation
    for (int i = 0; i < 16; ++i) {
        int *value = cecs_dynarray_push(&arr, allocator, sizeof(int));
        *value = i * 10;
        if (cecs_dynarray_count(&arr) != (size_t)(i + 1)) {
            fprintf(stderr, "ERROR: Count mismatch after push %d\n", i);
            assert(false && "Count mismatch after push");
            exit(EXIT_FAILURE);
        }
        printf("Pushed %d: count=%zu, capacity=%zu\n", *value, cecs_dynarray_count(&arr), cecs_dynarray_capacity(&arr));
    }

    // Test 2: Access operations
    printf("\n--- Test 2: Access operations ---\n");
    if (*(const int*)cecs_dynarray_first(&arr) != 0) {
        fprintf(stderr, "ERROR: First element incorrect\n");
        assert(false && "First element incorrect");
        exit(EXIT_FAILURE);
    }
    if (*(const int*)cecs_dynarray_last(&arr, sizeof(int)) != 150) {
        fprintf(stderr, "ERROR: Last element incorrect\n");
        assert(false && "Last element incorrect");
        exit(EXIT_FAILURE);
    }
    printf("First element: %d\n", *(const int*)cecs_dynarray_first(&arr));
    printf("Last element: %d\n", *(const int*)cecs_dynarray_last(&arr, sizeof(int)));
    
    for (size_t i = 0; i < cecs_dynarray_count(&arr); i += 3) {
        const int *value = cecs_dynarray_get(&arr, i, sizeof(int));
        if (*value != (int)(i * 10)) {
            fprintf(stderr, "ERROR: Element at index %zu incorrect: expected %d, got %d\n", i, (int)(i * 10), *value);
            assert(false && "Element value incorrect");
            exit(EXIT_FAILURE);
        }
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
    cecs_dynarray_insert_many_copy(&arr, allocator, 0, new_values, 3, sizeof(int));
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
    
    cecs_dynarray_destroy(&arr, allocator, sizeof(int));
    printf("=== Dynamic Array tests completed ===\n");
}

void test_sparse_set(cecs_allocator *allocator) {
    printf("\n=== Testing Sparse Set ===\n");
    
    // Test 1: Creation and basic insertions
    printf("\n--- Test 1: Creation and basic insertions ---\n");
    cecs_sparse_set set = cecs_sparse_set_create_with_capacity(allocator, 8, sizeof(int));
    if (cecs_sparse_set_value_count(&set) != 0) {
        fprintf(stderr, "ERROR: Initial sparse set count should be 0\n");
        assert(false && "Initial sparse set count should be 0");
        exit(EXIT_FAILURE);
    }
    printf("Initial capacity: %u, count: %u\n", cecs_sparse_set_value_capacity(&set), cecs_sparse_set_value_count(&set));

    // Insert values with non-sequential keys to test sparse behavior
    size_t test_keys[] = {1, 5, 10, 15, 3, 7, 20, 25, 100, 50};
    for (size_t i = 0; i < 10; ++i) {
        int *value = cecs_sparse_set_insert_expect(&set, allocator, test_keys[i], sizeof(int));
        *value = (int)test_keys[i] * 100;
        if (cecs_sparse_set_value_count(&set) != i + 1) {
            fprintf(stderr, "ERROR: Sparse set count mismatch after insertion %zu\n", i);
            assert(false && "Sparse set count mismatch");
            exit(EXIT_FAILURE);
        }
        printf("Inserted key=%zu, value=%d (count: %u)\n", test_keys[i], *value, cecs_sparse_set_value_count(&set));
    }

    // Test 2: Lookup operations
    printf("\n--- Test 2: Lookup operations ---\n");
    for (size_t i = 0; i < 5; ++i) {
        if (!cecs_sparse_set_contains(&set, test_keys[i])) {
            fprintf(stderr, "ERROR: Expected key %zu to be found\n", test_keys[i]);
            assert(false && "Expected key to be found");
            exit(EXIT_FAILURE);
        }
        const int *value = cecs_sparse_set_get_value(&set, test_keys[i], sizeof(int));
        if (*value != (int)test_keys[i] * 100) {
            fprintf(stderr, "ERROR: Value mismatch for key %zu\n", test_keys[i]);
            assert(false && "Value mismatch");
            exit(EXIT_FAILURE);
        }
        printf("Found key=%zu, value=%d\n", test_keys[i], *value);
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

    cecs_sparse_set_destroy(&set, allocator, sizeof(int));
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
    cecs_flatset set = cecs_flatset_create_with_capacity(allocator, 1, sizeof(pair)); // 1 bucket * 8 = 8 capacity
    if (cecs_flatset_capacity(&set) != 8 || cecs_flatset_count(&set) != 0 || cecs_flatset_bucket_count(&set) != 1) {
        fprintf(stderr, "ERROR: Initial flatset state incorrect: capacity=%zu, count=%zu, buckets=%zu\n",
                cecs_flatset_capacity(&set), cecs_flatset_count(&set), cecs_flatset_bucket_count(&set));
        assert(false && "Initial flatset state incorrect");
        exit(EXIT_FAILURE);
    }
    printf("Initial capacity: %zu, count: %zu, buckets: %zu\n", 
           cecs_flatset_capacity(&set), cecs_flatset_count(&set), cecs_flatset_bucket_count(&set));
    
    // Insert values to test basic functionality (6 elements = 75% load factor, approaching 80% threshold)
    printf("Inserting elements approaching 80%% load factor threshold...\n");
    for (size_t i = 1; i <= 6; ++i) {
        pair *inserted = cecs_flatset_insert_expect(&set, allocator, i, sizeof(pair), offsetof(pair, hash));
        inserted->hash = i;
        inserted->value = (int)i * 100;
        if (cecs_flatbucket_get_count(*cecs_flatset_get_bucket(&set, 0, sizeof(pair))) != i) {
            fprintf(stderr, "ERROR: Bucket count mismatch after insert %zu: expected %zu, got %hu\n", 
                    i, i, cecs_flatbucket_get_count(*cecs_flatset_get_bucket(&set, 0, sizeof(pair))));
            assert(false && "Bucket count mismatch");
            exit(EXIT_FAILURE);
        }
        if (cecs_flatset_count(&set) != i) {
            fprintf(stderr, "ERROR: Count mismatch after insert %zu: expected %zu, got %zu\n", i, i, cecs_flatset_count(&set));
            assert(false && "Count mismatch after insert");
            exit(EXIT_FAILURE);
        }
        if (i % 2 == 0) { // Print every 2nd element to reduce output
            printf("Inserted hash=%zu, value=%d (count: %zu, capacity: %zu, load: %.1f%%)\n", 
                   i, inserted->value, cecs_flatset_count(&set), cecs_flatset_capacity(&set),
                   (double)cecs_flatset_count(&set) / cecs_flatset_capacity(&set) * 100.0);
        }
    }
    printf("Reached 75%% load factor: count=%zu, capacity=%zu\n", 
           cecs_flatset_count(&set), cecs_flatset_capacity(&set));
    
    // Test 2: Trigger upward reallocation (insert one more to exceed 80%)
    printf("\n--- Test 2: Trigger upward reallocation ---\n");
    printf("Current state before triggering reallocation: count=%zu, capacity=%zu, buckets=%zu\n",
           cecs_flatset_count(&set), cecs_flatset_capacity(&set), cecs_flatset_bucket_count(&set));
    
    pair *trigger_resize = cecs_flatset_insert_expect(&set, allocator, 7, sizeof(pair), offsetof(pair, hash));
    trigger_resize->hash = 7;
    trigger_resize->value = 700;
    if (cecs_flatset_bucket_count(&set) != 2 || cecs_flatset_capacity(&set) != 16) {
        fprintf(stderr, "ERROR: Expected reallocation to double buckets to 2 (capacity 16), got buckets=%zu, capacity=%zu\n",
                cecs_flatset_bucket_count(&set), cecs_flatset_capacity(&set));
        assert(false && "Expected reallocation to double buckets");
        exit(EXIT_FAILURE);
    }
    printf("After inserting element 7: count=%zu, capacity=%zu, buckets=%zu (should have doubled)\n",
           cecs_flatset_count(&set), cecs_flatset_capacity(&set), cecs_flatset_bucket_count(&set));

    // Test 3: Insert more elements to utilize some of the new space
    printf("\n--- Test 3: Insert additional elements ---\n");
    for (size_t i = 8; i <= 12; ++i) {
        pair *inserted = cecs_flatset_insert_expect(&set, allocator, i, sizeof(pair), offsetof(pair, hash));
        inserted->hash = i;
        inserted->value = (int)i * 100;
    }
    if (cecs_flatset_count(&set) != 12) {
        fprintf(stderr, "ERROR: Expected count 12 after additional insertions, got %zu\n", cecs_flatset_count(&set));
        assert(false && "Expected count 12 after additional insertions");
        exit(EXIT_FAILURE);
    }
    printf("After inserting 5 more elements: count=%zu, capacity=%zu, load=%.1f%%\n",
           cecs_flatset_count(&set), cecs_flatset_capacity(&set),
           (double)cecs_flatset_count(&set) / cecs_flatset_capacity(&set) * 100.0);

    // Test 4: Find operations
    printf("\n--- Test 4: Find operations ---\n");
    for (size_t i = 1; i <= 5; ++i) {
        const void *found_value;
        if (!cecs_flatset_find(&set, i, sizeof(pair), offsetof(pair, hash), &found_value)) {
            fprintf(stderr, "ERROR: Expected to find hash %zu\n", i);
            assert(false && "Expected to find hash");
            exit(EXIT_FAILURE);
        }
        const pair *found_pair = (const pair *)found_value;
        if (found_pair->hash != i || found_pair->value != (int)i * 100) {
            fprintf(stderr, "ERROR: Found incorrect value for hash %zu\n", i);
            assert(false && "Found incorrect value for hash");
            exit(EXIT_FAILURE);
        }
        printf("Found hash=%zu: value=%d\n", found_pair->hash, found_pair->value);
    }

    // Test unsuccessful finds
    size_t missing_hashes[] = {99, 404, 777};
    for (size_t i = 0; i < 3; ++i) {
        const void *found_value;
        if (cecs_flatset_find(&set, missing_hashes[i], sizeof(pair), offsetof(pair, hash), &found_value)) {
            fprintf(stderr, "ERROR: Unexpectedly found hash %zu\n", missing_hashes[i]);
            assert(false && "Unexpectedly found hash");
            exit(EXIT_FAILURE);
        }
        printf("Hash=%zu not found (expected)\n", missing_hashes[i]);
    }

    // Test 5: Find or insert operations
    printf("\n--- Test 5: Find or insert operations ---\n");
    // Test with existing values
    for (size_t i = 10; i <= 12; ++i) {
        pair *found = cecs_flatset_find_or_insert(&set, allocator, i, sizeof(pair), offsetof(pair, hash));
        if (found->hash != i || found->value != (int)i * 100) {
            fprintf(stderr, "ERROR: Find or insert returned incorrect existing value for hash %zu\n", i);
            assert(false && "Find or insert returned incorrect existing value");
            exit(EXIT_FAILURE);
        }
        printf("Find or insert hash=%zu: found value=%d\n", i, found->value);
    }
    
    // Test with new values
    pair new_pairs[] = {{100, 10000}, {200, 20000}};
    size_t count_before_new = cecs_flatset_count(&set);
    for (size_t i = 0; i < 2; ++i) {
        pair *inserted = cecs_flatset_find_or_insert(&set, allocator, new_pairs[i].hash, sizeof(pair), offsetof(pair, hash));
        *inserted = new_pairs[i];
        if (cecs_flatset_count(&set) != count_before_new + i + 1) {
            fprintf(stderr, "ERROR: Count mismatch after find_or_insert new value %zu\n", i);
            assert(false && "Count mismatch after find_or_insert");
            exit(EXIT_FAILURE);
        }
        printf("Find or insert hash=%zu: inserted value=%d (count: %zu)\n", 
               new_pairs[i].hash, new_pairs[i].value, cecs_flatset_count(&set));
    }

    // Test 6: Remove elements to trigger downward reallocation
    printf("\n--- Test 6: Remove elements to trigger downward reallocation ---\n");
    printf("Current state before mass removal: count=%zu, capacity=%zu, buckets=%zu\n",
           cecs_flatset_count(&set), cecs_flatset_capacity(&set), cecs_flatset_bucket_count(&set));
    
    // Calculate 20% threshold for current capacity (16 * 0.2 = 3.2, so 3)
    size_t capacity = cecs_flatset_capacity(&set);
    size_t threshold_20_percent = capacity / 5; // 20% of capacity
    size_t current_count = cecs_flatset_count(&set);
    size_t elements_to_remove = current_count - threshold_20_percent + 1; // Remove enough to go below 20%
    
    printf("Current count: %zu, capacity: %zu, 20%% threshold: %zu\n", 
           current_count, capacity, threshold_20_percent);
    printf("Need to remove %zu elements to go below 20%% threshold\n", elements_to_remove);
    
    // Remove elements systematically
    size_t removed_count = 0;
    for (size_t i = 1; i <= 12 && removed_count < elements_to_remove; ++i) {
        if (cecs_flatset_find_remove(&set, allocator, i, sizeof(pair), offsetof(pair, hash))) {
            removed_count++;
            if (removed_count % 2 == 0) { // Print every 2nd removal to reduce output
                printf("Removed hash=%zu (count: %zu, capacity: %zu, load: %.1f%%)\n", 
                       i, cecs_flatset_count(&set), cecs_flatset_capacity(&set),
                       (double)cecs_flatset_count(&set) / cecs_flatset_capacity(&set) * 100.0);
            }
        }
    }
    
    printf("After mass removal: count=%zu, capacity=%zu, buckets=%zu\n",
           cecs_flatset_count(&set), cecs_flatset_capacity(&set), cecs_flatset_bucket_count(&set));

    // Test 7: Verify remaining elements
    printf("\n--- Test 7: Verify remaining elements ---\n");
    printf("Checking remaining new pairs...\n");
    
    // Check the new pairs we added (should still be there)
    for (size_t i = 0; i < 2; ++i) {
        const void *found_value;
        if (!cecs_flatset_find(&set, new_pairs[i].hash, sizeof(pair), offsetof(pair, hash), &found_value)) {
            fprintf(stderr, "ERROR: Expected to find remaining new pair hash %zu\n", new_pairs[i].hash);
            assert(false && "Expected to find remaining new pair hash");
            exit(EXIT_FAILURE);
        }
        const pair *found_pair = (const pair *)found_value;
        printf("Remaining new pair: hash=%zu, value=%d\n", found_pair->hash, found_pair->value);
    }

    // Test 8: Test removal of non-existent elements
    printf("\n--- Test 8: Test removal of non-existent elements ---\n");
    for (size_t i = 0; i < 3; ++i) {
        if (cecs_flatset_find_remove(&set, allocator, missing_hashes[i], sizeof(pair), offsetof(pair, hash))) {
            fprintf(stderr, "ERROR: Unexpectedly removed non-existent hash %zu\n", missing_hashes[i]);
            assert(false && "Unexpectedly removed non-existent hash");
            exit(EXIT_FAILURE);
        }
        printf("Failed to remove hash=%zu (expected)\n", missing_hashes[i]);
    }

    // Test 9: Insert more elements to test growing again
    printf("\n--- Test 9: Test growing again ---\n");
    printf("Adding more elements to test growth...\n");
    size_t count_before_growth = cecs_flatset_count(&set);
    for (size_t i = 500; i <= 506; ++i) {
        pair *inserted = cecs_flatset_insert_expect(&set, allocator, i, sizeof(pair), offsetof(pair, hash));
        inserted->hash = i;
        inserted->value = (int)i * 100;
    }
    if (cecs_flatset_count(&set) != count_before_growth + 7) {
        fprintf(stderr, "ERROR: Count mismatch after growth test\n");
        assert(false && "Count mismatch after growth test");
        exit(EXIT_FAILURE);
    }
    printf("After adding 7 more elements: count=%zu, capacity=%zu, load=%.1f%%\n",
           cecs_flatset_count(&set), cecs_flatset_capacity(&set),
           (double)cecs_flatset_count(&set) / cecs_flatset_capacity(&set) * 100.0);

    // Test 10: Final state verification
    printf("\n--- Test 10: Final state verification ---\n");
    printf("Final flatset state: count=%zu, capacity=%zu, buckets=%zu, load=%.1f%%\n", 
           cecs_flatset_count(&set), cecs_flatset_capacity(&set), cecs_flatset_bucket_count(&set),
           (double)cecs_flatset_count(&set) / cecs_flatset_capacity(&set) * 100.0);

    // Clean up
    cecs_flatset_destroy(&set, allocator, sizeof(pair));
    printf("=== Flatset tests completed ===\n");
}

void test_flatset_simd(cecs_allocator *allocator) {
    printf("\n=== Testing Flatset SIMD Operations ===\n");
    
    typedef struct test_value {
        size_t hash;
        int data;
        float weight;
        uint32_t flags;
    } test_value;
    
    // Test 1: Setup flatset with multiple buckets for SIMD testing
    printf("\n--- Test 1: Setup flatset with test data ---\n");
    cecs_flatset set = cecs_flatset_create_with_capacity(allocator, 4, sizeof(test_value)); // 4 buckets * 8 = 32 capacity
    printf("Initial capacity: %zu, count: %zu, buckets: %zu\n", 
           cecs_flatset_capacity(&set), cecs_flatset_count(&set), cecs_flatset_bucket_count(&set));
    
    // Insert test data across multiple buckets
    for (size_t i = 1; i <= 20; ++i) {
        test_value *inserted = cecs_flatset_insert_expect(&set, allocator, i, sizeof(test_value), offsetof(test_value, hash));
        inserted->hash = i;
        inserted->data = (int)(i * 10);
        inserted->weight = (float)(i * 0.5f);
        inserted->flags = (uint32_t)(i % 8); // 0-7 pattern
        
        if (i % 5 == 0) {
            printf("Inserted hash=%zu, data=%d, weight=%.1f, flags=%u\n", 
                   inserted->hash, inserted->data, inserted->weight, inserted->flags);
        }
    }
    printf("Setup complete: count=%zu, capacity=%zu, buckets=%zu\n",
           cecs_flatset_count(&set), cecs_flatset_capacity(&set), cecs_flatset_bucket_count(&set));
    
    // Test 2: Full SIMD traversal using unchecked access - process all 8 elements per bucket
    printf("\n--- Test 2: Full SIMD traversal using unchecked access ---\n");
    int total_data_sum_simd = 0;
    float total_weight_sum_simd = 0.0f;
    uint32_t total_flags_or_simd = 0;
    
    for (size_t bucket_idx = 0; bucket_idx < cecs_flatset_bucket_count(&set); ++bucket_idx) {
        const cecs_flatbucket *bucket = cecs_flatset_get_bucket(&set, bucket_idx, sizeof(test_value));
        
        // SIMD-style processing: always process all 8 elements - memory is always valid and initialized
        for (uint_fast8_t i = 0; i < CECS_FLATBUCKET8_MAX_COUNT; ++i) {
            const test_value *value = (const test_value *)cecs_flatset_bucket_get_value_unchecked(bucket, i, sizeof(test_value));
            // Process all 8 elements - memory is always valid and initialized
            total_data_sum_simd += value->data;
            total_weight_sum_simd += value->weight;
            total_flags_or_simd |= value->flags;
        }
        
        printf("Bucket %zu SIMD (unchecked): data_sum=%d, weight_sum=%.1f, flags_or=%u (processed all 8 elements)\n",
               bucket_idx, total_data_sum_simd, total_weight_sum_simd, total_flags_or_simd);
    }
    
    printf("Total SIMD results: data_sum=%d, weight_sum=%.1f, flags_or=%u\n",
           total_data_sum_simd, total_weight_sum_simd, total_flags_or_simd);
    
    // Test 3: Selective traversal using checked access - only process actual contained elements
    printf("\n--- Test 3: Selective traversal using checked access ---\n");
    int total_data_sum_selective = 0;
    float total_weight_sum_selective = 0.0f;
    uint32_t total_flags_or_selective = 0;
    
    for (size_t bucket_idx = 0; bucket_idx < cecs_flatset_bucket_count(&set); ++bucket_idx) {
        const cecs_flatbucket *bucket = cecs_flatset_get_bucket(&set, bucket_idx, sizeof(test_value));
        uint_fast8_t bucket_count = cecs_flatbucket_get_count(*bucket);
        
        // Only process contained elements using checked access
        for (uint_fast8_t i = 0; i < bucket_count; ++i) {
            const test_value *value = (const test_value *)cecs_flatset_bucket_get_value(bucket, i, sizeof(test_value));
            total_data_sum_selective += value->data;
            total_weight_sum_selective += value->weight;
            total_flags_or_selective |= value->flags;
        }
        
        printf("Bucket %zu selective (checked): data_sum=%d, weight_sum=%.1f, flags_or=%u (processed %u elements)\n",
               bucket_idx, total_data_sum_selective, total_weight_sum_selective, total_flags_or_selective, bucket_count);
    }
    
    printf("Total selective results: data_sum=%d, weight_sum=%.1f, flags_or=%u\n",
           total_data_sum_selective, total_weight_sum_selective, total_flags_or_selective);
    
    // Test 4: SIMD intrinsics test for integer data
    printf("\n--- Test 4: SIMD intrinsics for integer operations ---\n");
    for (size_t bucket_idx = 0; bucket_idx < cecs_flatset_bucket_count(&set); ++bucket_idx) {
        const cecs_flatbucket *bucket = cecs_flatset_get_bucket(&set, bucket_idx, sizeof(test_value));
        
        // Load 8 integer values using unchecked access
        int data_array[8];
        for (uint_fast8_t i = 0; i < 8; ++i) {
            const test_value *value = (const test_value *)cecs_flatset_bucket_get_value_unchecked(bucket, i, sizeof(test_value));
            data_array[i] = value->data;
        }
        
        // Use AVX2 to process 8 integers at once
        __m256i data_vec = _mm256_loadu_si256((const __m256i*)data_array);
        
        // Horizontal sum using SIMD
        __m256i sum_low = _mm256_unpacklo_epi32(data_vec, _mm256_setzero_si256());
        __m256i sum_high = _mm256_unpackhi_epi32(data_vec, _mm256_setzero_si256());
        __m256i sum_64 = _mm256_add_epi64(sum_low, sum_high);
        
        // Extract and sum the 4 64-bit values
        int64_t sum_parts[4];
        _mm256_storeu_si256((__m256i*)sum_parts, sum_64);
        int simd_sum = (int)(sum_parts[0] + sum_parts[1] + sum_parts[2] + sum_parts[3]);
        
        // Compare with scalar sum
        int scalar_sum = 0;
        for (int i = 0; i < 8; ++i) {
            scalar_sum += data_array[i];
        }
        
        if (simd_sum != scalar_sum) {
            fprintf(stderr, "ERROR: SIMD sum (%d) != scalar sum (%d) for bucket %zu\n", 
                    simd_sum, scalar_sum, bucket_idx);
            assert(false && "SIMD sum mismatch");
            exit(EXIT_FAILURE);
        }
        
        printf("Bucket %zu SIMD intrinsics: sum=%d (verified)\n", bucket_idx, simd_sum);
    }
    
    // Test 5: SIMD intrinsics test for float data
    printf("\n--- Test 5: SIMD intrinsics for float operations ---\n");
    for (size_t bucket_idx = 0; bucket_idx < cecs_flatset_bucket_count(&set); ++bucket_idx) {
        const cecs_flatbucket *bucket = cecs_flatset_get_bucket(&set, bucket_idx, sizeof(test_value));
        
        // Use SIMD gather to load 8 float values directly with stride
        const test_value *base_value = (const test_value *)cecs_flatset_bucket_get_value_unchecked(bucket, 0, sizeof(test_value));
        const float *weight_base = &base_value->weight;
        
        // Create indices for strided access (0, 1, 2, 3, 4, 5, 6, 7) * stride_in_floats
        const size_t stride_in_floats = sizeof(test_value) / sizeof(float);
        __m256i indices = _mm256_setr_epi32(
            0 * (int)stride_in_floats, 1 * (int)stride_in_floats, 2 * (int)stride_in_floats, 3 * (int)stride_in_floats,
            4 * (int)stride_in_floats, 5 * (int)stride_in_floats, 6 * (int)stride_in_floats, 7 * (int)stride_in_floats
        );
        
        // Gather 8 float values at once using strided load
        __m256 weight_vec = _mm256_i32gather_ps(weight_base, indices, sizeof(float));
        
        // Horizontal sum using SIMD
        __m128 sum_low = _mm256_castps256_ps128(weight_vec);
        __m128 sum_high = _mm256_extractf128_ps(weight_vec, 1);
        __m128 sum = _mm_add_ps(sum_low, sum_high);
        
        // Further reduce to single value
        sum = _mm_hadd_ps(sum, sum);
        sum = _mm_hadd_ps(sum, sum);
        float simd_sum = _mm_cvtss_f32(sum);
        
        // Compare with scalar sum using unchecked access
        float scalar_sum = 0.0f;
        for (uint_fast8_t i = 0; i < 8; ++i) {
            const test_value *value = (const test_value *)cecs_flatset_bucket_get_value_unchecked(bucket, i, sizeof(test_value));
            scalar_sum += value->weight;
        }
        
        if (fabsf(simd_sum - scalar_sum) > 0.001f) {
            fprintf(stderr, "ERROR: SIMD gather float sum (%.3f) != scalar sum (%.3f) for bucket %zu\n", 
                    simd_sum, scalar_sum, bucket_idx);
            assert(false && "SIMD gather float sum mismatch");
            exit(EXIT_FAILURE);
        }
        
        printf("Bucket %zu SIMD gather float intrinsics: sum=%.3f (verified)\n", bucket_idx, simd_sum);
    }
    
    // Test 5b: SIMD gather for integer data as well
    printf("\n--- Test 5b: SIMD gather for integer operations ---\n");
    for (size_t bucket_idx = 0; bucket_idx < cecs_flatset_bucket_count(&set); ++bucket_idx) {
        const cecs_flatbucket *bucket = cecs_flatset_get_bucket(&set, bucket_idx, sizeof(test_value));
        
        // Use SIMD gather to load 8 integer values directly with stride
        const test_value *base_value = (const test_value *)cecs_flatset_bucket_get_value_unchecked(bucket, 0, sizeof(test_value));
        const int *data_base = &base_value->data;
        
        // Create indices for strided access
        const size_t stride_in_ints = sizeof(test_value) / sizeof(int);
        __m256i indices = _mm256_setr_epi32(
            0 * (int)stride_in_ints, 1 * (int)stride_in_ints, 2 * (int)stride_in_ints, 3 * (int)stride_in_ints,
            4 * (int)stride_in_ints, 5 * (int)stride_in_ints, 6 * (int)stride_in_ints, 7 * (int)stride_in_ints
        );
        
        // Gather 8 integer values at once using strided load
        __m256i data_vec = _mm256_i32gather_epi32(data_base, indices, sizeof(int));
        
        // Horizontal sum using SIMD
        __m256i sum_low = _mm256_unpacklo_epi32(data_vec, _mm256_setzero_si256());
        __m256i sum_high = _mm256_unpackhi_epi32(data_vec, _mm256_setzero_si256());
        __m256i sum_64 = _mm256_add_epi64(sum_low, sum_high);
        
        // Extract and sum the 4 64-bit values
        int64_t sum_parts[4];
        _mm256_storeu_si256((__m256i*)sum_parts, sum_64);
        int simd_sum = (int)(sum_parts[0] + sum_parts[1] + sum_parts[2] + sum_parts[3]);
        
        // Compare with scalar sum
        int scalar_sum = 0;
        for (uint_fast8_t i = 0; i < 8; ++i) {
            const test_value *value = (const test_value *)cecs_flatset_bucket_get_value_unchecked(bucket, i, sizeof(test_value));
            scalar_sum += value->data;
        }
        
        if (simd_sum != scalar_sum) {
            fprintf(stderr, "ERROR: SIMD gather int sum (%d) != scalar sum (%d) for bucket %zu\n", 
                    simd_sum, scalar_sum, bucket_idx);
            assert(false && "SIMD gather int sum mismatch");
            exit(EXIT_FAILURE);
        }
        
        printf("Bucket %zu SIMD gather integer intrinsics: sum=%d (verified)\n", bucket_idx, simd_sum);
    }

    // Test 7: Vectorized search using SIMD intrinsics with gather
    printf("\n--- Test 7: Vectorized search using SIMD gather intrinsics ---\n");
    const int search_threshold = 100;
    size_t total_matches_simd = 0;
    
    for (size_t bucket_idx = 0; bucket_idx < cecs_flatset_bucket_count(&set); ++bucket_idx) {
        const cecs_flatbucket *bucket = cecs_flatset_get_bucket(&set, bucket_idx, sizeof(test_value));
        uint_fast8_t bucket_count = cecs_flatbucket_get_count(*bucket);
        
        // Use SIMD gather to load 8 data values directly
        const test_value *base_value = (const test_value *)cecs_flatset_bucket_get_value_unchecked(bucket, 0, sizeof(test_value));
        const int *data_base = &base_value->data;
        
        const size_t stride_in_ints = sizeof(test_value) / sizeof(int);
        __m256i indices = _mm256_setr_epi32(
            0 * (int)stride_in_ints, 1 * (int)stride_in_ints, 2 * (int)stride_in_ints, 3 * (int)stride_in_ints,
            4 * (int)stride_in_ints, 5 * (int)stride_in_ints, 6 * (int)stride_in_ints, 7 * (int)stride_in_ints
        );
        
        // Gather and compare all 8 values at once
        __m256i data_vec = _mm256_i32gather_epi32(data_base, indices, sizeof(int));
        __m256i threshold_vec = _mm256_set1_epi32(search_threshold);
        __m256i cmp_result = _mm256_cmpgt_epi32(data_vec, threshold_vec);
        
        // Extract comparison mask
        int mask = _mm256_movemask_ps(_mm256_castsi256_ps(cmp_result));
        
        // Apply validity mask for only contained elements
        uint8_t valid_mask = (1 << bucket_count) - 1;
        int valid_matches = mask & valid_mask;
        
        // Count matches
        size_t bucket_matches = 0;
        for (uint_fast8_t i = 0; i < 8; ++i) {
            if (valid_matches & (1 << i)) {
                bucket_matches++;
            }
        }
        
        total_matches_simd += bucket_matches;
        
        if (bucket_matches > 0) {
            printf("Bucket %zu SIMD gather search: %zu matches > %d (mask=0x%02X, valid=0x%02X)\n",
                   bucket_idx, bucket_matches, search_threshold, mask, valid_mask);
        }
    }
    
    // Verify against scalar search
    size_t total_matches_scalar = 0;
    for (size_t i = 1; i <= 20; ++i) {
        const void *found_value;
        if (cecs_flatset_find(&set, i, sizeof(test_value), offsetof(test_value, hash), &found_value)) {
            const test_value *value = (const test_value *)found_value;
            if (value->data > search_threshold) {
                total_matches_scalar++;
            }
        }
    }
    
    if (total_matches_simd != total_matches_scalar) {
        fprintf(stderr, "ERROR: SIMD search found %zu matches, scalar found %zu\n", 
                total_matches_simd, total_matches_scalar);
        assert(false && "SIMD search result mismatch");
        exit(EXIT_FAILURE);
    }
    
    printf("✓ SIMD search verification passed: %zu matches found\n", total_matches_simd);
    
    // Test 8: Bucket-wise SIMD aggregations with gather intrinsics
    printf("\n--- Test 8: Bucket-wise SIMD aggregations with gather intrinsics ---\n");
    for (size_t bucket_idx = 0; bucket_idx < cecs_flatset_bucket_count(&set); ++bucket_idx) {
        const cecs_flatbucket *bucket = cecs_flatset_get_bucket(&set, bucket_idx, sizeof(test_value));
        uint_fast8_t bucket_count = cecs_flatbucket_get_count(*bucket);
        
        if (bucket_count == 0) continue;
        
        // Use SIMD gather to load all 8 values directly
        const test_value *base_value = (const test_value *)cecs_flatset_bucket_get_value_unchecked(bucket, 0, sizeof(test_value));
        const int *data_base = &base_value->data;
        
        const size_t stride_in_ints = sizeof(test_value) / sizeof(int);
        __m256i indices = _mm256_setr_epi32(
            0 * (int)stride_in_ints, 1 * (int)stride_in_ints, 2 * (int)stride_in_ints, 3 * (int)stride_in_ints,
            4 * (int)stride_in_ints, 5 * (int)stride_in_ints, 6 * (int)stride_in_ints, 7 * (int)stride_in_ints
        );
        
        // Gather data and perform min/max operations
        __m256i data_vec = _mm256_i32gather_epi32(data_base, indices, sizeof(int));
        
        // Find min and max using SIMD (AVX2)
        __m256i perm_mask = _mm256_set_epi32(3, 2, 1, 0, 7, 6, 5, 4);
        __m256i data_swapped = _mm256_permutevar8x32_epi32(data_vec, perm_mask);
        
        __m256i min_vec = _mm256_min_epi32(data_vec, data_swapped);
        __m256i max_vec = _mm256_max_epi32(data_vec, data_swapped);
        
        // Continue reduction
        __m128i min_low = _mm256_castsi256_si128(min_vec);
        __m128i min_high = _mm256_extracti128_si256(min_vec, 1);
        __m128i max_low = _mm256_castsi256_si128(max_vec);
        __m128i max_high = _mm256_extracti128_si256(max_vec, 1);
        
        min_low = _mm_min_epi32(min_low, min_high);
        max_low = _mm_max_epi32(max_low, max_high);
        
        // Final reduction
        min_low = _mm_min_epi32(min_low, _mm_srli_si128(min_low, 8));
        max_low = _mm_max_epi32(max_low, _mm_srli_si128(max_low, 8));
        min_low = _mm_min_epi32(min_low, _mm_srli_si128(min_low, 4));
        max_low = _mm_max_epi32(max_low, _mm_srli_si128(max_low, 4));
        
        int simd_min = _mm_extract_epi32(min_low, 0);
        int simd_max = _mm_extract_epi32(max_low, 0);
        
        // Apply validity mask and recalculate for valid elements only using checked access
        int valid_min = INT_MAX, valid_max = INT_MIN;
        for (uint_fast8_t i = 0; i < bucket_count; ++i) {
            const test_value *value = (const test_value *)cecs_flatset_bucket_get_value(bucket, i, sizeof(test_value));
            if (value->data < valid_min) valid_min = value->data;
            if (value->data > valid_max) valid_max = value->data;
        }
        
        printf("Bucket %zu SIMD gather aggregation:\n", bucket_idx);
        printf("  All 8 elements: min=%d, max=%d\n", simd_min, simd_max);
        printf("  Valid %u elements: min=%d, max=%d\n", bucket_count, valid_min, valid_max);
    }
    
    // Clean up
    cecs_flatset_destroy(&set, allocator, sizeof(test_value));
    printf("=== Flatset SIMD tests completed ===\n");
}

void test_flatmap(cecs_allocator *allocator) {
    printf("\n=== Testing Flatmap ===\n");
    
    typedef struct test_data {
        int value;
        char name[16];
    } test_data;
    
    // Test 1: Creation and initial insertions
    printf("\n--- Test 1: Creation and initial insertions ---\n");
    cecs_flatmap map = cecs_flatmap_create_with_capacity(allocator, 1, sizeof(test_data)); // 1 bucket * 8 = 8 capacity
    if (cecs_flatmap_capacity(&map) != 8 || cecs_flatmap_count(&map) != 0 || cecs_flatmap_bucket_count(&map) != 1) {
        fprintf(stderr, "ERROR: Initial flatmap state incorrect: capacity=%zu, count=%zu, buckets=%zu\n",
                cecs_flatmap_capacity(&map), cecs_flatmap_count(&map), cecs_flatmap_bucket_count(&map));
        assert(false && "Initial flatmap state incorrect");
        exit(EXIT_FAILURE);
    }
    printf("Initial capacity: %zu, count: %zu, buckets: %zu\n", 
           cecs_flatmap_capacity(&map), cecs_flatmap_count(&map), cecs_flatmap_bucket_count(&map));
    
    // Insert values to test basic functionality (6 elements = 75% load factor, approaching 80% threshold)
    printf("Inserting elements approaching 80%% load factor threshold...\n");
    for (size_t i = 1; i <= 6; ++i) {
        test_data *inserted = cecs_flatmap_insert_expect(&map, allocator, i, sizeof(test_data));
        inserted->value = (int)i * 100;
        snprintf(inserted->name, sizeof(inserted->name), "item_%zu", i);
        
        // Verify the key is stored correctly
        const cecs_flatbucket *bucket = cecs_flatmap_get_bucket(&map, 0, sizeof(test_data));
        const cecs_flatmap_hash *key = cecs_flatmap_bucket_get_key(bucket, (uint_fast8_t)(i - 1));
        if (*key != i) {
            fprintf(stderr, "ERROR: Key mismatch after insert %zu: expected %zu, got %zu\n", i, i, *key);
            assert(false && "Key mismatch after insert");
            exit(EXIT_FAILURE);
        }
        
        if (cecs_flatbucket_get_count(*bucket) != i) {
            fprintf(stderr, "ERROR: Bucket count mismatch after insert %zu: expected %zu, got %hu\n", 
                    i, i, cecs_flatbucket_get_count(*cecs_flatmap_get_bucket(&map, 0, sizeof(test_data))));
            assert(false && "Bucket count mismatch");
            exit(EXIT_FAILURE);
        }
        if (cecs_flatmap_count(&map) != i) {
            fprintf(stderr, "ERROR: Count mismatch after insert %zu: expected %zu, got %zu\n", i, i, cecs_flatmap_count(&map));
            assert(false && "Count mismatch after insert");
            exit(EXIT_FAILURE);
        }
        if (i % 2 == 0) { // Print every 2nd element to reduce output
            printf("Inserted key=%zu, value=%d, name=%s (count: %zu, capacity: %zu, load: %.1f%%)\n", 
                   i, inserted->value, inserted->name, cecs_flatmap_count(&map), cecs_flatmap_capacity(&map),
                   (double)cecs_flatmap_count(&map) / cecs_flatmap_capacity(&map) * 100.0);
        }
    }
    printf("Reached 75%% load factor: count=%zu, capacity=%zu\n", 
           cecs_flatmap_count(&map), cecs_flatmap_capacity(&map));
    
    // Test 2: Trigger upward reallocation (insert one more to exceed 80%)
    printf("\n--- Test 2: Trigger upward reallocation ---\n");
    printf("Current state before triggering reallocation: count=%zu, capacity=%zu, buckets=%zu\n",
           cecs_flatmap_count(&map), cecs_flatmap_capacity(&map), cecs_flatmap_bucket_count(&map));
    
    test_data *trigger_resize = cecs_flatmap_insert_expect(&map, allocator, 7, sizeof(test_data));
    trigger_resize->value = 700;
    snprintf(trigger_resize->name, sizeof(trigger_resize->name), "item_7");
    
    if (cecs_flatmap_bucket_count(&map) != 2 || cecs_flatmap_capacity(&map) != 16) {
        fprintf(stderr, "ERROR: Expected reallocation to double buckets to 2 (capacity 16), got buckets=%zu, capacity=%zu\n",
                cecs_flatmap_bucket_count(&map), cecs_flatmap_capacity(&map));
        assert(false && "Expected reallocation to double buckets");
        exit(EXIT_FAILURE);
    }
    printf("After inserting key 7: count=%zu, capacity=%zu, buckets=%zu (should have doubled)\n",
           cecs_flatmap_count(&map), cecs_flatmap_capacity(&map), cecs_flatmap_bucket_count(&map));

    // Test 3: Insert more elements to utilize some of the new space
    printf("\n--- Test 3: Insert additional elements ---\n");
    for (size_t i = 8; i <= 12; ++i) {
        test_data *inserted = cecs_flatmap_insert_expect(&map, allocator, i, sizeof(test_data));
        inserted->value = (int)i * 100;
        snprintf(inserted->name, sizeof(inserted->name), "item_%zu", i);
    }
    if (cecs_flatmap_count(&map) != 12) {
        fprintf(stderr, "ERROR: Expected count 12 after additional insertions, got %zu\n", cecs_flatmap_count(&map));
        assert(false && "Expected count 12 after additional insertions");
        exit(EXIT_FAILURE);
    }
    printf("After inserting 5 more elements: count=%zu, capacity=%zu, load=%.1f%%\n",
           cecs_flatmap_count(&map), cecs_flatmap_capacity(&map),
           (double)cecs_flatmap_count(&map) / cecs_flatmap_capacity(&map) * 100.0);

    // Test 4: Find operations
    printf("\n--- Test 4: Find operations ---\n");
    for (size_t i = 1; i <= 5; ++i) {
        const void *found_value;
        if (!cecs_flatmap_find(&map, i, sizeof(test_data), &found_value)) {
            fprintf(stderr, "ERROR: Expected to find key %zu\n", i);
            assert(false && "Expected to find key");
            exit(EXIT_FAILURE);
        }
        const test_data *found_data = (const test_data *)found_value;
        if (found_data->value != (int)i * 100) {
            fprintf(stderr, "ERROR: Found incorrect value for key %zu\n", i);
            assert(false && "Found incorrect value for key");
            exit(EXIT_FAILURE);
        }
        printf("Found key=%zu: value=%d, name=%s\n", i, found_data->value, found_data->name);
    }

    // Test unsuccessful finds
    size_t missing_keys[] = {99, 404, 777};
    for (size_t i = 0; i < 3; ++i) {
        const void *found_value;
        if (cecs_flatmap_find(&map, missing_keys[i], sizeof(test_data), &found_value)) {
            fprintf(stderr, "ERROR: Unexpectedly found key %zu\n", missing_keys[i]);
            assert(false && "Unexpectedly found key");
            exit(EXIT_FAILURE);
        }
        printf("Key=%zu not found (expected)\n", missing_keys[i]);
    }

    // Test 5: Find or insert operations
    printf("\n--- Test 5: Find or insert operations ---\n");
    // Test with existing values
    for (size_t i = 10; i <= 12; ++i) {
        test_data *found = cecs_flatmap_find_or_insert(&map, allocator, i, sizeof(test_data));
        if (found->value != (int)i * 100) {
            fprintf(stderr, "ERROR: Find or insert returned incorrect existing value for key %zu\n", i);
            assert(false && "Find or insert returned incorrect existing value");
            exit(EXIT_FAILURE);
        }
        printf("Find or insert key=%zu: found value=%d, name=%s\n", i, found->value, found->name);
    }
    
    // Test with new values
    struct { size_t key; int value; const char* name; } new_pairs[] = {{100, 10000, "special_100"}, {200, 20000, "special_200"}};
    size_t count_before_new = cecs_flatmap_count(&map);
    for (size_t i = 0; i < 2; ++i) {
        test_data *inserted = cecs_flatmap_find_or_insert(&map, allocator, new_pairs[i].key, sizeof(test_data));
        inserted->value = new_pairs[i].value;
        snprintf(inserted->name, sizeof(inserted->name), "%s", new_pairs[i].name);
        if (cecs_flatmap_count(&map) != count_before_new + i + 1) {
            fprintf(stderr, "ERROR: Count mismatch after find_or_insert new value %zu\n", i);
            assert(false && "Count mismatch after find_or_insert");
            exit(EXIT_FAILURE);
        }
        printf("Find or insert key=%zu: inserted value=%d, name=%s (count: %zu)\n", 
               new_pairs[i].key, new_pairs[i].value, new_pairs[i].name, cecs_flatmap_count(&map));
    }

    // Test 6: Key verification through bucket access
    printf("\n--- Test 6: Key verification through bucket access ---\n");
    for (size_t bucket_idx = 0; bucket_idx < cecs_flatmap_bucket_count(&map); ++bucket_idx) {
        const cecs_flatbucket *bucket = cecs_flatmap_get_bucket(&map, bucket_idx, sizeof(test_data));
        uint_fast8_t bucket_count = cecs_flatbucket_get_count(*bucket);
        
        printf("Bucket %zu contents (%u elements):\n", bucket_idx, bucket_count);
        for (uint_fast8_t i = 0; i < bucket_count; ++i) {
            const cecs_flatmap_hash *key = cecs_flatmap_bucket_get_key(bucket, i);
            const test_data *value = cecs_flatmap_bucket_get_value(bucket, i, sizeof(test_data));
            printf("  [%u] key=%zu, value=%d, name=%s\n", i, *key, value->value, value->name);
        }
    }

    // Test 7: Remove elements to trigger downward reallocation
    printf("\n--- Test 7: Remove elements to trigger downward reallocation ---\n");
    printf("Current state before mass removal: count=%zu, capacity=%zu, buckets=%zu\n",
           cecs_flatmap_count(&map), cecs_flatmap_capacity(&map), cecs_flatmap_bucket_count(&map));
    
    // Calculate 20% threshold for current capacity (16 * 0.2 = 3.2, so 3)
    size_t capacity = cecs_flatmap_capacity(&map);
    size_t threshold_20_percent = capacity / 5; // 20% of capacity
    size_t current_count = cecs_flatmap_count(&map);
    size_t elements_to_remove = current_count - threshold_20_percent + 1; // Remove enough to go below 20%
    
    printf("Current count: %zu, capacity: %zu, 20%% threshold: %zu\n", 
           current_count, capacity, threshold_20_percent);
    printf("Need to remove %zu elements to go below 20%% threshold\n", elements_to_remove);
    
    // Remove elements systematically
    size_t removed_count = 0;
    for (size_t i = 1; i <= 12 && removed_count < elements_to_remove; ++i) {
        if (cecs_flatmap_find_remove(&map, allocator, i, sizeof(test_data))) {
            removed_count++;
            if (removed_count % 2 == 0) { // Print every 2nd removal to reduce output
                printf("Removed key=%zu (count: %zu, capacity: %zu, load: %.1f%%)\n", 
                       i, cecs_flatmap_count(&map), cecs_flatmap_capacity(&map),
                       (double)cecs_flatmap_count(&map) / cecs_flatmap_capacity(&map) * 100.0);
            }
        }
    }
    
    printf("After mass removal: count=%zu, capacity=%zu, buckets=%zu\n",
           cecs_flatmap_count(&map), cecs_flatmap_capacity(&map), cecs_flatmap_bucket_count(&map));

    // Test 8: Verify remaining elements
    printf("\n--- Test 8: Verify remaining elements ---\n");
    printf("Checking remaining new pairs...\n");
    
    // Check the new pairs we added (should still be there)
    for (size_t i = 0; i < 2; ++i) {
        const void *found_value;
        if (!cecs_flatmap_find(&map, new_pairs[i].key, sizeof(test_data), &found_value)) {
            fprintf(stderr, "ERROR: Expected to find remaining new pair key %zu\n", new_pairs[i].key);
            assert(false && "Expected to find remaining new pair key");
            exit(EXIT_FAILURE);
        }
        const test_data *found_data = (const test_data *)found_value;
        printf("Remaining new pair: key=%zu, value=%d, name=%s\n", new_pairs[i].key, found_data->value, found_data->name);
    }

    // Test 9: Test removal of non-existent elements
    printf("\n--- Test 9: Test removal of non-existent elements ---\n");
    for (size_t i = 0; i < 3; ++i) {
        if (cecs_flatmap_find_remove(&map, allocator, missing_keys[i], sizeof(test_data))) {
            fprintf(stderr, "ERROR: Unexpectedly removed non-existent key %zu\n", missing_keys[i]);
            assert(false && "Unexpectedly removed non-existent key");
            exit(EXIT_FAILURE);
        }
        printf("Failed to remove key=%zu (expected)\n", missing_keys[i]);
    }

    // Test 10: Insert more elements to test growing again
    printf("\n--- Test 10: Test growing again ---\n");
    printf("Adding more elements to test growth...\n");
    size_t count_before_growth = cecs_flatmap_count(&map);
    for (size_t i = 500; i <= 506; ++i) {
        test_data *inserted = cecs_flatmap_insert_expect(&map, allocator, i, sizeof(test_data));
        inserted->value = (int)i * 100;
        snprintf(inserted->name, sizeof(inserted->name), "growth_%zu", i);
    }
    if (cecs_flatmap_count(&map) != count_before_growth + 7) {
        fprintf(stderr, "ERROR: Count mismatch after growth test\n");
        assert(false && "Count mismatch after growth test");
        exit(EXIT_FAILURE);
    }
    printf("After adding 7 more elements: count=%zu, capacity=%zu, load=%.1f%%\n",
           cecs_flatmap_count(&map), cecs_flatmap_capacity(&map),
           (double)cecs_flatmap_count(&map) / cecs_flatmap_capacity(&map) * 100.0);

    // Test 11: Final state verification
    printf("\n--- Test 11: Final state verification ---\n");
    printf("Final flatmap state: count=%zu, capacity=%zu, buckets=%zu, load=%.1f%%\n", 
           cecs_flatmap_count(&map), cecs_flatmap_capacity(&map), cecs_flatmap_bucket_count(&map),
           (double)cecs_flatmap_count(&map) / cecs_flatmap_capacity(&map) * 100.0);

    // Clean up
    cecs_flatmap_destroy(&map, allocator, sizeof(test_data));
    printf("=== Flatmap tests completed ===\n");
}

void test_flatmap_simd(cecs_allocator *allocator) {
    printf("\n=== Testing Flatmap SIMD Operations ===\n");
    
    typedef struct test_value {
        int data;
        float weight;
        uint32_t flags;
        char tag[8];
    } test_value;
    
    // Test 1: Setup flatmap with multiple buckets for SIMD testing
    printf("\n--- Test 1: Setup flatmap with test data ---\n");
    cecs_flatmap map = cecs_flatmap_create_with_capacity(allocator, 4, sizeof(test_value)); // 4 buckets * 8 = 32 capacity
    printf("Initial capacity: %zu, count: %zu, buckets: %zu\n", 
           cecs_flatmap_capacity(&map), cecs_flatmap_count(&map), cecs_flatmap_bucket_count(&map));
    
    // Insert test data across multiple buckets
    for (size_t i = 1; i <= 20; ++i) {
        const cecs_flatmap_hash key = i + 1000; // Simple key
        test_value *inserted = cecs_flatmap_insert_expect(&map, allocator, key, sizeof(test_value)); // Use offset keys
        inserted->data = (int)(i * 10);
        inserted->weight = (float)(i * 0.5f);
        inserted->flags = (uint32_t)(i % 8); // 0-7 pattern
        snprintf(inserted->tag, sizeof(inserted->tag), "T%zu", i);
        
        if (i % 5 == 0) {
            printf("Inserted key=%zu, data=%d, weight=%.1f, flags=%u, tag=%s\n", 
                   key, inserted->data, inserted->weight, inserted->flags, inserted->tag);
        }
    }
    printf("Setup complete: count=%zu, capacity=%zu, buckets=%zu\n",
           cecs_flatmap_count(&map), cecs_flatmap_capacity(&map), cecs_flatmap_bucket_count(&map));
    
    // Test 2: Full SIMD traversal using unchecked access - process all 8 elements per bucket
    printf("\n--- Test 2: Full SIMD traversal using unchecked access ---\n");
    size_t total_key_sum_simd = 0;
    int total_data_sum_simd = 0;
    float total_weight_sum_simd = 0.0f;
    uint32_t total_flags_or_simd = 0;
    
    for (size_t bucket_idx = 0; bucket_idx < cecs_flatmap_bucket_count(&map); ++bucket_idx) {
        const cecs_flatbucket *bucket = cecs_flatmap_get_bucket(&map, bucket_idx, sizeof(test_value));
        
        // SIMD-style processing: always process all 8 elements using unchecked access
        size_t bucket_key_sum = 0;
        int bucket_data_sum = 0;
        float bucket_weight_sum = 0.0f;
        uint32_t bucket_flags_or = 0;
        
        for (uint_fast8_t i = 0; i < CECS_FLATBUCKET8_MAX_COUNT; ++i) {
            const cecs_flatmap_hash *key = cecs_flatmap_bucket_get_key_unchecked(bucket, i);
            const test_value *value = (const test_value *)cecs_flatmap_bucket_get_value_unchecked(bucket, i, sizeof(test_value));
            
            // Process all 8 elements - memory is always valid and initialized
            bucket_key_sum += *key;
            bucket_data_sum += value->data;
            bucket_weight_sum += value->weight;
            bucket_flags_or |= value->flags;
        }
        
        total_key_sum_simd += bucket_key_sum;
        total_data_sum_simd += bucket_data_sum;
        total_weight_sum_simd += bucket_weight_sum;
        total_flags_or_simd |= bucket_flags_or;
        
        printf("Bucket %zu SIMD (unchecked): key_sum=%zu, data_sum=%d, weight_sum=%.1f, flags_or=%u (processed all 8 elements)\n",
               bucket_idx, bucket_key_sum, bucket_data_sum, bucket_weight_sum, bucket_flags_or);
    }
    
    printf("Total SIMD results: key_sum=%zu, data_sum=%d, weight_sum=%.1f, flags_or=%u\n",
           total_key_sum_simd, total_data_sum_simd, total_weight_sum_simd, total_flags_or_simd);
    
    // Test 3: Selective traversal using checked access - only process actual contained elements
    printf("\n--- Test 3: Selective traversal using checked access ---\n");
    size_t total_key_sum_selective = 0;
    int total_data_sum_selective = 0;
    float total_weight_sum_selective = 0.0f;
    uint32_t total_flags_or_selective = 0;
    
    for (size_t bucket_idx = 0; bucket_idx < cecs_flatmap_bucket_count(&map); ++bucket_idx) {
        const cecs_flatbucket *bucket = cecs_flatmap_get_bucket(&map, bucket_idx, sizeof(test_value));
        uint_fast8_t bucket_count = cecs_flatbucket_get_count(*bucket);
        
        size_t bucket_key_sum = 0;
        int bucket_data_sum = 0;
        float bucket_weight_sum = 0.0f;
        uint32_t bucket_flags_or = 0;
        
        // Only process contained elements using checked access
        for (uint_fast8_t i = 0; i < bucket_count; ++i) {
            const cecs_flatmap_hash *key = cecs_flatmap_bucket_get_key(bucket, i);
            const test_value *value = (const test_value *)cecs_flatmap_bucket_get_value(bucket, i, sizeof(test_value));
            
            bucket_key_sum += *key;
            bucket_data_sum += value->data;
            bucket_weight_sum += value->weight;
            bucket_flags_or |= value->flags;
        }
        
        total_key_sum_selective += bucket_key_sum;
        total_data_sum_selective += bucket_data_sum;
        total_weight_sum_selective += bucket_weight_sum;
        total_flags_or_selective |= bucket_flags_or;
        
        printf("Bucket %zu selective (checked): key_sum=%zu, data_sum=%d, weight_sum=%.1f, flags_or=%u (processed %u elements)\n",
               bucket_idx, bucket_key_sum, bucket_data_sum, bucket_weight_sum, bucket_flags_or, bucket_count);
    }
    
    printf("Total selective results: key_sum=%zu, data_sum=%d, weight_sum=%.1f, flags_or=%u\n",
           total_key_sum_selective, total_data_sum_selective, total_weight_sum_selective, total_flags_or_selective);
    
    // Test 4: SIMD gather for keys and values
    printf("\n--- Test 4: SIMD gather for keys and values ---\n");
    for (size_t bucket_idx = 0; bucket_idx < cecs_flatmap_bucket_count(&map); ++bucket_idx) {
        const cecs_flatbucket *bucket = cecs_flatmap_get_bucket(&map, bucket_idx, sizeof(test_value));
        
        // Gather 8 keys using unchecked access
        size_t key_array[8];
        int data_array[8];
        for (uint_fast8_t i = 0; i < 8; ++i) {
            const cecs_flatmap_hash *key = cecs_flatmap_bucket_get_key_unchecked(bucket, i);
            const test_value *value = cecs_flatmap_bucket_get_value_unchecked(bucket, i, sizeof(test_value));
            key_array[i] = *key;
            data_array[i] = value->data;
        }
        
        // Use SIMD for data processing
        __m256i data_vec = _mm256_loadu_si256((const __m256i*)data_array);
        
        // Horizontal sum using SIMD
        __m256i sum_low = _mm256_unpacklo_epi32(data_vec, _mm256_setzero_si256());
        __m256i sum_high = _mm256_unpackhi_epi32(data_vec, _mm256_setzero_si256());
        __m256i sum_64 = _mm256_add_epi64(sum_low, sum_high);
        
        // Extract and sum the 4 64-bit values
        int64_t sum_parts[4];
        _mm256_storeu_si256((__m256i*)sum_parts, sum_64);
        int simd_data_sum = (int)(sum_parts[0] + sum_parts[1] + sum_parts[2] + sum_parts[3]);
        
        // Scalar sum for keys and verification
        size_t scalar_key_sum = 0;
        int scalar_data_sum = 0;
        for (int i = 0; i < 8; ++i) {
            scalar_key_sum += key_array[i];
            scalar_data_sum += data_array[i];
        }
        
        if (simd_data_sum != scalar_data_sum) {
            fprintf(stderr, "ERROR: SIMD data sum (%d) != scalar data sum (%d) for bucket %zu\n", 
                    simd_data_sum, scalar_data_sum, bucket_idx);
            assert(false && "SIMD data sum mismatch");
            exit(EXIT_FAILURE);
        }
        
        printf("Bucket %zu SIMD gather: key_sum=%zu, data_sum=%d (verified)\n", bucket_idx, scalar_key_sum, simd_data_sum);
    }
    
    // Test 5: SIMD gather for float weights using strided access
    printf("\n--- Test 5: SIMD gather for float weights ---\n");
    for (size_t bucket_idx = 0; bucket_idx < cecs_flatmap_bucket_count(&map); ++bucket_idx) {
        const cecs_flatbucket *bucket = cecs_flatmap_get_bucket(&map, bucket_idx, sizeof(test_value));
        
        // Use SIMD gather to load 8 float weights directly with stride
        const test_value *base_value = (const test_value *)cecs_flatmap_bucket_get_value_unchecked(bucket, 0, sizeof(test_value));
        const float *weight_base = &base_value->weight;
        
        // Create indices for strided access
        const size_t stride_in_floats = sizeof(test_value) / sizeof(float);
        __m256i indices = _mm256_setr_epi32(
            0 * (int)stride_in_floats, 1 * (int)stride_in_floats, 2 * (int)stride_in_floats, 3 * (int)stride_in_floats,
            4 * (int)stride_in_floats, 5 * (int)stride_in_floats, 6 * (int)stride_in_floats, 7 * (int)stride_in_floats
        );
        
        // Gather 8 float values at once
        __m256 weight_vec = _mm256_i32gather_ps(weight_base, indices, sizeof(float));
        
        // Horizontal sum using SIMD
        __m128 sum_low = _mm256_castps256_ps128(weight_vec);
        __m128 sum_high = _mm256_extractf128_ps(weight_vec, 1);
        __m128 sum = _mm_add_ps(sum_low, sum_high);
        
        sum = _mm_hadd_ps(sum, sum);
        sum = _mm_hadd_ps(sum, sum);
        float simd_weight_sum = _mm_cvtss_f32(sum);
        
        // Compare with scalar sum
        float scalar_weight_sum = 0.0f;
        for (uint_fast8_t i = 0; i < 8; ++i) {
            const test_value *value = cecs_flatmap_bucket_get_value_unchecked(bucket, i, sizeof(test_value));
            scalar_weight_sum += value->weight;
        }
        
        if (fabsf(simd_weight_sum - scalar_weight_sum) > 0.001f) {
            fprintf(stderr, "ERROR: SIMD weight sum (%.3f) != scalar weight sum (%.3f) for bucket %zu\n", 
                    simd_weight_sum, scalar_weight_sum, bucket_idx);
            assert(false && "SIMD weight sum mismatch");
            exit(EXIT_FAILURE);
        }
        
        printf("Bucket %zu SIMD gather weights: sum=%.3f (verified)\n", bucket_idx, simd_weight_sum);
    }
    
    // Test 6: Vectorized key search using SIMD
    printf("\n--- Test 6: Vectorized key search using SIMD ---\n");
    const size_t search_key = 1010; // Look for specific key
    size_t total_key_matches_simd = 0;
    
    for (size_t bucket_idx = 0; bucket_idx < cecs_flatmap_bucket_count(&map); ++bucket_idx) {
        const cecs_flatbucket *bucket = cecs_flatmap_get_bucket(&map, bucket_idx, sizeof(test_value));
        uint_fast8_t bucket_count = cecs_flatbucket_get_count(*bucket);
        
        // Load 8 keys using unchecked access (keys are size_t, need to handle properly)
        size_t key_array[8];
        for (uint_fast8_t i = 0; i < 8; ++i) {
            const cecs_flatmap_hash *key = cecs_flatmap_bucket_get_key_unchecked(bucket, i);
            key_array[i] = *key;
        }
        
        // For simplicity, use scalar comparison for size_t keys
        // In practice, you might convert to smaller integers for SIMD
        uint8_t valid_mask = (1 << bucket_count) - 1;
        size_t bucket_matches = 0;
        
        for (uint_fast8_t i = 0; i < 8; ++i) {
            if ((valid_mask & (1 << i)) && key_array[i] == search_key) {
                bucket_matches++;
            }
        }
        
        total_key_matches_simd += bucket_matches;
        
        if (bucket_matches > 0) {
            printf("Bucket %zu key search: %zu matches for key %zu (valid=0x%02X)\n",
                   bucket_idx, bucket_matches, search_key, valid_mask);
        }
    }
    
    // Verify against scalar search
    const void *found_value;
    bool found_scalar = cecs_flatmap_find(&map, search_key, sizeof(test_value), &found_value);
    size_t expected_matches = found_scalar ? 1 : 0;
    
    if (total_key_matches_simd != expected_matches) {
        fprintf(stderr, "ERROR: SIMD key search found %zu matches, expected %zu\n", 
                total_key_matches_simd, expected_matches);
        assert(false && "SIMD key search result mismatch");
        exit(EXIT_FAILURE);
    }
    
    printf("✓ SIMD key search verification passed: %zu matches found\n", total_key_matches_simd);
    
    // Test 7: Emulated SIMD operations with validity masks for key-value pairs
    printf("\n--- Test 7: Emulated SIMD operations with validity masks ---\n");
    for (size_t bucket_idx = 0; bucket_idx < cecs_flatmap_bucket_count(&map); ++bucket_idx) {
        const cecs_flatbucket *bucket = cecs_flatmap_get_bucket(&map, bucket_idx, sizeof(test_value));
        uint_fast8_t bucket_count = cecs_flatbucket_get_count(*bucket);
        
        // Create validity mask for contained elements
        uint8_t valid_mask = (1 << bucket_count) - 1;
        
        // Load all 8 key-value pairs using unchecked access
        size_t key_values[8];
        int data_values[8];
        uint32_t flag_values[8];
        
        for (uint_fast8_t i = 0; i < 8; ++i) {
            const cecs_flatmap_hash *key = cecs_flatmap_bucket_get_key_unchecked(bucket, i);
            const test_value *value = cecs_flatmap_bucket_get_value_unchecked(bucket, i, sizeof(test_value));
            key_values[i] = *key;
            data_values[i] = value->data;
            flag_values[i] = value->flags;
        }
        
        // Emulated SIMD: conditional operations based on mask
        size_t masked_key_sum = 0;
        int masked_data_sum = 0;
        uint32_t masked_flags_or = 0;
        size_t min_key = SIZE_MAX;
        size_t max_key = 0;
        
        for (uint_fast8_t i = 0; i < 8; ++i) {
            if (valid_mask & (1 << i)) {
                masked_key_sum += key_values[i];
                masked_data_sum += data_values[i];
                masked_flags_or |= flag_values[i];
                if (key_values[i] < min_key) min_key = key_values[i];
                if (key_values[i] > max_key) max_key = key_values[i];
            }
        }
        
        // Verify against checked iteration
        size_t checked_key_sum = 0;
        int checked_data_sum = 0;
        uint32_t checked_flags_or = 0;
        size_t checked_min_key = SIZE_MAX;
        size_t checked_max_key = 0;
        
        for (uint_fast8_t i = 0; i < bucket_count; ++i) {
            const cecs_flatmap_hash *key = cecs_flatmap_bucket_get_key(bucket, i);
            const test_value *value = cecs_flatmap_bucket_get_value(bucket, i, sizeof(test_value));
            checked_key_sum += *key;
            checked_data_sum += value->data;
            checked_flags_or |= value->flags;
            if (*key < checked_min_key) checked_min_key = *key;
            if (*key > checked_max_key) checked_max_key = *key;
        }
        
        if (masked_key_sum != checked_key_sum || masked_data_sum != checked_data_sum || 
            masked_flags_or != checked_flags_or || min_key != checked_min_key || max_key != checked_max_key) {
            fprintf(stderr, "ERROR: Emulated SIMD results don't match checked results for bucket %zu\n", bucket_idx);
            assert(false && "Emulated SIMD mismatch");
            exit(EXIT_FAILURE);
        }
        
        printf("Bucket %zu emulated SIMD: key_sum=%zu, data_sum=%d, flags_or=%u, key_range=[%zu,%zu] (mask=0x%02X, verified)\n",
               bucket_idx, masked_key_sum, masked_data_sum, masked_flags_or, min_key, max_key, valid_mask);
    }
    
    // Clean up
    cecs_flatmap_destroy(&map, allocator, sizeof(test_value));
    printf("=== Flatmap SIMD tests completed ===\n");
}

int main(void) {
    cecs_allocator allocator = cecs_allocator_create_bump_virtual(256);

    // test_msn(); // Test the msnh15_u4_dbg function
    test_dynarray(&allocator);
    test_sparse_set(&allocator);
    test_flatset(&allocator);
    test_flatset_simd(&allocator);
    test_flatmap(&allocator);
    test_flatmap_simd(&allocator);

    printf("\n=== All tests completed successfully ===\n");

    // cleanup
    cecs_allocator_destroy(&allocator);
    return 0;
}
