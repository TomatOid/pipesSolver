#include "board_util.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <locale.h>

// define all of the arrays
union cell_state split[12] = {
    {{ IN, OUT, NONE, OUT }},
    {{ OUT, OUT, NONE, IN }},
    {{ OUT, IN, NONE, OUT }},
};

union cell_state pipe[4] = {
    {{ NONE, OUT, NONE, IN }},
};

union cell_state elbow[8] = {
    {{ IN, OUT, NONE, NONE }},
    {{ OUT, IN, NONE, NONE }},
};

union cell_state sink[4] = {
    {{ IN, NONE, NONE, NONE }},
};

union cell_state split_src[4] = {
    {{ OUT, OUT, NONE, OUT }},
};

// special, do not call createRotations on this, as it is already full
union cell_state pipe_src[2] = {
    {{ NONE, OUT, NONE, OUT }},
    {{ OUT, NONE, OUT, NONE }},
};

union cell_state elbow_src[4] = {
    {{ OUT, OUT, NONE, NONE }},
};

union cell_state single_src[4] = {
    {{ OUT, NONE, NONE, NONE }},
};

union cell_state edge[1] = {
    {{ NONE, NONE, NONE, NONE }},
};

union cell_state all_pipes[28];

union cell_state all_src[14];

const union cell_state in_out_none = {
    { IN | OUT | NONE, IN | OUT | NONE, IN | OUT | NONE, IN | OUT | NONE },
};

const union cell_state in_none = {
    { IN | NONE, IN | NONE, IN | NONE, IN | NONE },
};

const union cell_state out_none = {
    { OUT | NONE, OUT | NONE, OUT | NONE, OUT | NONE },
};

union cell_state rotate(union cell_state s)
{
    unsigned t = s.n;
    s.n = s.e;
    s.e = s.s;
    s.s = s.w;
    s.w = t;
    return s;
}

int createRotations(union cell_state *array, size_t len)
{
    if (len % 4)
        return 1;

    size_t quarter = len / 4;
    for (size_t i = quarter; i < len; i++) {
        array[i] = rotate(array[i - quarter]);
    }
    return 0;
}

inline int entropy(struct state_set s)
{
    return __builtin_popcount(s.bitmask);
}

void initArrays() 
{
    assert(sizeof(union cell_state) == sizeof(uint16_t));
    createRotations(split, array_len(split));
    createRotations(pipe, array_len(pipe));
    createRotations(elbow, array_len(elbow));
    createRotations(sink, array_len(sink));
    createRotations(split_src, array_len(split_src));
    createRotations(elbow_src, array_len(elbow_src));
    createRotations(single_src, array_len(single_src));

    char *all_pipes_curr = (char *)all_pipes;
    memcpy(all_pipes_curr, split, sizeof(split));
    memcpy((all_pipes_curr += sizeof(split)), pipe, sizeof(pipe));
    memcpy((all_pipes_curr += sizeof(pipe)), elbow, sizeof(elbow));
    memcpy((all_pipes_curr += sizeof(elbow)), sink, sizeof(sink));

    char *all_src_curr = (char *)all_src;
    memcpy(all_src_curr, split_src, sizeof(split_src));
    memcpy((all_src_curr += sizeof(split_src)), pipe_src, sizeof(pipe_src));
    memcpy((all_src_curr += sizeof(pipe_src)), elbow_src, sizeof(elbow_src));
    memcpy((all_src_curr += sizeof(elbow_src)), single_src, sizeof(single_src));

    srandom((unsigned) time(NULL));
}

struct state_set *parseBoard(const char *str, size_t *width, size_t *height)
{
    size_t chars_since_newline = 0;
    *width = 0;
    *height = 0;
    for (const char *s = str; *s; s++, chars_since_newline++) {
        if (*s == '\n') {
            if (*width && chars_since_newline != *width)
                return NULL;
            *width = chars_since_newline;
            chars_since_newline = -1;
        }
        else if (!chars_since_newline)
            (*height)++;
    }
    struct state_set *board = malloc(*width * *height * sizeof(struct state_set));
    if (!board)
        return NULL;
    
    for (size_t i = 0; *str; str++, i++) {
        switch (*str) {
        case 't':
            board[i] = (struct state_set) { .array = split, .possible = in_out_none, .bitmask = (1 << 12) - 1 };
            break;
        case 'p':
            board[i] = (struct state_set) { .array = pipe, .possible = in_out_none, .bitmask = (1 << 4) - 1 };
            break;
        case 'l':
            board[i] = (struct state_set) { .array = elbow, .possible = in_out_none, .bitmask = (1 << 8) - 1 };
            break;
        case 's':
            board[i] = (struct state_set) { .array = sink, .possible = in_none, .bitmask = (1 << 4) - 1 };
            break;
        case 'T':
            board[i] = (struct state_set) { .array = split_src, .possible = out_none, .bitmask = (1 << 4) - 1 };
            break;
        case 'P':
            board[i] = (struct state_set) { .array = pipe_src, .possible = out_none, .bitmask = (1 << 2) - 1 };
            break;
        case 'L':
            board[i] = (struct state_set) { .array = elbow_src, .possible = out_none, .bitmask = (1 << 4) - 1 };
            break;
        case 'S':
            board[i] = (struct state_set) { .array = single_src, .possible = out_none, .bitmask = (1 << 4) - 1 };
            break;
        case 'e':
            board[i] = (struct state_set) { .array = edge, .possible = {{ NONE, NONE, NONE, NONE }}, .bitmask = 1 };
            break;
        case 'a':
            board[i] = (struct state_set) { .array = all_pipes, .possible = in_out_none, .bitmask = (1 << 28) - 1 };
            break;
        case 'A':
            board[i] = (struct state_set) { .array = all_src, .possible = out_none, .bitmask = (1 << 14) - 1 };
            break;
        case '\n':
            i--;
            continue;
        default:
            free(board);
            return NULL;
        }
    }
    return board;
}

void printCell(union cell_state c)
{
    const wchar_t *lut = 
        L" ╵╹╶└┖╺┕┗╷│╿┌├┞┍┝┡╻╽┃┎┟┠┏┢┣"
        L"╴┘┚─┴┸╼┶┺┐┤┦┬┼╀┮┾╄┒┧┨┰╁╂┲╆╊"
        L"╸┙┛╾┵┹━┷┻┑┥┩┭┽╃┯┿╇┓┪┫┱╆╉┳╈╋";

    int n = (c.n & IN) ? 2 : ((c.n & OUT) ? 1 : 0);
    int e = (c.e & IN) ? 2 : ((c.e & OUT) ? 1 : 0);
    int s = (c.s & IN) ? 2 : ((c.s & OUT) ? 1 : 0);
    int w = (c.w & IN) ? 2 : ((c.w & OUT) ? 1 : 0);

    printf("%lc%lc", lut[n + 3 * e + 9 * s + 27 * w], 
            (c.e & IN) ? L'━' : ((c.e & OUT) ? L'─' : L' '));
}

void printResult(struct state_set *board)
{
    setlocale(LC_ALL, "");

    for (int y = 0; y < dim_y; y++) { 
        for (int x = 0; x < dim_x; x++) { 
            printCell(board[x + y * dim_x].possible);
        }
        puts("");
    }
}

