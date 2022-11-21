#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <stdio.h>
#include <limits.h>
#include <locale.h>
#include <curl/curl.h>
#include <regex.h>
#include <string.h>
#include <math.h>
//#include <unistd.h>

#define ANIMATE

int usleep(uint64_t usec);

size_t dim_x, dim_y;

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
    uint16_t bitmask;
};

union cell_state split[12] = {
    { IN, OUT, NONE, OUT },
    { OUT, OUT, NONE, IN },
    { OUT, IN, NONE, OUT },
};

union cell_state pipe[4] = {
    { NONE, OUT, NONE, IN },
};

union cell_state elbow[8] = {
    { IN, OUT, NONE, NONE },
    { OUT, IN, NONE, NONE },
};

union cell_state sink[4] = {
    { IN, NONE, NONE, NONE },
};

union cell_state split_src[4] = {
    { OUT, OUT, NONE, OUT },
};

union cell_state pipe_src[2] = {
    { NONE, OUT, NONE, OUT },
    { OUT, NONE, OUT, NONE },
};

union cell_state elbow_src[4] = {
    { OUT, OUT, NONE, NONE },
};

union cell_state single_src[4] = {
    { OUT, NONE, NONE, NONE },
};

union cell_state edge[1] = {
    { NONE, NONE, NONE, NONE },
};

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
            board[i] = (struct state_set) { .array = edge, .possible = { NONE, NONE, NONE, NONE }, .bitmask = 1 };
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
    b.bits = (b.bits << 2) & 04444 | b.bits & 02222 | (b.bits >> 2) & 01111;
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

void DFSCheck(struct state_set *board, uint32_t *explore, int x, int y)
{
    struct state_set current = board[x + y * dim_x];
    uint32_t *mark = &explore[x + y * dim_x];

    if (*mark == SPECIAL) {
        return;
    }

    *mark = SPECIAL;

    int n_y = (y - 1 + dim_y) % dim_y;
    int s_y = (y + 1) % dim_y;
    int e_x = (x + 1) % dim_x;
    int w_x = (x - 1 + dim_x) % dim_x;

    if (current.possible.n & 5) {
        DFSCheck(board, explore, x, n_y);
    }
    if (current.possible.e & 5) {
        DFSCheck(board, explore, e_x, y);
    }
    if (current.possible.s & 5) {
        DFSCheck(board, explore, x, s_y);
    }
    if (current.possible.w & 5) {
        DFSCheck(board, explore, w_x, y);
    }
}    

int solve(struct state_set *board)
{
#ifdef ANIMATE
    system("clear");
#endif
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
    srandom(31542);

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
#ifdef ANIMATE
            printf("\033[0;0H");
            printResult(board);
            usleep(15000);
#endif
            
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
            struct state_set *new_board = calloc(sizeof(struct state_set), size);
            memcpy(new_board, board, sizeof(struct state_set) * size);

            // collapse orientation of a pipe but not flow direction
            uint16_t new_mask;
            int rand_pool;
            struct state_set *curr;
            do {
                rand_pool = random() % explored_len;
                curr = &new_board[pool[rand_pool].x + pool[rand_pool].y * dim_x];
                int j = __builtin_ctz((int)curr->bitmask);
                new_mask = 1 << j;
                int j_max = CHAR_BIT * sizeof(int) - __builtin_clz((int)curr->bitmask);
                uint16_t first = curr->array[j].bits & 02222;
                
                for (; j < j_max; j++)
                    if (curr->array[j].bits & 02222 == first)
                        new_mask |= 1 << j;

            } while (__builtin_popcount((int)curr->bitmask <= 1));

            curr->bitmask = new_mask;

            if (solve(new_board)) {
                memcpy(board, new_board, size * sizeof(struct state_set));
                free(new_board);
                goto success;
            }
            else {
                board[pool[rand_pool].x + pool[rand_pool].y * dim_x].bitmask &= ~new_mask;
                pool_len += explored_len;
                explored_len = 0;
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
    return 1;
fail:
    free(pool);
    free(queue);
    free(explore);
    return 0;
}

#define array_len(x) ((sizeof x) / (sizeof *x))

// puzzle id 2,796,944
const char *test = 
    "ssss\n"
    "tptt\n"
    "lsPp\n"
    "stts\n";

const char *test_1 =
    "tsllslt\n"
    "ttlllsl\n"
    "ppsllss\n"
    "tllTlss\n"
    "lpttlst\n"
    "stttttl\n"
    "sssspss\n";

const char *test_2 =
    "ttsslptlst\n"
    "slstslsssl\n"
    "lsltstslpt\n"
    "ptlttpllls\n"
    "tsltltslls\n"
    "pspssTttts\n"
    "lllstpsstt\n"
    "lssstpstll\n"
    "lltpttssps\n"
    "lptssttstt\n";

const char *test_inf =
    "ll\n"
    "ll\n";

struct write_string {
    char *str;
    size_t len;
    size_t size;
};

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

struct state_set *parseOnlineBoard(const char *str, size_t *width, size_t *height, int is_wrap)
{
    *width = (size_t)sqrt(strlen(str)) + !is_wrap;
    *height = *width;

    size_t middle = (*width - !is_wrap) / 2;
    middle += *height * middle;

    struct state_set *board = malloc(*width * *height * sizeof(struct state_set));
    if (!board)
        return NULL;
    
    for (size_t i = 0; i < *width * *height; str++, i++) {
        if (!is_wrap && ((i + 1) % *width == 0 || i / *width >= *height - 1)) {
            board[i] = (struct state_set) { .array = edge, .possible = { NONE, NONE, NONE, NONE }, .bitmask = 1 };
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

void boardOutput(struct state_set *board, char *str)
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

#define SIZE "8"

int main()
{
    assert(sizeof(union cell_state) == sizeof(uint16_t));
    createRotations(split, array_len(split));
    createRotations(pipe, array_len(pipe));
    createRotations(elbow, array_len(elbow));
    createRotations(sink, array_len(sink));
    createRotations(split_src, array_len(split_src));
    createRotations(elbow_src, array_len(elbow_src));
    createRotations(single_src, array_len(single_src));

    regex_t task_reg;
    if (regcomp(&task_reg, "var task = '[1-9a-e]+';", REG_EXTENDED))
        return 1;

    regex_t param_reg;
    if (regcomp(&param_reg, "input type=\"hidden\" name=\"param\" value=\"[^\"]*\"", REG_EXTENDED))
        return 1;

    regex_t magic_reg;
    if (regcomp(&magic_reg, "input type=\"hidden\" name=\"solparams\" value=\"[^\"]*\"", REG_EXTENDED))
        return 1;

    CURL *curl;
    CURLcode res;

    struct write_string wstr = { 0 };

    curl_global_init(CURL_GLOBAL_ALL);
    
    curl = curl_easy_init();

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "https://www.puzzle-pipes.com/?size=" SIZE);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &wstr);

        int res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "Could not connect to puzzle webpage");
            return 1;
        }

        regmatch_t pmatch[2];
        regmatch_t qmatch[2];

        if (regexec(&task_reg, wstr.str, 2, pmatch, 0) != 0) {
            fprintf(stderr, "Could not find task string in page\n");
            return 1;
        }

        if (regexec(&param_reg, wstr.str, 2, qmatch, 0) != 0) {
            fprintf(stderr, "Could not find the submit paramater in page\n");
            return 1;
        }

        wstr.str[pmatch[0].rm_eo - 2] = 0;
        char *task_match = wstr.str + pmatch[0].rm_so + 12;

        wstr.str[qmatch[0].rm_eo - 1] = 0;
        char *param_match = wstr.str + qmatch[0].rm_so + 40;

        struct state_set *s = parseOnlineBoard(task_match, &dim_x, &dim_y, SIZE[1] != 0);
        if (!s) {
            fprintf(stderr, "Could not parse board input, API changed or malloc() failed\n");
            return 1;
        }
        
        printf("%d\n", solve(s));

        boardOutput(s, task_match);
        char *zeros = malloc(dim_x * dim_y + 1);
        if (!zeros)
            return 1;
        memset(zeros, '0', dim_x * dim_y);
        zeros[dim_x * dim_y] = 0;

        curl_easy_setopt(curl, CURLOPT_POST, 1L);

        char magic_buf[512] = { 0 };

        for (int i = 0, j = 0; j < sizeof(magic_buf) && param_match[i]; i++, j++) {
            if (strchr(" <>#%+{}|\\^~[]`;/?:@=&$", param_match[i])) {
                if (j + 3 < sizeof(magic_buf)) {
                    magic_buf[j++] = '%';
                    magic_buf[j++] = (param_match[i] >> 4 > 9) ? (param_match[i] >> 4) + 'A' - 10 : (param_match[i] >> 4) + '0';
                    magic_buf[j] = (param_match[i] & 15 > 9) ? (param_match[i] & 15) + 'A' - 10 : (param_match[i] & 15) + '0';
                }
            }
            else {
                magic_buf[j] = param_match[i];
            }
        }

        char header_buf[2048];
        snprintf(header_buf, sizeof(header_buf), "jstimer=0&jsPersonalTimer=&jstimerPersonal=1&stopClock=0&fromSolved=0&robot=1&zoomSlider=1&jsTimerShow=&jsTimerShowPersonal=&b=1&size=" SIZE "&param=%s&w=%zu&h=%zu&ansH=%s%%3A%s&ready=+++Done+++",
                magic_buf, dim_x, dim_y, task_match, zeros);

        curl_easy_setopt(curl, CURLOPT_URL, "https://www.puzzle-pipes.com/");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, header_buf);

        free(wstr.str);
        wstr = (struct write_string) { 0 };

        // timer stops after this POST completes
        res = curl_easy_perform(curl);

        setlocale(LC_ALL, "");
        printResult(s);

        if (res != CURLE_OK) {
            fprintf(stderr, "Could not connect to webpage to submit puzzle\n");
            return 1;
        }

        if (regexec(&magic_reg, wstr.str, 2, pmatch, 0) != 0) {
            fprintf(stderr, "Could not submit puzzle\n");
            return 1;
        }

        wstr.str[pmatch[0].rm_eo - 1] = 0;
        char *magic_match = wstr.str + pmatch[0].rm_so + 44;

        memset(magic_buf, 0, sizeof(magic_buf));

        for (int i = 0, j = 0; j < sizeof(magic_buf) && magic_match[i]; i++, j++) {
            if (strchr(" <>#%+{}|\\^~[]`;/?:@=&$", magic_match[i])) {
                if (j + 3 < sizeof(magic_buf)) {
                    magic_buf[j++] = '%';
                    magic_buf[j++] = (magic_match[i] >> 4 > 9) ? (magic_match[i] >> 4) + 'A' - 10 : (magic_match[i] >> 4) + '0';
                    magic_buf[j] = (magic_match[i] & 15 > 9) ? (magic_match[i] & 15) + 'A' - 10 : (magic_match[i] & 15) + '0';
                }
            }
            else {
                magic_buf[j] = magic_match[i];
            }
        }

        snprintf(header_buf, sizeof(header_buf), "submitscore=1&solparams=%s&robot=1&email=connormc757%%40gmail.com", magic_buf);

        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_URL, "https://www.puzzle-pipes.com/hallsubmit.php");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, header_buf);

        if (curl_easy_perform(curl) != CURLE_OK) {
            fprintf(stderr, "Could not submit to hall of fame\n");
            return 1;
        }
        
        return 0;
    }
// Weird test case:
// 4c4482d83468328155cb26dd4d2ea63ed1aa2da8e2d5115eb3be9626be59293b1bd25925141378dc4a73ea95d88dc2a45a9691d7819977eb7ebdcd288dbe75e88b3dbb9967392d8375919215bde621addaad1cdb45a3ae995372aa732833b5a9222aaa9d128db2e26744683ab44887568
}

