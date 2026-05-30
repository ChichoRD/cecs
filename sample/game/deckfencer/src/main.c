#include <cecs_world.h>
#include <cecs_flatmap.h>
#include <algebra/linear/cecs_vector.h>
#include <relations/cecs_ordering.h>
#include <dckf_input.h>
#include <time.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <cecs_query.h>
#include <cecs_iter.h>

#define RENDER_GAME_WIDTH 18ull
#define RENDER_GAME_HEIGHT 12ull
static size_t render_game_width = RENDER_GAME_WIDTH;
static size_t render_game_height = RENDER_GAME_HEIGHT;
static size_t render_game_stride = RENDER_GAME_WIDTH + 1ull;


static cecs_component_type component_position;
static cecs_component_type component_velocity;
static cecs_component_type component_inputbuffer;
static cecs_component_type component_drawable;

static cecs_component_type component_block;
static cecs_component_type component_block_turret;
static cecs_component_type component_block_turret_descriptor;

static cecs_component_type component_live;
static cecs_component_type component_hurtbox;

static cecs_component_type component_hand;


typedef cecs_vec2_f32 dckf_position2_f32;
typedef cecs_vec2_f32 dckf_velocity2_f32;

#define DCKF_INPUTBUFFER_NEXT_MAX UINT8_MAX
typedef struct dckf_inputbuffer {
    char keys[6];
    // read <= write
    uint8_t key_next_read;
    uint8_t key_next_write;
} dckf_inputbuffer;
typedef struct dckf_drawable {
    char sprite[15];
    uint8_t sprite_length;
} dckf_drawable;
// FIXME: optimize for zero sized components (ZST)
typedef struct dckf_block {
    unsigned char unused;
} dckf_block;
typedef struct dckf_block_turret {
    struct timespec last_shot_timestamp;
    struct timespec shot_cooldown;
} dckf_block_turret;
typedef struct dckf_block_turret_descriptor {
    struct timespec shot_cooldown_min;
    struct timespec shot_cooldown_max;
    uint8_t damage;
    uint8_t range;
} dckf_block_turret_descriptor;
typedef struct dckf_live {
    uint8_t hitpoints;
} dckf_live;
typedef struct dckf_hurtbox {
    uint8_t damage;
} dckf_hurtbox;
typedef enum dckf_hand_state { 
    dckf_hand_state_none = 0,

    dckf_hand_state_above = (1 << 0),
    dckf_hand_state_below_left = (1 << 1),
    dckf_hand_state_below_center = (1 << 2),
    dckf_hand_state_below_right = (1 << 3),
    
    dckf_hand_state_action = (1 << 4)
} dckf_hand_state;
typedef uint8_t dckf_hand_state_value;
static const dckf_hand_state_value dckf_hand_state_mask_above = dckf_hand_state_above;
static const dckf_hand_state_value dckf_hand_state_mask_below =
    dckf_hand_state_below_left | dckf_hand_state_below_center | dckf_hand_state_below_right;
static const dckf_hand_state_value dckf_hand_state_mask_position = dckf_hand_state_mask_above | dckf_hand_state_mask_below;
static const dckf_hand_state_value dckf_hand_state_mask_action =
    dckf_hand_state_action;
typedef struct dckf_hand {
    dckf_hand_state_value state;
} dckf_hand;


void dckf_init(cecs_world *const world, cecs_allocator *const allocator, void *const context);
void dckf_deinit(cecs_world *const world, cecs_allocator *const allocator, void *const context);

void dckf_prerender(cecs_world *const world, cecs_allocator *const allocator, void *const context);
void dckf_render(const cecs_world *const world, cecs_allocator *const allocator, void *const context);
void dckf_input(cecs_world *const world, cecs_allocator *const allocator, void *const context);
void dckf_update(cecs_world *const world, cecs_allocator *const allocator, void *const context);

int main(void) {
    cecs_allocator alloc = cecs_allocator_create_bump_virtual(16);
    // cecs_world w = {0};
    cecs_world w = cecs_world_create_with(&alloc, 64, 16);
    cecs_flatmap static_entity_from_position = cecs_flatmap_create_with_capacity(&alloc, 4, sizeof(cecs_entity));
    dckf_init(&w, &alloc, &static_entity_from_position);


    bool running = true;
    const struct timespec target_frame_time = {
        .tv_sec = 0,
        .tv_nsec = 16 * 1000 * 1000, // 16ms
    };
    cecs_allocator render_alloc = cecs_allocator_alloc_bump_view(&alloc, render_game_stride * render_game_height * sizeof(char));
    (void)render_alloc;

    while (running) {
        dckf_prerender(&w, &alloc, NULL);
        dckf_render(&w, &alloc, cecs_allocator_bump_mut(&render_alloc));

        struct timespec remaining_frame_time = target_frame_time;
        while (nanosleep(&remaining_frame_time, &remaining_frame_time) != 0) { }
        nanosleep(&target_frame_time, NULL);

        dckf_input(&w, &alloc, &running);
        dckf_update(&w, &alloc, &static_entity_from_position);
    }


    dckf_deinit(&w, &alloc, NULL);
    cecs_world_destroy(&w, &alloc);
    return 0;
}

static void dckf_init_component_types(cecs_world *const world, cecs_allocator *const allocator) {
    component_position =        cecs_world_register_component(world, allocator, cecs_component_storage_type_sparse_set, sizeof(dckf_position2_f32),             64);
    component_velocity =        cecs_world_register_component(world, allocator, cecs_component_storage_type_sparse_set, sizeof(dckf_velocity2_f32),             16);
    component_inputbuffer =     cecs_world_register_component(world, allocator, cecs_component_storage_type_sparse_set, sizeof(dckf_inputbuffer),               1);
    component_drawable =        cecs_world_register_component(world, allocator, cecs_component_storage_type_sparse_set, sizeof(dckf_drawable),                  64);
    component_block =           cecs_world_register_component(world, allocator, cecs_component_storage_type_sparse_set, sizeof(dckf_block),                     32);
    component_block_turret =    cecs_world_register_component(world, allocator, cecs_component_storage_type_sparse_set, sizeof(dckf_block_turret),              8);
    component_block_turret_descriptor =
                                cecs_world_register_component(world, allocator, cecs_component_storage_type_sparse_set, sizeof(dckf_block_turret_descriptor),   8);
    component_live =            cecs_world_register_component(world, allocator, cecs_component_storage_type_sparse_set, sizeof(dckf_live),                      64);
    component_hurtbox =         cecs_world_register_component(world, allocator, cecs_component_storage_type_sparse_set, sizeof(dckf_hurtbox),                   32);
    component_hand =            cecs_world_register_component(world, allocator, cecs_component_storage_type_sparse_set, sizeof(dckf_hand),                      1);
}
static void dckf_init_platform_create(cecs_world *const world, cecs_allocator *const allocator, cecs_flatmap *const static_entity_from_position) {
    cecs_view_alloc position_view_alloc = cecs_world_acquire_view_alloc(
        world,
        cecs_allocator_alloc_bump_view(allocator, /*stub*/ 64 * sizeof(unsigned char)),
        component_position
    );
    cecs_view_alloc drawable_view_alloc = cecs_world_acquire_view_alloc(
        world,
        cecs_allocator_alloc_bump_view(allocator, /*stub*/ 64 * sizeof(unsigned char)),
        component_drawable
    );
    cecs_view_alloc block_view_alloc = cecs_world_acquire_view_alloc(
        world,
        cecs_allocator_alloc_bump_view(allocator, /*stub*/ 64 * sizeof(unsigned char)),
        component_block
    );
    cecs_view_alloc live_view_alloc = cecs_world_acquire_view_alloc(
        world,
        cecs_allocator_alloc_bump_view(allocator, /*stub*/ 64 * sizeof(unsigned char)),
        component_live
    );

#define DCKF_PLATFORM_BLOCK_COUNT 10 
    cecs_entity platform[DCKF_PLATFORM_BLOCK_COUNT];
    for (size_t i = 0; i < DCKF_PLATFORM_BLOCK_COUNT; i++) {
        platform[i] = cecs_world_alloc_entity(world, allocator);
    }

    // ..#####..#####..
    dckf_position2_f32 positions[DCKF_PLATFORM_BLOCK_COUNT];
    const uint8_t height = render_game_height - 3ull;
    for (size_t i = 0; i < DCKF_PLATFORM_BLOCK_COUNT >> 1ull; i++) {
        positions[i] = (dckf_position2_f32){
            .x = (float)(i + 2ull + 1ull),
            .y = (float)(height),
        };
    }
    for (size_t i = 0; i < DCKF_PLATFORM_BLOCK_COUNT >> 1ull; i++) {
        const size_t index = (DCKF_PLATFORM_BLOCK_COUNT >> 1ull) + i;
        positions[index] = (dckf_position2_f32){
            .x = (float)(index + 2ull + 2ull + 1ull),
            .y = (float)(height),
        };
    }

    for (size_t i = 0; i < DCKF_PLATFORM_BLOCK_COUNT; i++) {
        dckf_position2_f32 *const position = cecs_view_alloc_insert_expect(&position_view_alloc, &world->components, &world->entities, platform[i]);
        dckf_drawable *const drawable = cecs_view_alloc_insert_expect(&drawable_view_alloc, &world->components, &world->entities, platform[i]);
        dckf_block *const block = cecs_view_alloc_insert_expect(&block_view_alloc, &world->components, &world->entities, platform[i]);
        dckf_live *const live = cecs_view_alloc_insert_expect(&live_view_alloc, &world->components, &world->entities, platform[i]);

        *position = positions[i];
        *drawable = (dckf_drawable){
            .sprite = {'#', '\0'},
            .sprite_length = 1,
        };
        *block = (dckf_block){
            .unused = 0,
        };
        *live = (dckf_live){
            .hitpoints = 1,
        };

        cecs_entity *const static_entity = (cecs_entity *)cecs_flatmap_insert_expect(
            static_entity_from_position,
            allocator,
            (size_t)positions[i].x << 16 | (size_t)positions[i].y,
            sizeof(cecs_entity)
        );
        *static_entity = platform[i];
    }
#undef DCKF_PLATFORM_CAPACITY
    cecs_view_alloc_release(&position_view_alloc, &world->components);
    cecs_view_alloc_release(&drawable_view_alloc, &world->components);
    cecs_view_alloc_release(&block_view_alloc, &world->components);
    cecs_view_alloc_release(&live_view_alloc, &world->components);
}
static void dckf_init_player_create(cecs_world *const world, cecs_allocator *const allocator) {
    cecs_view_alloc position_view_alloc = cecs_world_acquire_view_alloc(
        world,
        cecs_allocator_alloc_bump_view(allocator, sizeof(dckf_position2_f32) * 1ull),
        component_position
    );
    cecs_view_alloc velocity_view_alloc = cecs_world_acquire_view_alloc(
        world,
        cecs_allocator_alloc_bump_view(allocator, sizeof(dckf_velocity2_f32) * 1ull),
        component_velocity
    );
    cecs_view_alloc drawable_view_alloc = cecs_world_acquire_view_alloc(
        world,
        cecs_allocator_alloc_bump_view(allocator, sizeof(dckf_drawable) * 1ull),
        component_drawable
    );
    cecs_view_alloc inputbuffer_view_alloc = cecs_world_acquire_view_alloc(
        world,
        cecs_allocator_alloc_bump_view(allocator, sizeof(dckf_inputbuffer) * 1ull + 48ull),
        component_inputbuffer
    );

    const cecs_entity player = cecs_world_alloc_entity(world, allocator);
    dckf_position2_f32 *const position = cecs_view_alloc_insert_expect(&position_view_alloc, &world->components, &world->entities, player);
    dckf_velocity2_f32 *const velocity = cecs_view_alloc_insert_expect(&velocity_view_alloc, &world->components, &world->entities, player);
    dckf_drawable *const drawable = cecs_view_alloc_insert_expect(&drawable_view_alloc, &world->components, &world->entities, player);
    dckf_inputbuffer *const inputbuffer = cecs_view_alloc_insert_expect(&inputbuffer_view_alloc, &world->components, &world->entities, player);

    *position = (dckf_position2_f32){
        .x = (float)(render_game_width - 4ull - 2ull),
        .y = (float)(render_game_height - 4ull),
    };
    *velocity = (dckf_velocity2_f32){
        .x = 0.0f,
        .y = 0.0f,
    };
    *drawable = (dckf_drawable){
        .sprite = {'@', '\0'},
        .sprite_length = 1,
    };
    *inputbuffer = (dckf_inputbuffer){
        .keys = {0},
        .key_next_read = 0,
        .key_next_write = 0,
    };

    cecs_view_alloc_release(&position_view_alloc, &world->components);
    cecs_view_alloc_release(&velocity_view_alloc, &world->components);
    cecs_view_alloc_release(&drawable_view_alloc, &world->components);
    cecs_view_alloc_release(&inputbuffer_view_alloc, &world->components);
}
static void dckf_init_hand_create(cecs_world *const world, cecs_allocator *const allocator) {
    cecs_view_alloc hand_view_alloc = cecs_world_acquire_view_alloc(
        world,
        cecs_allocator_alloc_bump_view(allocator, sizeof(dckf_hand) * 1ull + 63ull),
        component_hand
    );
    cecs_view_alloc position_view_alloc = cecs_world_acquire_view_alloc(
        world,
        cecs_allocator_alloc_bump_view(allocator, sizeof(dckf_position2_f32) * 1ull),
        component_position
    );
    cecs_view_alloc drawable_view_alloc = cecs_world_acquire_view_alloc(
        world,
        cecs_allocator_alloc_bump_view(allocator, sizeof(dckf_drawable) * 1ull),
        component_drawable
    );
    cecs_view_alloc inputbuffer_view_alloc = cecs_world_acquire_view_alloc(
        world,
        cecs_allocator_alloc_bump_view(allocator, sizeof(dckf_inputbuffer) * 1ull + 128ull),
        component_inputbuffer
    );

    const cecs_entity hand = cecs_world_alloc_entity(world, allocator);
    dckf_hand *const hand_component = cecs_view_alloc_insert_expect(&hand_view_alloc, &world->components, &world->entities, hand);
    dckf_position2_f32 *const position = cecs_view_alloc_insert_expect(&position_view_alloc, &world->components, &world->entities, hand);
    dckf_drawable *const drawable = cecs_view_alloc_insert_expect(&drawable_view_alloc, &world->components, &world->entities, hand);
    dckf_inputbuffer *const inputbuffer = cecs_view_alloc_insert_expect(&inputbuffer_view_alloc, &world->components, &world->entities, hand);

    *hand_component = (dckf_hand){
        .state = dckf_hand_state_above,
    };
    *position = (dckf_position2_f32){
        .x = 0.0f,
        .y = 0.0f,
    };
    *drawable = (dckf_drawable){
        .sprite = {'a', '\0'},
        .sprite_length = 1,
    };
    *inputbuffer = (dckf_inputbuffer){
        .keys = {0},
        .key_next_read = 0,
        .key_next_write = 0,
    };

    cecs_view_alloc_release(&hand_view_alloc, &world->components);
    cecs_view_alloc_release(&position_view_alloc, &world->components);
    cecs_view_alloc_release(&drawable_view_alloc, &world->components);
    cecs_view_alloc_release(&inputbuffer_view_alloc, &world->components);
}
void dckf_init(cecs_world *const world, cecs_allocator *const allocator, void *const context) {
    cecs_flatmap *const static_entity_from_position = (cecs_flatmap *)context;
    printf("DeckFencer - Press 'q' to quit\n");
    dckf_set_conio_terminal_mode();
    dckf_init_component_types(world, allocator);
    dckf_init_platform_create(world, allocator, static_entity_from_position);
    dckf_init_player_create(world, allocator);
    dckf_init_hand_create(world, allocator);
}
void dckf_deinit(cecs_world *const world, cecs_allocator *const allocator, void *const context) {
    (void)world;
    (void)allocator;
    (void)context;
    dckf_reset_terminal_mode();
}


void dckf_prerender(cecs_world *const world, cecs_allocator *const allocator, void *const context) {
    (void)world;
    (void)allocator;
    (void)context;
}


static void dckf_render_linefeed_column(char *const render_buffer, const size_t stride, const size_t buffer_height) {
    for (size_t y = 0; y < buffer_height - 1ull; y++) {
        render_buffer[(y + 1ull) * stride - 1ull] = '\n';
    }
}
static void dckf_render_background(char *const render_buffer, const size_t buffer_width, const size_t buffer_height, const size_t stride) {
    for (size_t y = 0; y < buffer_height; y++) {
        for (size_t x = 0; x < buffer_width; x++) {
            render_buffer[x + y * stride] = '.';
        }
    }
}
static void dckf_render_border(char *const render_buffer, const size_t buffer_width, const size_t buffer_height, const size_t stride) {
    for (size_t x = 0; x < buffer_width; x++) {
        render_buffer[x] = '-';
        render_buffer[(buffer_height - 1ull) * stride + x] = '_';
    }
    for (size_t y = 0; y < buffer_height; y++) {
        render_buffer[y * stride] = '|';
        render_buffer[y * stride + buffer_width - 1ull] = '|';
    }
}
static void dckf_render_game(const cecs_world *const world, char *const render_buffer, const size_t buffer_width, const size_t buffer_height, const size_t stride) {
    cecs_view view_drawable = cecs_world_acquire_view(world, component_drawable);
    cecs_view view_position = cecs_world_acquire_view(world, component_position);

    const cecs_sparse_set_storage *const drawable_storage = cecs_component_storage_sparse_set(&cecs_view_registry(view_drawable, &world->components)->storage);
    const cecs_sparse_set_storage *const position_storage = cecs_component_storage_sparse_set(&cecs_view_registry(view_position, &world->components)->storage);
    const size_t min_entity_count = cecs_min(
        cecs_sparse_set_value_count(&drawable_storage->set),
        cecs_sparse_set_value_count(&position_storage->set)
    );

    const cecs_sparse_set *drawable_lead = &drawable_storage->set;
    size_t drawn_count = 0;
    size_t i = 0;
    while (i < cecs_sparse_set_value_count(drawable_lead) && drawn_count < min_entity_count) {
        const cecs_dense_index drawable_index = cecs_dense_index_create_valid(i);
        const dckf_drawable *const drawable = cecs_sparse_set_get_value_by_index(drawable_lead, drawable_index, sizeof(dckf_drawable));
        const cecs_entity entity = cecs_entity_storage_get_used(
            &world->entities,
            *cecs_sparse_set_get_sparse_key_by_index(drawable_lead, drawable_index)
        );
        const size_t entity_index = cecs_entity_index_of(entity);

        if (cecs_sparse_set_storage_contains(position_storage, entity_index)) {
            const dckf_position2_f32 *const position =
                cecs_sparse_set_storage_get(position_storage, cecs_entity_index_of(entity), sizeof(dckf_position2_f32));

            const uint8_t x = (uint8_t)position->x;
            const uint8_t y = (uint8_t)position->y;
            if (x < buffer_width - 1ull && y < buffer_height - 1ull) {
                cecs_debugbreak_fail_unless(
                    drawable->sprite_length <= 1ull,
                    "currently only supports rendering sprites of length less than or equal to 1, // TODO rendering longer sprites is not yet implemented"
                );
                for (size_t s = 0; s < drawable->sprite_length; s++) {
                    render_buffer[x + y * stride] = drawable->sprite[s];
                }
            }
            ++drawn_count;
        }
        ++i;
    }
    cecs_view_release(&view_drawable, &world->components);
    cecs_view_release(&view_position, &world->components);
}
void dckf_render(const cecs_world *const world, cecs_allocator *const allocator, void *const context) {
    (void)allocator;
    cecs_bump_view_allocator *const alloc = (cecs_bump_view_allocator *)context;
    cecs_bump_view_allocator_reset(alloc);
    char *const render_buffer = cecs_bump_view_allocator_alloc_expect(alloc, render_game_stride * render_game_height * sizeof(char));    
    memset(render_buffer, '\0', render_game_stride * render_game_height * sizeof(char));


    dckf_render_linefeed_column(render_buffer, render_game_stride, render_game_height);
    dckf_render_background(render_buffer, render_game_width, render_game_height, render_game_stride);
    dckf_render_border(render_buffer, render_game_width, render_game_height, render_game_stride);
    dckf_render_game(world, render_buffer, render_game_width, render_game_height, render_game_stride);

    // clear screen and move cursor to top-left corner
    // TODO: maybe include in cecs_colorcode.h
    fputs("\x1b[0;0H\x1b[J", stdout);
    fflush(stdout);
    fwrite(render_buffer, sizeof(char), render_game_stride * render_game_height, stdout);

    // // debug input buffers
    // printf("\n\n");
    // cecs_view view_inputbuffer = cecs_world_acquire_view(world, component_inputbuffer);
    // const cecs_sparse_set_storage *const inputbuffer_storage = cecs_component_storage_sparse_set(&cecs_view_registry(view_inputbuffer, &world->components)->storage);
    // for (size_t i = 0; i < cecs_sparse_set_value_count(&inputbuffer_storage->set); i++) {
    //     const cecs_dense_index inputbuffer_index = cecs_dense_index_create_valid(i);
    //     const dckf_inputbuffer *const inputbuffer = cecs_sparse_set_get_value_by_index(&inputbuffer_storage->set, inputbuffer_index, sizeof(dckf_inputbuffer));
    //     printf("InputBuffer %zu: keys=[", i);
    //     // for (size_t k = inputbuffer->key_next_read; k < inputbuffer->key_next_write; k++) {
    //     //     printf("%c", inputbuffer->keys[k % (sizeof(inputbuffer->keys) / sizeof(char))]);
    //     // }
    //     for (size_t k = 0; k < sizeof(inputbuffer->keys) / sizeof(char); k++) {
    //         printf("%c", inputbuffer->keys[k]);
    //     }
    //     printf("]\n");
    // }
    // cecs_view_release(&view_inputbuffer, &world->components);
}


static void dckf_input_gather(cecs_world *const world, bool *const running) {
    cecs_view_mut view_inputbuffer = cecs_world_acquire_view_mut(world, component_inputbuffer);
    
    cecs_sparse_set_storage *const inputbuffer_storage = cecs_view_mut_storage_sparse_set_mut_expect(view_inputbuffer, &world->components);
    while (dckf_kbhit()) {
        const int ch = dckf_getch();
        if (ch == 'q' || ch == 'Q') {
            *running = false;
        }

        for (size_t i = 0; i < cecs_sparse_set_value_count(&inputbuffer_storage->set); i++) {
            const cecs_dense_index inputbuffer_index = cecs_dense_index_create_valid(i);
            dckf_inputbuffer *const inputbuffer = cecs_sparse_set_get_value_by_index_mut(&inputbuffer_storage->set, inputbuffer_index, sizeof(dckf_inputbuffer));
            
            const size_t end = inputbuffer->key_next_read + (sizeof(inputbuffer->keys) / sizeof(char));
            if (inputbuffer->key_next_write < end) {
                inputbuffer->keys[inputbuffer->key_next_write % (sizeof(inputbuffer->keys) / sizeof(char))] = (char)ch;
                ++inputbuffer->key_next_write;
            }
            cecs_debugbreak_fail_unless(
                inputbuffer->key_next_read <= inputbuffer->key_next_write,
                "fatal error: failed to normalize inputbuffer indices after gathering input, key_next_read must be less than or equal to key_next_write"
            );
        }
    }
    cecs_view_mut_release(&view_inputbuffer, &world->components);
}
void dckf_input(cecs_world *const world, cecs_allocator *const allocator, void *const context) {
    (void)world;
    (void)allocator;
    bool *const running = (bool *)context;
    dckf_input_gather(world, running);
}

static void dckf_update_velocity_input(cecs_world *const world) {
    cecs_view_mut view_inputbuffer = cecs_world_acquire_view_mut(world, component_inputbuffer);
    cecs_view_mut view_velocity = cecs_world_acquire_view_mut(world, component_velocity);

    cecs_sparse_set_storage *const inputbuffer_storage = cecs_view_mut_storage_sparse_set_mut_expect(view_inputbuffer, &world->components);
    cecs_sparse_set_storage *const velocity_storage = cecs_view_mut_storage_sparse_set_mut_expect(view_velocity, &world->components);

    const size_t min_entity_count = cecs_min(
        cecs_sparse_set_value_count(&inputbuffer_storage->set),
        cecs_sparse_set_value_count(&velocity_storage->set)
    );

    cecs_sparse_set *const inputbuffer_lead = &inputbuffer_storage->set;
    size_t updated_count = 0;
    size_t i = 0;
    while (i < cecs_sparse_set_value_count(inputbuffer_lead) && updated_count < min_entity_count) {
        const cecs_dense_index inputbuffer_index = cecs_dense_index_create_valid(i);
        dckf_inputbuffer *const inputbuffer = cecs_sparse_set_get_value_by_index_mut(inputbuffer_lead, inputbuffer_index, sizeof(dckf_inputbuffer));
        const cecs_entity entity = cecs_entity_storage_get_used(
            &world->entities,
            *cecs_sparse_set_get_sparse_key_by_index(inputbuffer_lead, inputbuffer_index)
        );
        const size_t entity_index = cecs_entity_index_of(entity);

        if (cecs_sparse_set_storage_contains(velocity_storage, entity_index)) {
            dckf_velocity2_f32 *const velocity =
                cecs_sparse_set_storage_get_mut(velocity_storage, entity_index, sizeof(dckf_velocity2_f32));
            
            while (inputbuffer->key_next_read < inputbuffer->key_next_write) {
                const char key = inputbuffer->keys[inputbuffer->key_next_read % (sizeof(inputbuffer->keys) / sizeof(char))];
                switch (key) {
                    case 'w':
                    case 'W':
                        velocity->y = -1.0f;
                        break;
                    case 'a':
                    case 'A':
                        velocity->x = -1.0f;
                        break;
                    case 's':
                    case 'S':
                        velocity->y = 1.0f;
                        break;
                    case 'd':
                    case 'D':
                        velocity->x = 1.0f;
                        break;
                }
                ++inputbuffer->key_next_read;
            }
            inputbuffer->key_next_read %= (sizeof(inputbuffer->keys) / sizeof(char));
            inputbuffer->key_next_write %= (sizeof(inputbuffer->keys) / sizeof(char));
            cecs_debugbreak_fail_unless(
                inputbuffer->key_next_read <= inputbuffer->key_next_write,
                "fatal error: failed to normalize inputbuffer indices after processing input, key_next_read must be less than or equal to key_next_write"
            );
            ++updated_count;
        }
        ++i;
    }

    cecs_view_mut_release(&view_inputbuffer, &world->components);
    cecs_view_mut_release(&view_velocity, &world->components);
}
static void dckf_update_position_velocity(cecs_world *const world, const cecs_flatmap *const static_entity_from_position) {
    cecs_view_mut view_position = cecs_world_acquire_view_mut(world, component_position);
    cecs_view_mut view_velocity = cecs_world_acquire_view_mut(world, component_velocity);
    cecs_view view_inputbuffer = cecs_world_acquire_view(world, component_inputbuffer);


    cecs_query_registry registries[3];
    // const cecs_query query = {
    //     .terms = (cecs_query_term[]){
    //         { .views = (cecs_query_views){ view_position.view, view_velocity.view }, .view_count = 2, .access = cecs_query_access_mut, .match = cecs_query_match_type_all },
    //         { .views = (cecs_query_views){ view_inputbuffer.view }, .view_count = 1, .access = cecs_query_access_shared, .match = cecs_query_match_type_all }
    //     },
    //     .term_count = 2,
    // };
    const cecs_query query = cecs_query_create(
        (cecs_query_term[]){
            cecs_query_term_create((cecs_query_views){ view_position.view, view_velocity.view }, 2, cecs_query_access_mut, cecs_query_match_type_all),
            cecs_query_term_create((cecs_query_views){ view_inputbuffer.view }, 1, cecs_query_access_shared, cecs_query_match_type_all)
        },
        2
    );
    const size_t registry_count = cecs_query_find_storages_mut(query, &world->components, registries, 3);
    cecs_debugbreak_fail_unless(
        registry_count == sizeof(registries) / sizeof(registries[0]),
        "failed to find storages for position, velocity, and inputbuffer components for update_position_velocity system"
    );
    const size_t min_entity_count = cecs_query_count_min_mut(query, registries, registry_count);

    cecs_sparse_set_storage *const position_lead = cecs_view_mut_storage_sparse_set_mut_expect(view_position, &world->components);
    cecs_sparse_set_storage *const velocity_storage = cecs_view_mut_storage_sparse_set_mut_expect(view_velocity, &world->components);
    const cecs_sparse_set_storage *const inputbuffer_storage = cecs_view_storage_sparse_set_expect(view_inputbuffer, &world->components);
    size_t updated_count = 0;
    size_t i = 0;
    while (i < cecs_sparse_set_storage_count(position_lead) && updated_count < min_entity_count) {
        const cecs_dense_index position_index = cecs_dense_index_create_valid(i);
        dckf_position2_f32 *const position = cecs_sparse_set_get_value_by_index_mut(&position_lead->set, position_index, sizeof(dckf_position2_f32));
        const cecs_entity_index entity = cecs_entity_index_from_storage_sparse_set(position_lead, &world->entities, position_index);
        
        if (cecs_sparse_set_storage_contains(velocity_storage, entity)) {
            dckf_velocity2_f32 *const velocity =
                cecs_sparse_set_storage_get_mut(velocity_storage, entity, sizeof(dckf_velocity2_f32));
            
            const dckf_position2_f32 previous_position = *position;
            position->x += velocity->x;
            position->y += velocity->y;

            // player case
            if (cecs_sparse_set_storage_contains(inputbuffer_storage, entity)) {
                velocity->x = 0.0f;
                velocity->y = 0.0f;

                // TODO: look if has block below
                const size_t key = (size_t)position->x << 16 | ((size_t)position->y + 1ull);
                const void *found_entity;
                // TODO: should check that these are blocks
                if (!cecs_flatmap_find(static_entity_from_position, key, sizeof(cecs_entity), &found_entity)) {
                    (void)found_entity;
                    position->x = previous_position.x;
                    position->y = previous_position.y;
                }
            }
            ++updated_count;
        }
        ++i;
    }

    cecs_view_mut_release(&view_position, &world->components);
    cecs_view_mut_release(&view_velocity, &world->components);
    cecs_view_release(&view_inputbuffer, &world->components);
}

static cecs_entity dckf_find_fencer(
    const cecs_entity_storage *const entities,
    const cecs_sparse_set_storage *const inputbuffer_storage,
    const cecs_sparse_set_storage *const position_storage,
    const cecs_sparse_set_storage *const velocity_storage
) {
    const size_t min_entity_count = cecs_min(
        cecs_sparse_set_value_count(&inputbuffer_storage->set),
        cecs_min(
            cecs_sparse_set_value_count(&position_storage->set),
            cecs_sparse_set_value_count(&velocity_storage->set)
        )
    );
    const cecs_sparse_set *const velocity_lead = &velocity_storage->set;
    size_t player_count = 0;
    size_t i = 0;
    cecs_entity player_entity;
    while (i < cecs_sparse_set_value_count(velocity_lead) && player_count < min_entity_count) {
        const cecs_dense_index velocity_index = cecs_dense_index_create_valid(i);
        const cecs_entity entity = cecs_entity_storage_get_used(
            entities,
            *cecs_sparse_set_get_sparse_key_by_index(velocity_lead, velocity_index)
        );
        const size_t entity_index = cecs_entity_index_of(entity);

        if (cecs_sparse_set_storage_contains(inputbuffer_storage, entity_index)
            && cecs_sparse_set_storage_contains(position_storage, entity_index)
        ) {
            player_entity = entity;
            ++player_count;
        }
        ++i;
    }
    cecs_debugbreak_fail_unless(
        player_count == 1,
        "currently only supports exactly one player entity"
    );
    return player_entity;
}
static void dckf_update_hand_input(cecs_world *const world) {
    cecs_view_mut view_hand = cecs_world_acquire_view_mut(world, component_hand);
    cecs_view_mut view_inputbuffer = cecs_world_acquire_view_mut(world, component_inputbuffer);

    // first identify player position
    cecs_sparse_set_storage *const hand_storage = cecs_view_mut_storage_sparse_set_mut_expect(view_hand, &world->components);
    cecs_sparse_set_storage *const inputbuffer_storage = cecs_view_mut_storage_sparse_set_mut_expect(view_inputbuffer, &world->components);

    const size_t hand_min_entity_count = cecs_min(
        cecs_sparse_set_value_count(&hand_storage->set),
        cecs_sparse_set_value_count(&inputbuffer_storage->set)
    );

    cecs_sparse_set *const hand_lead = &hand_storage->set;
    size_t updated_count = 0;
    size_t i = 0;
    while (i < cecs_sparse_set_value_count(hand_lead) && updated_count < hand_min_entity_count) {
        const cecs_dense_index hand_index = cecs_dense_index_create_valid(i);
        dckf_hand *const hand = cecs_sparse_set_get_value_by_index_mut(hand_lead, hand_index, sizeof(dckf_hand));
        dckf_hand_state hand_action = hand->state & dckf_hand_state_mask_action;
        const cecs_entity hand_entity = cecs_entity_storage_get_used(
            &world->entities,
            *cecs_sparse_set_get_sparse_key_by_index(hand_lead, hand_index)
        );
        const size_t hand_entity_index = cecs_entity_index_of(hand_entity);

        if (cecs_sparse_set_storage_contains(inputbuffer_storage, hand_entity_index)) {
            dckf_inputbuffer *const inputbuffer =
                cecs_sparse_set_storage_get_mut(inputbuffer_storage, hand_entity_index, sizeof(dckf_inputbuffer));
            while (inputbuffer->key_next_read < inputbuffer->key_next_write) {
                const char key = inputbuffer->keys[inputbuffer->key_next_read % (sizeof(inputbuffer->keys) / sizeof(char))];
                switch (key) {
                    case 'i':
                    case 'I':
                        hand->state = dckf_hand_state_above;
                        break;
                    case 'j':
                    case 'J':
                        hand->state = dckf_hand_state_below_left ;
                        break;
                    case 'k':
                    case 'K':
                        hand->state = dckf_hand_state_below_center;
                        break;
                    case 'l':
                    case 'L':
                        hand->state = dckf_hand_state_below_right;
                        break;
                    case ' ':
                        hand_action = dckf_hand_state_action;
                        break;
                }
                ++inputbuffer->key_next_read;
            }
            hand->state |= hand_action;
            inputbuffer->key_next_read %= (sizeof(inputbuffer->keys) / sizeof(char));
            inputbuffer->key_next_write %= (sizeof(inputbuffer->keys) / sizeof(char));
            cecs_debugbreak_fail_unless(
                inputbuffer->key_next_read <= inputbuffer->key_next_write,
                "fatal error: failed to normalize inputbuffer indices after processing hand input, key_next_read must be less than or equal to key_next_write"
            );
            ++updated_count;
        }
        ++i;
    }

    cecs_view_mut_release(&view_hand, &world->components);
    cecs_view_mut_release(&view_inputbuffer, &world->components);
}
static void dckf_update_hand_fencer(cecs_world *const world) {
    cecs_view view_hand = cecs_world_acquire_view(world, component_hand);
    cecs_view_mut view_position = cecs_world_acquire_view_mut(world, component_position);
    
    const cecs_sparse_set_storage *const hand_storage = cecs_view_storage_sparse_set_expect(view_hand, &world->components);
    cecs_sparse_set_storage *const position_storage = cecs_view_mut_storage_sparse_set_mut_expect(view_position, &world->components);
    

    cecs_view view_velocity_player = cecs_world_acquire_view(world, component_velocity);
    cecs_view view_inputbuffer = cecs_world_acquire_view(world, component_inputbuffer);
        const cecs_entity fencer = dckf_find_fencer(
            &world->entities,
            cecs_view_storage_sparse_set_expect(view_inputbuffer, &world->components),
            position_storage,
            cecs_view_storage_sparse_set_expect(view_velocity_player, &world->components)
        );
        const dckf_position2_f32 fencer_position = *(const dckf_position2_f32 *)cecs_sparse_set_storage_get(
            position_storage,
            cecs_entity_index_of(fencer),
            sizeof(dckf_position2_f32)
        );
    cecs_view_release(&view_velocity_player, &world->components);
    cecs_view_release(&view_inputbuffer, &world->components);
    

    const size_t min_entity_count = cecs_min(
        cecs_sparse_set_value_count(&hand_storage->set),
        cecs_sparse_set_value_count(&position_storage->set)
    );
    const cecs_sparse_set *const hand_lead = &hand_storage->set;
    size_t updated_count = 0;
    size_t i = 0;
    while (i < cecs_sparse_set_value_count(hand_lead) && updated_count < min_entity_count) {
        const cecs_dense_index hand_index = cecs_dense_index_create_valid(i);
        const dckf_hand *const hand = cecs_sparse_set_get_value_by_index(hand_lead, hand_index, sizeof(dckf_hand));
        const cecs_entity hand_entity = cecs_entity_storage_get_used(
            &world->entities,
            *cecs_sparse_set_get_sparse_key_by_index(hand_lead, hand_index)
        );
        const size_t hand_entity_index = cecs_entity_index_of(hand_entity);

        if (cecs_sparse_set_storage_contains(position_storage, hand_entity_index)) {
            dckf_position2_f32 *const position =
                cecs_sparse_set_storage_get_mut(position_storage, hand_entity_index, sizeof(dckf_position2_f32));
            
            switch (hand->state & dckf_hand_state_mask_position) {
                case dckf_hand_state_none:
                    break;
                case dckf_hand_state_above:
                    position->x = fencer_position.x;
                    position->y = fencer_position.y - 1.0f;
                    break;
                case dckf_hand_state_below_left:
                    position->x = fencer_position.x - 1.0f;
                    position->y = fencer_position.y + 1.0f;
                    break;
                case dckf_hand_state_below_center:
                    position->x = fencer_position.x;
                    position->y = fencer_position.y + 1.0f;
                    break;
                case dckf_hand_state_below_right:
                    position->x = fencer_position.x + 1.0f;
                    position->y = fencer_position.y + 1.0f;
                    break;
            }
            ++updated_count;
        }
        ++i;
    }

    cecs_view_release(&view_hand, &world->components);
    cecs_view_mut_release(&view_position, &world->components);

}
static void dckf_update_hand_action(cecs_world *const world, cecs_allocator *const allocator, cecs_flatmap *const static_entity_from_position) {
    cecs_view_mut view_hand = cecs_world_acquire_view_mut(world, component_hand);
    cecs_view_mut view_position = cecs_world_acquire_view_mut(world, component_position);
    
    cecs_sparse_set_storage *const hand_storage = cecs_view_mut_storage_sparse_set_mut_expect(view_hand, &world->components);
    const cecs_sparse_set_storage *const position_storage = cecs_view_mut_storage_sparse_set_expect(view_position, &world->components);
    const size_t min_entity_count = cecs_min(
        cecs_sparse_set_value_count(&hand_storage->set),
        cecs_sparse_set_value_count(&position_storage->set)
    );
    cecs_sparse_set *const hand_lead = &hand_storage->set;
    size_t updated_count = 0;
    size_t i = 0;
    while (i < cecs_sparse_set_value_count(hand_lead) && updated_count < min_entity_count) {
        const cecs_dense_index hand_index = cecs_dense_index_create_valid(i);
        dckf_hand *const hand = cecs_sparse_set_get_value_by_index_mut(hand_lead, hand_index, sizeof(dckf_hand));
        const dckf_hand_state hand_action = hand->state & dckf_hand_state_mask_action;
        const dckf_hand_state hand_position = hand->state & dckf_hand_state_mask_position;
        hand->state &= ~dckf_hand_state_mask_action;

        const cecs_entity hand_entity = cecs_entity_storage_get_used(
            &world->entities,
            *cecs_sparse_set_get_sparse_key_by_index(hand_lead, hand_index)
        );
        const size_t hand_entity_index = cecs_entity_index_of(hand_entity);

        if (hand_action == dckf_hand_state_action) {
            if (
                hand_position != dckf_hand_state_above
                && cecs_sparse_set_storage_contains(position_storage, hand_entity_index)
            ) {
                const dckf_position2_f32 *const position =
                    cecs_sparse_set_storage_get(position_storage, hand_entity_index, sizeof(dckf_position2_f32));
                const size_t key = (size_t)position->x << 16 | (size_t)position->y;
                
                uint_fast8_t insert_index;
                const cecs_flatbucket_mut insert_bucket =
                    cecs_flatmap_find_insert_bucket(static_entity_from_position, key, sizeof(cecs_entity), &insert_index);
                if (insert_index >= cecs_flatbucket_get_count(cecs_flatbucket_from(insert_bucket))) {
                    const cecs_entity block_entity = cecs_world_alloc_entity(world, allocator);
                    cecs_view_alloc block_view_alloc = cecs_world_acquire_view_alloc(
                        world,
                        cecs_allocator_alloc_bump_view(allocator, sizeof(dckf_block) * 1ull),
                        component_block
                    );
                    cecs_allocator position_alloc = cecs_allocator_alloc_bump_view(allocator, sizeof(dckf_position2_f32) * 1ull);
                    cecs_view_alloc position_view_alloc = cecs_view_alloc_create_take(
                        &position_alloc,
                        &view_position
                    );
                    cecs_view_alloc drawable_view_alloc = cecs_world_acquire_view_alloc(
                        world,
                        cecs_allocator_alloc_bump_view(allocator, sizeof(dckf_drawable) * 1ull),
                        component_drawable
                    );
                    cecs_view_alloc live_view_alloc = cecs_world_acquire_view_alloc(
                        world,
                        cecs_allocator_alloc_bump_view(allocator, sizeof(dckf_live) * 1ull),
                        component_live
                    );
                    
                    dckf_block *const block = cecs_view_alloc_insert_expect(&block_view_alloc, &world->components, &world->entities, block_entity);
                    dckf_position2_f32 *const block_position = cecs_view_alloc_insert_expect(&position_view_alloc, &world->components, &world->entities, block_entity);
                    dckf_drawable *const block_drawable = cecs_view_alloc_insert_expect(&drawable_view_alloc, &world->components, &world->entities, block_entity);
                    dckf_live *const block_live = cecs_view_alloc_insert_expect(&live_view_alloc, &world->components, &world->entities, block_entity);
                    
                    *block = (dckf_block){0};
                    *block_position = (dckf_position2_f32){
                        .x = position->x,
                        .y = position->y,
                    };
                    *block_drawable = (dckf_drawable){
                        .sprite = {'#', '\0'},
                        .sprite_length = 1,
                    };
                    *block_live = (dckf_live){
                        .hitpoints = 3,
                    };
    
                    cecs_entity * const inserted_entity = (cecs_entity *)cecs_flatmap_insert_into_bucket_expect(
                        static_entity_from_position,
                        insert_bucket,
                        key,
                        sizeof(cecs_entity)
                    );
                    *inserted_entity = block_entity;
    
                    cecs_view_alloc_release(&block_view_alloc, &world->components);
                    cecs_view_alloc_release(&position_view_alloc, &world->components);
                    cecs_view_alloc_release(&drawable_view_alloc, &world->components);
                    cecs_view_alloc_release(&live_view_alloc, &world->components);
                }
                ++updated_count;
            } else if (hand_position == dckf_hand_state_above) {
                if (cecs_sparse_set_storage_contains(position_storage, hand_entity_index)) {
                    const dckf_position2_f32 *const position =
                    cecs_sparse_set_storage_get(position_storage, hand_entity_index, sizeof(dckf_position2_f32));
                    const size_t key = (size_t)position->x << 16 | (size_t)position->y;
                    
                    const void *found_entity;
                    if (cecs_flatmap_find(static_entity_from_position, key, sizeof(cecs_entity), &found_entity)) {
                        const cecs_entity block_entity = *(const cecs_entity *)found_entity;
                        cecs_world_free_entity(world, block_entity);

                        // XXX: ? this worked before with simply `cecs_flatmap_remove`
                        cecs_flatmap_find_remove_expect(static_entity_from_position, allocator, key, sizeof(cecs_entity));
                    }
                }
                ++updated_count;
            }
        }
        ++i;
    }

    cecs_view_mut_release(&view_hand, &world->components);
    if (cecs_view_mut_is_valid(view_position)) {
        cecs_view_mut_release(&view_position, &world->components);
    }
}

void dckf_update(cecs_world *const world, cecs_allocator *const allocator, void *const context) {
    (void)allocator;
    cecs_flatmap *const static_entity_from_position = (cecs_flatmap *)context;
    dckf_update_velocity_input(world);
    dckf_update_position_velocity(world, static_entity_from_position);
    dckf_update_hand_input(world);
    dckf_update_hand_fencer(world);
    dckf_update_hand_action(world, allocator, static_entity_from_position);
}
