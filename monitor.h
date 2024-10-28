#ifndef MONITOR_H
#define MONITOR_H

#include "common.h"

void init_logging(void);
void set_monitor_status(const char* format, ...);
void run_monitor(void);

// Define log_message only in monitor.c
#ifndef MONITOR_C
extern void log_message(const char* format, ...);
#endif

#endif // MONITOR_H