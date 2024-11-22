#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include "board_util.h"
#include "solve.h"

int force_unique = 0;

union cell_state getSurroundings(struct state_set *board, int x, int y)
{
    union cell_state res = { 0 };

    res.n = board[x + ((y + dim_y - 1) % dim_y) * dim_x].possible.s;
    res.e = board[(x + 1) % dim_x + y * dim_x].possible.w;
    res.s = board[x + ((y + 1) % dim_y) * dim_x].possible.n;
    res.w = board[(x + dim_x - 1) % dim_x + y * dim_x].possible.e;

    return res;
}

int isNotCompatible(union cell_state a, union cell_state b)
{
    b.bits = ((b.bits << 2) & 04444) | (b.bits & 02222) | ((b.bits >> 2) & 01111);
    uint16_t c = a.bits & b.bits;
    c |= c << 1 | c >> 1;
    c &= 02222;
    return c != 02222;
}

void updatePossible(struct state_set *board, int x, int y)
{
    struct state_set *current = &board[x + y * dim_x];
    int n = CHAR_BIT * sizeof(int) - __builtin_clz((int)current->bitmask);
    union cell_state surround = getSurroundings(board, x, y);
    union cell_state *array = current->array;
    uint16_t possible = 0;

    for (int i = __builtin_ctz((int)current->bitmask); i < n; i++) {
        int c = isNotCompatible(surround, array[i]);
        possible |= array[i].bits * !c;
        current->bitmask &= ~(c << i);
    }

    current->possible = (union cell_state)possible;
}

struct xy {
    int x;
    int y;
};

#define SPECIAL 0xFFFFFFFF

int DFSCheck(struct state_set *board, uint32_t *explore, int x, int y)
{
    struct state_set current = board[x + y * dim_x];
    uint32_t *mark = &explore[x + y * dim_x];

    if (*mark == SPECIAL) {
        return 1;
    }

    *mark = SPECIAL;

    int n_y = (y - 1 + dim_y) % dim_y;
    int s_y = (y + 1) % dim_y;
    int e_x = (x + 1) % dim_x;
    int w_x = (x - 1 + dim_x) % dim_x;

    int res = 0;

    if (current.possible.n & 5) {
        res += DFSCheck(board, explore, x, n_y);
    }
    if (current.possible.e & 5) {
        res += DFSCheck(board, explore, e_x, y);
    }
    if (current.possible.s & 5) {
        res += DFSCheck(board, explore, x, s_y);
    }
    if (current.possible.w & 5) {
        res += DFSCheck(board, explore, w_x, y);
    }
    
    return res;
}

int DFSCheckConfirmed(struct state_set *board, uint32_t *explore, int x, int y)
{
    struct state_set current = board[x + y * dim_x];
    uint32_t *mark = &explore[x + y * dim_x];

    if (*mark == SPECIAL) {
        return 1;
    }

    *mark = SPECIAL;

    int n_y = (y - 1 + dim_y) % dim_y;
    int s_y = (y + 1) % dim_y;
    int e_x = (x + 1) % dim_x;
    int w_x = (x - 1 + dim_x) % dim_x;

    int res = 0;

    if (current.possible.n  == OUT) {
        res += DFSCheckConfirmed(board, explore, x, n_y);
    }
    if (current.possible.e == OUT) {
        res += DFSCheckConfirmed(board, explore, e_x, y);
    }
    if (current.possible.s == OUT) {
        res += DFSCheckConfirmed(board, explore, x, s_y);
    }
    if (current.possible.w == OUT) {
        res += DFSCheckConfirmed(board, explore, w_x, y);
    }
    
    return res;
}

int checkLoopAndContinuity(struct state_set *board)
{
    const int size = dim_x * dim_y;
    uint32_t *explore = calloc(sizeof(uint32_t), size);
    if (!explore)
        return -1;
    
    for (int x = 0; x < dim_x; x++) {
        for (int y = 0; y < dim_y; y++) {
            if (DFSCheckConfirmed(board, explore, x, y)) {
                free(explore);
                return 1;
            }
            memset(explore, 0, sizeof(uint32_t) * size);
        }
    }
    
    DFSCheck(board, explore, 0, 0);
    for (int y = 0; y < dim_y; y++) {
        for (int x = 0; x < dim_x; x++) {
            if ((explore[x + y * dim_x] != SPECIAL) && board[x + y * dim_x].possible.bits != edge[0].bits) {
                free(explore);
                return 1;
            }
        }
    }
    free(explore);
    return 0;
}

void updatePossibleSingle(struct state_set *single)
{
    int k_max = CHAR_BIT * sizeof(int) - __builtin_clz((int)single->bitmask);
    single->possible.bits = 0;
    for (int k = 0; k < k_max; k++) {
        single->possible.bits |= ((single->bitmask >> k) & 1) ? single->array[k].bits : 0;
    }
}

uint32_t collapseState(uint32_t bitmask, int popcnt)
{
    // randomly pick a collapse state
    int rand_bit_num = random() % popcnt;
    int k = 0;
    for (int bit_count = 0; bit_count <= rand_bit_num; k++)
        bit_count += (bitmask >> k) & 1;
    uint32_t new_mask = 1 << (k - 1);
    //int k_max = CHAR_BIT * sizeof(uint32_t) - __builtin_clz((int)bitmask);
    //uint32_t first = array[k].bits & 02222;
    //
    //// collapse orientation of a pipe but not flow direction
    //for (k = 0; k < k_max; k++)
    //    if ((array[k].bits & 02222) == first && (bitmask >> k & 1))
    //        new_mask |= 1 << k;

    return new_mask;
}

void classifyBoard(struct state_set *board)
{
    for (int i = 0; i < dim_x * dim_y; i++) {
        if (board[i].array == all_pipes) {
            switch (__builtin_ctz((int)board[i].bitmask)) {
                case 0 ... 11: // split
                    board[i].array = split;
                    board[i].bitmask = (1 << 12) - 1;
                    break;
                case 12 ... 15: // pipe
                    board[i].array = pipe;
                    board[i].bitmask = (1 << 4) - 1;
                    break;
                case 16 ... 23: // elbow
                    board[i].array = elbow;
                    board[i].bitmask = (1 << 8) - 1;
                    break;
                case 24 ... 27: // sink
                    board[i].array = sink;
                    board[i].bitmask = (1 << 4) - 1;
                    break;
            }
        } else if (board[i].array == all_src) {
            switch (__builtin_ctz((int)board[i].bitmask)) {
                case 0 ... 3: // split
                    board[i].array = split_src;
                    board[i].bitmask = (1 << 4) - 1;
                    break;
                case 4 ... 5: // pipe
                    board[i].array = pipe_src;
                    board[i].bitmask = (1 << 2) - 1;
                    break;
                case 6 ... 9: // elbow
                    board[i].array = elbow_src;
                    board[i].bitmask = (1 << 4) - 1;
                    break;
                case 10 ... 13: // single
                    board[i].array = single_src;
                    board[i].bitmask = (1 << 4) - 1;
                    break;
            }
        } else {
            if (board[i].array == split) {
                board[i].bitmask = (1 << 12) - 1;
            } else if (board[i].array == pipe || board[i].array == sink) {
                board[i].bitmask = (1 << 4) - 1;
            } else if (board[i].array == elbow) {
                board[i].bitmask = (1 << 8) - 1;
            }
        }
        updatePossibleSingle(&board[i]);
    }
}

int solveCheckUnique(struct state_set *board, char *diff_map)
{
    if (animate)
        system("clear");
    const int size = dim_x * dim_y;
    uint32_t *explore = calloc(sizeof(uint32_t), size);
    struct xy *queue = calloc(sizeof(struct xy), size);
    struct xy *pool = calloc(sizeof(struct xy), size);
    if (!explore || !queue || !pool)
        goto fail;
    int pool_len = 0;
    int explored_len = 0;
    int q_start = 0;
    int q_end = 0;
    int solved = 0;
    int return_state = SOLVE_SUCCESS_UNIQUE;

    for (int i = 0; i < size; i++) {
        updatePossible(board, i % dim_x, i / dim_x);
        if (__builtin_popcount(board[i].bitmask) != 1) {
            pool[pool_len].x = i % dim_x;
            pool[pool_len++].y = i / dim_x;
        }
        if (__builtin_popcount(board[i].bitmask) == 0)
            goto fail;
    }
    
    for (uint32_t breadth = 0; pool_len > 0;) {
        int selection = random() % pool_len;
        int x_r = pool[selection].x;
        int y_r = pool[selection].y;
        queue[q_end % size].x = x_r;
        queue[q_end++ % size].y = y_r;
        explore[x_r + y_r * dim_x] = breadth + 1;

        while (q_end - q_start > 0) {
            //if (animate) {
            //    printf("\033[0;0H");
            //    printResult(board);
            //    usleep(animate_delay_us);
            //}
            
            int x = queue[q_start % size].x;
            int y = queue[q_start++ % size].y;
            breadth = explore[x + y * dim_x];

            struct state_set *current = &board[x + y * dim_x];
            union cell_state init = current->possible;

            updatePossible(board, x, y);

            int pop = __builtin_popcount(current->bitmask);
            if (pop == 0)
                goto fail;
            else if (pop == 1) {
                int k = 0;
                while ((pool[k].x != x || pool[k].y != y) && k < pool_len + explored_len)
                    k++;
                if (k < pool_len) {
                    pool[k] = pool[pool_len - 1];
                    pool[pool_len - 1] = pool[pool_len + explored_len - 1];
                    pool_len--;
                }
                else if (k < pool_len + explored_len) {
                    pool[k] = pool[pool_len + explored_len - 1];
                    explored_len--;
                }
            } else if (init.bits == current->possible.bits) {
                int k = 0;
                while ((pool[k].x != x || pool[k].y != y) && k < pool_len)
                    k++;
                if (k != pool_len) {
                    struct xy temp = pool[k];
                    pool[k] = pool[pool_len - 1];
                    pool[--pool_len] = temp;
                    explored_len++;
                }
            } else {
                pool_len += explored_len;
                explored_len = 0;
            }


            union cell_state diff = (union cell_state)(uint16_t)(init.bits ^ current->possible.bits);

            int n_y = (y - 1 + dim_y) % dim_y;
            int s_y = (y + 1) % dim_y;
            int e_x = (x + 1) % dim_x;
            int w_x = (x - 1 + dim_x) % dim_x;

            if (diff.n && explore[x + n_y * dim_x] != breadth + 1) {
                queue[q_end % size].x = x;
                queue[q_end++ % size].y = n_y;
                explore[x + n_y * dim_x] = breadth + 1;
            }
            if (diff.e && explore[e_x + y * dim_x] != breadth + 1) {
                queue[q_end % size].x = e_x;
                queue[q_end++ % size].y = y;
                explore[e_x + y * dim_x] = breadth + 1;
            }
            if (diff.s && explore[x + s_y * dim_x] != breadth + 1) {
                queue[q_end % size].x = x;
                queue[q_end++ % size].y = s_y;
                explore[x + s_y * dim_x] = breadth + 1;
            }
            if (diff.w && explore[w_x + y * dim_x] != breadth + 1) {
                queue[q_end % size].x = w_x;
                queue[q_end++ % size].y = y;
                explore[w_x + y * dim_x] = breadth + 1;
            }
        }
        if (pool_len == 0 && explored_len > 0) {
            if (checkLoopAndContinuity(board)) {
                goto fail;
            }
            struct state_set *new_board = calloc(sizeof(struct state_set), size);
            memcpy(new_board, board, sizeof(struct state_set) * size);

            // shuffle pool to randomize collapse
            if (explored_len > 1) {
                for (int j = 0; j < explored_len; j++) {
                    int k = j + random() / (RAND_MAX / (explored_len - j) + 1);
                    struct xy t = pool[k];
                    pool[k] = pool[j];
                    pool[j] = t;
                }
            }

            uint32_t min_mask;
            struct state_set *least_entropy = NULL;
            int min = 33;
            int min_idx;
            for (int j = 0; j < explored_len; j++) {
                struct state_set *curr = &new_board[pool[j].x + pool[j].y * dim_x];
                int popcnt = __builtin_popcount((int)curr->bitmask);
                if (popcnt < min) {
                    uint32_t new_mask = collapseState(curr->bitmask, popcnt);

                    min = popcnt;
                    min_idx = j;
                    least_entropy = curr;
                    min_mask = new_mask;
                }
            }

            if (!least_entropy) {
                free(new_board);
                goto fail;
            }
            
            least_entropy->bitmask = min_mask;
            // update the possible cell_states
            updatePossibleSingle(least_entropy);
            

            switch (solveCheckUnique(new_board, diff_map)) {
                case SOLVE_SUCCESS_UNIQUE: 
                    {
                    struct state_set *test_board = calloc(sizeof(struct state_set), size);

                    uint32_t reduced_bitmask = board[pool[min_idx].x + pool[min_idx].y * dim_x].bitmask;
                    return_state = SOLVE_SUCCESS_MULTIPLE;
                    do {
                        reduced_bitmask &= ~min_mask;
                        if (reduced_bitmask == 0) {
                            return_state = SOLVE_SUCCESS_UNIQUE;
                            break;
                        }

                        memcpy(test_board, board, sizeof(struct state_set) * size);

                        int popcnt = __builtin_popcount(reduced_bitmask);

                        min_mask = collapseState(reduced_bitmask, popcnt);

                        test_board[pool[min_idx].x + pool[min_idx].y * dim_x].bitmask = min_mask;
                        updatePossibleSingle(&test_board[pool[min_idx].x + pool[min_idx].y * dim_x]);
                    } while (solveCheckUnique(test_board, diff_map) == SOLVE_FAILED);

                    if (diff_map)
                        for (int i = 0; i < dim_x * dim_y; i++) {
                            diff_map[i] = test_board[i].bitmask != new_board[i].bitmask;
                        }
                    memcpy(board, new_board, size * sizeof(struct state_set));
                    free(new_board);
                    free(test_board);
                    goto success;
                } break;
                case SOLVE_SUCCESS_MULTIPLE:
                    memcpy(board, new_board, size * sizeof(struct state_set));
                    free(new_board);
                    return_state = SOLVE_SUCCESS_MULTIPLE;
                    goto success;
                    break;
                case SOLVE_FAILED:
                    board[pool[min_idx].x + pool[min_idx].y * dim_x].bitmask &= ~min_mask;
                    updatePossibleSingle(&board[pool[min_idx].x + pool[min_idx].y * dim_x]);
                    pool_len += explored_len;
                    explored_len = 0;
                    break;
            }

            free(new_board);
        }
    }
    DFSCheck(board, explore, 0, 0);

    for (int i = 0; i < size; i++)
        if (explore[i] != SPECIAL && board[i].array != edge)
            goto fail;


success:
    free(pool);
    free(queue);
    free(explore);
    return return_state;
fail:
    free(pool);
    free(queue);
    free(explore);
    return SOLVE_FAILED;
}

int sol_count = 0;

int solve(struct state_set *board)
{
    if (animate)
        system("clear");
    const int size = dim_x * dim_y;
    uint32_t *explore = calloc(sizeof(uint32_t), size);
    struct xy *queue = calloc(sizeof(struct xy), size);
    struct xy *pool = calloc(sizeof(struct xy), size);
    if (!explore || !queue || !pool)
        goto fail;
    int pool_len = 0;
    int explored_len = 0;
    int q_start = 0;
    int q_end = 0;
    int solved = 0;
    int return_state = SOLVE_SUCCESS_UNKNOWN;

    for (int i = 0; i < size; i++) {
        updatePossible(board, i % dim_x, i / dim_x);
        if (__builtin_popcount(board[i].bitmask) != 1) {
            pool[pool_len].x = i % dim_x;
            pool[pool_len++].y = i / dim_x;
        }
        if (__builtin_popcount(board[i].bitmask) == 0)
            goto fail;
    }
    
    for (uint32_t breadth = 0; pool_len > 0;) {
        int selection = random() % pool_len;
        int x_r = pool[selection].x;
        int y_r = pool[selection].y;
        queue[q_end % size].x = x_r;
        queue[q_end++ % size].y = y_r;
        explore[x_r + y_r * dim_x] = breadth + 1;

        while (q_end - q_start > 0) {
            if (animate) {
                printf("\033[0;0H");
                printResult(board);
                usleep(animate_delay_us);
            }
            
            int x = queue[q_start % size].x;
            int y = queue[q_start++ % size].y;
            breadth = explore[x + y * dim_x];

            struct state_set *current = &board[x + y * dim_x];
            union cell_state init = current->possible;

            updatePossible(board, x, y);

            int pop = __builtin_popcount(current->bitmask);
            if (pop == 0)
                goto fail;
            else if (pop == 1) {
                int k = 0;
                while ((pool[k].x != x || pool[k].y != y) && k < pool_len + explored_len)
                    k++;
                if (k < pool_len) {
                    pool[k] = pool[pool_len - 1];
                    pool[pool_len - 1] = pool[pool_len + explored_len - 1];
                    pool_len--;
                }
                else if (k < pool_len + explored_len) {
                    pool[k] = pool[pool_len + explored_len - 1];
                    explored_len--;
                }
            } else if (init.bits == current->possible.bits) {
                int k = 0;
                while ((pool[k].x != x || pool[k].y != y) && k < pool_len)
                    k++;
                if (k != pool_len) {
                    struct xy temp = pool[k];
                    pool[k] = pool[pool_len - 1];
                    pool[--pool_len] = temp;
                    explored_len++;
                }
            } else {
                pool_len += explored_len;
                explored_len = 0;
            }


            union cell_state diff = (union cell_state)(uint16_t)(init.bits ^ current->possible.bits);

            int n_y = (y - 1 + dim_y) % dim_y;
            int s_y = (y + 1) % dim_y;
            int e_x = (x + 1) % dim_x;
            int w_x = (x - 1 + dim_x) % dim_x;

            if (diff.n && explore[x + n_y * dim_x] != breadth + 1) {
                queue[q_end % size].x = x;
                queue[q_end++ % size].y = n_y;
                explore[x + n_y * dim_x] = breadth + 1;
            }
            if (diff.e && explore[e_x + y * dim_x] != breadth + 1) {
                queue[q_end % size].x = e_x;
                queue[q_end++ % size].y = y;
                explore[e_x + y * dim_x] = breadth + 1;
            }
            if (diff.s && explore[x + s_y * dim_x] != breadth + 1) {
                queue[q_end % size].x = x;
                queue[q_end++ % size].y = s_y;
                explore[x + s_y * dim_x] = breadth + 1;
            }
            if (diff.w && explore[w_x + y * dim_x] != breadth + 1) {
                queue[q_end % size].x = w_x;
                queue[q_end++ % size].y = y;
                explore[w_x + y * dim_x] = breadth + 1;
            }
        }
        if (pool_len == 0 && explored_len > 0) {
            if (checkLoopAndContinuity(board)) {
                goto fail;
            }
            struct state_set *new_board = calloc(sizeof(struct state_set), size);
            memcpy(new_board, board, sizeof(struct state_set) * size);

            // shuffle pool to randomize collapse
            if (explored_len > 1) {
                for (int j = 0; j < explored_len; j++) {
                    int k = j + random() / (RAND_MAX / (explored_len - j) + 1);
                    struct xy t = pool[k];
                    pool[k] = pool[j];
                    pool[j] = t;
                }
            }

            uint32_t min_mask;
            struct state_set *least_entropy = NULL;
            int min = 33;
            int min_idx;
            for (int j = 0; j < explored_len; j++) {
                struct state_set *curr = &new_board[pool[j].x + pool[j].y * dim_x];
                int popcnt = __builtin_popcount((int)curr->bitmask);
                if (popcnt < min) {
                    uint32_t new_mask = collapseState(curr->bitmask, popcnt);

                    min = popcnt;
                    min_idx = j;
                    least_entropy = curr;
                    min_mask = new_mask;
                }
            }

            if (!least_entropy) {
                free(new_board);
                goto fail;
            }
            
            least_entropy->bitmask = min_mask;
            // update the possible cell_states
            updatePossibleSingle(least_entropy);
            

            switch (solve(new_board)) {
                case SOLVE_SUCCESS_UNKNOWN: 
                if (force_unique) {
                    struct state_set *unique_check_board = 
                        malloc(dim_x * dim_y * sizeof(struct state_set));
                    memcpy(unique_check_board, new_board, dim_x * dim_y * sizeof(struct state_set));
                    
                    classifyBoard(unique_check_board);

                    if (solveCheckUnique(unique_check_board, NULL) != SOLVE_SUCCESS_UNIQUE) {
                        free(unique_check_board);
                        board[pool[min_idx].x + pool[min_idx].y * dim_x].bitmask &= ~min_mask;
                        updatePossibleSingle(&board[pool[min_idx].x + pool[min_idx].y * dim_x]);
                        pool_len += explored_len;
                        explored_len = 0;
                        break;
                    }
                    free(unique_check_board);
                } // fall-through intentional
                case SOLVE_SUCCESS_UNIQUE: 
                {
                    return_state = SOLVE_SUCCESS_UNIQUE;
                    memcpy(board, new_board, size * sizeof(struct state_set));
                    free(new_board);
                    goto success;
                } break;
                case SOLVE_FAILED:
                    board[pool[min_idx].x + pool[min_idx].y * dim_x].bitmask &= ~min_mask;
                    updatePossibleSingle(&board[pool[min_idx].x + pool[min_idx].y * dim_x]);
                    pool_len += explored_len;
                    explored_len = 0;
                    break;
            }

            free(new_board);
        }
    }
    DFSCheck(board, explore, 0, 0);

    for (int i = 0; i < size; i++)
        if (explore[i] != SPECIAL && board[i].array != edge)
            goto fail;


success:
    free(pool);
    free(queue);
    free(explore);
    return return_state;
fail:
    free(pool);
    free(queue);
    free(explore);
    return SOLVE_FAILED;
}
