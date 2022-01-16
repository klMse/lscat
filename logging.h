#ifndef _LOGGING_H
#define _LOGGING_H

#include <string.h>

enum log_type {
    LOG_INFO,
    LOG_ERROR
};

void _log(const enum log_type type, const char* const fmt, ...);

#define log_perror(err, fmt, ...) _log(LOG_ERROR, fmt ": %s", ##__VA_ARGS__, strerror(err))
#define log_error(fmt, ...) _log(LOG_ERROR, fmt, ##__VA_ARGS__)
#define log_info(fmt, ...) _log(LOG_INFO, fmt, ##__VA_ARGS__)

#endif /* _LOGGING_H */