#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "solve.h"
#include "board_util.h"

int animate = 1;
int animate_delay_us = 500;

size_t dim_x, dim_y;

const char *blank_6 =
    "aaaaaaae\n"
    "aaaaaaae\n"
    "aaaaaaae\n"
    "aaaAaaae\n"
    "aaaaaaae\n"
    "aaaaaaae\n"
    "aaaaaaae\n"
    "eeeeeeee\n";

const char *blank_8 =
    "aaaaaaaaae\n"
    "aaaaaaaaae\n"
    "aaaaaaaaae\n"
    "aaaaaaaaae\n"
    "aaaaAaaaae\n"
    "aaaaaaaaae\n"
    "aaaaaaaaae\n"
    "aaaaaaaaae\n"
    "aaaaaaaaae\n"
    "eeeeeeeeee\n";

const char *blank_11_wrap =
    "aaaaaaaaaaa\n"
    "aaaaaaaaaaa\n"
    "aaaaaaaaaaa\n"
    "aaaaaaaaaaa\n"
    "aaaaaaaaaaa\n"
    "aaaaaAaaaaa\n"
    "aaaaaaaaaaa\n"
    "aaaaaaaaaaa\n"
    "aaaaaaaaaaa\n"
    "aaaaaaaaaaa\n"
    "aaaaaaaaaaa\n";

const char *blank_25_wrap =
    "aaaaaaaaaaaaaaaaaaaaaaaaa\n"
    "aaaaaaaaaaaaaaaaaaaaaaaaa\n"
    "aaaaaaaaaaaaaaaaaaaaaaaaa\n"
    "aaaaaaaaaaaaaaaaaaaaaaaaa\n"
    "aaaaaaaaaaaaaaaaaaaaaaaaa\n"
    "aaaaaaaaaaaaaaaaaaaaaaaaa\n"
    "aaaaaaaaaaaaaaaaaaaaaaaaa\n"
    "aaaaaaaaaaaaaaaaaaaaaaaaa\n"
    "aaaaaaaaaaaaaaaaaaaaaaaaa\n"
    "aaaaaaaaaaaaaaaaaaaaaaaaa\n"
    "aaaaaaaaaaaaaaaaaaaaaaaaa\n"
    "aaaaaaaaaaaaaaaaaaaaaaaaa\n"
    "aaaaaaaaaaaaAaaaaaaaaaaaa\n"
    "aaaaaaaaaaaaaaaaaaaaaaaaa\n"
    "aaaaaaaaaaaaaaaaaaaaaaaaa\n"
    "aaaaaaaaaaaaaaaaaaaaaaaaa\n"
    "aaaaaaaaaaaaaaaaaaaaaaaaa\n"
    "aaaaaaaaaaaaaaaaaaaaaaaaa\n"
    "aaaaaaaaaaaaaaaaaaaaaaaaa\n"
    "aaaaaaaaaaaaaaaaaaaaaaaaa\n"
    "aaaaaaaaaaaaaaaaaaaaaaaaa\n"
    "aaaaaaaaaaaaaaaaaaaaaaaaa\n"
    "aaaaaaaaaaaaaaaaaaaaaaaaa\n"
    "aaaaaaaaaaaaaaaaaaaaaaaaa\n"
    "aaaaaaaaaaaaaaaaaaaaaaaaa\n";

void generatePuzzleDFS(size_t width, size_t height)
{
    
}

void generatePuzzle(const char *blank)
{
    struct state_set *blank_board = parseBoard(blank, &dim_x, &dim_y);
    struct state_set *board = parseBoard(blank, &dim_x, &dim_y);
    char *diff_area = malloc(dim_x * dim_y * sizeof(char));
    char *new_diff_area = malloc(dim_x * dim_y * sizeof(char));

    int ret_val = -1;
    do {
        solve(board);
        //animate = 1;

        classifyBoard(board);

        if (ret_val == -1)
            ret_val = solveCheckUnique(board, diff_area);
        else {
            ret_val = solveCheckUnique(board, new_diff_area);
            if (ret_val == SOLVE_SUCCESS_MULTIPLE) {
                for (int i = 0; i < dim_x * dim_y; i++)
                    diff_area[i] |= new_diff_area[i] || diff_area[i];
            }
        }
            

        if (ret_val == SOLVE_SUCCESS_MULTIPLE) {
            for (int x = 0; x < dim_x; x++) {
                for (int y = 0; y < dim_y; y++) {
                    if (diff_area[(x + 1) % dim_x + y * dim_x] || diff_area[(x + dim_x - 1) % dim_x + y * dim_x]
                            || diff_area[x + ((y + dim_y + 1) % dim_y) * dim_x] || diff_area[x + ((y + dim_y + 1) % dim_y) * dim_x]
                            || diff_area[x + y * dim_x]) {
                        new_diff_area[x + y * dim_x] = 1;
                    }
                }
            }
            memcpy(diff_area, new_diff_area, dim_x * dim_y * sizeof(char));
            
            printResult(board);
            for (int i = 0; i < dim_x * dim_y; i++) {
                if (diff_area[i]) {
                    board[i].bitmask = blank_board[i].bitmask;
                    board[i].possible.bits = blank_board[i].possible.bits;
                    board[i].array = blank_board[i].array;
                }
            }
            //classifyBoard(board);
        }

    } while (ret_val == SOLVE_SUCCESS_MULTIPLE);


    printf("\nPuzzle is %s\n", (ret_val == SOLVE_SUCCESS_UNIQUE) ? "unique" : "not unique");
    printResult(board);

    free(diff_area);
    free(new_diff_area);
}

int main(int argc, char **argv)
{
    initArrays();
    generatePuzzle(blank_25_wrap);  
}

