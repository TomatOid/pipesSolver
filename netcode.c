#include "board_util.h"
#include "netcode.h"
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <curl/curl.h>

struct state_set *parseOnlineBoard(const char *str, int is_wrap)
{
    if (dim_x * dim_y != strlen(str)) {
        return NULL;
    }
    dim_x += !is_wrap;
    dim_y += !is_wrap;

    size_t middle = (dim_x - !is_wrap) / 2;
    middle += dim_y * middle;

    struct state_set *board = malloc(dim_x * dim_y * sizeof(struct state_set));
    if (!board)
        return NULL;
    
    for (size_t i = 0; i < dim_x * dim_y; str++, i++) {
        // Adds the implicit empty cells for non-wrap puzzles
        if (!is_wrap && ((i + 1) % dim_x == 0 || i / dim_x >= dim_y - 1)) {
            board[i] = (struct state_set) { .array = edge, .possible = {{ NONE, NONE, NONE, NONE }}, .bitmask = 1 };
            str--;
            continue;
        }
        if (i == middle) {
            switch (*str) {
            case '7':
            case 'b':
            case 'd':
            case 'e':
                board[i] = (struct state_set) { .array = split_src, .possible = out_none, .bitmask = (1 << 4) - 1 };
                break;
            case '5':
            case 'a':
                board[i] = (struct state_set) { .array = pipe_src, .possible = out_none, .bitmask = (1 << 2) - 1 };
                break;
            case '3':
            case '6':
            case '9':
            case 'c':
                board[i] = (struct state_set) { .array = elbow_src, .possible = out_none, .bitmask = (1 << 4) - 1 };
                break;
            case '1':
            case '2':
            case '4':
            case '8':
                board[i] = (struct state_set) { .array = single_src, .possible = out_none, .bitmask = (1 << 4) - 1 };
                break;
            default:
                free(board);
                return NULL;
            }
            continue;
        }

        switch (*str) {
        case '7':
        case 'b':
        case 'd':
        case 'e':
            board[i] = (struct state_set) { .array = split, .possible = in_out_none, .bitmask = (1 << 12) - 1 };
            break;
        case '5':
        case 'a':
            board[i] = (struct state_set) { .array = pipe, .possible = in_out_none, .bitmask = (1 << 4) - 1 };
            break;
        case '3':
        case '6':
        case '9':
        case 'c':
            board[i] = (struct state_set) { .array = elbow, .possible = in_out_none, .bitmask = (1 << 8) - 1 };
            break;
        case '1':
        case '2':
        case '4':
        case '8':
            board[i] = (struct state_set) { .array = sink, .possible = in_none, .bitmask = (1 << 4) - 1 };
            break;
        default:
            free(board);
            return NULL;
        }
    }
    return board;
}

void serializeBoardToOnline(struct state_set *board, char *str)
{
    int size = dim_x * dim_y;
    for (int i = 0; i < size; i++) { 
        if (board[i].array != edge) {
            int final = !(board[i].possible.e & NONE) | !(board[i].possible.n & NONE) << 1
                | !(board[i].possible.w & NONE) << 2 | !(board[i].possible.s & NONE) << 3;
            int initial = (*str <= '9') ? *str - '0' : *str - 'a' + 10;
            int num = 0;
            for (; final != initial; num++)
                initial = 0xf & (initial << 1 | initial >> 3);
            *(str++) = (num < 10) ? '0' + num : 'a' + num - 10;
        }
    }
    *str = 0;
}

char *url_encode(char *str) 
{
    // always long enough
    char *buf = calloc(3 * strlen(str) + 1, 1);
    if (!buf)
        return NULL;
    for (int i = 0, j = 0; str[i]; i++, j++) {
        if (strchr(" <>#%+{}|\\^~[]`;/?:@=&$", str[i])) {
            buf[j++] = '%';
            buf[j++] = (str[i] >> 4 > 9) ? (str[i] >> 4) + 'A' - 10 : (str[i] >> 4) + '0';
            buf[j] = ((str[i] & 15) > 9) ? (str[i] & 15) + 'A' - 10 : (str[i] & 15) + '0';
        }
        else {
            buf[j] = str[i];
        }
    }
    return buf;
}

size_t write_callback(char *ptr, size_t size, size_t nmemb, struct write_string *user_data)
{
    size_t real_size = size * nmemb;
    if (user_data->size < real_size + user_data->len + 1) {
        size_t new_size = CURL_MAX_WRITE_SIZE;
        while (new_size < real_size + user_data->len + 1)
            new_size *= 2;

        user_data->str = realloc(user_data->str, new_size);
        if (!user_data->str)
            return 0;

        user_data->size = new_size;
    }
    memcpy(user_data->str + user_data->len, ptr, real_size);
    user_data->len += real_size;
    user_data->str[user_data->len] = 0;
    return real_size;
}

void rewind_write(struct write_string *write_str)
{
    write_str->len = 0;
}

void cleanup_write(struct write_string *write_str)
{
    if (write_str->str)
        free(write_str->str);
    *write_str = (struct write_string){ 0 };
}
