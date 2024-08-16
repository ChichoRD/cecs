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

COMPONENT_DEFINE(struct, cell) {
    const uint16_t x;
    const uint16_t y;
} cell;

RESOURCE_DEFINE(struct, board) {
    cell_state cells[BOARD_WIDTH][BOARD_HEIGHT];
} board;

RESOURCE_DEFINE(struct, console_buffer) {
    char *buffer[BOARD_WIDTH][BOARD_HEIGHT];
} console_buffer;

TAG_DEFINE(is_fire) is_fire;
TAG_DEFINE(is_ice) is_ice;
TAG_DEFINE(is_water) is_water;

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
    console_buffer cb;
    for (uint16_t x = 0; x < BOARD_WIDTH; x++) {
        for (uint16_t y = 0; y < BOARD_HEIGHT; y++) {
            b.cells[x][y] = CELL_STATE_DEAD;
            cb.buffer[x][y] = ". ";
            //b.cells[x][y] = rand() % 2 == 0 ? CELL_STATE_ALIVE : CELL_STATE_DEAD;
            entity_id cell_entity = world_add_entity(w);
            WORLD_SET_OR_ADD_COMPONENT(cell, w, cell_entity, &((cell){ .x = x, .y = y }));
            WORLD_SET_OR_ADD_COMPONENT(entity_reference, w, cell_entity, &((entity_reference){ .id = cell_entity }));
            
            switch (rand() % 6) {
                case 0:
                    WORLD_ADD_TAG(is_fire, w, cell_entity);
                    break;
                case 1:
                    WORLD_ADD_TAG(is_ice, w, cell_entity);
                    break;
                case 2:
                    WORLD_ADD_TAG(is_water, w, cell_entity);
                    break;
                default:
                    break;
            }
        }
    }
    b.cells[1][0] = CELL_STATE_ALIVE;
    b.cells[2][1] = CELL_STATE_ALIVE;
    b.cells[0][2] = CELL_STATE_ALIVE;
    b.cells[1][2] = CELL_STATE_ALIVE;
    b.cells[2][2] = CELL_STATE_ALIVE;

    WORLD_ADD_RESOURCE(board, w, &b);
    WORLD_ADD_RESOURCE(console_buffer, w, &cb);
    return EXIT_SUCCESS;
}

bool cell_update_system(world *w, query_context *qc) {
    query q = QUERY_CREATE(WITH_COMPONENTS(cell), WITHOUT_TAGS);
    struct QUERY_RESULT(cell) *result = QUERY_CONTEXT_RUN_QUERY(qc, q, w, cell);

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

    *b = new_b;
    return EXIT_SUCCESS;
}

bool cell_render_system(world *w, query_context *qc) {
    #define ANSI_COLOR_RED     "\x1b[31m"
    #define ANSI_COLOR_GREEN   "\x1b[32m"
    #define ANSI_COLOR_YELLOW  "\x1b[33m"
    #define ANSI_COLOR_BLUE    "\x1b[34m"
    #define ANSI_COLOR_MAGENTA "\x1b[35m"
    #define ANSI_COLOR_CYAN    "\x1b[36m"
    #define ANSI_COLOR_RESET   "\x1b[0m"

    console_buffer *cb = WORLD_GET_RESOURCE(console_buffer, w);
    console_buffer new_cb = *cb;
    board *b = WORLD_GET_RESOURCE(board, w);
    for (size_t x = 0; x < BOARD_WIDTH; x++) {
        for (size_t y = 0; y < BOARD_HEIGHT; y++) {
            new_cb.buffer[x][y] = b->cells[x][y] == CELL_STATE_ALIVE
                ? (ANSI_COLOR_GREEN "O " ANSI_COLOR_RESET)
                : ". ";
        }
    }

    query fq = QUERY_CREATE(WITH_COMPONENTS(cell), WITH_TAGS(is_fire));
    struct QUERY_RESULT(cell)* fire_cells = ((struct query_result_cell {
        struct entity_id_view {
            size_t count; entity_id* elements;
        } entity_id_view; struct cell_view {
            size_t count; cell** elements;
        } cell_view;
    }*)query_context_run_query(qc, fq, w));

    {
        for (size_t i = 0; i < fire_cells->cell_view.count; i++) {
            cell* c = fire_cells->cell_view.elements[i];
            if (b->cells[c->x][c->y] == CELL_STATE_ALIVE) {
				new_cb.buffer[c->x][c->y] = (ANSI_COLOR_RED "F " ANSI_COLOR_RESET);
			}
        }
    }

    query iq = QUERY_CREATE(WITH_COMPONENTS(cell), WITH_TAGS(is_ice));
    struct QUERY_RESULT(cell) *ice_cells = NULL;
    {
        ice_cells = QUERY_CONTEXT_RUN_QUERY(qc, iq, w, cell);

        for (size_t i = 0; i < ice_cells->cell_view.count; i++) {
            cell *c = ice_cells->cell_view.elements[i];
            if (b->cells[c->x][c->y] == CELL_STATE_ALIVE) {
				new_cb.buffer[c->x][c->y] = (ANSI_COLOR_BLUE "I " ANSI_COLOR_RESET);
			}
        }
    }
        
    query wq = QUERY_CREATE(WITH_COMPONENTS(cell), WITH_TAGS(is_water));
    struct QUERY_RESULT(cell) *water_cells = NULL;
    {
        water_cells = QUERY_CONTEXT_RUN_QUERY(qc, wq, w, cell);
        for (size_t i = 0; i < water_cells->cell_view.count; i++) {
           cell *c = water_cells->cell_view.elements[i];
            if (b->cells[c->x][c->y] == CELL_STATE_ALIVE) {
                new_cb.buffer[c->x][c->y] = (ANSI_COLOR_BLUE "W " ANSI_COLOR_RESET);
            }
        }
    }

    //for (size_t y = 0; y < BOARD_HEIGHT; y++) {
    //    for (size_t x = 0; x < BOARD_WIDTH; x++) {
    //        printf(b->cells[x][y] == CELL_STATE_ALIVE
				//? ANSI_COLOR_GREEN "O " ANSI_COLOR_RESET
				//: ". ");
    //    }
    //    printf("\n");
    //}
    
    for (size_t y = 0; y < BOARD_HEIGHT; y++) {
        for (size_t x = 0; x < BOARD_WIDTH; x++) {
            printf("%s", new_cb.buffer[x][y]);
        }
        printf("\n");
    }
    printf("\n\n");

    *cb = new_cb;
    return EXIT_SUCCESS;
}

bool update(world *w, query_context *qc, double delta_time_seconds) {
    bool result = cell_update_system(w, qc, qc)
        || cell_render_system(w, qc, qc);
    //query_context_clear_temporal(qc);
    return result;
}

void main(void) {
    {
        world w = world_create();
        query_context qc = query_context_create();

        bool quitting = false;
        bool app_error = false;

        if (init(&w)) {
            app_error = true;
        }
        assert((world_entity_count(&w) == (BOARD_WIDTH * BOARD_HEIGHT)) && "debug unreachable");

        game_time* t = WORLD_GET_RESOURCE(game_time, &w);
        timespec_get(&t->frame_start, TIME_UTC);
        Sleep(1000.0 / TARGET_FPS);
        while (!quitting && !app_error)
        {
            timespec_get(&t->frame_end, TIME_UTC);

            if (update(&w, &qc, game_time_update_delta_time(t))) {
                app_error = true;
            }

            timespec_get(&t->frame_start, TIME_UTC);
            Sleep(1000.0 / TARGET_FPS);
        }

        query_context_free(&qc);
        world_free(&w);
        printf("Freed world\n");
    }
    scanf_s("%*c");
}