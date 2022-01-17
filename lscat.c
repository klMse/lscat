#define _GNU_SOURCE
#include "constants.h"
#include "logging.h"
#include "utils.h"
#include <errno.h>
#include <sched.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <wordexp.h>

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

enum exec_types { T_FILE, T_DIR, T_NULL };
typedef struct {
    char *path;
    enum exec_types type;
} arg_t;
static arg_t *entries;

void parse_option(char *option_str) {
    char *ptr = option_str;
    while (*ptr != '\0' && *ptr++ != '=') { }
    if (*ptr == '\0') {
        return;
    }

    const size_t key_length = ptr - option_str - 2;
    const char *value = option_str + key_length + 2;
    // For now it's this
    if (strncmp("FILE_EXEC", option_str, key_length) == 0) {
        config.file_exec_path = strdup(value);
    } else if (strncmp("DIR_EXEC", option_str, key_length) == 0) {
        config.dir_exec_path = strdup(value);
    } else if (strncmp("FILE_ARGS", option_str, key_length) == 0) {
        config.file_exec_args = string_to_argv(value);
    } else if (strncmp("DIR_ARGS", option_str, key_length) == 0) {
        config.dir_exec_args = string_to_argv(value);
    } else if (strncmp("MULTIPLE_CALLS", option_str, key_length) == 0) {
        if (strcmp(value, "true") == 0) {
            config.multiple_calls = true;
        } else if (strcmp(value, "false") == 0) {
            config.multiple_calls = false;
        } else {
            log_info("Value \"%s\" of key \"MULTIPLE_CALL\" is unkown", value);
        }
    } else {
        char *key = strndup(option_str, key_length);
        log_info("Key \"%s\" in config unkown", key);
        free(key);
    }
    return;
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

void parse_config(const char* const config_path) {
    FILE *fp = fopen(config_path, "r");
    if (fp == NULL) {
        log_perror(errno, "Open of config at \"%s\" failed", config_path);
        return;
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
    return;
}

void sanitize_config() {
    if (!config.file_exec_path) {
        config.file_exec_path = DEFAULT_FILE_EXEC;
    }
    if (!config.dir_exec_path) {
        config.dir_exec_path = DEFAULT_DIR_EXEC;
    }
    if (!config.file_exec_args) {
        config.file_exec_args = malloc(sizeof(char*));
        config.file_exec_args = NULL;
    }
    if (!config.dir_exec_args) {
        config.dir_exec_args = malloc(sizeof(char*));
        config.dir_exec_args = NULL;
    }
}

void convert_argv(int argc, char **argv) {
    int retval;
    size_t index = 0;
    struct stat buf;
    for (size_t i = 0; i < (size_t) argc; ++i) {
        retval = stat(argv[i], &buf);
        if (retval < 0) {
            if (errno == ENOENT) {
                log_perror(errno, "\"%s\"", argv[i]);
            } else {
                log_perror(errno, "stat failed for file/dir \"%s\"", argv[i]);
            }
            continue;
        }
        entries[index].path = strdup(argv[i]);
        if (S_ISDIR(buf.st_mode)) {
            entries[index].type = T_DIR;
        } else {
            entries[index].type = T_FILE;
        }
        ++index;
    }
    entries[index].path = NULL;
    entries[index].type = T_NULL;
}

int run_child(void* ptr) {
    char **argv = ptr;
    int retval = execv(argv[0], argv);
    if (retval < 0) {
        log_perror(errno, "execve failed");
    }
    // Only reached in case of an error
    return -1;
}

void run(char *child_argv[]) {
    pid_t pid;
    void *stack = mmap(NULL, STACKSIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_STACK, -1, 0);
    if (stack == MAP_FAILED) {
        log_perror(errno, "mmap failed");
        exit(-1);
    }
    pid = clone(run_child, stack + STACKSIZE, SIGCHLD, child_argv);
    if (pid < 0) {
        log_perror(errno, "clone failed");
        exit(-1);
    }
    waitpid(pid, NULL, 0);
    int retval = munmap(stack, STACKSIZE);
    if (retval < 0) {
        log_perror(errno, "munmap failed");
        exit(-1);
    }
}

void execute_entries() {
    size_t file_exec_args_length = 0, dir_exec_args_length = 0;

    char **ptr = config.file_exec_args;
    while (ptr && *ptr++) {
        ++file_exec_args_length;
    }
    ptr = config.dir_exec_args;
    while (ptr && *ptr++) {
        ++dir_exec_args_length;
    }

    if (config.multiple_calls) {
        char **child_argv_file = calloc(file_exec_args_length + 3, sizeof(char*));
        char **child_argv_dir = calloc(dir_exec_args_length + 3, sizeof(char*));
        child_argv_file[0] = config.file_exec_path;
        child_argv_dir[0] = config.dir_exec_path;
        child_argv_file[file_exec_args_length + 2] = NULL;
        child_argv_dir[dir_exec_args_length + 2] = NULL;
        if (file_exec_args_length) {
            memcpy(child_argv_file + 1, config.file_exec_args, sizeof(char*) * file_exec_args_length);
        }
        if (dir_exec_args_length) {
            memcpy(child_argv_dir + 1, config.dir_exec_args, sizeof(char*) * dir_exec_args_length);
        }
        arg_t *arg_ptr = entries;

        while (arg_ptr->type != T_NULL) {
            if (arg_ptr->type == T_DIR) {
                child_argv_dir[dir_exec_args_length + 1] = arg_ptr->path;
                run(child_argv_dir);
            } else if (arg_ptr->type == T_FILE) {
                child_argv_file[file_exec_args_length + 1] = arg_ptr->path;
                run(child_argv_file);
            }
            ++arg_ptr;
        }
        free(child_argv_dir);
        free(child_argv_file);
    } else {
        size_t file_index = 0, dir_index = 0;
        size_t file_counter = 0, dir_counter = 0;
        arg_t *arg_ptr = entries;
        while (arg_ptr->type != T_NULL) {
            if (arg_ptr->type == T_FILE) {
                ++file_counter;
            } else if (arg_ptr->type == T_DIR) {
                ++dir_counter;
            }
            ++arg_ptr;
        }

        char **child_argv_dir, **child_argv_file;
        arg_ptr = entries;

        if (dir_counter) {
            child_argv_dir = calloc(dir_counter + dir_exec_args_length + 2, sizeof(char*));
            child_argv_dir[0] = config.dir_exec_path;
            child_argv_dir[dir_exec_args_length + dir_counter + 1] = NULL;
            if (dir_exec_args_length) {
                memcpy(child_argv_dir + 1, config.dir_exec_args, sizeof(char*) * dir_exec_args_length);
            }
            while (arg_ptr->type != T_NULL) {
                if (arg_ptr->type == T_DIR) {
                    child_argv_dir[dir_exec_args_length + dir_index + 1] = arg_ptr->path;
                    ++dir_index;
                }
                ++arg_ptr;
            }
            run(child_argv_dir);
            free(child_argv_dir);
        }
        if (file_counter) {
            child_argv_file = calloc(file_counter + file_exec_args_length + 2, sizeof(char*));
            child_argv_file[0] = config.file_exec_path;
            child_argv_file[file_exec_args_length + file_counter + 1] = NULL;
            if (file_exec_args_length) {
                memcpy(child_argv_file + 1, config.file_exec_args, sizeof(char*) * file_exec_args_length);
            }
            while (arg_ptr->type != T_NULL) {
                if (arg_ptr->type == T_FILE) {
                    child_argv_file[file_exec_args_length + file_index + 1] = arg_ptr->path;
                    ++file_index;
                }
                ++arg_ptr;
            }
            run(child_argv_file);
            free(child_argv_file);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        entries = calloc(2, sizeof(arg_t));
        entries[0].path = ".";
        entries[0].type = T_DIR;
        entries[1].path = NULL;
        entries[1].type = T_NULL;
    } else {
        entries = calloc(argc, sizeof(arg_t));
        convert_argv(argc - 1, argv + 1);
    }

    const char *config_path = get_config_path();
    if (config_path)
        parse_config(config_path);
    sanitize_config();
    execute_entries();
}