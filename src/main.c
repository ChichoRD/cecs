#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include "../include/arena.h"
#include "../include/list.h"
#include "../include/world.h"
#include "../include/entity.h"
#include "../include/query.h"
#include "../include/resource.h"

COMPONENT_DEFINE(vec2int) {
    int x;
    int y;
} vec2int;

COMPONENT_DEFINE(tag) {
    char *value;
} tag;

void main(void) {
    world za = world_create();
    world_free(&za);
    printf("Freed world\n");
}