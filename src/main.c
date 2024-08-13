#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <windows.h>
#include <time.h>
#include "../include/lib.h"

#define TARGET_FPS 8.0

#define BOARD_WIDTH 16
#define BOARD_HEIGHT 16

typedef enum cell_state {
    CELL_STATE_ALIVE,
    CELL_STATE_DEAD
} cell_state;

COMPONENT_DEFINE(cell) {
    const uint16_t x;
    const uint16_t y;
} cell;

RESOURCE_DEFINE(board) {
    cell_state cells[BOARD_WIDTH][BOARD_HEIGHT];
} board;

bool init(world *w) {
    game_time *t = WORLD_ADD_RESOURCE(
        game_time,
        w,
        &((game_time) {
            .game_start = {0},
            .frame_start = {0},
            .frame_end = {0},
            .delta_time_seconds = 0.0,
            .time_since_start_seconds = 0.0
        })
    );
    srand(timespec_get(&t->game_start, TIME_UTC));

    board b;
    for (uint16_t x = 0; x < BOARD_WIDTH; x++) {
        for (uint16_t y = 0; y < BOARD_HEIGHT; y++) {
            b.cells[x][y] = CELL_STATE_DEAD;
            //b.cells[x][y] = rand() % 2 == 0 ? CELL_STATE_ALIVE : CELL_STATE_DEAD;
            entity_id cell_entity = world_add_entity(w);
            WORLD_SET_OR_ADD_COMPONENT(cell, w, cell_entity, &((cell){ .x = x, .y = y }));
            WORLD_SET_OR_ADD_COMPONENT(entity_reference, w, cell_entity, &((entity_reference){ .id = cell_entity }));
        }
    }
    b.cells[1][0] = CELL_STATE_ALIVE;
    b.cells[2][1] = CELL_STATE_ALIVE;
    b.cells[0][2] = CELL_STATE_ALIVE;
    b.cells[1][2] = CELL_STATE_ALIVE;
    b.cells[2][2] = CELL_STATE_ALIVE;
    return WORLD_ADD_RESOURCE(board, w, &b) == NULL;
}

bool update(world *w, double delta_time_seconds) {
    arena query_arena = arena_create();
    query q = QUERY_CREATE(cell);
    struct QUERY_RESULT(cell) *result = QUERY_RUN(q, *w, query_arena, cell);

    board *b = WORLD_GET_RESOURCE(board, w);
    board new_b = *b;

    size_t alive_count = 0;
    for (size_t i = 0; i < result->cell_view.count; i++) {
        cell *c = result->cell_view.elements[i];
        int16_t alive_neighbours = 0;
        
        for (int16_t x = -1; x <= 1; x++) {
            for (int16_t y = -1; y <= 1; y++) {
                if (x == 0 && y == 0) {
                    continue;
                }

                int16_t nx = c->x + x;
                int16_t ny = c->y + y;

                if (nx < 0 || nx >= BOARD_WIDTH || ny < 0 || ny >= BOARD_HEIGHT) {
                    continue;
                }
                alive_neighbours += b->cells[nx][ny] == CELL_STATE_ALIVE;
            }
        }

        if (b->cells[c->x][c->y] == CELL_STATE_ALIVE) {
            ++alive_count;
            if (alive_neighbours < 2 || alive_neighbours > 3) {
                new_b.cells[c->x][c->y] = CELL_STATE_DEAD;
            }
        } else {
            if (alive_neighbours == 3) {
                new_b.cells[c->x][c->y] = CELL_STATE_ALIVE;
            }
        }
    }

    //printing board
    for (uint16_t y = 0; y < BOARD_HEIGHT; y++) {
        for (uint16_t x = 0; x < BOARD_WIDTH; x++) {
            printf("%s", new_b.cells[x][y] == CELL_STATE_ALIVE ? "X " : ". ");
        }
        printf("\n");
    }

    *WORLD_GET_RESOURCE(board, w) = new_b;

    printf("\n\n");
    //printf("fps: %f\n", 1.0 / delta_time_seconds);
    arena_free(&query_arena);
    return alive_count <= 4;
}

void main(void) {
    {
        world w = world_create();

        bool quitting = false;
        bool app_error = false;

        if (init(&w)) {
            app_error = true;
        }

        game_time* t = WORLD_GET_RESOURCE(game_time, &w);
        timespec_get(&t->frame_start, TIME_UTC);
        Sleep(1000.0 / TARGET_FPS);
        while (!quitting && !app_error)
        {
            timespec_get(&t->frame_end, TIME_UTC);

            if (update(&w, game_time_update_delta_time(t))) {
                app_error = true;
            }

            timespec_get(&t->frame_start, TIME_UTC);
            Sleep(1000.0 / TARGET_FPS);
        }

        world_free(&w);
        printf("Freed world\n");
    }
    fscanf(stdin, "%*c");
}