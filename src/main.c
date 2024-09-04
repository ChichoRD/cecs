#include "../include/lib.h"

// typedef struct v2_i16 {
//     int16_t x;
//     int16_t y;
// } v2_i16;

// typedef v2_i16 position;
// COMPONENT_IMPLEMENT(position);

// typedef v2_i16 velocity;
// COMPONENT_IMPLEMENT(velocity);

#include "../include/containers/tagged_union.h"

typedef OPTION_STRUCT(size_t, size_t) optional_index;

void main(void) {
    // world w = world_create(512, 64);
    
    // entity_id e = world_add_entity(&w);
    // position *p = WORLD_SET_COMPONENT(
    //     position,
    //     &w,
    //     e,
    //     &((position) { .x = 0, .y = 0 })
    // );
    // velocity *v = WORLD_SET_COMPONENT(
    //     velocity,
    //     &w,
    //     e,
    //     &((velocity) { .x = 1, .y = 1 })
    // );

    // printf("x: %d, y: %d\n", p->x, p->y);
    // printf("x: %d, y: %d\n", v->x, v->y);
    // printf("\n");

    // position *stored_position = WORLD_GET_COMPONENT(position, &w, e);
    // velocity *stored_velocity = WORLD_GET_COMPONENT(velocity, &w, e);

    // printf("x: %d, y: %d\n", stored_position->x, stored_position->y);
    // printf("x: %d, y: %d\n", stored_velocity->x, stored_velocity->y);
    // world_free(&w);
    optional_index a = OPTION_CREATE_SOME(size_t, 5);
}