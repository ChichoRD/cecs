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
        if (i == 0) {
            WORLD_REMOVE_COMPONENT(velocity, w, e);
        }

        // printf("x: %d, y: %d\n", p->x, p->y);
        // printf("x: %d, y: %d\n", v->x, v->y);
        // printf("\n");

        position* stored_position = WORLD_GET_COMPONENT(position, w, e);
        if (i != 0) {
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
         component_iterator it =
             component_iterator_create(component_iterator_descriptor_create( &w.components, &iteration_arena, ((components_type_info){ .component_ids = (component_id[]){ ((component_id)position_component_type_id()) , ((component_id)velocity_component_type_id()) }, .component_count = (sizeof((component_id[]){ ((component_id)position_component_type_id()) , ((component_id)velocity_component_type_id()) }) / sizeof(component_id)) }) ));
         !component_iterator_done(&it);
         component_iterator_next(&it)
     ) {
         COMPONENT_ITERATION_HANDLE_STRUCT(position, velocity) handle;
         component_iterator_current(&it, &handle);
         printf("current entity: %d\n", handle.entity_id);
         printf("current position: %d, %d\n", handle.position_component->x, handle.position_component->y);
         printf("current velocity: %d, %d\n", handle.velocity_component->x, handle.velocity_component->y);
     }
     arena_free(&iteration_arena);

    world_free(&w);
    printf("freed world");
}