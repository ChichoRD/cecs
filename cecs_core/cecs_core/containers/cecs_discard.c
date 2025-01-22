#include <assert.h>
#include "cecs_discard.h"

cecs_discard cecs_discard_create_with_size(cecs_arena *a, size_t size) {
    return (cecs_discard){
        .handle = cecs_arena_alloc(a, size),
        .size = size
    };
}

size_t cecs_discard_ensure(cecs_discard *discard, cecs_arena *a, size_t size) {
    if (discard->size < size) {
        discard->handle = cecs_arena_realloc(a, discard->handle, discard->size, size);
        discard->size = size;
    }
    return size;
}

void *cecs_discard_use(cecs_discard *discard, cecs_arena *a, size_t size) {
    const size_t ensured_size = cecs_discard_ensure(discard, a, size);
    assert(ensured_size >= size && "fatal error: discard size could not be ensured");
    return discard->handle;
}
