#ifndef ERROR_H
#define ERROR_H

#include "common.h"

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

void log_error(ErrorCode code, const char* message);

// Use log_message from monitor.h
#include "monitor.h"

#endif // ERROR_H