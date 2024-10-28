#include "config.h"
#include "monitor.h"
#include <stdarg.h>
#include <time.h>
static char log_file[MAX_PATH];
static HANDLE log_mutex = NULL;




void log_message(const char* format, ...);


void init_logging(void) {
    time_t now;
    struct tm* tm_info;
    char timestamp[32];
    
    time(&now);
    tm_info = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", tm_info);
    
    snprintf(log_file, sizeof(log_file), "server_%s.log", timestamp);
    log_mutex = CreateMutex(NULL, FALSE, NULL);
}

void set_monitor_status(const char* format, ...) {
    if (!log_mutex) return;
    
    WaitForSingleObject(log_mutex, INFINITE);
    
    va_list args;
    char message[1024];
    time_t now;
    struct tm* tm_info;
    char timestamp[32];
    
    time(&now);
    tm_info = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    FILE* file = fopen(log_file, "a");
    if (file) {
        fprintf(file, "[%s] STATUS: %s\n", timestamp, message);
        fclose(file);
    }
    
    printf("[%s] STATUS: %s\n", timestamp, message);
    
    ReleaseMutex(log_mutex);
}

/* void log_message(const char* format, ...) {
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
 */
void run_monitor(void) {
    log_message("Monitor started");
    while (1) {
        Sleep(1000);  // Update every second
        // Add monitoring logic here
    }
}