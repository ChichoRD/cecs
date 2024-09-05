#include "../include/lib.h"

typedef struct v2_i16 {
    int16_t x;
    int16_t y;
} v2_i16;

typedef v2_i16 position;
COMPONENT_IMPLEMENT(position);

typedef v2_i16 velocity;
COMPONENT_IMPLEMENT(velocity);

bool init(world *w) {
    for (size_t i = 0; i < 10; i++) {
        entity_id e = world_add_entity(w);
        // printf("e: %d\n", e);
        position* p = WORLD_SET_COMPONENT(
            position,
            w,
            e,
            &((position) { .x = 0, .y = 0 })
        );
        velocity* v = WORLD_SET_COMPONENT(
            velocity,
            w,
            e,
            &((velocity) { .x = 1, .y = 1 })
        );
        if (i == 8) {
            WORLD_REMOVE_COMPONENT(velocity, w, e);
        }

        // printf("x: %d, y: %d\n", p->x, p->y);
        // printf("x: %d, y: %d\n", v->x, v->y);
        // printf("\n");

        position* stored_position = WORLD_GET_COMPONENT(position, w, e);
        if (i != 8) {
            velocity *stored_velocity = WORLD_GET_COMPONENT(velocity, w, e);
        }

        // printf("x: %d, y: %d\n", stored_position->x, stored_position->y);
        // printf("x: %d, y: %d\n", stored_velocity->x, stored_velocity->y);
        // printf("\n");
    }

    return EXIT_SUCCESS;
}

void main(void) {
    world w = world_create(512, 64);
    
    printf("created world\n");
    if (init(&w) == EXIT_FAILURE) {
        assert(false && "init failed");
        exit(EXIT_FAILURE);
    }
    arena iteration_arena = arena_create();
    for (
        struct COMPONENT_ITERATOR(position, velocity) *it =
            COMPONENT_ITERATOR_CREATE(&w.components, &iteration_arena, position, velocity);
        !component_iterator_done(&it->iterator);
        component_iterator_next(&it->iterator)
    ) {
        it = component_iterator_current(&it->iterator);
        printf("current entity: %d\n", it->iterator.current_entity_id);
        printf("current position: %d, %d\n", it->position_handle.component->x, it->position_handle.component->y);
        printf("current velocity: %d, %d\n", it->velocity_handle.component->x, it->velocity_handle.component->y);
    }
    arena_free(&iteration_arena);

    world_free(&w);
    printf("freed world");
}