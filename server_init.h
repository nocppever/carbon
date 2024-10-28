#ifndef SERVER_INIT_H
#define SERVER_INIT_H

#include "system_utils.h"
#include "port_utils.h"
#include "error.h"
#include <time.h>
#include "common.h"

typedef struct {
    // Define ServerContext structure
} ServerContext;

// Remove or comment out the unused variable
// static ServerContext g_server_context = {0};

// Initialize logging
void init_logging(void);

// Check system requirements
ErrorCode check_system_requirements(SystemCheck* sys_check);

// Configure the system
static inline ErrorCode configure_system(Config* config) {
    // Implementation of configure_system
    char exe_path[MAX_PATH];
    GetModuleFileNameA(NULL, exe_path, MAX_PATH);
    
    if (!add_firewall_rule("File Sync Server", exe_path)) {
        log_message("ERROR: Failed to add firewall rule");
        return ERROR_CONFIG;
    }
    
    return ERROR_NONE;
}

// Initialize server
static inline ErrorCode initialize_server(Config* config) {
    SystemCheck sys_check = {0};
    ErrorCode error = check_system_requirements(&sys_check);
    if (error != ERROR_NONE) {
        return error;
    }
    
    // Additional server initialization code
    return ERROR_NONE;
}

#endif // SERVER_INIT_H