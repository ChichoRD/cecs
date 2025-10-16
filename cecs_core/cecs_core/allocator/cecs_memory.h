#ifndef CECS_MEMORY_H
#define CECS_MEMORY_H

#include <stdint.h>
#include <stdbool.h>
#include <cecs_math/arithmetic/cecs_integer_arithmetic.h>

typedef struct cecs_raw_memory_block {
    uint8_t *memory_start;
    size_t committed;
    size_t reserved;
} cecs_raw_memory_block;

// source: https://github.com/odin-lang/Odin/tree/master/core/mem/virtual/virtual.odin#L61
typedef struct cecs_memory_block {
    uint8_t *memory_start;
    uint8_t *memory_end;
    size_t reserved;
} cecs_memory_block;

inline size_t cecs_memory_block_committed_size(const cecs_memory_block block) {
    return (size_t)(block.memory_end - block.memory_start);
}
inline size_t cecs_memory_block_uncommited_size(const cecs_memory_block block) {
    return block.reserved - cecs_memory_block_committed_size(block);
}

size_t cecs_system_page_size(void);
inline size_t cecs_system_page_count_for(const size_t bytes) {
    return ((bytes - 1) / cecs_system_page_size()) + 1;
}
inline size_t cecs_system_pages_size_for(const size_t bytes) {
    const size_t page_size = cecs_system_page_size();
    return (((bytes - 1) / page_size) + 1) * page_size;
}


bool cecs_memory_block_is_valid(const cecs_raw_memory_block *block);
cecs_raw_memory_block cecs_memory_block_map(const size_t size);
cecs_memory_block cecs_memory_block_map_expect(const size_t size);

bool cecs_memory_block_commit(cecs_memory_block *const block, const size_t size, uint8_t *const commit_start);
void cecs_memory_block_commit_expect(cecs_memory_block *block, const size_t size, uint8_t *const commit_start);

bool cecs_memory_block_decommit(cecs_memory_block *block, const size_t size, uint8_t *const decommit_end);
void cecs_memory_block_decommit_expect(cecs_memory_block *block, const size_t size, uint8_t *const decommit_end);

bool cecs_memory_block_unmap_raw(cecs_raw_memory_block *block);
void cecs_memory_block_unmap_raw_expect(cecs_raw_memory_block *block);
bool cecs_memory_block_unmap(cecs_memory_block *block);
void cecs_memory_block_unmap_expect(cecs_memory_block *block);

size_t cecs_max_alignment_from_size(const size_t size);
const uint8_t *cecs_aligned_ptr(const uint8_t *const address, const size_t alignment);
uint8_t *cecs_aligned_ptr_mut(uint8_t *const address, const size_t alignment);

#endif
