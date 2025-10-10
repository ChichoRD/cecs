#include "cecs_memory.h"

#include <cstdlib>
#include <cassert>

#include "cecs_error.h"

#define CECS_MEMORY_OS_NONE 0
#define CECS_MEMORY_OS_WINDOWS 1
#define CECS_MEMORY_OS_UNIX 2


#ifdef _WIN32
#define CECS_MEMORY_OS CECS_MEMORY_OS_WINDOWS
    #include <memoryapi.h>
#elif defined(__unix__)
#define CECS_MEMORY_OS CECS_MEMORY_OS_UNIX
    #include <sys/mman.h>
    #include <unistd.h>
#else
#define CECS_MEMORY_OS CECS_MEMORY_OS_NONE

#endif

#if CECS_MEMORY_OS == CECS_MEMORY_OS_NONE
#error "unsupported platform"
#endif


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
    cecs_exit_assert_failure_if_not(
        cecs_memory_block_is_valid(&block),
        "error: cecs_memory_block_reserve_expect failed to reserve memory"
    );
    return (cecs_memory_block){
        .memory_start = block.memory_start,
        .memory_end = block.memory_start,
        .reserved = block.reserved
    };
}

bool cecs_memory_block_commit(cecs_memory_block *const block, const size_t size, const size_t offset) {
    cecs_exit_assert_failure_if(
        block->reserved < (offset + size),
        "error: cecs_memory_block_commit called with out of bounds offset and size"
    );
    uint8_t *const memory_commit_start = block->memory_start + offset;
#if CECS_MEMORY_OS == CECS_MEMORY_OS_WINDOWS
    bool success = VirtualAlloc(memory_commit_start, size, MEM_COMMIT, PAGE_READWRITE) != NULL;
#elif CECS_MEMORY_OS == CECS_MEMORY_OS_UNIX
    bool success = mprotect(memory_commit_start, size, PROT_READ | PROT_WRITE) == 0;
#endif
    if (cecs_expect(success)) {
        block->memory_end = memory_commit_start + size;
        return true;
    } else {
        return false;
    }
}
cecs_memory_block cecs_memory_block_commit_expect(cecs_memory_block block, const size_t size, const size_t offset) {
    cecs_exit_assert_failure_if_not(
        cecs_memory_block_commit(&block, size, offset),
        "error: cecs_memory_block_commit_expect failed to commit memory"
    );
    return block;
}
