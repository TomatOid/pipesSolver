#pragma once

// used by the curl callbacks
struct write_string {
    char *str;
    size_t len;
    size_t size;
};

// make public so it is availiable for the free callback
extern struct write_string wstr;

// Parse from the online alphanumeric format to our internal one
// Do note that this allocates space
// returns NULL on failure
struct state_set *parseOnlineBoard(const char *str, int is_wrap);

// Reverse the operation, convert from internal board format to online format
// IMPORTANT: please ensure that str has enough allocated space before calling
void serializeBoardToOnline(struct state_set *board, char *str);

// Replaces naughty characters with their corresponding url escape sequences
// Do note that the return value points to a heap allocation
// returns NULL on failure
char *url_encode(char *str);

// Callback for cURL, expands or creates allocation automatically
size_t write_callback(char *ptr, size_t size, size_t nmemb, struct write_string *user_data);

// Rewind the cURL write buffer without freeing
void rewind_write(struct write_string *write_str);

// Resets the cURL write buffer, frees memory
void cleanup_write(struct write_string *write_str);
