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

typedef struct v2_i16 {
    int16_t x;
    int16_t y;
} v2_i16;

typedef v2_i16 position;
COMPONENT_IMPLEMENT(position);

typedef v2_i16 velocity;
COMPONENT_IMPLEMENT(velocity);

typedef struct velocity_register {
    v2_i16 velocity;
} velocity_register;
COMPONENT_IMPLEMENT(velocity_register);

typedef struct box {
    position corner_position;
    v2_i16 size;
} box;
COMPONENT_IMPLEMENT(box);

typedef bool is_solid;
TAG_IMPLEMENT(is_solid);

typedef bool is_duck;
TAG_IMPLEMENT(is_duck);

typedef bool is_shockwave;
TAG_IMPLEMENT(is_shockwave);

typedef struct controllable {
    #define CONTROLLABLE_BUFFER_SIZE 32
    bool active;
    uint8_t buffer_count;
    char buffer[CONTROLLABLE_BUFFER_SIZE];
} controllable;
COMPONENT_IMPLEMENT(controllable);

typedef struct renderable {
    v2_i16 offset;
    v2_i16 size;
    char **sprite;
} renderable;
COMPONENT_IMPLEMENT(renderable);

typedef struct dies_by {
    entity_id cause;
} dies_by;
COMPONENT_IMPLEMENT(dies_by);

static renderable renderable_create(arena *a, char **sprite, v2_i16 offset, v2_i16 size) {
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
        (v2_i16){0, 0},
        (v2_i16){1, 2}
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
        (v2_i16){-radius, -radius},
        (v2_i16){size, size}
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
        (v2_i16){ 0, 0 },
        (v2_i16){ 1, 1 }
    );

}

static renderable renderable_create_slime(arena *a) {
    char *sprite[1][2] = { { GRN "\xf0\x9f\xa7\xbf" CRESET, "\0" } };
    return renderable_create(
        a,
        sprite,
        (v2_i16){ 0, 0 },
        (v2_i16){ 2, 1 }
    );
}

static renderable renderable_create_duck(arena *a, char *color) {
    char *sprite[1][2] = { { YEL "\xc2\xac" CRESET, color } };
    return renderable_create(
        a,
        sprite,
        (v2_i16){ 0, 0 },
        (v2_i16){ 2, 1 }
    );
}

typedef struct console_buffer {
    char *buffer[BOARD_WIDTH][BOARD_HEIGHT];
} console_buffer;
RESOURCE_IMPLEMENT(console_buffer);

typedef arena strings_arena;
RESOURCE_IMPLEMENT(strings_arena);

bool create_duck(arena *a, world *w, v2_i16 initial_position, const entity_id *const threats, size_t threats_count) {
    entity_id e = world_add_entity(w);
    WORLD_SET_COMPONENT(
        position,
        w,
        e,
        &initial_position
    );

    char *default_color = WHT "\xe2\x96\x84" CRESET;
    char *golden_color = YEL "\xe2\x96\x84" CRESET;
    renderable r = renderable_create_duck(a, rand() % 1000 < 45 ? golden_color : default_color);
    WORLD_SET_COMPONENT(
        renderable,
        w,
        e,
        &r
    );
    WORLD_ADD_TAG(is_duck, w, e);
    for (size_t i = 0; i < threats_count; i++) {
        WORLD_SET_COMPONENT_RELATION(dies_by, w, e, &(dies_by){ threats[i] }, threats[i]);
    }
    return EXIT_SUCCESS;
}

bool create_slime(arena *a, world *w, v2_i16 initial_position, entity_id parent) {
    entity_id e = world_add_entity(w);
    WORLD_SET_COMPONENT(
        position,
        w,
        e,
        &initial_position
    );
    renderable r = renderable_create_slime(a);
    WORLD_SET_COMPONENT(
        renderable,
        w,
        e,
        &r
    );
    WORLD_SET_COMPONENT_RELATION(is_child_of, w, e, &(is_child_of){parent}, parent);
    return EXIT_SUCCESS;
}

entity_id create_lonk(arena *a, world *w) {
    entity_id e = world_add_entity(w);
    WORLD_SET_COMPONENT(
        position,
        w,
        e,
        &((position) {
            BOARD_WIDTH / 2, BOARD_HEIGHT / 2
        })
    );
    WORLD_SET_COMPONENT(
        velocity,
        w,
        e,
        &((velocity) {
            0, 0
        })
    );
    WORLD_SET_COMPONENT(
        velocity_register,
        w,
        e,
        &((velocity_register) {
            .velocity = {0, 0}
        })
    );
    renderable r = renderable_create_lonk(a);
    WORLD_SET_COMPONENT(
        renderable,
        w,
        e,
        &r
    );
    WORLD_SET_COMPONENT(
        controllable,
        w,
        e,
        &((controllable) {
            .active = true,
            .buffer_count = 0,
            .buffer = {0}
        })
    );
    return e;
}

bool create_map(arena *a, world *w) {
    for (uint16_t x = 0; x < BOARD_WIDTH; x++) {
        for (uint16_t y = 0; y < BOARD_HEIGHT; y++) {
            entity_id e = world_add_entity(w);

            if (x == 0 || x == BOARD_WIDTH - 1 || y == 0 || y == BOARD_HEIGHT - 1) {
                WORLD_SET_COMPONENT(
                    position,
                    w,
                    e,
                    &((position) {
                        x, y
                    })
                );
                WORLD_SET_COMPONENT(
                    box,
                    w,
                    e,
                    &((box) {
                        .corner_position = {0, 0},
                        .size = {1, 1}
                    })
                );
                renderable r = renderable_create_wall(a, BLU "\xe2\x96\x88" CRESET);
                renderable *wr = WORLD_SET_COMPONENT(
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
    game_time *t = WORLD_SET_RESOURCE(
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
    strings_arena *sa = WORLD_SET_RESOURCE(strings_arena, w, &a);

    console_buffer cb;
    for (uint16_t x = 0; x < BOARD_WIDTH; x++) {
        for (uint16_t y = 0; y < BOARD_HEIGHT; y++) {
            cb.buffer[x][y] = " ";
        }
    }
    SetConsoleOutputCP(65001);
    entity_id lonk = create_lonk(sa, w);
    create_map(sa, w);
    for (size_t i = 0; i < 2; i++) {
        create_duck(sa, w, (v2_i16){BOARD_WIDTH / 2, BOARD_HEIGHT / 2 - 2}, (entity_id[]){lonk}, 1);
    }
    create_slime(sa, w, (v2_i16){BOARD_WIDTH / 4, BOARD_HEIGHT / 4 + 2}, lonk);
    create_slime(sa, w, (v2_i16){BOARD_WIDTH / 4 * 3, BOARD_HEIGHT / 4 + 2}, world_add_entity(w));

    WORLD_SET_RESOURCE(console_buffer, w, &cb);
    return EXIT_SUCCESS;
}

bool create_shockwave(world *w, position *p, velocity *v) {
    strings_arena *sa = WORLD_GET_RESOURCE(strings_arena, w);
    entity_id e = world_add_entity(w);

    WORLD_SET_COMPONENT(
        position,
        w,
        e,
        p
    );
    const int16_t SPEED_MULTIPLIER = 3;
    const int16_t RADIUS = SPEED_MULTIPLIER;
    renderable r = renderable_create_shockwave(sa, RADIUS);
    WORLD_SET_COMPONENT(
        renderable,
        w,
        e,
        &r
    );
    WORLD_SET_COMPONENT(
        velocity,
        w,
        e,
        &((velocity) {
            v->x * SPEED_MULTIPLIER, v->y * SPEED_MULTIPLIER
        })
    );
    WORLD_ADD_TAG(is_shockwave, w, e);
    return EXIT_SUCCESS;
}

void update_controllables(COMPONENT_ITERATION_HANDLE_STRUCT(velocity, controllable) *handle, world *w, double delta_time_seconds) {
    velocity *v = handle->velocity_component;
    controllable c = *handle->controllable_component;
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


void update_lonk(
    COMPONENT_ITERATION_HANDLE_STRUCT(position, velocity, renderable, velocity_register, controllable) *handle,
    world *w,
    double delta_time_seconds
) {
    position *p = handle->position_component;
    velocity v = *handle->velocity_component;
    p->x += v.x;
    p->y += v.y;
    velocity_register *vr = handle->velocity_register_component;
    if (v.x != 0 || v.y != 0) {
        vr->velocity = v;
    }
    controllable c = *handle->controllable_component;
    for (size_t k = 0; k < c.buffer_count; k++) {
        switch (c.buffer[k]) {
            case 'x':
                create_shockwave(w, p, &vr->velocity);
                break;
        }
    }
}

void update_ducks(
    COMPONENT_ITERATION_HANDLE_STRUCT(position) *handle,
    world *w,
    double delta_time_seconds
) {
    position *p = handle->position_component;
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

void update_shockwaves(
    COMPONENT_ITERATION_HANDLE_STRUCT(position, velocity, renderable) *handle,
    world *w,
    double delta_time_seconds
) {
    strings_arena *sa = WORLD_GET_RESOURCE(strings_arena, w);
    position *p = handle->position_component;
    velocity *v = handle->velocity_component;
    p->x += v->x;
    p->y += v->y;
    renderable new_r = renderable_create_shockwave(sa, abs(v->x) > abs(v->y) ? abs(v->x) : abs(v->y));
    *handle->renderable_component = new_r;
    if (p->x < 0 || p->x >= BOARD_WIDTH
        || p->y < 0 || p->y >= BOARD_HEIGHT
        || v->x == 0 && v->y == 0) {
        world_remove_entity(w, handle->entity_id);
    }
    if (abs(v->x) > 0) {
        v->x -= v->x / abs(v->x);
    }
    if (abs(v->y) > 0) {
        v->y -= v->y / abs(v->y);
    }
}

void update_children_position(
    COMPONENT_ITERATION_HANDLE_STRUCT(position, renderable, is_child_of) *handle,
    world *w,
    double delta_time_seconds
) {
    position p = *handle->position_component;
    position parent_position = *WORLD_GET_COMPONENT(position, w, handle->is_child_of_component->parent);
    handle->renderable_component->offset = (v2_i16){ parent_position.x - p.x, parent_position.y - p.y };
}

void update_threatened(
    COMPONENT_ITERATION_HANDLE_STRUCT(position, dies_by) *handle,
    world *w,
    double delta_time_seconds
) {
    position p = *handle->position_component;
    position *threat_position = NULL;
    if (WORLD_TRY_GET_COMPONENT(position, w, handle->dies_by_component->cause, &threat_position)
        && p.x == threat_position->x && p.y == threat_position->y) {
        world_remove_entity(w, handle->entity_id);
    }
}

bool update_entities(world *w, arena *iteration_arena, double delta_time_seconds) {
    WORLD_SYSTEM_ITER_GENERIC(
        world_dt_system_predicate,
        WORLD_SYSTEM_CREATE(velocity, controllable),
        w,
        delta_time_seconds,
        iteration_arena,
        update_controllables
    );
    WORLD_SYSTEM_ITER_GENERIC(
        world_dt_system_predicate,
        WORLD_SYSTEM_CREATE(position, velocity, renderable, velocity_register, controllable),
        w,
        delta_time_seconds,
        iteration_arena,
        update_lonk
    );
    WORLD_SYSTEM_ITER_GENERIC(
        world_dt_system_predicate,
        WORLD_SYSTEM_CREATE(position, velocity, renderable, is_shockwave),
        w,
        delta_time_seconds,
        iteration_arena,
        update_shockwaves
    );
    entity_count duck_count = WORLD_SYSTEM_ITER_GENERIC(
        world_dt_system_predicate,
        WORLD_SYSTEM_CREATE(position, is_duck),
        w,
        delta_time_seconds,
        iteration_arena,
        update_ducks
    );
    WORLD_SYSTEM_ITER_GENERIC(
        world_dt_system_predicate,
        WORLD_SYSTEM_CREATE(position, dies_by),
        w,
        delta_time_seconds,
        iteration_arena,
        update_threatened
    );
    // entity_id lonk;
    // WORLD_GET_ENTITY_WITH(w, &lonk, position, velocity, renderable, velocity_register, controllable);
    
    // WORLD_SYSTEM_ITER_GENERIC(
    //     world_dt_system_predicate,
    //     WORLD_SYSTEM_CREATE_FROM_IDS(COMPONENT_ID(position), COMPONENT_ID(renderable), (RELATION_ID(is_child_of, lonk))),
    //     w,
    //     delta_time_seconds,
    //     iteration_arena,
    //     update_children_position
    // );
    return EXIT_SUCCESS;
}

bool render(const world *w, arena *iteration_arena) {
    console_buffer *cb = WORLD_GET_RESOURCE(console_buffer, w);
    console_buffer new_console_buffer = *cb;

    for (uint16_t x = 0; x < BOARD_WIDTH; x++) {
        for (uint16_t y = 0; y < BOARD_HEIGHT; y++) {
            new_console_buffer.buffer[x][y] = " ";
        }
    }

    COMPONENT_ITERATION_HANDLE_STRUCT(position, renderable) handle;
    for (
        component_iterator it = COMPONENT_ITERATOR_CREATE(
            &w->components,
            iteration_arena,
            position,
            renderable
        );
        !component_iterator_done(&it);
        component_iterator_next(&it)
    ) {
        component_iterator_current(&it, &handle);
        position p = *handle.position_component;
        renderable r = *handle.renderable_component;
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
    
    printf("\x1b[%d;%dH\x1b[J", 0, 0);
    fflush(stdout);
    //system("cls");
    arena screen_arena = arena_create();
    list screen = list_create_with_capacity(&screen_arena, sizeof(char) * BOARD_WIDTH * BOARD_HEIGHT);
    for (uint16_t y = 0; y < BOARD_HEIGHT; y++) {
        for (uint16_t x = 0; x < BOARD_WIDTH; x++) {
            size_t i = 0;
            while (new_console_buffer.buffer[x][y][i] != '\0')
                list_add(&screen, &screen_arena, &new_console_buffer.buffer[x][y][i++], sizeof(char));        
        }
        list_add(&screen, &screen_arena, "\n", sizeof(char));
    }
    printf("%s", (char *)screen.elements);
    arena_free(&screen_arena);
    printf("fps: %f\n", 1.0 / WORLD_GET_RESOURCE(game_time, w)->averaged_delta_time_seconds);

    *cb = new_console_buffer;
    return EXIT_SUCCESS;
}

bool process_input(world *w, arena *iteration_arena) {
    controllable c = (controllable) {
        .active = true,
        .buffer_count = 0,
        .buffer = {0}
    };
    while (_kbhit()) {
        char input = getch();
        if (input == '\x1b')
            return EXIT_FAILURE;

        if (c.buffer_count >= CONTROLLABLE_BUFFER_SIZE) {
            for (size_t j = 0; j < CONTROLLABLE_BUFFER_SIZE - 1; j++) {
                c.buffer[j] = c.buffer[j + 1];
            }
            c.buffer_count = CONTROLLABLE_BUFFER_SIZE - 1;
        }
        c.buffer[c.buffer_count++] = input;
    }

    COMPONENT_ITERATION_HANDLE_STRUCT(controllable) handle;
    for (
        component_iterator it = COMPONENT_ITERATOR_CREATE(
            &w->components,
            iteration_arena,
            controllable
        );
        !component_iterator_done(&it);
        component_iterator_next(&it)
    ) {
        component_iterator_current(&it, &handle);
        controllable *current = handle.controllable_component;
        if (current->active) {
            *current = c;
        }
    }

    return EXIT_SUCCESS;
}

bool update(world *w, double delta_time_seconds) {
    arena iteration_arena = arena_create();
    bool result = update_entities(w, &iteration_arena, delta_time_seconds)
        || render(w, &iteration_arena)
        || process_input(w, &iteration_arena);
    arena_free(&iteration_arena);
    return result;
}

bool finalize(world *w) {
    arena_free(WORLD_GET_RESOURCE(strings_arena, w));
    return EXIT_SUCCESS;
}

int main(void) {
    world w = world_create(1024, 32, 4);

    bool quitting = false;
    bool app_error = false;
    if (init(&w)) {
        app_error = true;
    }

    game_time* t = WORLD_GET_RESOURCE(game_time, &w);
    timespec_get(&t->frame_start, TIME_UTC);
    const DWORD sleep_milliseconds = 1000 / TARGET_FPS;
    Sleep(sleep_milliseconds);
    while (!quitting && !app_error)
    {
        timespec_get(&t->frame_end, TIME_UTC);
        game_time_update_time_since_start(t);

        if (update(&w, game_time_update_delta_time(t))) {
            app_error = true;
        }

        timespec_get(&t->frame_start, TIME_UTC);
        Sleep(sleep_milliseconds);
    }

    if (finalize(&w)) {
        app_error = true;
    }

    world_free(&w);
    return app_error ? EXIT_FAILURE : EXIT_SUCCESS;
}
