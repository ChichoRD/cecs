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

typedef size_t entity_count;
typedef raw_iteration_handle_reference system_predicate(raw_iteration_handle_reference handle);
entity_count world_system_iter(const world_system *s, world_components *wc, arena *iteration_arena, system_predicate *predicate) {
    entity_count count = 0;
    raw_iteration_handle_reference handle = malloc(component_iteration_handle_size(s->components_type_info));
    for (
         component_iterator it =
            component_iterator_create(
                component_iterator_descriptor_create(
                    wc,
                    iteration_arena,
                    s->components_type_info
                )
            );
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

typedef void system_final_predicate(raw_iteration_handle_reference handle);
entity_count world_system_iter_final(const world_system *s, world_components *wc, arena *iteration_arena, system_final_predicate *predicate) {
    entity_count count = 0;
    raw_iteration_handle_reference handle = malloc(component_iteration_handle_size(s->components_type_info));
    for (
         component_iterator it =
            component_iterator_create(
                component_iterator_descriptor_create(
                    wc,
                    iteration_arena,
                    s->components_type_info
                )
            );
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

#endif