#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <windows.h>
#include <time.h>
#include "../include/arena.h"
#include "../include/list.h"
#include "../include/world.h"
#include "../include/entity.h"
#include "../include/query.h"
#include "../include/resource.h"

#define TARGET_FPS 1000.0

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
    //srand(time(NULL));

    board b;
    for (uint16_t x = 0; x < BOARD_WIDTH; x++) {
        for (uint16_t y = 0; y < BOARD_HEIGHT; y++) {
            b.cells[x][y] = CELL_STATE_DEAD;
            //b.cells[x][y] = rand() % 2 == 0 ? CELL_STATE_ALIVE : CELL_STATE_DEAD;
            WORLD_SET_OR_ADD_COMPONENT(cell, *w, world_add_entity(w), ((cell){ .x = x, .y = y }));
        }
    }
    b.cells[1][0] = CELL_STATE_ALIVE;
    b.cells[2][1] = CELL_STATE_ALIVE;
    b.cells[0][2] = CELL_STATE_ALIVE;
    b.cells[1][2] = CELL_STATE_ALIVE;
    b.cells[2][2] = CELL_STATE_ALIVE;
    return WORLD_ADD_RESOURCE(board, *w, b) == NULL;
}

bool update(world *w, double delta_time_seconds) {
    arena query_arena = arena_create();
    query q = QUERY_CREATE(cell);
    struct QUERY_RESULT(cell) *result = QUERY_RUN(q, *w, query_arena, cell);

    board *b = WORLD_GET_RESOURCE(board, *w);
    board new_b = *b;

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

    *WORLD_GET_RESOURCE(board, *w) = new_b;

    printf("\n\n");
    //printf("fps: %f\n", 1.0 / delta_time_seconds);
    arena_free(&query_arena);
    return false;
}

void main(void) {
    world w = world_create();

    bool quitting = false;
    bool app_error = false;

    if (init(&w)) {
        app_error = true;
    }

    struct timespec start;
    struct timespec end;
    timespec_get(&start, TIME_UTC);
    Sleep(1000.0 / TARGET_FPS);
    while (!quitting && !app_error)
    {
        timespec_get(&end, TIME_UTC);
        double delta_time_seconds =
            (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;

        if (update(&w, delta_time_seconds)) {
            app_error = true;
        }

        timespec_get(&start, TIME_UTC);
        Sleep(1000.0 / TARGET_FPS);
    }
    
    world_free(&w);
    printf("Freed world\n");
}