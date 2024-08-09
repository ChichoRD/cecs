#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include "../include/arena.h"
#include "../include/list.h"
#include "../include/world.h"
#include "../include/entity.h"
#include "../include/query.h"

COMPONENT_DEFINE(vec2int) {
    int x;
    int y;
} vec2int;

COMPONENT_DEFINE(tag) {
    char *value;
} tag;

void main(void) {
    srand(time(NULL));
    world za = world_create();

    size_t entity_count = 10;
    for (size_t i = 0; i < entity_count; i++) {
        vec2int pos = {rand() % 100, rand() % 100};
        
        vec2int c = WORLD_SET_OR_ADD_COMPONENT(vec2int, &za, world_add_entity(&za, 0), pos);
    }

    world_remove_entity(&za, 2);

    tag t = {"Hello, World!"};
    tag c = WORLD_SET_OR_ADD_COMPONENT(tag, &za, world_add_entity(&za, 0), t);

    printf("\n");
    world_free(&za);
    printf("Freed world\n");
}