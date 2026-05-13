#include <cecs_world.h>
#include <dckf_input.h>
#include <time.h>
#include <stdio.h>

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
    dckf_init(&w, &alloc, NULL);

    bool running = true;
    const struct timespec target_frame_time = {
        .tv_sec = 0,
        .tv_nsec = 16 * 1000 * 1000, // 16ms
    };
    while (running) {
        dckf_prerender(&w, &alloc, NULL);
        dckf_render(&w, &alloc, NULL);
        
        struct timespec remaining_frame_time = target_frame_time;
        while (nanosleep(&remaining_frame_time, &remaining_frame_time) != 0) { }
        
        dckf_input(&w, &alloc, &running);
        dckf_update(&w, &alloc, NULL);
    }


    dckf_deinit(&w, &alloc, NULL);
    cecs_world_destroy(&w, &alloc);
    return 0;
}

void dckf_init(cecs_world *const world, cecs_allocator *const allocator, void *const context) {
    (void)world;
    (void)allocator;
    (void)context;
    printf("DeckFencer - Press 'q' to quit\n");
    dckf_set_conio_terminal_mode();
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

void dckf_render(const cecs_world *const world, cecs_allocator *const allocator, void *const context) {
    (void)world;
    (void)allocator;
    (void)context;
}

void dckf_input(cecs_world *const world, cecs_allocator *const allocator, void *const context) {
    (void)world;
    (void)allocator;
    bool *const running = (bool *)context;
    if (dckf_kbhit()) {
        const int ch = dckf_getch();
        if (ch == 'q' || ch == 'Q') {
            *running = false;
        }
    }
}

void dckf_update(cecs_world *const world, cecs_allocator *const allocator, void *const context) {
    (void)world;
    (void)allocator;
    (void)context;
}
