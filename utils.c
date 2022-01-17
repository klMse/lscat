#include <string.h>
#include <stdlib.h>
#include "logging.h"
#include "utils.h"

char **string_to_argv(const char * const p_string) {
    char *str = strdup(p_string);
    // Trim first
    size_t index = strlen(str) - 1;
    while (*(str + index) == ' ') {
        *(str + index) = '\0';
        --index;
    }

    size_t spaces_count = 0;
    char *ptr = str;

    while (*ptr != '\0') {
        if (*ptr == ' ') {
            ++spaces_count;
            while (*(ptr + 1) == ' ') {
                ++ptr;
            }
        }
        ++ptr;
    }

    char **ret = calloc(spaces_count + 2, sizeof(char*));
    if (ret == NULL) {
        log_error("malloc failed");
        exit(-1);
    }
    ret[spaces_count + 1] = NULL;

    char *ptr_prev = str;
    ptr = str;
    index = 0;

    while (*ptr != '\0') {
        if (*ptr == ' ') {
            ret[index] = strndup(ptr_prev, ptr - ptr_prev);
            while(*(ptr + 1) == ' ') {
                ++ptr;
            }
            ptr_prev = ptr + 1;
            ++index;
        }
        ++ptr;
    }
    // Copy last one in
    ret[index] = strndup(ptr_prev, ptr - ptr_prev);
    free(str);
    return ret;
}

void find_path(char **str) {
    // TODO Later
}