#ifndef CECS_MEMORY_H
#define CECS_MEMORY_H

#include <stdint.h>

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


bool cecs_memory_block_is_valid(const cecs_raw_memory_block *block);
cecs_raw_memory_block cecs_memory_block_reserve(const size_t size);
cecs_memory_block cecs_memory_block_reserve_expect(const size_t size);

bool cecs_memory_block_commit(cecs_memory_block *const block, const size_t size, const size_t offset);
cecs_memory_block cecs_memory_block_commit_expect(cecs_memory_block block, const size_t size, const size_t offset);

#endif