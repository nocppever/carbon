#ifndef ERROR_H
#define ERROR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>

typedef enum {
    ERROR_NONE = 0,
    ERROR_CONFIG = 1,
    ERROR_NETWORK = 2,
    ERROR_SSL = 3,
    ERROR_THREAD = 4,
    ERROR_FILE = 5,
    ERROR_MEMORY = 6,
    ERROR_SYNC = 7
} ErrorCode;

static inline void log_error(ErrorCode code, const char* message) {
    syslog(LOG_ERR, "Error [%d]: %s - %s", code, message, strerror(errno));
    fprintf(stderr, "Error [%d]: %s - %s\n", code, message, strerror(errno));
}

#define RETURN_ON_ERROR(condition, code, message) \
    do { \
        if (condition) { \
            log_error(code, message); \
            return code; \
        } \
    } while(0)

#endif