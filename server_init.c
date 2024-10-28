#include "server_init.h"
#include "system_utils.h"
#include "error.h"

ErrorCode check_system_requirements(SystemCheck* sys_check) {
    // Get system information
    *sys_check = get_system_info();
    log_message("System check - Username: %s, Computer: %s, Admin: %s",
                sys_check->username,
                sys_check->computer_name,
                sys_check->is_admin ? "Yes" : "No");
    
    // Check Windows version
    OSVERSIONINFOEX os_info = {sizeof(OSVERSIONINFOEX)};
    ULONGLONG condition_mask = 0;
    VER_SET_CONDITION(condition_mask, VER_MAJORVERSION, VER_GREATER_EQUAL);
    VER_SET_CONDITION(condition_mask, VER_MINORVERSION, VER_GREATER_EQUAL);
    
    os_info.dwMajorVersion = 6; // Windows 7 or later
    os_info.dwMinorVersion = 1;
    
    if (!VerifyVersionInfo(&os_info, 
                          VER_MAJORVERSION | VER_MINORVERSION,
                          condition_mask)) {
        log_message("ERROR: Unsupported Windows version");
        return ERROR_CONFIG;
    }
    
    // Check available memory
    MEMORYSTATUSEX mem_info = {sizeof(MEMORYSTATUSEX)};
    GlobalMemoryStatusEx(&mem_info);
    if (mem_info.ullAvailPhys < 100 * 1024 * 1024) { // 100 MB minimum
        log_message("WARNING: Low memory condition detected");
    }
    
    // Check disk space
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    path[3] = '\0'; // Get drive letter path
    
    ULARGE_INTEGER free_bytes, total_bytes, total_free_bytes;
    GetDiskFreeSpaceExA(path, &free_bytes, &total_bytes, &total_free_bytes);
    
    if (free_bytes.QuadPart < 500 * 1024 * 1024) { // 500 MB minimum
        log_message("WARNING: Low disk space condition detected");
    }
    
    return ERROR_NONE;
}