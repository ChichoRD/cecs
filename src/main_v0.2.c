#include "../include/lib.h"

typedef struct v2_i16 {
    int16_t x;
    int16_t y;
} v2_i16;

typedef v2_i16 position;
COMPONENT_IMPLEMENT(position);

typedef v2_i16 velocity;
COMPONENT_IMPLEMENT(velocity);

typedef void my_tag;
TAG_IMPLEMENT(my_tag);

bool init(world *w) {
    for (size_t i = 0; i < 15; i++) {
        entity_id e = world_add_entity(w);
        if (i & 1) {
            WORLD_SET_COMPONENT(
                position,
                w,
                e,
                &((position) {
                    i, i
                })
            );
            WORLD_SET_COMPONENT(
                velocity,
                w,
                e,
                &((velocity) {
                    i, i
                })
            );
        }
        if (i == 3) {
            WORLD_ADD_TAG(my_tag, w, e);
        }
    }
    return EXIT_SUCCESS;
}

void move_movables(COMPONENT_ITERATION_HANDLE_STRUCT(position, velocity) *handle) {
    position *p = handle->position_component;
    velocity *v = handle->velocity_component;
    p->x += v->x;
    p->y += v->y;
}

void print_movables(COMPONENT_ITERATION_HANDLE_STRUCT(position, velocity) *handle) {
    printf("entity: %d\n", handle->entity_id);
    printf("x: %d, y: %d\n", handle->position_component->x, handle->position_component->y);
    printf("x: %d, y: %d\n", handle->velocity_component->x, handle->velocity_component->y);
    printf("\n");
}

void main(void) {
    world w = world_create(512, 32, 4);

    printf("created world\n");
    if (init(&w) == EXIT_FAILURE) {
        assert(false && "init failed");
        exit(EXIT_FAILURE);
    }
    arena iteration_arena = arena_create();
    world_system movables_system = WORLD_SYSTEM_CREATE(position, velocity, my_tag);
    WORLD_SYSTEM_ITER_GENERIC_ALL(
        system_predicate,
        movables_system,
        &w,
        0.0, &iteration_arena, move_movables, print_movables
    );
    arena_free(&iteration_arena);

    world_free(&w);
    printf("freed world");
}