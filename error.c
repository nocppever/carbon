#define IMPLEMENT_ERROR_FUNCTIONS
#include "error.h"
#include <stdarg.h>
#include <time.h>
#include "config.h"

// Implementation of the log functions
void log_error(ErrorCode code, const char* message) {
    char error_msg[1024];
    DWORD win_error = GetLastError();
    time_t now;
    struct tm* timeinfo;
    char timestamp[26];
    
    time(&now);
    timeinfo = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    if (win_error != 0) {
        char win_msg[512];
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, win_error,
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                      win_msg, sizeof(win_msg), NULL);
        snprintf(error_msg, sizeof(error_msg), "[%s] Error [%d]: %s - %s",
                timestamp, code, message, win_msg);
    } else {
        snprintf(error_msg, sizeof(error_msg), "[%s] Error [%d]: %s",
                timestamp, code, message);
    }
    
    fprintf(stderr, "%s\n", error_msg);
}

void log_message(const char* format, ...) {
    char message[1024];
    time_t now;
    struct tm* timeinfo;
    char timestamp[26];
    va_list args;
    
    time(&now);
    timeinfo = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    printf("[%s] %s\n", timestamp, message);
}