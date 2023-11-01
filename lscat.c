#define _GNU_SOURCE
#include "constants.h"
#include "logging.h"
#include <errno.h>
#include <sched.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <wordexp.h>

// clang-format off
enum exec_types { T_FILE, T_DIR, T_ARG, T_NULL };

static struct {
    char *file_exec_path;
    char **file_exec_args;
    char *dir_exec_path;
    char **dir_exec_args;
    size_t stack_size;
    enum exec_types invoked_as;
    enum exec_types mode;
} config = {
    .file_exec_path = NULL,
    .file_exec_args = NULL,
    .dir_exec_path = NULL,
    .dir_exec_args = NULL,
    .stack_size = 0,
    .invoked_as = T_NULL,
    .mode = T_NULL
};

typedef struct {
    char *string;
    enum exec_types type;
} arg_t;

static struct {
    arg_t *array;
    size_t file_counter;
    size_t dir_counter;
    size_t arg_counter;
} entries;

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
        if (retval == 0 && we.we_wordc) {
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

// Fill in missing pieces in the config
// Check once and for all if the user made some input that doesn't make sense
void sanitize_and_check_config() {
    if (!config.file_exec_path) {
        config.file_exec_path = DEFAULT_FILE_EXEC;
    }
    if (!config.dir_exec_path) {
        config.dir_exec_path = DEFAULT_DIR_EXEC;
    }
    if (!config.file_exec_args) {
        config.file_exec_args = malloc(sizeof(char*));
        *config.file_exec_args = NULL;
    }
    if (!config.dir_exec_args) {
        config.dir_exec_args = malloc(sizeof(char*));
        *config.dir_exec_args = NULL;
    }

    if (config.invoked_as == T_DIR && entries.file_counter)
        config.mode = T_NULL;
    else if (config.invoked_as == T_FILE && entries.dir_counter)
        config.mode = T_NULL;
    else
        config.mode = config.invoked_as;

    if (config.mode == T_NULL && entries.arg_counter) {
        arg_t *ptr = entries.array;
        while (ptr->type != T_NULL) {
            if (ptr->type == T_ARG) {
                log_info("Running in dir mode, ignoring argument \"%s\"", ptr->string);
            }
            ++ptr;
        }
    }

    struct rlimit limit;
    int retval = getrlimit(RLIMIT_STACK, &limit);
    if (retval < 0 || limit.rlim_cur <= 0) {
        log_perror(errno, "getrlimit failed, using default of %ld", STACKSIZE);
        config.stack_size = STACKSIZE;
    } else {
        config.stack_size = limit.rlim_cur;
    }
}

// Turn argv into arg_t in entries
void convert_argv(int argc, char **argv) {
    int retval;
    entries.array = calloc(argc, sizeof(arg_t));

    size_t index = 0;
    struct stat buf;
    for (size_t i = 0; i < (size_t) argc; ++i) {
        retval = stat(argv[i], &buf);
        if (retval < 0) {
            if (*argv[i] == '-') {
                entries.array[index].type = T_ARG;
                entries.array[index].string = strdup(argv[i]);
                ++entries.arg_counter;
                ++index;
                continue;
            }
            if (errno == ENOENT) {
                log_perror(errno, "\"%s\"", argv[i]);
            } else {
                log_perror(errno, "stat failed for file/dir \"%s\"", argv[i]);
            }
            continue;
        }

        entries.array[index].string = strdup(argv[i]);
        if (S_ISDIR(buf.st_mode)) {
            entries.array[index].type = T_DIR;
            ++entries.dir_counter;
        } else {
            entries.array[index].type = T_FILE;
            ++entries.file_counter;
        }
        ++index;
    }
    entries.array[index].string = NULL;
    entries.array[index].type = T_NULL;
}

int run_child(void* ptr) {
    char **argv = ptr;
    int retval = execvp(argv[0], argv);
    if (retval < 0) {
        log_perror(errno, "execvp failed");
    }
    // Only reached in case of an error
    return -1;
}

void run(char *child_argv[]) {
    void *stack = mmap(NULL, config.stack_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_STACK, -1, 0);
    if (stack == MAP_FAILED) {
        log_perror(errno, "mmap failed");
        exit(-1);
    }
    pid_t pid = clone(run_child, (char*) stack + config.stack_size, SIGCHLD, child_argv);
    if (pid < 0) {
        log_perror(errno, "clone failed");
        exit(-1);
    }
    waitpid(pid, NULL, 0);
    int retval = munmap(stack, config.stack_size);
    if (retval < 0) {
        log_perror(errno, "munmap failed");
        exit(-1);
    }
}

// Determines how lscat was called, generic call, lscat dir mode or lscat file mode
void determine_call_type(char *prog_name) {
    size_t prog_name_length = strlen(prog_name);
    size_t suffix_len;
    if ((suffix_len = strlen(EXEC_DIR_NAME)) <= prog_name_length) {
        if (strncmp(prog_name + prog_name_length - suffix_len, EXEC_DIR_NAME, suffix_len) == 0) {
            config.invoked_as = T_DIR;
            return;
        }
    }
    if ((suffix_len = strlen(EXEC_FILE_NAME)) <= prog_name_length) {
        if (strncmp(prog_name + prog_name_length - suffix_len, EXEC_FILE_NAME, suffix_len) == 0) {
            config.invoked_as = T_FILE;
            return;
        }
    }
    config.invoked_as = T_NULL;
}

// Takes NULL terminated char** arrays and concatenates them, adds the program name as first element
char** concatenate_char_arrays_impl(char *prog_name, size_t count, ...) {
    char ***array = calloc(count, sizeof(char**));
    char **ptr;
    size_t element_count = 0;
    va_list ap;
    va_start(ap, count);
    for (size_t i = 0; i < count; ++i) {
        ptr = va_arg(ap, char**);
        array[i] = ptr;
        while (ptr && *ptr++) {
            ++element_count;
        }
    }
    va_end(ap);
    char **ret = calloc(element_count + 2, sizeof(char*));
    size_t index = 1;
    ret[0] = prog_name;
    for (size_t i = 0; i < count; ++i)  {
        ptr = array[i];
        while (ptr && *ptr) {
            ret[index++] = *ptr++;
        }
    }
    ret[index] = NULL;
    free(array);
    return ret;
}
#define _GET_COUNT(a01, a02, a03, a04, a05, a06, a07, a08, a09, a10, a11, a12, ...) a12
#define concatenate_char_arrays(prog_name, ...) concatenate_char_arrays_impl(prog_name, _GET_COUNT(dummy, ##__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0), ##__VA_ARGS__)

void execute_entries() {
    char **child_argv;
    arg_t *entry;
    char **entries_array;
    size_t array_size;
    size_t index;
    switch (config.mode) {
    case T_NULL:
        if (entries.dir_counter) {
            index = 0;
            entries_array = calloc(entries.dir_counter + 1, sizeof(char*));
            entries_array[entries.dir_counter] = NULL;
            entry = entries.array;
            while (entry && entry->type != T_NULL) {
                if (entry->type == T_DIR) {
                    entries_array[index++] = entry->string;
                }
                ++entry;
            }
            child_argv = concatenate_char_arrays(config.dir_exec_path, config.dir_exec_args, entries_array);
            free(entries_array);
            run(child_argv);
            free(child_argv);
        }

        if (entries.file_counter) {
            index = 0;
            entries_array = calloc(entries.file_counter + 1, sizeof(char*));
            entries_array[entries.file_counter] = NULL;
            entry = entries.array;
            while (entry && entry->type != T_NULL) {
                if (entry->type == T_FILE) {
                    entries_array[index++] = entry->string;
                }
                ++entry;
            }
            child_argv = concatenate_char_arrays(config.file_exec_path, config.file_exec_args, entries_array);
            free(entries_array);
            run(child_argv);
            free(child_argv);
        }
        break;
    case T_FILE:
        array_size = entries.file_counter + entries.arg_counter;
        entries_array = calloc(array_size + 1, sizeof(char*));
        entries_array[array_size] = NULL;
        for (size_t i = 0; i < array_size; ++i) {
            entries_array[i] = entries.array[i].string;
        }
        child_argv = concatenate_char_arrays(config.file_exec_path, config.file_exec_args, entries_array);
        run(child_argv);
        free(child_argv);
        free(entries_array);
        break;
    case T_DIR:
        array_size = entries.dir_counter + entries.arg_counter;
        entries_array = calloc(array_size + 1, sizeof(char*));
        entries_array[array_size] = NULL;
        for (size_t i = 0; i < array_size; ++i) {
            entries_array[i] = entries.array[i].string;
        }
        child_argv = concatenate_char_arrays(config.dir_exec_path, config.dir_exec_args, entries_array);
        run(child_argv);
        free(child_argv);
        free(entries_array);
        break; 
    default:
        break;
    }
}

int main(int argc, char *argv[]) {
    determine_call_type(argv[0]);

    convert_argv(argc - 1, argv + 1);

    const char *config_path = get_config_path();
    if (config_path) {
        parse_config(config_path);
    }
    sanitize_and_check_config();

    execute_entries();
}
