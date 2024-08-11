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

#define BOARD_WIDTH 64
#define BOARD_HEIGHT 64

typedef enum cell_state {
    CELL_STATE_ALIVE,
    CELL_STATE_DEAD
} cell_state;

COMPONENT_DEFINE(cell) {
    uint16_t x;
    uint16_t y;
    cell_state state;
} cell;

RESOURCE_DEFINE(board) {
    cell cells[BOARD_WIDTH][BOARD_HEIGHT];
} board;


bool Update(double delta_time_seconds) {
    printf("Delta time: %f\n", delta_time_seconds);
    return false;
}

void main(void) {
    world w = world_create();

    bool quitting = false;
    bool update_error = false;

    struct timespec start, end;
    timespec_get(&start, TIME_UTC);

    LARGE_INTEGER StartingTime, EndingTime, ElapsedMicroseconds;
    LARGE_INTEGER Frequency;
    QueryPerformanceFrequency(&Frequency); 
    QueryPerformanceCounter(&StartingTime);

    Sleep(1000 / 60);
    while (!quitting && !update_error)
    {
        timespec_get(&end, TIME_UTC);

        QueryPerformanceCounter(&EndingTime);
        ElapsedMicroseconds.QuadPart = EndingTime.QuadPart - StartingTime.QuadPart;
        ElapsedMicroseconds.QuadPart *= 1000000;
        ElapsedMicroseconds.QuadPart /= Frequency.QuadPart;

        //double delta_time_seconds = (double)ElapsedMicroseconds.QuadPart / 1000000;
        double delta_time_seconds = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;

        if (Update(delta_time_seconds)) {
            update_error = true;
        }

        timespec_get(&start, TIME_UTC);

        QueryPerformanceFrequency(&Frequency); 
        QueryPerformanceCounter(&StartingTime);    
        Sleep(1000 / 2);
    }
    
    world_free(&w);
    printf("Freed world\n");
}