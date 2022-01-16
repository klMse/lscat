#include "constants.h"
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
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

void log_perror(int err, char *msg, ...) {
    size_t msg_len = strlen(msg);
    char *buf = malloc(msg_len * 2);
    va_list ap;
    va_start(ap, msg);
    vsnprintf(buf, msg_len * 2, msg, ap);
    va_end(ap);
    if (err) {
        char *string_error = strerror(err);
        fprintf(stderr, "[Error]: %s: %s\n", buf, string_error);
    } else {
        fprintf(stderr, "[Error]: %s\n", msg);
    }
    free(msg);
}

void log_error(char *msg, ...) {
    va_list ap;
    va_start(ap, msg);
    log_perror(0, msg, ap);
    va_end(ap);
}

char **string_to_args(char *str) {

    return NULL;
}

int parse_option(char *option_str) {

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
        if (line[read - 1] == '\n')
            line[--read] = '\0';
        if (line[0] == '\0' || line[0] == '#')
            continue;
        parse_option(line);
    }

    if (line)
        free(line);
    fclose(fp);
    return 0;
}
