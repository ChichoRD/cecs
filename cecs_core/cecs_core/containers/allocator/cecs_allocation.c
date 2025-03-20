#include "cecs_allocation.h"

extern inline bool cecs_raw_alloction_check(const cecs_raw_alloction allocation);
extern inline void *cecs_raw_alloction_look(const cecs_raw_alloction allocation);
extern inline void *cecs_raw_alloction_expect(const cecs_raw_alloction allocation);


#ifndef CECS_ALLOC_FUNC
#define CECS_ALLOC_FUNC_DEFAULT malloc

#define CECS_ALLOC_FUNC CECS_ALLOC_FUNC_DEFAULT
#endif

#ifndef CECS_REALLOC_FUNC
#define CECS_REALLOC_FUNC_DEFAULT realloc

#define CECS_REALLOC_FUNC CECS_REALLOC_FUNC_DEFAULT
#endif

#ifndef CECS_FREE_FUNC
#define CECS_FREE_FUNC_DEFAULT free

#define CECS_FREE_FUNC CECS_FREE_FUNC_DEFAULT
#endif


cecs_raw_alloction cecs_alloc_raw(const size_t size) {
    return (cecs_raw_alloction){
        .block = CECS_ALLOC_FUNC(size)
    };
}
cecs_raw_alloction cecs_realloc_raw(const cecs_raw_alloction block, const size_t block_size, const size_t new_size) {
    (void)block_size;
    return (cecs_raw_alloction){
        .block = CECS_REALLOC_FUNC(block.block, new_size)
    };
}
void cecs_free_raw(const cecs_raw_alloction block, const size_t block_size) {
    (void)block_size;
    CECS_FREE_FUNC(block.block);
}


void *cecs_alloc_expect(const size_t size) {
    return cecs_raw_alloction_expect(cecs_alloc_raw(size));
}
void *cecs_realloc_expect(void *block, const size_t block_size, const size_t new_size) {
    return cecs_raw_alloction_expect(cecs_realloc_raw((cecs_raw_alloction){ .block = block }, block_size, new_size));
}
void cecs_free_expect(void *block, const size_t block_size) {
    cecs_free_raw((cecs_raw_alloction){ .block = block }, block_size);
}
