#pragma once
#include <stdint.h>
#include "board_util.h"

extern int force_unique;
enum {
    SOLVE_FAILED,
    SOLVE_SUCCESS_UNIQUE,
    SOLVE_SUCCESS_MULTIPLE,
    SOLVE_SUCCESS_UNKNOWN,
};

int solve(struct state_set *board);
void updatePossibleSingle(struct state_set *single);
int solveCheckUnique(struct state_set *board, char *diff_map); //, int depth, int *depth_map, char *diff_map)
uint32_t collapseState(uint32_t bitmask, int popcnt);
void classifyBoard(struct state_set *board);
