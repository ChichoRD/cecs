#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include "../include/arena.h"
#include "../include/list.h"
#include "../include/world.h"
#include "../include/entity.h"

typedef struct vec2int {
    int x;
    int y;
} vec2int;

void main(void) {
    srand(time(NULL));
    world za = world_create();

    size_t entity_count = 10;
    for (size_t i = 0; i < entity_count; i++) {
        vec2int pos = {rand() % 100, rand() % 100};
        
        vec2int c = WORLD_ADD_COMPONENT(vec2int, &za, world_add_entity(&za, 0), 0, pos);
        printf("Entity %zu: %d, %d\n", i, c.x, c.y);
        printf("Entity count: %zu\n", LIST_COUNT(entity, za.entities.entities));
    }

    world_free(&za);
}