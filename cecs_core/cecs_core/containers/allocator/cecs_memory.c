#include "cecs_memory.h"

#include <cstdlib>
#include <cassert>

#include <cecs_core/cecs_error.h>

#define CECS_MEMORY_OS_NONE 0
#define CECS_MEMORY_OS_WINDOWS 1
#define CECS_MEMORY_OS_UNIX 2


#ifdef _WIN32
#define CECS_MEMORY_OS CECS_MEMORY_OS_WINDOWS
#include <memoryapi.h>
#include <sysinfoapi.h>
const cecs_raw_memory_block cecs_raw_memory_block_invalid = {0};
const cecs_memory_block cecs_memory_block_invalid = {0};

#elif defined(__unix__)
#define CECS_MEMORY_OS CECS_MEMORY_OS_UNIX
#include <sys/mman.h>
#include <unistd.h>
const cecs_raw_memory_block cecs_raw_memory_block_invalid = {
    .memory_start = MAP_FAILED,
    .committed = 0,
    .reserved = 0
};
const cecs_memory_block cecs_memory_block_invalid = {
    .memory_start = MAP_FAILED,
    .memory_end = NULL,
    .reserved = 0
};

#else
#define CECS_MEMORY_OS CECS_MEMORY_OS_NONE

#endif

#if CECS_MEMORY_OS == CECS_MEMORY_OS_NONE
#error "unsupported platform"
#endif


extern inline size_t cecs_memory_block_committed_size(const cecs_memory_block block);
extern inline size_t cecs_memory_block_uncommited_size(const cecs_memory_block block);

size_t cecs_system_page_size(void) {
#if CECS_MEMORY_OS == CECS_MEMORY_OS_WINDOWS
    SYSTEM_INFO system_info;
    GetSystemInfo(&system_info);
    return (size_t)system_info.dwPageSize;
#elif CECS_MEMORY_OS == CECS_MEMORY_OS_UNIX
    return (size_t)getpagesize();
#endif
}
extern inline size_t cecs_system_page_count_for(const size_t bytes);
extern inline size_t cecs_system_pages_size_for(const size_t bytes);


bool cecs_memory_block_is_valid(const cecs_raw_memory_block *block) {
#if CECS_MEMORY_OS == CECS_MEMORY_OS_WINDOWS
    return block->memory_start != NULL;
#elif CECS_MEMORY_OS == CECS_MEMORY_OS_UNIX
    return block->memory_start != MAP_FAILED;
#endif
}

cecs_raw_memory_block cecs_memory_block_reserve(const size_t size) {
#if CECS_MEMORY_OS == CECS_MEMORY_OS_WINDOWS
    void *const memory = VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_NOACCESS);
#elif CECS_MEMORY_OS == CECS_MEMORY_OS_UNIX
    void *const memory = mmap(NULL, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0); 
#endif
    return (cecs_raw_memory_block){
        .memory_start = (uint8_t *)memory,
        .committed = 0,
        .reserved = size
    };
}
cecs_memory_block cecs_memory_block_reserve_expect(const size_t size) {
    cecs_raw_memory_block block = cecs_memory_block_reserve(size);
    cecs_assert_or_exit(
        cecs_memory_block_is_valid(&block),
        "error: cecs_memory_block_reserve_expect failed to reserve memory"
    );
    return (cecs_memory_block){
        .memory_start = block.memory_start,
        .memory_end = block.memory_start,
        .reserved = block.reserved
    };
}

bool cecs_memory_block_commit(cecs_memory_block *const block, const size_t size, uint8_t *const commit_start) {
    cecs_assert_or_exit(
        block->memory_start + block->reserved < commit_start + size,
        "error: cecs_memory_block_commit called with out of bounds commit_start and size"
    );
#if CECS_MEMORY_OS == CECS_MEMORY_OS_WINDOWS
    const bool success = VirtualAlloc(commit_start, size, MEM_COMMIT, PAGE_READWRITE) != NULL;
#elif CECS_MEMORY_OS == CECS_MEMORY_OS_UNIX
    const bool success = mprotect(memory_commit_start, size, PROT_READ | PROT_WRITE) == 0;
#endif
    if (cecs_expect(success)) {
        block->memory_end = commit_start + size;
        return true;
    } else {
        return false;
    }
}
void cecs_memory_block_commit_expect(cecs_memory_block *block, const size_t size, uint8_t *const commit_start) {
    cecs_assert_or_exit(
        cecs_memory_block_commit(block, size, commit_start),
        "error: cecs_memory_block_commit_expect failed to commit memory"
    );
}

bool cecs_memory_block_decommit(cecs_memory_block *block, const size_t size, uint8_t *const decommit_end) {
    cecs_assert_or_exit(
        decommit_end >= block->memory_end,
        "error: cecs_memory_block_uncommit called with memory range less than the block's committed memory"
    );
    cecs_assert_or_exit(
        decommit_end <= block->memory_start + block->reserved,
        "error: cecs_memory_block_uncommit called with out of bounds decommit_start and size"
    );
    uint8_t *const decommit_start = decommit_end - size;
#if CECS_MEMORY_OS == CECS_MEMORY_OS_WINDOWS
    const bool success = VirtualFree(decommit_start, size, MEM_DECOMMIT) != NULL;
#elif CECS_MEMORY_OS == CECS_MEMORY_OS_UNIX
    const bool success = mprotect(decommit_start, size, PROT_NONE) == 0;
#endif
    if (cecs_expect(success)) {
        block->memory_end = decommit_start;
        return true;
    } else {
        return false;
    }
}
void cecs_memory_block_decommit_expect(cecs_memory_block *block, const size_t size, uint8_t *const decommit_end) {
    cecs_assert_or_exit(
        cecs_memory_block_decommit(block, size, decommit_end),
        "error: cecs_memory_block_decommit_expect failed to decommit memory"
    );
}

bool cecs_memory_block_unmap_raw(cecs_raw_memory_block *block) {
#if CECS_MEMORY_OS == CECS_MEMORY_OS_WINDOWS
    const bool success = VirtualFree(block->memory_start, 0, MEM_RELEASE) != 0;
#elif CECS_MEMORY_OS == CECS_MEMORY_OS_UNIX
    const bool success = munmap(block->memory_start, block->reserved) == 0;
#endif
    if (cecs_expect(success)) {
        *block = cecs_raw_memory_block_invalid;
        return true;
    } else {
        return false;
    }
}
void cecs_memory_block_unmap_raw_expect(cecs_raw_memory_block *block) {
    cecs_assert_or_exit(
        cecs_memory_block_unmap_raw(block),
        "error: cecs_memory_block_unmap_raw_expect failed to unmap memory"
    );
}

bool cecs_memory_block_unmap(cecs_memory_block *block) {
#if CECS_MEMORY_OS == CECS_MEMORY_OS_WINDOWS
    const bool success = VirtualFree(block->memory_start, 0, MEM_RELEASE) != 0;
#elif CECS_MEMORY_OS == CECS_MEMORY_OS_UNIX
    const bool success = munmap(block->memory_start, block->reserved) == 0;
#endif
    if (cecs_expect(success)) {
        *block = cecs_memory_block_invalid;
        return true;
    } else {
        return false;
    }
}
void cecs_memory_block_unmap_expect(cecs_memory_block *block) {
    cecs_assert_or_exit(
        cecs_memory_block_unmap(block),
        "error: cecs_memory_block_unmap_expect failed to unmap memory"
    );
}

size_t cecs_max_alignment_from_size(const size_t size) {
    const size_t alignment = 1 << cecs_log2(size);
    return cecs_min(alignment, sizeof(uintmax_t));
}
extern inline const uint8_t *cecs_aligned_ptr(const uint8_t *const address, const size_t alignment);
extern inline uint8_t *cecs_aligned_ptr_mut(uint8_t *const address, const size_t alignment);
