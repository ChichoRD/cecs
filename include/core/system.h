#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdbool.h>
#include <stdlib.h>
#include "containers/tagged_union.h"
#include "component/component_iterator.h"
#include "world.h"

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

// TODO: cows
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


typedef void world_system_predicate(raw_iteration_handle_reference handle, world *world);
typedef void dt_system_predicate(raw_iteration_handle_reference handle, double delta_time);
typedef void world_dt_system_predicate(raw_iteration_handle_reference handle, world *world, double delta_time);
typedef TAGGED_UNION_STRUCT(
    generic_system_predicate,
    system_predicate *const,
    system_predicate,
    world_system_predicate *const,
    world_system_predicate,
    dt_system_predicate *const,
    dt_system_predicate,
    world_dt_system_predicate *const,
    world_dt_system_predicate
) generic_system_predicate;

#define _WORLD_SYTEM_ITER_FOR(it_identifier, descriptor) \
    for ( \
        component_iterator it = component_iterator_create(descriptor); \
        !component_iterator_done(&it); \
        component_iterator_next(&it) \
    )

#define _WORLD_SYSTEM_ITER_FOR_BODY(count, it, handle) \
    do { \
        ++count; \
        component_iterator_current(&it, handle); \
    } while (0)

entity_count world_system_iter_generic(
    const world_system *s,
    world *world,
    double delta_time_seconds,
    arena *iteration_arena,
    generic_system_predicate predicate
) {
    entity_count count = 0;
    component_iterator_descriptor descriptor = component_iterator_descriptor_create(
        &world->components,
        iteration_arena,
        s->components_type_info
    );
    raw_iteration_handle_reference handle = component_iterator_descriptor_allocate_handle(descriptor);

    MATCH(predicate) {
        case TAGGED_UNION_VARIANT(system_predicate, generic_system_predicate): {
            _WORLD_SYTEM_ITER_FOR(it, descriptor) {
                _WORLD_SYSTEM_ITER_FOR_BODY(count, it, handle);
                TAGGED_UNION_GET_UNCHECKED(system_predicate, predicate)(handle);
            }
            break;
        }

        case TAGGED_UNION_VARIANT(world_system_predicate, generic_system_predicate): {
            _WORLD_SYTEM_ITER_FOR(it, descriptor) {
                _WORLD_SYSTEM_ITER_FOR_BODY(count, it, handle);
                TAGGED_UNION_GET_UNCHECKED(world_system_predicate, predicate)(handle, world);
            }
            break;
        }

        case TAGGED_UNION_VARIANT(dt_system_predicate, generic_system_predicate): {
            _WORLD_SYTEM_ITER_FOR(it, descriptor) {
                _WORLD_SYSTEM_ITER_FOR_BODY(count, it, handle);
                TAGGED_UNION_GET_UNCHECKED(dt_system_predicate, predicate)(handle, delta_time_seconds);
            }
            break;
        }

        case TAGGED_UNION_VARIANT(world_dt_system_predicate, generic_system_predicate): {
            _WORLD_SYTEM_ITER_FOR(it, descriptor) {
                _WORLD_SYSTEM_ITER_FOR_BODY(count, it, handle);
                TAGGED_UNION_GET_UNCHECKED(world_dt_system_predicate, predicate)(handle, world, delta_time_seconds);
            }
            break;
        }
    }

    free(handle);
    return count;
}

typedef void raw_system_predicate();
#define _GENERIC_PREDICATES(predicate) CAT2(predicate, s)
typedef struct generic_system_predicates {
    TAGGED_UNION_STRUCT(
        generic_system_predicates,
        raw_system_predicate *const *const,
        _GENERIC_PREDICATES(raw_system_predicate),
        system_predicate *const *const,
        _GENERIC_PREDICATES(system_predicate),
        world_system_predicate *const *const,
        _GENERIC_PREDICATES(world_system_predicate),
        dt_system_predicate *const *const,
        _GENERIC_PREDICATES(dt_system_predicate),
        world_dt_system_predicate *const *const,
        _GENERIC_PREDICATES(world_dt_system_predicate)
    ) predicates;
    const size_t predicate_count;
} generic_system_predicates;

generic_system_predicates generic_system_predicates_create(
    const raw_system_predicate *const *predicates,
    size_t predicate_count,
    int predicate_variant
) {
    generic_system_predicates g;
    return (generic_system_predicates) {
        .predicate_count = predicate_count,
        .predicates = TAGGED_UNION_CREATE_VARIANT(
            raw_system_predicates,
            predicate_variant,
            predicates
        )
    };
}
#define GENERIC_SYSTEM_PREDICATES_CREATE(predicate_type, ...) \
    generic_system_predicates_create( \
        (raw_system_predicate *[]) { __VA_ARGS__ }, \
        sizeof((raw_system_predicate *[]) { __VA_ARGS__ }) / sizeof(((raw_system_predicate *[]) { __VA_ARGS__ })[0]), \
        TAGGED_UNION_VARIANT(_GENERIC_PREDICATES(predicate_type), generic_system_predicates) \
    )

entity_count world_system_iter_generic_all(
    const world_system *s,
    world *world,
    double delta_time_seconds,
    arena *iteration_arena,
    generic_system_predicates predicates
) {
    entity_count count = 0;
    component_iterator_descriptor descriptor = component_iterator_descriptor_create(
        &world->components,
        iteration_arena,
        s->components_type_info
    );
    raw_iteration_handle_reference handle = component_iterator_descriptor_allocate_handle(descriptor);

    MATCH(predicates.predicates) {
        case TAGGED_UNION_VARIANT(system_predicate, generic_system_predicate): {
            _WORLD_SYTEM_ITER_FOR(it, descriptor) {
                _WORLD_SYSTEM_ITER_FOR_BODY(count, it, handle);
                for (size_t i = 0; i < predicates.predicate_count; i++) {
                    TAGGED_UNION_GET_UNCHECKED(_GENERIC_PREDICATES(system_predicate), predicates.predicates)[i](handle);
                }
            }
            break;
        }

        case TAGGED_UNION_VARIANT(world_system_predicate, generic_system_predicate): {
            _WORLD_SYTEM_ITER_FOR(it, descriptor) {
                _WORLD_SYSTEM_ITER_FOR_BODY(count, it, handle);
                for (size_t i = 0; i < predicates.predicate_count; i++) {
                    TAGGED_UNION_GET_UNCHECKED(_GENERIC_PREDICATES(world_system_predicate), predicates.predicates)[i](handle, world);
                }
            }
            break;
        }

        case TAGGED_UNION_VARIANT(dt_system_predicate, generic_system_predicate): {
            _WORLD_SYTEM_ITER_FOR(it, descriptor) {
                _WORLD_SYSTEM_ITER_FOR_BODY(count, it, handle);
                for (size_t i = 0; i < predicates.predicate_count; i++) {
                    TAGGED_UNION_GET_UNCHECKED(_GENERIC_PREDICATES(dt_system_predicate), predicates.predicates)[i](handle, delta_time_seconds);
                }
            }
            break;
        }

        case TAGGED_UNION_VARIANT(world_dt_system_predicate, generic_system_predicate): {
            _WORLD_SYTEM_ITER_FOR(it, descriptor) {
                _WORLD_SYSTEM_ITER_FOR_BODY(count, it, handle);
                for (size_t i = 0; i < predicates.predicate_count; i++) {
                    TAGGED_UNION_GET_UNCHECKED(_GENERIC_PREDICATES(world_dt_system_predicate), predicates.predicates)[i](handle, world, delta_time_seconds);
                }
            }
            break;
        }
    }

    free(handle);
    return count;
}
#define WORLD_SYSTEM_ITER_GENERIC_ALL(predicate_type, world_system_ref, world_ref, delta_time, iteration_arena_ref, ...) \
    world_system_iter_generic_all( \
        (world_system_ref), \
        (world_ref), \
        (delta_time), \
        (iteration_arena_ref), \
        GENERIC_SYSTEM_PREDICATES_CREATE(predicate_type, __VA_ARGS__) \
    )


#endif