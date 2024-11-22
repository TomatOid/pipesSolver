#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <stdio.h>
#include <limits.h>
#include <curl/curl.h>
#include <regex.h>
#include <string.h>
#include <time.h>
#include <getopt.h>
#include <errno.h>
#include <ctype.h>
#include <sys/times.h>
#include <sys/types.h>
#include "board_util.h"
#include "netcode.h"
#include "solve.h"
//#include <unistd.h>

int animate = 0;
int animate_delay_us = 1000;

size_t dim_x, dim_y;
CURL *curl;

// make public so it is availiable for the free callback
struct write_string wstr = { 0 };

void exit_cleanup()
{
    curl_easy_cleanup(curl);
    cleanup_write(&wstr);
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "invalid usage, try: %s <size> [-(a)nimate [delay_in_us]] [--(u)ser user_email]\n", argv[0]);
        return 1;
    }

    struct option long_options[] = {
        { .name = "animate", .has_arg = optional_argument, .flag = NULL, .val = 'a' },
        { .name = "user"   , .has_arg = required_argument, .flag = NULL, .val = 'u' },
        { .name = 0        , .has_arg = 0                , .flag = NULL, .val = 0   }
    };

    char *online_size = argv[1];
    // verify that this is a valid board size string
    int size_len = strlen(online_size);
    if (size_len > 2 || size_len < 1) {
        fputs("Invalid board size\n", stderr);
        exit(EXIT_FAILURE);
    }
    if (size_len == 2) {
        if (online_size[0] != '1' || !isdigit(online_size[1])) {
            fputs("Invalid board size\n", stderr);
            exit(EXIT_FAILURE);
        }
    } else if (size_len == 1) {
        if (!isdigit(online_size[0])) {
            fputs("Invalid board size\n", stderr);
            exit(EXIT_FAILURE);
        }
    }

    // only submit puzzle to hall of fame when not null
    char *online_user = NULL;

    int c;
    while ((c = getopt_long_only(argc - 1, argv + 1, "a::u:", long_options, NULL)) != -1) {
        switch (c) {
            case 'a':
                animate = 1;
                if (optarg) {
                    char *endptr = NULL;
                    errno = 0;
                    long long_delay = strtol(optarg, &endptr, 10);
                    if (errno != 0 || long_delay < 0 || long_delay > 1000000) {
                        fprintf(stderr, "invalid value for animation delay\n");
                        return 1;
                    }
                    animate_delay_us = long_delay;
                }
                break;
            case 'u':
                online_user = optarg;
                break;
        }
    }

    initArrays();

    regex_t task_reg;
    if (regcomp(&task_reg, "var task = '[1-9a-e]+';", REG_EXTENDED))
        return 1;

    regex_t width_reg;
    if (regcomp(&width_reg, "puzzleWidth: [0-9]+[ ,]", REG_EXTENDED))
        return 1;

    regex_t height_reg;
    if (regcomp(&height_reg, "puzzleHeight: [0-9]+[ ,]", REG_EXTENDED))
        return 1;

    regex_t param_reg;
    if (regcomp(&param_reg, "input type=\"hidden\" name=\"param\" value=\"[^\"]*\"", REG_EXTENDED))
        return 1;

    regex_t magic_reg;
    if (regcomp(&magic_reg, "input type=\"hidden\" name=\"solparams\" value=\"[^\"]*\"", REG_EXTENDED))
        return 1;

    CURLcode res;

    curl_global_init(CURL_GLOBAL_ALL);
    
    curl = curl_easy_init();
    atexit(exit_cleanup);

    if (curl) {
        char url_buf[40];
        snprintf(url_buf, sizeof(url_buf), "https://www.puzzle-pipes.com/?size=%s", online_size);
        curl_easy_setopt(curl, CURLOPT_URL, url_buf);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &wstr);

        //curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_NONE);

        int res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "Could not connect to puzzle webpage\n");
            exit(EXIT_FAILURE);
        }

        regmatch_t pmatch[2];
        regmatch_t qmatch[2];
        regmatch_t wmatch[2];
        regmatch_t hmatch[2];

        if (regexec(&task_reg, wstr.str, 2, pmatch, 0) != 0) {
            fprintf(stderr, "Could not find task string in page\n");
            exit(EXIT_FAILURE);
        }

        if (regexec(&param_reg, wstr.str, 2, qmatch, 0) != 0) {
            fprintf(stderr, "Could not find the submit paramater in page\n");
            exit(EXIT_FAILURE);
        }

        if (regexec(&width_reg, wstr.str, 2, wmatch, 0) != 0) {
            fprintf(stderr, "Could not find the puzzle width in page\n");
            exit(EXIT_FAILURE);
        }

        if (regexec(&height_reg, wstr.str, 2, hmatch, 0) != 0) {
            fprintf(stderr, "Could not find the puzzle height in page\n");
            exit(EXIT_FAILURE);
        }

        // The task string has the puzzle data in it
        wstr.str[pmatch[0].rm_eo - 2] = 0;
        char *task_match = wstr.str + pmatch[0].rm_so + 12;
        
        // We will need this token when we submit the puzzle so the server
        // can recall which puzzle the our solution corresponds to
        wstr.str[qmatch[0].rm_eo - 1] = 0;
        char *param_match = wstr.str + qmatch[0].rm_so + 40;

        // set width and height based on the website.
        wstr.str[wmatch[0].rm_eo - 1] = 0;
        dim_x = atoi(wstr.str + wmatch[0].rm_so + 13);

        wstr.str[hmatch[0].rm_eo - 1] = 0;
        dim_y = atoi(wstr.str + hmatch[0].rm_so + 14);

        struct state_set *s = parseOnlineBoard(task_match, strlen(online_size) > 1);
        if (!s) {
            fprintf(stderr, "Could not parse board input, API changed or malloc() failed\n");
            exit(EXIT_FAILURE);
        }

        struct timespec begin, end;
        clock_gettime(CLOCK_MONOTONIC_RAW, &begin);
        int result_code = solve(s);
        clock_gettime(CLOCK_MONOTONIC_RAW, &end);

        if (result_code) {
            printf("Solved successfully in %fs\n", (end.tv_nsec - begin.tv_nsec) / 1000000000.0 + (end.tv_sec  - begin.tv_sec));
            printf("return code: %d\n", result_code);
        } else {
            puts("Board solving failed");
            exit(EXIT_FAILURE);
        }

        serializeBoardToOnline(s, task_match);
        char *zeros = malloc(dim_x * dim_y + 1);
        if (!zeros)
            exit(EXIT_FAILURE);
        memset(zeros, '0', dim_x * dim_y);
        zeros[dim_x * dim_y] = 0;

        curl_easy_setopt(curl, CURLOPT_POST, 1L);

        char *param_encoded = url_encode(param_match);

        char header_buf[4096];
        snprintf(header_buf, sizeof(header_buf), 
                "jstimer=0&jsPersonalTimer=&jstimerPersonal=1&stopClock=0&fromSolved=0&robot=1&zoomSlider=1&"
                "jsTimerShow=&jsTimerShowPersonal=&b=1&size=%s&param=%s&w=%zu&h=%zu&ansH=%s%%3A%s&ready=+++Done+++",
                online_size, param_encoded, dim_x, dim_y, task_match, zeros);

        free(param_encoded);

        curl_easy_setopt(curl, CURLOPT_URL, "https://www.puzzle-pipes.com/");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, header_buf);
        //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);

        free(wstr.str);
        wstr = (struct write_string) { 0 };

        // timer stops after this POST completes
        res = curl_easy_perform(curl);

        printResult(s);

        if (res != CURLE_OK) {
            fprintf(stderr, "Could not connect to webpage to submit puzzle\n");
            exit(EXIT_FAILURE);
        }

        if (regexec(&magic_reg, wstr.str, 2, pmatch, 0) != 0) {
            fprintf(stderr, "Could not submit puzzle\n");
            exit(EXIT_FAILURE);
        }

        wstr.str[pmatch[0].rm_eo - 1] = 0;
        char *magic_match = wstr.str + pmatch[0].rm_so + 44;

        if (!online_user) {
            puts("No email provided, not submitting to hall of fame");
            exit(EXIT_SUCCESS);
        }
        
        char *solparams_encoded = url_encode(magic_match);
        char *email_encoded = url_encode(online_user);

        snprintf(header_buf, sizeof(header_buf), "submitscore=1&solparams=%s&robot=1&email=%s", solparams_encoded, email_encoded);

        free(solparams_encoded);
        free(email_encoded);

        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_URL, "https://www.puzzle-pipes.com/hallsubmit.php");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, header_buf);
        rewind_write(&wstr);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &wstr);


        if (curl_easy_perform(curl) != CURLE_OK) {
            fprintf(stderr, "Could not submit to hall of fame\n");
            exit(EXIT_FAILURE);
        }

        char *found;
        
        if ((found = strstr(wstr.str, "Congratulations! You have solved the puzzle in "))) {
            found += 47;
            if (strlen(found) < 9)
                puts("Successfully submitted puzzle to hall of fame, server measured time unknown");
            else {
                found[9] = '\0';
                printf("Successfully submitted puzzle to hall of fame, server measured time: %s\n", found);
            }
        } else {
            fprintf(stderr, "Puzzle could not be submitted to hall of fame, perhaps an unregistered or invalid email, or a special daily puzzle\n");
            exit(EXIT_FAILURE);
        }
        
        exit(EXIT_SUCCESS);
    }
// Weird test case:
// 4c4482d83468328155cb26dd4d2ea63ed1aa2da8e2d5115eb3be9626be59293b1bd25925141378dc4a73ea95d88dc2a45a9691d7819977eb7ebdcd288dbe75e88b3dbb9967392d8375919215bde621addaad1cdb45a3ae995372aa732833b5a9222aaa9d128db2e26744683ab44887568
}

