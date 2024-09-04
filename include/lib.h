#ifndef LIB_H
#define LIB_H

#include <time.h>
#include <stdint.h>
#include "../include/containers/arena.h"
#include "../include/containers/list.h"
#include "../include/core/world.h"
#include "../include/core/component/entity/entity.h"
#include "../include/core/query.h"
#include "../include/core/resource.h"
#include "../include/core/query_context.h"

RESOURCE_DEFINE(struct, game_time) {
    struct timespec game_start;
    struct timespec frame_start;
    struct timespec frame_end;
    double delta_time_seconds;
    double averaged_delta_time_seconds;
    double time_since_start_seconds;
} game_time;

double game_time_update_delta_time(game_time *t) {
    t->delta_time_seconds =
        ((t->frame_end.tv_sec - t->frame_start.tv_sec) + (t->frame_end.tv_nsec - t->frame_start.tv_nsec) / 1000000000.0);
    t->averaged_delta_time_seconds =
        t->averaged_delta_time_seconds
            + (t->delta_time_seconds - t->averaged_delta_time_seconds) / (t->time_since_start_seconds / t->delta_time_seconds);
    return t->delta_time_seconds;
}

double game_time_update_time_since_start(game_time *t) {
    return (t->time_since_start_seconds = 
        ((t->frame_end.tv_sec - t->game_start.tv_sec) + (t->frame_end.tv_nsec - t->game_start.tv_nsec) / 1000000000.0));
}


// COMPONENT_DEFINE(struct, entity_reference) {
//     const entity_id id;
// } entity_reference;

#endif