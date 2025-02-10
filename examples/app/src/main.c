#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <windows.h>
#include <time.h>
#include <conio.h>
#include <cecs_core/cecs_core.h>
#include "color.h"


#define TARGET_FPS 240.0

#define BOARD_WIDTH 48
#define BOARD_HEIGHT 16

typedef struct v2_i16 {
    int16_t x;
    int16_t y;
} v2_i16;

typedef v2_i16 position;
CECS_COMPONENT_DECLARE(position);
CECS_COMPONENT_DEFINE(position);

typedef v2_i16 velocity;
CECS_COMPONENT_DECLARE(velocity);
CECS_COMPONENT_DEFINE(velocity);

typedef struct velocity_register {
    v2_i16 velocity;
} velocity_register;
CECS_COMPONENT_DECLARE(velocity_register);
CECS_COMPONENT_DEFINE(velocity_register);

typedef struct box {
    position corner_position;
    v2_i16 size;
} box;
CECS_COMPONENT_DECLARE(box);
CECS_COMPONENT_DEFINE(box);

typedef bool is_solid;
CECS_TAG_DECLARE(is_solid);
CECS_TAG_DEFINE(is_solid);

typedef bool is_duck;
CECS_TAG_DECLARE(is_duck);
CECS_TAG_DEFINE(is_duck);

typedef bool is_shockwave;
CECS_TAG_DECLARE(is_shockwave);
CECS_TAG_DEFINE(is_shockwave);

typedef struct controllable {
    #define CONTROLLABLE_BUFFER_SIZE 32
    bool active;
    uint8_t buffer_count;
    char buffer[CONTROLLABLE_BUFFER_SIZE];
} controllable;
CECS_COMPONENT_DECLARE(controllable);
CECS_COMPONENT_DEFINE(controllable);

typedef struct renderable {
    v2_i16 offset;
    v2_i16 size;
    char **sprite;
} renderable;
CECS_COMPONENT_DECLARE(renderable);
CECS_COMPONENT_DEFINE(renderable);

typedef bool owns_renderable;
CECS_TAG_DECLARE(owns_renderable);
CECS_TAG_DEFINE(owns_renderable);

typedef struct dies_by {
    cecs_entity_id cause;
} dies_by;
CECS_COMPONENT_DECLARE(dies_by);
CECS_COMPONENT_DEFINE(dies_by);

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
CECS_RESOURCE_DECLARE(console_buffer);
CECS_RESOURCE_DEFINE(console_buffer);

cecs_entity_id create_duck(cecs_world *w, v2_i16 initial_position, const cecs_entity_id *const threats, size_t threats_count) {
    cecs_entity_id e = cecs_world_add_entity(w);
    CECS_WORLD_SET_COMPONENT(
        position,
        w,
        e,
        &initial_position
    );

    char *default_color = WHT "\xe2\x96\x84" CRESET;
    char *golden_color = YEL "\xe2\x96\x84" CRESET;
    renderable r = renderable_create_duck_alloc(rand() % 1000 < 45 ? golden_color : default_color);
    CECS_WORLD_SET_COMPONENT(
        renderable,
        w,
        e,
        &r
    );
    CECS_WORLD_ADD_TAG(owns_renderable, w, e);
    CECS_WORLD_ADD_TAG(is_duck, w, e);
    for (size_t i = 0; i < threats_count; i++) {
        CECS_WORLD_SET_COMPONENT_RELATION(dies_by, w, e, &(dies_by){ threats[i] }, threats[i]);
    }
    return e;
}

bool create_slime(cecs_world *w, v2_i16 initial_position, cecs_entity_id parent) {
    cecs_entity_id e = cecs_world_add_entity(w);
    CECS_WORLD_SET_COMPONENT(
        position,
        w,
        e,
        &initial_position
    );
    renderable r = renderable_create_slime_alloc();
    CECS_WORLD_SET_COMPONENT(
        renderable,
        w,
        e,
        &r
    );
    CECS_WORLD_ADD_TAG(owns_renderable, w, e);
    CECS_WORLD_SET_COMPONENT_RELATION(cecs_is_child_of, w, e, &(cecs_is_child_of){parent}, parent);
    return EXIT_SUCCESS;
}

cecs_entity_id create_lonk_prefab(cecs_world *w) {
    cecs_entity_id e = cecs_world_add_entity(w);
    CECS_WORLD_SET_COMPONENT(
        position,
        w,
        e,
        &((position) {
            BOARD_WIDTH / 2, BOARD_HEIGHT / 2
        })
    );
    CECS_WORLD_SET_COMPONENT(
        velocity,
        w,
        e,
        &((velocity) {
            0, 0
        })
    );
    CECS_WORLD_SET_COMPONENT(
        velocity_register,
        w,
        e,
        &((velocity_register) {
            .velocity = {0, 0}
        })
    );
    renderable r = renderable_create_lonk_alloc();
    CECS_WORLD_SET_COMPONENT(
        renderable,
        w,
        e,
        &r
    );
    CECS_WORLD_ADD_TAG(owns_renderable, w, e);
    CECS_WORLD_SET_COMPONENT(
        controllable,
        w,
        e,
        &((controllable) {
            .active = true,
            .buffer_count = 0,
            .buffer = {0}
        })
    );
    cecs_world_get_or_set_default_entity_flags(w, e)->is_permanent = true;
    return cecs_world_set_prefab(w, e);
}

cecs_entity_id create_lonk(cecs_world *w, cecs_prefab_id prefab) {
    cecs_entity_id lonk = cecs_world_add_entity_from_prefab(w, prefab);
    CECS_WORLD_REMOVE_TAG(owns_renderable, w, lonk);
    return lonk;
}

cecs_prefab_id create_wall_prefab(cecs_world *w) {
    cecs_entity_id e = cecs_world_add_entity(w);
    CECS_WORLD_SET_COMPONENT(
        position,
        w,
        e,
        &((position) {
            0, 0
        })
    );
    renderable r = renderable_create_wall_alloc(BLU "\xe2\x96\x88" CRESET);
    CECS_WORLD_SET_COMPONENT(
        renderable,
        w,
        e,
        &r
    );
    CECS_WORLD_ADD_TAG(owns_renderable, w, e);
    CECS_WORLD_ADD_TAG(is_solid, w, e);
    return cecs_world_set_prefab(w, e);
}

bool create_map(cecs_world *w, cecs_prefab_id prefab) {
    for (uint16_t x = 0; x < BOARD_WIDTH; x++) {
        for (uint16_t y = 0; y < BOARD_HEIGHT; y++) {
            if (x == 0 || x == BOARD_WIDTH - 1 || y == 0 || y == BOARD_HEIGHT - 1) {
                cecs_entity_id wall = cecs_world_add_entity_from_prefab(w, prefab);
                CECS_WORLD_SET_COMPONENT(position, w, wall, (&(position){x, y}));
                CECS_WORLD_REMOVE_TAG(owns_renderable, w, wall);
            }
        }
    } 
    return EXIT_SUCCESS;
}

bool init(cecs_world *w) {
    cecs_game_time *t = CECS_WORLD_SET_RESOURCE(
        cecs_game_time,
        w,
        &((cecs_game_time) {
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
    cecs_entity_id lonk = create_lonk(w, create_lonk_prefab(w));
    cecs_world_add_entity_to_scene(w, lonk, 0);
    // entity_id lonk2 = cecs_world_add_entity(w);
    // cecs_world_add_entity_to_scene(w, lonk2, 1);
    // CECS_WORLD_COPY_ENTITY_ONTO_AND_GRAB(controllable, w, lonk2, lonk);//->active = false;

    create_map(w, create_wall_prefab(w));
    cecs_entity_id duck = create_duck(w, (v2_i16){BOARD_WIDTH / 2, BOARD_HEIGHT / 2 - 2}, (cecs_entity_id[]){lonk}, 1);
    cecs_world_copy_entity_range_onto(
        w,
        cecs_world_add_entity_range(w, 10 - 1),
        cecs_exclusive_range_singleton(duck),
        duck
    );

    // 2 test lonks
    cecs_entity_id_range lonk_range = cecs_world_add_entity_range(w, 2);
    size_t copied_component_types =
        cecs_world_copy_entity_range_onto(w, lonk_range, cecs_exclusive_range_singleton(lonk), lonk);
    CECS_WORLD_SET_COMPONENT(position, w, lonk_range.start, &((position){BOARD_WIDTH / 2, BOARD_HEIGHT / 2 + 2}));
    CECS_WORLD_SET_COMPONENT(position, w, lonk_range.start + 1, &((position){BOARD_WIDTH / 2 + 2, BOARD_HEIGHT / 2 + 4}));

    // for (size_t i = 0; i < 2; i++) {
    //     create_duck(w, (v2_i16){BOARD_WIDTH / 2, BOARD_HEIGHT / 2 - 2}, (cecs_entity_id[]){lonk}, 1);
    // }
    create_slime(w, (v2_i16){BOARD_WIDTH / 4, BOARD_HEIGHT / 4 + 2}, lonk);
    create_slime(w, (v2_i16){BOARD_WIDTH / 4 * 3, BOARD_HEIGHT / 4 + 2}, cecs_world_add_entity(w));

    CECS_WORLD_SET_RESOURCE(console_buffer, w, &cb);
    return EXIT_SUCCESS;
}

bool create_shockwave(cecs_world *w, position p, velocity v) {
    cecs_entity_id e = cecs_world_add_entity(w);

    CECS_WORLD_SET_COMPONENT(
        position,
        w,
        e,
        &p
    );
    const int16_t SPEED_MULTIPLIER = 4;
    const int16_t RADIUS = SPEED_MULTIPLIER;
    renderable r = renderable_create_shockwave_alloc(RADIUS);
    CECS_WORLD_SET_COMPONENT(
        renderable,
        w,
        e,
        &r
    );
    CECS_WORLD_ADD_TAG(owns_renderable, w, e);
    CECS_WORLD_SET_COMPONENT(
        velocity,
        w,
        e,
        &((velocity) {
            v.x * SPEED_MULTIPLIER, v.y * SPEED_MULTIPLIER
        })
    );
    CECS_WORLD_ADD_TAG(is_shockwave, w, e);
    return EXIT_SUCCESS;
}

void update_controllables(
    CECS_COMPONENT_ITERATION_HANDLE_STRUCT(velocity, controllable) *handle,
    const cecs_entity_id entity,
    cecs_world *w,
    const cecs_system_predicate_data delta_time_seconds
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
    CECS_COMPONENT_ITERATION_HANDLE_STRUCT(position, velocity, renderable, velocity_register, controllable) *handle,
    const cecs_entity_id entity,
    cecs_world *w,
    cecs_system_predicate_data delta_time_seconds
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
    CECS_COMPONENT_ITERATION_HANDLE_STRUCT(position) *handle,
    const cecs_entity_id entity,
    cecs_world *w,
    cecs_system_predicate_data delta_time_seconds
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
    CECS_COMPONENT_ITERATION_HANDLE_STRUCT(position, velocity, renderable) *handle,
    const cecs_entity_id entity,
    cecs_world *w,
    cecs_system_predicate_data delta_time_seconds
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
        cecs_world_remove_entity(w, entity);
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
    const CECS_COMPONENT_ITERATION_HANDLE_STRUCT(position, renderable, cecs_is_child_of) *handle,
    const cecs_entity_id entity,
    cecs_world *w,
    cecs_system_predicate_data delta_time_seconds
) {
    if (handle->cecs_is_child_of_component) {
        //*handle = (cecs_is_child_of){0};
    }
    // position p = *handle->position_component;
    // position parent_position = *CECS_WORLD_GET_COMPONENT(position, w, handle->cecs_is_child_of_component->parent);
    // handle->renderable_component->offset = (v2_i16){ parent_position.x - p.x, parent_position.y - p.y };
}

bool update_entities(cecs_world *w, cecs_arena *iteration_arena, double delta_time_seconds) {
    void *component_handles[8] = {0};
    cecs_scene_world_system s = cecs_scene_world_system_create(0, iteration_arena);
    cecs_dynamic_world_system_set_or_extend_range(&s.world_system, iteration_arena, 1, (cecs_component_iteration_group[]){
        CECS_COMPONENT_GROUP(cecs_component_access_mutable, cecs_component_group_search_all, velocity),
        CECS_COMPONENT_GROUP(cecs_component_access_inmmutable, cecs_component_group_search_all, controllable)
    }, 2);
    
    CECS_WORLD_SYSTEM_ITER(
        cecs_world_system_from_dynamic_range(&s.world_system, (cecs_component_iteration_group_range){1, 3}),
        w,
        iteration_arena,
        component_handles,
        cecs_system_predicate_data_create_none(),
        update_controllables
    );
    CECS_WORLD_SYSTEM_ITER(
        CECS_WORLD_SYSTEM_CREATE(cecs_component_access_mutable, cecs_component_group_search_all, position, velocity, renderable, velocity_register, controllable),
        w,
        iteration_arena,
        component_handles,
        cecs_system_predicate_data_create_none(),
        update_lonk
    );
    CECS_WORLD_SYSTEM_ITER(
        CECS_WORLD_SYSTEM_CREATE(cecs_component_access_mutable, cecs_component_group_search_all, position, velocity, renderable),
        w,
        iteration_arena,
        component_handles,
        cecs_system_predicate_data_create_none(),
        update_shockwaves
    );
    const cecs_entity_count duck_count = CECS_WORLD_SYSTEM_ITER(
        CECS_WORLD_SYSTEM_CREATE(cecs_component_access_mutable, cecs_component_group_search_all, position, is_duck),
        w,
        iteration_arena,
        component_handles,
        cecs_system_predicate_data_create_none(),
        update_ducks
    );
    (void)duck_count;
    
    CECS_WORLD_SYSTEM_ITER(
        CECS_WORLD_SYSTEM_CREATE_GROUPED(
            CECS_COMPONENT_GROUP(cecs_component_access_mutable, cecs_component_group_search_all, position, renderable),
            CECS_COMPONENT_GROUP(cecs_component_access_inmmutable, cecs_component_group_search_or_all, cecs_is_child_of)),
        w,
        iteration_arena,
        component_handles,
        cecs_system_predicate_data_create_none(),
        update_children_position
    );
    return EXIT_SUCCESS;
}


void update_console_buffer(
    CECS_COMPONENT_ITERATION_HANDLE_STRUCT(position, renderable) *handle,
    const cecs_entity_id entity,
    cecs_world *w,
    cecs_system_predicate_data buffer_user_data
) {
    console_buffer *new_console_buffer = cecs_system_predicate_data_user_data(buffer_user_data);
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

bool render(const cecs_world *w, cecs_arena *iteration_arena) {
    console_buffer *cb = CECS_WORLD_GET_RESOURCE(console_buffer, w);
    console_buffer new_console_buffer = *cb;

    for (uint16_t x = 0; x < BOARD_WIDTH; x++) {
        for (uint16_t y = 0; y < BOARD_HEIGHT; y++) {
            new_console_buffer.buffer[x][y] = " ";
        }
    }
    
    void *component_handles[2] = {0};
    CECS_WORLD_SYSTEM_ITER(
        CECS_WORLD_SYSTEM_CREATE(cecs_component_access_inmmutable, cecs_component_group_search_all, position, renderable),
        (cecs_world *)w,
        iteration_arena,
        component_handles,
        cecs_system_predicate_data_create_user_data(&new_console_buffer),
        update_console_buffer
    );
    
    printf("\x1b[%d;%dH\x1b[J", 0, 0);
    fflush(stdout);
    //system("cls");
    cecs_arena screen_arena = cecs_arena_create();
    cecs_dynamic_array screen = cecs_dynamic_array_create_with_capacity(&screen_arena, sizeof(char) * BOARD_WIDTH * BOARD_HEIGHT);
    for (uint16_t y = 0; y < BOARD_HEIGHT; y++) {
        for (uint16_t x = 0; x < BOARD_WIDTH; x++) {
            size_t i = 0;
            while (new_console_buffer.buffer[x][y][i] != '\0')
                cecs_dynamic_array_add(&screen, &screen_arena, &new_console_buffer.buffer[x][y][i++], sizeof(char));        
        }
        cecs_dynamic_array_add(&screen, &screen_arena, "\n", sizeof(char));
    }
    printf("%s", (char *)screen.values);
    cecs_arena_free(&screen_arena);
    printf("fps: %f\n", 1.0 / CECS_WORLD_GET_RESOURCE(cecs_game_time, w)->averaged_delta_time_seconds);
    // cecs_arena_dbg_info dbg = cecs_arena_get_dbg_info_compare_size(&w->components.components_arena);
    // printf(
    //    "arena (%d owned / %d blocks): %d / %d\n"
    //    "\tminimums: %d / %d\n"
    //    "\tmaximums: %d / %d\n"
    //    "\tlargest unused: %d\n",
    //    dbg.owned_block_count, dbg.block_count, dbg.total_size, dbg.total_capacity,
    //    dbg.smallest_block_size, dbg.smallest_block_capacity,
    //    dbg.largest_block_size, dbg.largest_block_capacity,
    //    dbg.largest_remaining_capacity
    // );

    *cb = new_console_buffer;
    return EXIT_SUCCESS;
}


void write_controllables(
    CECS_COMPONENT_ITERATION_HANDLE_STRUCT(controllable) *handle,
    const cecs_entity_id entity,
    cecs_world *w,
    cecs_system_predicate_data controllable_user_data
) {
    controllable *c = cecs_system_predicate_data_user_data(controllable_user_data);
    if (handle->controllable_component->active) {
        *handle->controllable_component = *c;
    }
}

bool process_input(cecs_world *w, cecs_arena *iteration_arena) {
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

    void *component_handles[1] = {0};
    CECS_WORLD_SYSTEM_ITER(
        CECS_WORLD_SYSTEM_CREATE(cecs_component_access_mutable, cecs_component_group_search_all, controllable),
        w,
        iteration_arena,
        component_handles,
        cecs_system_predicate_data_create_user_data(&c),
        write_controllables
    );

    return EXIT_SUCCESS;
}

bool update(cecs_world *w, double delta_time_seconds) {
    cecs_arena iteration_arena = cecs_arena_create();
    bool result = update_entities(w, &iteration_arena, delta_time_seconds)
        || render(w, &iteration_arena)
        || process_input(w, &iteration_arena);
    cecs_arena_free(&iteration_arena);
    return result;
}

bool finalize(cecs_world *w) {
    cecs_arena a = cecs_arena_create();
    CECS_COMPONENT_ITERATION_HANDLE_STRUCT(renderable) handle;
    //size_t count = 0;
    // BUG: we kaboom here
    // for (
    //     cecs_component_iterator it = cecs_component_iterator_create(cecs_component_iterator_descriptor_create(
    //         &w->components,
    //         &a,
    //         CECS_COMPONENTS_SEARCH_GROUPS_CREATE(
    //             CECS_COMPONENTS_ALL(renderable, owns_renderable)
    //         )
    //     ));
    //     !cecs_component_iterator_done(&it);
    //     cecs_component_iterator_next(&it)
    // ) {
    //     cecs_component_iterator_current(&it, &handle);
    //     free(handle.renderable_component->sprite);
    //     //++count;
    // }
    //printf("freed %d renderables\n", count);
    cecs_arena_free(&a);
    return EXIT_SUCCESS;
}

#include "cecs_core/containers/cecs_flatmap.h"

int main(void) {
    cecs_world w = cecs_world_create(1024, 32, 4);

    bool quitting = false;
    bool app_error = false;
    if (init(&w)) {
        app_error = true;
    }

    cecs_game_time* t = CECS_WORLD_GET_RESOURCE(cecs_game_time, &w);
    timespec_get(&t->frame_start, TIME_UTC);
    const DWORD sleep_milliseconds = (DWORD)(1000.0 / TARGET_FPS);
    Sleep(sleep_milliseconds);
    while (!quitting && !app_error)
    {
        timespec_get(&t->frame_end, TIME_UTC);
        cecs_game_time_update_time_since_start(t);

        if (update(&w, cecs_game_time_update_delta_time(t))) {
            app_error = true;
        }

        timespec_get(&t->frame_start, TIME_UTC);
        Sleep(sleep_milliseconds);
    }

    if (finalize(&w)) {
        app_error = true;
    }

    cecs_world_free(&w);
    return app_error ? EXIT_FAILURE : EXIT_SUCCESS;
}
