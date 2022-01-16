#include "constants.h"
#include "logging.h"
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <wordexp.h>
#include <unistd.h>

// clang-format off
static struct {
    char *file_exec_path;
    char **file_exec_args;
    char *dir_exec_path;
    char **dir_exec_args;
    bool multiple_calls;
} config = {
    .file_exec_path = NULL,
    .file_exec_args = NULL,
    .dir_exec_path = NULL,
    .dir_exec_args = NULL,
    .multiple_calls = false
};


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

    return ret;
}

int parse_option(char *option_str) {
    char *ptr = option_str;
    while (*ptr != '\0' && *ptr++ != '=') { }
    const size_t key_length = ptr - option_str;
    // For now it's this
    if (strncmp("IF_FILE_EXEC", option_str, key_length) == 0) {

    } else if (strncmp("IF_DIR_EXEC", option_str, key_length) == 0) {
    
    } else if (strncmp("IF_FILE_ARGS", option_str, key_length) == 0) {

    } else if (strncmp("IF_DIR_ARGS", option_str, key_length) == 0) {

    } else if (strncmp("SINGLE_CALL", option_str, key_length) == 0) {

    } else {
        char *key = strndup(option_str, key_length);
        log_info("Key \"%s\" in config unkown", key);
        free(key);
    }
    return 0;
}

char *get_config_path() {
    // In case this has to be expanded
    static const char *paths[] = {
        "$HOME/.config/lscat/config"
    };

    char *ret_path;
    int retval;
    wordexp_t we;
    for (size_t i = 0; i < sizeof(paths) / sizeof(char*); ++i) {
        retval = wordexp(paths[i], &we, 0);
        if (retval == 0) {
            retval = access(we.we_wordv[0], R_OK);
            if (retval == 0) {
                ret_path = strdup(we.we_wordv[0]);
                wordfree(&we);
                return ret_path;
            } else {
                wordfree(&we);
            }
        }
    }
    return NULL;
}

int parse_config(char *config_path) {
    if (!config_path) {
        config.file_exec_path = DEFAULT_FILE_EXEC;
        config.dir_exec_path = DEFAULT_DIR_EXEC;
    }
    FILE *fp = fopen(config_path, "r");
    if (fp == NULL) {
        log_perror(errno, "Open of config at \"%s\" failed", config_path);
        return -1;
    }

    char *line = NULL;
    size_t line_length = 0;
    ssize_t read;
    while ((read = getline(&line, &line_length, fp)) > 0) {
        if (line[read - 1] == '\n') {
            line[read - 1] = '\0';
        }
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }
        parse_option(line);
    }

    if (line)
        free(line);
    fclose(fp);
    return 0;
}

int run_child(void *ptr) {
    
}

int run(char *child_argv[]) {

}

int main(int argc, char *argv[]) {

}