#include "logging.h"

#include <stdarg.h>
#include <stdio.h>

static const char* type_names[] = {[LOG_INFO] = "Info", [LOG_ERROR] = "Error"};

void _log(const enum log_type type, const char* const fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "[%s]: ", type_names[type]);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}
