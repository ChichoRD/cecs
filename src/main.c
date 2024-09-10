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
    world w = world_create(512, 64);

    //sizeof(NULL);
    printf("created world\n");
    if (init(&w) == EXIT_FAILURE) {
        assert(false && "init failed");
        exit(EXIT_FAILURE);
    }
    arena iteration_arena = arena_create();
    world_system movables_system = WORLD_SYSTEM_CREATE(position, velocity);
    WORLD_SYSTEM_ITER_ALL(&movables_system, &w.components, &iteration_arena, move_movables, print_movables);
    
    for (size_t i = 0; i < paged_sparse_set_count_of_size(&w.components.component_storages, sizeof(component_storage)); i++) {
        printf("key %d: %x\n", i, paged_sparse_set_keys(&w.components.component_storages)[i]);
    }
    scanf("%d", NULL);
    arena_free(&iteration_arena);

    world_free(&w);
    printf("freed world");
}