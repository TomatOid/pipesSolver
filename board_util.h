#pragma once
#include <stdint.h>
#include <stddef.h>

#define array_len(x) ((sizeof x) / (sizeof *x))

extern int animate;
extern int animate_delay_us;

int usleep(uint64_t usec);

extern size_t dim_x, dim_y;

enum face_state_t {
    IN = 1 << 0,
    NONE = 1 << 1,
    OUT = 1 << 2,
};

union __attribute__((__packed__)) cell_state {
    struct __attribute__((__packed__)) {
        unsigned n : 3;
        unsigned e : 3;
        unsigned s : 3;
        unsigned w : 3;
    };
    uint16_t bits;
};

struct state_set {
    union cell_state *array;
    union cell_state possible;
    uint32_t bitmask;
};

extern union cell_state split[12];
extern union cell_state pipe[4];
extern union cell_state elbow[8];
extern union cell_state sink[4];
extern union cell_state split_src[4];
extern union cell_state pipe_src[2];
extern union cell_state elbow_src[4];
extern union cell_state single_src[4];
extern union cell_state edge[1];
extern union cell_state all_pipes[28];
extern union cell_state all_src[14];
extern const union cell_state in_out_none;
extern const union cell_state in_none;
extern const union cell_state out_none;

// rotate the edges of a single cell_state counterclockwise
union cell_state rotate(union cell_state s);

// fill an array with all of it's possible rotations. Assumes the array
// is quarter-full and that its length is divisible by 4
int createRotations(union cell_state *array, size_t len);

// IMPORTANT: must be run before solving!!
void initArrays();

// Parses human-readable string-format test puzzles into an array of 
// state_sets. Sets the dim_x and dim_y globals accordingly
struct state_set *parseBoard(const char *str, size_t *width, size_t *height);

// Prints a single cell_state with unicode characters
void printCell(union cell_state c);

// Prints the entire board, assumes dim_x and dim_y are correctly set
void printResult(struct state_set *board);
