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

typedef bool owns_renderable;
TAG_IMPLEMENT(owns_renderable);

typedef struct dies_by {
    entity_id cause;
} dies_by;
COMPONENT_IMPLEMENT(dies_by);

static renderable renderable_create_alloc(char **sprite, v2_i16 offset, v2_i16 size) {
	renderable r = (renderable) {
		.sprite = calloc(size.x * size.y, sizeof(char*)),
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

static renderable renderable_create_lonk_alloc() {
    char *sprite[2][1] = {
        { GRN "\xe2\x96\xb2" CRESET },
        { YEL "\xe2\x96\xa0" CRESET },
    };
    return renderable_create_alloc(
        sprite,
        (v2_i16){0, 0},
        (v2_i16){1, 2}
    );
}

static renderable renderable_create_shockwave_alloc(int16_t radius) {
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
    renderable r = renderable_create_alloc(
        sprite,
        (v2_i16){-radius, -radius},
        (v2_i16){size, size}
    );
    free(sprite);
    return r;
}

static renderable renderable_create_wall_alloc(char *color) {
    // cheat: ░      ▒      ▓      █      ▄      ▀      ■      ▲      ■      ¬      ×     🧿
    // cheat: \xe2\x96\x91  \xe2\x96\x92    \xe2\x96\x93    \xe2\x96\x88    \xe2\x96\x84    \xe2\x96\x80    \xe2\x96\xa0    \xe2\x96\xb2    \xe2\x96\xa0    \xc2\xac    \xc3\x97    \xf0\x9f\xa7\xbf
    char *sprite[1][1] = {
		{ color }
	};
    return renderable_create_alloc(
        sprite,
        (v2_i16){ 0, 0 },
        (v2_i16){ 1, 1 }
    );

}

static renderable renderable_create_slime_alloc() {
    char *sprite[1][2] = { { GRN "\xf0\x9f\xa7\xbf" CRESET, "\0" } };
    return renderable_create_alloc(
        sprite,
        (v2_i16){ 0, 0 },
        (v2_i16){ 2, 1 }
    );
}

static renderable renderable_create_duck_alloc(char *color) {
    char *sprite[1][2] = { { YEL "\xc2\xac" CRESET, color } };
    return renderable_create_alloc(
        sprite,
        (v2_i16){ 0, 0 },
        (v2_i16){ 2, 1 }
    );
}

typedef struct console_buffer {
    char *buffer[BOARD_WIDTH][BOARD_HEIGHT];
} console_buffer;
RESOURCE_IMPLEMENT(console_buffer);

bool create_duck(world *w, v2_i16 initial_position, const entity_id *const threats, size_t threats_count) {
    entity_id e = world_add_entity(w);
    WORLD_SET_COMPONENT(
        position,
        w,
        e,
        &initial_position
    );

    char *default_color = WHT "\xe2\x96\x84" CRESET;
    char *golden_color = YEL "\xe2\x96\x84" CRESET;
    renderable r = renderable_create_duck_alloc(rand() % 1000 < 45 ? golden_color : default_color);
    WORLD_SET_COMPONENT(
        renderable,
        w,
        e,
        &r
    );
    WORLD_ADD_TAG(owns_renderable, w, e);
    WORLD_ADD_TAG(is_duck, w, e);
    for (size_t i = 0; i < threats_count; i++) {
        WORLD_SET_COMPONENT_RELATION(dies_by, w, e, &(dies_by){ threats[i] }, threats[i]);
    }
    return EXIT_SUCCESS;
}

bool create_slime(world *w, v2_i16 initial_position, entity_id parent) {
    entity_id e = world_add_entity(w);
    WORLD_SET_COMPONENT(
        position,
        w,
        e,
        &initial_position
    );
    renderable r = renderable_create_slime_alloc();
    WORLD_SET_COMPONENT(
        renderable,
        w,
        e,
        &r
    );
    WORLD_ADD_TAG(owns_renderable, w, e);
    WORLD_SET_COMPONENT_RELATION(is_child_of, w, e, &(is_child_of){parent}, parent);
    return EXIT_SUCCESS;
}

entity_id create_lonk_prefab(world *w) {
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
    renderable r = renderable_create_lonk_alloc();
    WORLD_SET_COMPONENT(
        renderable,
        w,
        e,
        &r
    );
    WORLD_ADD_TAG(owns_renderable, w, e);
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
    world_get_or_set_default_entity_flags(w, e)->is_permanent = true;
    return world_set_prefab(w, e);
}

entity_id create_lonk(world *w, prefab_id prefab) {
    entity_id lonk = world_add_entity_from_prefab(w, prefab);
    WORLD_REMOVE_TAG(owns_renderable, w, lonk);
    return lonk;
}

prefab_id create_wall_prefab(world *w) {
    entity_id e = world_add_entity(w);
    WORLD_SET_COMPONENT(
        position,
        w,
        e,
        &((position) {
            0, 0
        })
    );
    renderable r = renderable_create_wall_alloc(BLU "\xe2\x96\x88" CRESET);
    WORLD_SET_COMPONENT(
        renderable,
        w,
        e,
        &r
    );
    WORLD_ADD_TAG(owns_renderable, w, e);
    WORLD_ADD_TAG(is_solid, w, e);
    return world_set_prefab(w, e);
}

bool create_map(world *w, prefab_id prefab) {
    for (uint16_t x = 0; x < BOARD_WIDTH; x++) {
        for (uint16_t y = 0; y < BOARD_HEIGHT; y++) {
            entity_id e = world_add_entity(w);
            if (x == 0 || x == BOARD_WIDTH - 1 || y == 0 || y == BOARD_HEIGHT - 1) {
                entity_id wall = world_add_entity_from_prefab(w, prefab);
                WORLD_SET_COMPONENT(position, w, wall, (&(position){x, y}));
                WORLD_REMOVE_TAG(owns_renderable, w, wall);
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

    console_buffer cb;
    for (uint16_t x = 0; x < BOARD_WIDTH; x++) {
        for (uint16_t y = 0; y < BOARD_HEIGHT; y++) {
            cb.buffer[x][y] = " ";
        }
    }
    SetConsoleOutputCP(65001);
    entity_id lonk = create_lonk(w, create_lonk_prefab(w));
    world_add_entity_to_scene(w, lonk, 0);
    // entity_id lonk2 = world_add_entity(w);
    // world_add_entity_to_scene(w, lonk2, 1);
    // WORLD_COPY_ENTITY_ONTO_AND_GRAB(controllable, w, lonk2, lonk);//->active = false;

    create_map(w, create_wall_prefab(w));
    for (size_t i = 0; i < 2; i++) {
        create_duck(w, (v2_i16){BOARD_WIDTH / 2, BOARD_HEIGHT / 2 - 2}, (entity_id[]){lonk}, 1);
    }
    create_slime(w, (v2_i16){BOARD_WIDTH / 4, BOARD_HEIGHT / 4 + 2}, lonk);
    create_slime(w, (v2_i16){BOARD_WIDTH / 4 * 3, BOARD_HEIGHT / 4 + 2}, world_add_entity(w));

    WORLD_SET_RESOURCE(console_buffer, w, &cb);
    return EXIT_SUCCESS;
}

bool create_shockwave(world *w, position p, velocity v) {
    entity_id e = world_add_entity(w);

    WORLD_SET_COMPONENT(
        position,
        w,
        e,
        &p
    );
    const int16_t SPEED_MULTIPLIER = 4;
    const int16_t RADIUS = SPEED_MULTIPLIER;
    renderable r = renderable_create_shockwave_alloc(RADIUS);
    WORLD_SET_COMPONENT(
        renderable,
        w,
        e,
        &r
    );
    WORLD_ADD_TAG(owns_renderable, w, e);
    WORLD_SET_COMPONENT(
        velocity,
        w,
        e,
        &((velocity) {
            v.x * SPEED_MULTIPLIER, v.y * SPEED_MULTIPLIER
        })
    );
    WORLD_ADD_TAG(is_shockwave, w, e);
    return EXIT_SUCCESS;
}

void update_controllables(
    COMPONENT_ITERATION_HANDLE_STRUCT(velocity, controllable) *handle,
    world *w,
    system_predicate_data delta_time_seconds
) {
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
    system_predicate_data delta_time_seconds
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
                create_shockwave(w, *p, vr->velocity);
                break;
        }
    }
}

void update_ducks(
    COMPONENT_ITERATION_HANDLE_STRUCT(position) *handle,
    world *w,
    system_predicate_data delta_time_seconds
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
    system_predicate_data delta_time_seconds
) {
    position *p = handle->position_component;
    velocity *v = handle->velocity_component;
    p->x += v->x;
    p->y += v->y;
    free(handle->renderable_component->sprite);
    handle->renderable_component->sprite = NULL;
    if (p->x < 0 || p->x >= BOARD_WIDTH
        || p->y < 0 || p->y >= BOARD_HEIGHT
        || v->x == 0 && v->y == 0) {
        world_remove_entity(w, handle->entity_id);
    } else {
        renderable new_r = renderable_create_shockwave_alloc(abs(v->x) > abs(v->y) ? abs(v->x) : abs(v->y));
        *handle->renderable_component = new_r;
    }
    
    if (abs(v->x) > 0) {
        v->x -= v->x / abs(v->x);
    }
    if (abs(v->y) > 0) {
        v->y -= v->y / abs(v->y);
    }
}

void update_children_position(
    const COMPONENT_ITERATION_HANDLE_STRUCT(position, renderable, is_child_of) *handle,
    world *w,
    system_predicate_data delta_time_seconds
) {
    if (handle->is_child_of_component) {
        //*handle = (is_child_of){0};
    }
    // position p = *handle->position_component;
    // position parent_position = *WORLD_GET_COMPONENT(position, w, handle->is_child_of_component->parent);
    // handle->renderable_component->offset = (v2_i16){ parent_position.x - p.x, parent_position.y - p.y };
}

bool update_entities(world *w, arena *iteration_arena, double delta_time_seconds) {
    scene_world_system s = scene_world_system_create(0, iteration_arena);
    WORLD_SYSTEM_ITER_RANGE(
        scene_world_system_get_with(&s, iteration_arena, COMPONENTS_ALL(velocity, controllable)),
        w,
        iteration_arena,
        ((entity_id_range){0, 2}),
        system_predicate_data_create_none(),
        update_controllables
    );
    WORLD_SYSTEM_ITER(
        WORLD_SYSTEM_CREATE(position, velocity, renderable, velocity_register, controllable),
        w,
        iteration_arena,
        system_predicate_data_create_none(),
        update_lonk
    );
    WORLD_SYSTEM_ITER(
        WORLD_SYSTEM_CREATE(position, velocity, renderable, is_shockwave),
        w,
        iteration_arena,
        system_predicate_data_create_none(),
        update_shockwaves
    );
    entity_count duck_count = WORLD_SYSTEM_ITER(
        WORLD_SYSTEM_CREATE(position, is_duck),
        w,
        iteration_arena,
        system_predicate_data_create_none(),
        update_ducks
    );
    //  entity_id lonk;
    // WORLD_GET_ENTITY_WITH(w, &lonk, COMPONENTS_ALL(position, velocity, renderable, velocity_register, controllable));
    
    WORLD_SYSTEM_ITER(
        WORLD_SYSTEM_CREATE_GROUPED(COMPONENTS_ALL(position, renderable), COMPONENTS_OR_ALL(is_child_of)),
        w,
        iteration_arena,
        system_predicate_data_create_none(),
        update_children_position
    );
    return EXIT_SUCCESS;
}


void update_console_buffer(
    COMPONENT_ITERATION_HANDLE_STRUCT(position, renderable) *handle,
    world *w,
    system_predicate_data buffer_user_data
) {
    console_buffer *new_console_buffer = system_predicate_data_user_data(buffer_user_data);
    position p = *handle->position_component;
    renderable r = *handle->renderable_component;
    for (int16_t x = 0; x < r.size.x; x++) {
        for (int16_t y = 0; y < r.size.y; y++) {
            int16_t cx = p.x + x + r.offset.x;
            int16_t cy = p.y + y + r.offset.y;
            if (cx >= 0 && cx < BOARD_WIDTH && cy >= 0 && cy < BOARD_HEIGHT) {
                new_console_buffer->buffer[cx][cy] = r.sprite[x + r.size.x * y];
            }
        }
    }
}

bool render(const world *w, arena *iteration_arena) {
    console_buffer *cb = WORLD_GET_RESOURCE(console_buffer, w);
    console_buffer new_console_buffer = *cb;

    for (uint16_t x = 0; x < BOARD_WIDTH; x++) {
        for (uint16_t y = 0; y < BOARD_HEIGHT; y++) {
            new_console_buffer.buffer[x][y] = " ";
        }
    }
    //scene_world_system s = scene_world_system_create(0, iteration_arena);
    WORLD_SYSTEM_ITER(
        WORLD_SYSTEM_CREATE(position, renderable),
        (world *)w,
        iteration_arena,
        system_predicate_data_create_user_data(&new_console_buffer),
        update_console_buffer
    );
    
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
    //arena_dbg_info dbg = arena_get_dbg_info_compare_size(&w->entities.entity_ids_arena);
    //printf(
    //    "arena (%d owned / %d blocks): %d / %d\n"
    //    "\tminimums: %d / %d\n"
    //    "\tmaximums: %d / %d\n"
    //    "\tlargest unused: %d\n",
    //    dbg.owned_block_count, dbg.block_count, dbg.total_size, dbg.total_capacity,
    //    dbg.smallest_block_size, dbg.smallest_block_capacity,
    //    dbg.largest_block_size, dbg.largest_block_capacity,
    //    dbg.largest_remaining_capacity
    //);

    *cb = new_console_buffer;
    return EXIT_SUCCESS;
}


void write_controllables(
    COMPONENT_ITERATION_HANDLE_STRUCT(controllable) *handle,
    world *w,
    system_predicate_data controllable_user_data
) {
    controllable *c = system_predicate_data_user_data(controllable_user_data);
    if (handle->controllable_component->active) {
        *handle->controllable_component = *c;
    }
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

    WORLD_SYSTEM_ITER(
        WORLD_SYSTEM_CREATE(controllable),
        w,
        iteration_arena,
        system_predicate_data_create_user_data(&c),
        write_controllables
    );

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
    arena a = arena_create();
    COMPONENT_ITERATION_HANDLE_STRUCT(renderable) handle;
    //size_t count = 0;
    for (
        component_iterator it = component_iterator_create(component_iterator_descriptor_create(
            &w->components,
            &a,
            COMPONENTS_SEARCH_GROUPS_CREATE(
                COMPONENTS_ALL(renderable, owns_renderable)
            )
        ));
        !component_iterator_done(&it);
        component_iterator_next(&it)
    ) {
        component_iterator_current(&it, &handle);
        free(handle.renderable_component->sprite);
        //++count;
    }
    //printf("freed %d renderables\n", count);
    arena_free(&a);
    return EXIT_SUCCESS;
}

int main(void) {
    // TODO: maybe prefix everything with "cecs_"
    world w = world_create(1024, 32, 4);

    bool quitting = false;
    bool app_error = false;
    if (init(&w)) {
        app_error = true;
    }

    game_time* t = WORLD_GET_RESOURCE(game_time, &w);
    timespec_get(&t->frame_start, TIME_UTC);
    const DWORD sleep_milliseconds = (DWORD)(1000.0 / TARGET_FPS);
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
