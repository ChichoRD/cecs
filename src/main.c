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

COMPONENT_DEFINE(entity_reference) {
    entity_id id;
} entity_reference;

RESOURCE_DEFINE(world_time) {
    double time;
} world_time;

RESOURCE_DEFINE(input_buffer) {
    char *buffer[16];
} input_buffer;

RESOURCE_DEFINE(emerald) {
    int count;
} emerald;

void main(void) {
    world w = world_create();
    {
        entity_id e = world_add_entity(&w);
        vec2int *c = WORLD_SET_OR_ADD_COMPONENT(vec2int, w, e, ((vec2int){1, 2}));
        tag *t = WORLD_SET_OR_ADD_COMPONENT(tag, w, e, ((tag){"Hello"}));
        entity_reference *er = WORLD_SET_OR_ADD_COMPONENT(entity_reference, w, e, ((entity_reference){e}));

        t->value = "Bye";
    }

    {
        world_time *wt = WORLD_ADD_RESOURCE(world_time, w, ((world_time){.time = 0.0}));
        input_buffer *ib = WORLD_ADD_RESOURCE(input_buffer, w, ((input_buffer){.buffer = {"Up", "Down"}}));
        emerald *e = WORLD_ADD_RESOURCE(emerald, w, ((emerald){.count = 10}));

        world_time new_wt = WORLD_REMOVE_RESOURCE(world_time, w);
        input_buffer new_ib = WORLD_REMOVE_RESOURCE(input_buffer, w);

        WORLD_ADD_RESOURCE(input_buffer, w, new_ib);
    }

    arena a = arena_create();
    query q = QUERY_CREATE(vec2int, tag, entity_reference);
    struct QUERY_RESULT(vec2int, tag, entity_reference) *result = QUERY_RUN(q, w, a, vec2int, tag, entity_reference);

    for (size_t i = 0; i < result->vec2int_view.count; i++) {
        vec2int *v = result->vec2int_view.elements[i];
        tag *t = result->tag_view.elements[i];
        entity_reference *er = result->entity_reference_view.elements[i];
        printf("vec2int: %d, %d, tag: %s, entity: %d\n", v->x, v->y, t->value, er->id);

        world_time *wt = WORLD_GET_RESOURCE(world_time, w);
        printf("Time: %f\n", wt->time);

        input_buffer *ib = WORLD_GET_RESOURCE(input_buffer, w);
        for (size_t j = 0; j < 16; j++) {
            printf("Input: %s\n", ib->buffer[j]);
        }
        emerald *e = WORLD_GET_RESOURCE(emerald, w);
        printf("Emeralds: %d\n", e->count);
    }
    arena_free(&a);
    world_free(&w);
    printf("Freed world\n");
}