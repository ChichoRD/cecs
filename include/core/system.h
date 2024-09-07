#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdbool.h>
#include <stdlib.h>
#include "component/component_iterator.h"

typedef struct world_system {
    components_type_info components_type_info;
    //list subsystems;
} world_system;

world_system world_system_create(components_type_info components_type_info) {
    return (world_system){ 
        .components_type_info = components_type_info
    };
}
#define WORLD_SYSTEM_CREATE(...) world_system_create(COMPONENTS_TYPE_INFO_CREATE(__VA_ARGS__))
#define WORLD_SYSTEM_CREATE_FROM_IDS(...) world_system_create(COMPONENTS_TYPE_INFO_CREATE_FROM_IDS(__VA_ARGS__))

typedef size_t entity_count;
typedef void system_predicate(raw_iteration_handle_reference handle);
entity_count world_system_iter(const world_system *s, world_components *wc, arena *iteration_arena, system_predicate *const predicate) {
    entity_count count = 0;
    component_iterator_descriptor descriptor = component_iterator_descriptor_create(wc, iteration_arena, s->components_type_info);
    raw_iteration_handle_reference handle = component_iterator_descriptor_allocate_handle(descriptor);
    for (
         component_iterator it = component_iterator_create(descriptor);
         !component_iterator_done(&it);
         component_iterator_next(&it)
     ) {
        ++count;
        component_iterator_current(&it, handle);
        predicate(handle);
    }
    free(handle);
    return count;
}

typedef struct system_predicates {
    system_predicate *const *const predicates;
    const size_t predicate_count;
} system_predicates;
system_predicates system_predicates_create(const system_predicate *const *predicates, size_t predicate_count) {
    return (system_predicates){
        .predicates = predicates,
        .predicate_count = predicate_count
    };
}
#define SYSTEM_PREDICATES_CREATE(...) \
    system_predicates_create( \
        (system_predicate *[]) { __VA_ARGS__ }, \
        sizeof((system_predicate *[]) { __VA_ARGS__ }) / sizeof(((system_predicate *[]) { __VA_ARGS__ })[0]) \
    )

typedef union {
    struct {
        const system_predicate *const *predicates;
        const size_t predicate_count;
    };
    system_predicates system_predicates;
} system_predicates_deconstruct;

entity_count world_system_iter_all(const world_system *s, world_components *wc, arena *iteration_arena, system_predicates predicates) {
    entity_count count = 0;
    component_iterator_descriptor descriptor = component_iterator_descriptor_create(wc, iteration_arena, s->components_type_info);
    raw_iteration_handle_reference handle = component_iterator_descriptor_allocate_handle(descriptor);
    for (
         component_iterator it = component_iterator_create(descriptor);
         !component_iterator_done(&it);
         component_iterator_next(&it)
     ) {
        ++count;
        component_iterator_current(&it, handle);
        for (size_t i = 0; i < predicates.predicate_count; i++) {
            predicates.predicates[i](handle);
        }
    }
    free(handle);
    return count;
}
#define WORLD_SYSTEM_ITER_ALL(world_system_ref, world_components_ref, iteration_arena_ref, ...) \
    world_system_iter_all(world_system_ref, world_components_ref, iteration_arena_ref, SYSTEM_PREDICATES_CREATE(__VA_ARGS__))

#endif