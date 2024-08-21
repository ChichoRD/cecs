#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <windows.h>
#include <time.h>
#include <conio.h>
#include "../include/lib.h"
#include "../include/color.h"
#include "../include/core/relation.h"

#define TARGET_FPS 240.0

#define BOARD_WIDTH 48
#define BOARD_HEIGHT 16

typedef struct vector2int {
    int16_t x;
    int16_t y;
} vector2int;

COMPONENT_DEFINE(vector2int, position);
COMPONENT_DEFINE(vector2int, velocity);

COMPONENT_DEFINE(struct, velocity_register) {
    vector2int velocity;
} velocity_register;

COMPONENT_DEFINE(struct, box) {
    position corner_position;
    vector2int size;
} box;

TAG_DEFINE(is_solid);

TAG_DEFINE(is_duck);

TAG_DEFINE(is_shockwave);

COMPONENT_DEFINE(struct, controllable) {
    #define CONTROLLABLE_BUFFER_SIZE 32
    bool active;
    uint8_t buffer_count;
    char buffer[CONTROLLABLE_BUFFER_SIZE];
} controllable;

COMPONENT_DEFINE(struct, renderable) {
    vector2int offset;
    vector2int size;
    char **sprite;
} renderable;

//RELATION_DEFINE(is_duck, position);

static renderable renderable_create(arena *a, char **sprite, vector2int offset, vector2int size) {
	renderable r = (renderable) {
		.sprite = arena_alloc(a, sizeof(char*) * size.x * size.y),
		.offset = offset,
		.size = size
	};
    for (int16_t x = 0; x < size.x; x++) {
		for (int16_t y = 0; y < size.y; y++) {
            r.sprite[x + size.x * y] = sprite[y + size.y * x];
		}
	}
    return r;
}

static renderable renderable_create_lonk(arena *a) {
    char *sprite[2][1] = {
        { GRN "\xe2\x96\xb2" CRESET },
        { YEL "\xe2\x96\xa0" CRESET },
    };
    return renderable_create(
        a,
        sprite,
        (vector2int){0, 0},
        (vector2int){1, 2}
    );
}

static renderable renderable_create_shockwave(arena *a, int16_t radius) {
    int16_t size = radius * 2 + 1;
    char **sprite = calloc(size * size, sizeof(char *));
    for (int16_t i = -radius; i <= radius; i++) {
        for (int16_t j = -radius; j <= radius; j++) {
            size_t index = i + radius + (j + radius) * size;
            if (abs(i) + abs(j) == radius) {
                sprite[index] = RED "\xc3\x97" CRESET;
            } else {
                sprite[index] = " ";
            }
        }
    }
    renderable r = renderable_create(
        a,
        sprite,
        (vector2int){-radius, -radius},
        (vector2int){size, size}
    );
    free(sprite);
    return r;
}

static renderable renderable_create_wall(arena *a, char *color) {
    // cheat: ░      ▒      ▓      █      ▄      ▀      ■      ▲      ■      ¬      ×     🧿
    // cheat: \xe2\x96\x91  \xe2\x96\x92    \xe2\x96\x93    \xe2\x96\x88    \xe2\x96\x84    \xe2\x96\x80    \xe2\x96\xa0    \xe2\x96\xb2    \xe2\x96\xa0    \xc2\xac    \xc3\x97    \xf0\x9f\xa7\xbf
    char *sprite[1][1] = {
		{ color }
	};
    return renderable_create(
        a,
        sprite,
        (vector2int){ 0, 0 },
        (vector2int){ 1, 1 }
    );

}

static renderable renderable_create_slime(arena *a) {
    char *sprite[1][2] = { { GRN "\xf0\x9f\xa7\xbf" CRESET, "\0" } };
    return renderable_create(
        a,
        sprite,
        (vector2int){ 0, 0 },
        (vector2int){ 2, 1 }
    );
}

static renderable renderable_create_duck(arena *a) {
    char *sprite[1][2] = { { YEL "\xc2\xac" CRESET, WHT "\xe2\x96\x84" CRESET } };
    return renderable_create(
        a,
        sprite,
        (vector2int){ 0, 0 },
        (vector2int){ 2, 1 }
    );
}

RESOURCE_DEFINE(struct, console_buffer) {
    char *buffer[BOARD_WIDTH][BOARD_HEIGHT];
} console_buffer;

RESOURCE_DEFINE(struct, strings_arena) {
    arena a;
} strings_arena;

bool create_duck(arena *a, world *w, vector2int initial_position) {
    entity_id e = world_add_entity(w);
    WORLD_SET_OR_ADD_COMPONENT(
        position,
        w,
        e,
        &initial_position
    );
    renderable r = renderable_create_duck(a);
    WORLD_SET_OR_ADD_COMPONENT(
        renderable,
        w,
        e,
        &r
    );
    WORLD_ADD_TAG(is_duck, w, e);
    return EXIT_SUCCESS;
}

bool create_slime(arena *a, world *w, vector2int initial_position) {
    entity_id e = world_add_entity(w);
    WORLD_SET_OR_ADD_COMPONENT(
        position,
        w,
        e,
        &initial_position
    );
    renderable r = renderable_create_slime(a);
    WORLD_SET_OR_ADD_COMPONENT(
        renderable,
        w,
        e,
        &r
    );
    return EXIT_SUCCESS;
}

bool create_lonk(arena *a, world *w) {
    entity_id e = world_add_entity(w);
    WORLD_SET_OR_ADD_COMPONENT(
        position,
        w,
        e,
        &((position) {
            BOARD_WIDTH / 2, BOARD_HEIGHT / 2
        })
    );
    WORLD_SET_OR_ADD_COMPONENT(
        velocity,
        w,
        e,
        &((velocity) {
            0, 0
        })
    );
    WORLD_SET_OR_ADD_COMPONENT(
        velocity_register,
        w,
        e,
        &((velocity_register) {
            .velocity = {0, 0}
        })
    );
    renderable r = renderable_create_lonk(a);
    WORLD_SET_OR_ADD_COMPONENT(
        renderable,
        w,
        e,
        &r
    );
    WORLD_SET_OR_ADD_COMPONENT(
        controllable,
        w,
        e,
        &((controllable) {
            .active = true,
            .buffer_count = 0,
            .buffer = {0}
        })
    );
    return EXIT_SUCCESS;
}

bool create_map(arena *a, world *w) {
    for (uint16_t x = 0; x < BOARD_WIDTH; x++) {
        for (uint16_t y = 0; y < BOARD_HEIGHT; y++) {
            entity_id e = world_add_entity(w);

            if (x == 0 || x == BOARD_WIDTH - 1 || y == 0 || y == BOARD_HEIGHT - 1) {
                WORLD_SET_OR_ADD_COMPONENT(
                    position,
                    w,
                    e,
                    &((position) {
                        x, y
                    })
                );
                WORLD_SET_OR_ADD_COMPONENT(
                    box,
                    w,
                    e,
                    &((box) {
                        .corner_position = {0, 0},
                        .size = {1, 1}
                    })
                );
                renderable r = renderable_create_wall(a, BLU "\xe2\x96\x88" CRESET);
                renderable *wr = WORLD_SET_OR_ADD_COMPONENT(
                    renderable,
                    w,
                    e,
                    &r
                );
                WORLD_ADD_TAG(is_solid, w, e);
            }
        }
    } 
    return EXIT_SUCCESS;
}

bool init(world *w) {
    game_time *t = WORLD_SET_OR_ADD_RESOURCE(
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
    arena a = arena_create();
    strings_arena *sa = WORLD_SET_OR_ADD_RESOURCE(strings_arena, w, &a);

    console_buffer cb;
    for (uint16_t x = 0; x < BOARD_WIDTH; x++) {
        for (uint16_t y = 0; y < BOARD_HEIGHT; y++) {
            cb.buffer[x][y] = " ";
        }
    }
    SetConsoleOutputCP(65001);
    create_lonk(&sa->a, w);
    create_map(&sa->a, w);
    for (size_t i = 0; i < 10; i++) {
        create_duck(&sa->a, w, (vector2int){BOARD_WIDTH / 2, BOARD_HEIGHT / 2 - 2});
    }
    create_slime(&sa->a, w, (vector2int){BOARD_WIDTH / 4, BOARD_HEIGHT / 4 + 2});

    WORLD_SET_OR_ADD_RESOURCE(console_buffer, w, &cb);
    return EXIT_SUCCESS;
}

bool create_shockwave(world *w, query_context *qc, position *p, velocity *v) {
    strings_arena *sa = WORLD_GET_RESOURCE(strings_arena, w);
    entity_id e = world_add_entity(w);
    WORLD_SET_OR_ADD_COMPONENT(
        position,
        w,
        e,
        p
    );
    const int16_t SPEED_MULTIPLIER = 3;
    const int16_t RADIUS = SPEED_MULTIPLIER;
    renderable r = renderable_create_shockwave(&sa->a, RADIUS);
    WORLD_SET_OR_ADD_COMPONENT(
        renderable,
        w,
        e,
        &r
    );
    WORLD_SET_OR_ADD_COMPONENT(
        velocity,
        w,
        e,
        &((velocity) {
            v->x * SPEED_MULTIPLIER, v->y * SPEED_MULTIPLIER
        })
    );
    WORLD_ADD_TAG(is_shockwave, w, e);
    query_context_clear(qc);
    return EXIT_SUCCESS;
}

bool update_controllables(world *w, query_context *qc, double delta_time_seconds) {
    query q = QUERY_CREATE(WITH_COMPONENTS(velocity, controllable), WITHOUT_TAGS);
    struct QUERY_RESULT(velocity, controllable) *qr = QUERY_CONTEXT_RUN_QUERY(qc, q, w, velocity, controllable);

    for (size_t i = 0; i < qr->view_count; i++)
    {
        struct QUERY_VIEW(velocity, controllable) view = qr->query_views[i];
        for (size_t j = 0; j < view.count; j++)
        {
            velocity *v = &view.query_elements.velocity_elements[j];
            controllable c = view.query_elements.controllable_elements[j];
            if (c.buffer_count <= 0) {
                *v = (velocity){0, 0};
            } else {
                for (size_t k = 0; k < c.buffer_count; k++) {
                    switch (c.buffer[k]) {
                        case 'w':
                            *v = (velocity){0, -1};
                            break;
                        case 'a':
                            *v = (velocity){-1, 0};
                            break;
                        case 's':
                            *v = (velocity){0, 1};
                            break;
                        case 'd':
                            *v = (velocity){1, 0};
                            break;
                    }
                }
            }
        }
    }
        return EXIT_SUCCESS;
}

bool update_lonk(world *w, query_context *qc, double delta_time_seconds) {
    query q = QUERY_CREATE(WITH_COMPONENTS(position, velocity, renderable, velocity_register, controllable), WITHOUT_TAGS);

    // for (size_t i = 0; i < qr->view_count; i++) {
    //     struct QUERY_VIEW(position, velocity, renderable, velocity_register, controllable) view =
    //         qr->query_views[i];
    //     for (size_t j = 0; j < view.count; j++)
    //     {
    //         position *p = &view.query_elements.position_elements[j];
    //         velocity v = view.query_elements.velocity_elements[j];
    //         p->x += v.x;
    //         p->y += v.y;

    //         velocity_register *vr = &view.query_elements.velocity_register_elements[j];
    //         if (v.x != 0 || v.y != 0) {
    //             vr->velocity = v;
    //         }
    //         controllable c = view.query_elements.controllable_elements[j];

    //         for (size_t k = 0; k < c.buffer_count; k++) {
    //             switch (c.buffer[j]) {
    //                 case 'x':
    //                     create_shockwave(w, qc, p, &vr->velocity);
    //                     break;
    //             }
    //         }

    //     }
        
    // }
    for (
        query_iterator qi = QUERY_CONTEXT_RUN_QUERY_INTO_ITER(qc, q, w, position, velocity, renderable, velocity_register, controllable);
        !query_iterator_done(&qi);
        query_iterator_next(&qi)
        ) {
        struct QUERY_VIEW_ELEMENTS *view = QUERY_ITERATOR_CURRENT(&qi, position, velocity, renderable, velocity_register, controllable);
            position *p = &view->position_elements[qi.current_view_index];
            velocity v = view->velocity_elements[qi.current_view_index];
            p->x += v.x;
            p->y += v.y;

            velocity_register *vr = &view->velocity_register_elements[qi.current_view_index];
            if (v.x != 0 || v.y != 0) {
                vr->velocity = v;
            }
            controllable c = view->controllable_elements[qi.current_view_index];

            for (size_t k = 0; k < c.buffer_count; k++) {
                switch (c.buffer[k]) {
                    case 'x':
                        create_shockwave(w, qc, p, &vr->velocity);
                        break;
                }
            }
    }
    return EXIT_SUCCESS;
}

bool update_ducks(world *w, query_context *qc, double delta_time_seconds) {
    query q = QUERY_CREATE(WITH_COMPONENTS(position), WITH_TAGS(is_duck));
    struct QUERY_RESULT(position) *qr = QUERY_CONTEXT_RUN_QUERY(qc, q, w, position);

    for (size_t i = 0; i < qr->view_count; i++) {
        struct QUERY_VIEW(position) view = qr->query_views[i];
        for (size_t j = 0; j < view.count; j++) {
            position *p = &view.query_elements.position_elements[j];
            if (rand() % 2 == 0) {
                switch (rand() % 4) {
                    case 0:
                        p->y--;
                        break;
                    case 1:
                        p->x--;
                        break;
                    case 2:
                        p->y++;
                        break;
                    case 3:
                        p->x++;
                        break;
                }
            }
        }
    }
    return EXIT_SUCCESS;
}

bool update_shockwaves(world *w, query_context *qc, double delta_time_seconds) {
    query q = QUERY_CREATE(WITH_COMPONENTS(position, velocity, renderable), WITH_TAGS(is_shockwave));
    struct QUERY_RESULT(position, velocity, renderable) *qr = QUERY_CONTEXT_RUN_QUERY(qc, q, w, position, velocity, renderable);

    strings_arena *sa = WORLD_GET_RESOURCE(strings_arena, w);
    for (size_t i = 0; i < qr->view_count; i++) {
        struct QUERY_VIEW(position, velocity, renderable) view = qr->query_views[i];
        for (size_t j = 0; j < view.count; j++) {
            position *p = &view.query_elements.position_elements[j];
            velocity *v = &view.query_elements.velocity_elements[j];
            p->x += v->x;
            p->y += v->y;

            renderable new_r = renderable_create_shockwave(&sa->a, abs(v->x) > abs(v->y) ? abs(v->x) : abs(v->y));
            WORLD_SET_OR_ADD_COMPONENT(
                renderable,
                w,
                view.query_elements.entity_id_view_start + j,
                &new_r
            );

            if (p->x < 0 || p->x >= BOARD_WIDTH
                || p->y < 0 || p->y >= BOARD_HEIGHT
                || v->x == 0 && v->y == 0) {
                world_remove_entity(w, view.query_elements.entity_id_view_start + j);
                query_context_clear(qc);
            }

            if (abs(v->x) > 0) {
                v->x -= v->x / abs(v->x);
            }
            if (abs(v->y) > 0) {
                v->y -= v->y / abs(v->y);
            }

        }
    }
    return EXIT_SUCCESS;
}

bool update_entities(world *w, query_context *qc, double delta_time_seconds) {
    return update_controllables(w, qc, delta_time_seconds)
        || update_lonk(w, qc, delta_time_seconds)
        || update_shockwaves(w, qc, delta_time_seconds)
        || update_ducks(w, qc, delta_time_seconds);
}

bool render(const world *w, query_context *qc) {
    query q = QUERY_CREATE(WITH_COMPONENTS(position, renderable), WITHOUT_TAGS);
    struct QUERY_RESULT(position, renderable) *qr = QUERY_CONTEXT_RUN_QUERY(qc, q, w, position, renderable);

    console_buffer *cb = WORLD_GET_RESOURCE(console_buffer, w);
    console_buffer new_console_buffer = *cb;

    for (uint16_t x = 0; x < BOARD_WIDTH; x++) {
        for (uint16_t y = 0; y < BOARD_HEIGHT; y++) {
            new_console_buffer.buffer[x][y] = " ";
        }
    }

    for (size_t i = 0; i < qr->view_count; i++) {
        struct QUERY_VIEW(position, renderable) view = qr->query_views[i];
        for (size_t j = 0; j < view.count; j++) {
            position p = view.query_elements.position_elements[j];
            renderable r = view.query_elements.renderable_elements[j];
            for (int16_t x = 0; x < r.size.x; x++) {
                for (int16_t y = 0; y < r.size.y; y++) {
                    int16_t cx = p.x + x + r.offset.x;
                    int16_t cy = p.y + y + r.offset.y;
                    if (cx >= 0 && cx < BOARD_WIDTH && cy >= 0 && cy < BOARD_HEIGHT) {
                        new_console_buffer.buffer[cx][cy] = r.sprite[x + r.size.x * y];
                    }
                }
            }
        }
    }
    
    system("cls");
    for (uint16_t y = 0; y < BOARD_HEIGHT; y++) {
        for (uint16_t x = 0; x < BOARD_WIDTH; x++) {
            printf("%s", new_console_buffer.buffer[x][y]);
        }
        printf("\n");
    }
    //printf("\n");
    printf("fps: %f\n", 1.0 / WORLD_GET_RESOURCE(game_time, w)->averaged_delta_time_seconds);

    WORLD_SET_OR_ADD_RESOURCE(console_buffer, w, &new_console_buffer);
    return EXIT_SUCCESS;
}

bool process_input(world *w, query_context *qc) {
    controllable c = (controllable) {
        .active = true,
        .buffer_count = 0,
        .buffer = {0}
    };
    while (_kbhit()) {
        char input = getch();
        if (c.buffer_count >= CONTROLLABLE_BUFFER_SIZE) {
            for (size_t j = 0; j < CONTROLLABLE_BUFFER_SIZE - 1; j++) {
                c.buffer[j] = c.buffer[j + 1];
            }
            c.buffer_count = CONTROLLABLE_BUFFER_SIZE - 1;
        }
        c.buffer[c.buffer_count++] = input;
    }

    query q = QUERY_CREATE(WITH_COMPONENTS(controllable), WITHOUT_TAGS);
    struct QUERY_RESULT(controllable) *qr = QUERY_CONTEXT_RUN_QUERY(qc, q, w, controllable);

    for (size_t i = 0; i < qr->view_count; i++) {
        struct QUERY_VIEW(controllable) view = qr->query_views[i];
        for (size_t j = 0; j < view.count; j++) {
            controllable *current = &view.query_elements.controllable_elements[j];
            if (current->active) {
                *current = c;
            }
        }
    }
    return EXIT_SUCCESS;
}

bool update(world *w, query_context *qc, double delta_time_seconds) {
    bool result = update_entities(w, qc, delta_time_seconds)
        || render(w, qc)
        || process_input(w, qc);
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

        game_time* t = WORLD_GET_RESOURCE(game_time, &w);
        timespec_get(&t->frame_start, TIME_UTC);
        Sleep(1000.0 / TARGET_FPS);
        while (!quitting && !app_error)
        {
            timespec_get(&t->frame_end, TIME_UTC);

            game_time_update_time_since_start(t);
            if (update(&w, &qc, game_time_update_delta_time(t))) {
                app_error = true;
            }

            timespec_get(&t->frame_start, TIME_UTC);
            Sleep(1000.0 / TARGET_FPS);
        }

        query_context_free(&qc);
        world_free(&w);
    }
}