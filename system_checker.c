#include <stdio.h>
#include <windows.h>
#include "system_utils.h"
#include "error.h"
#include <psapi.h>

typedef struct {
    DWORD total_score;
    BOOL meets_requirements;
    char recommendations[1024];
} SystemCheck;

static void check_cpu_usage(SystemCheck* check) {
    FILETIME idle_time, kernel_time, user_time;
    GetSystemTimes(&idle_time, &kernel_time, &user_time);
    
    ULARGE_INTEGER idle, kernel, user;
    idle.LowPart = idle_time.dwLowDateTime;
    idle.HighPart = idle_time.dwHighDateTime;
    kernel.LowPart = kernel_time.dwLowDateTime;
    kernel.HighPart = kernel_time.dwHighDateTime;
    user.LowPart = user_time.dwLowDateTime;
    user.HighPart = user_time.dwHighDateTime;
    
    Sleep(1000);  // Wait 1 second
    
    FILETIME idle_time2, kernel_time2, user_time2;
    GetSystemTimes(&idle_time2, &kernel_time2, &user_time2);
    
    ULARGE_INTEGER idle2, kernel2, user2;
    idle2.LowPart = idle_time2.dwLowDateTime;
    idle2.HighPart = idle_time2.dwHighDateTime;
    kernel2.LowPart = kernel_time2.dwLowDateTime;
    kernel2.HighPart = kernel_time2.dwHighDateTime;
    user2.LowPart = user_time2.dwLowDateTime;
    user2.HighPart = user_time2.dwHighDateTime;
    
    ULONGLONG idle_diff = idle2.QuadPart - idle.QuadPart;
    ULONGLONG kernel_diff = kernel2.QuadPart - kernel.QuadPart;
    ULONGLONG user_diff = user2.QuadPart - user.QuadPart;
    
    ULONGLONG total_diff = kernel_diff + user_diff;
    ULONGLONG cpu_usage = ((total_diff - idle_diff) * 100) / total_diff;
    
    printf("CPU Usage: %llu%%\n", cpu_usage);
    
    if (cpu_usage > 80) {
        strcat(check->recommendations, 
               "High CPU usage detected. Consider closing unnecessary applications.\n");
        check->total_score -= 20;
    }
}

static void check_memory(SystemCheck* check) {
    MEMORYSTATUSEX mem_info = {sizeof(MEMORYSTATUSEX)};
    GlobalMemoryStatusEx(&mem_info);
    
    DWORDLONG total_physical = mem_info.ullTotalPhys / (1024 * 1024);  // MB
    DWORDLONG available_physical = mem_info.ullAvailPhys / (1024 * 1024);  // MB
    DWORDLONG memory_usage = ((total_physical - available_physical) * 100) / total_physical;
    
    printf("Memory Total: %llu MB\n", total_physical);
    printf("Memory Available: %llu MB\n", available_physical);
    printf("Memory Usage: %llu%%\n", memory_usage);
    
    if (available_physical < 512) {  // Less than 512MB available
        strcat(check->recommendations, 
               "Low memory available. Free up memory or add more RAM.\n");
        check->total_score -= 20;
    }
    
    if (memory_usage > 90) {
        strcat(check->recommendations, 
               "High memory usage. Close unnecessary applications.\n");
        check->total_score -= 15;
    }
}

static void check_disk_space(SystemCheck* check) {
    char path[MAX_PATH];
    GetModuleFileName(NULL, path, MAX_PATH);
    path[3] = '\0';  // Get drive letter path
    
    ULARGE_INTEGER free_bytes, total_bytes, total_free_bytes;
    GetDiskFreeSpaceEx(path, &free_bytes, &total_bytes, &total_free_bytes);
    
    DWORDLONG free_gb = free_bytes.QuadPart / (1024 * 1024 * 1024);
    DWORDLONG total_gb = total_bytes.QuadPart / (1024 * 1024 * 1024);
    DWORDLONG usage_percent = ((total_gb - free_gb) * 100) / total_gb;
    
    printf("Disk Total: %llu GB\n", total_gb);
    printf("Disk Free: %llu GB\n", free_gb);
    printf("Disk Usage: %llu%%\n", usage_percent);
    
    if (free_gb < 1) {  // Less than 1GB free
        strcat(check->recommendations, 
               "Critical: Very low disk space. Free up space immediately.\n");
        check->total_score -= 30;
    } else if (free_gb < 5) {  // Less than 5GB free
        strcat(check->recommendations, 
               "Low disk space. Consider freeing up space.\n");
        check->total_score -= 15;
    }
}

static void check_network(SystemCheck* check) {
    MIB_TCPSTATS tcp_stats;
    if (GetTcpStatistics(&tcp_stats) == NO_ERROR) {
        printf("Active TCP Connections: %lu\n", tcp_stats.dwCurrEstab);
        printf("TCP Segments Retransmitted: %lu\n", tcp_stats.dwRetransSegs);
        
        if (tcp_stats.dwRetransSegs > 1000) {
            strcat(check->recommendations, 
                   "High network retransmission rate. Check network stability.\n");
            check->total_score -= 10;
        }
    }
    
    // Test common ports
    uint16_t test_ports[] = {80, 443, 8080, config->server_port};
    for (int i = 0; i < sizeof(test_ports)/sizeof(test_ports[0]); i++) {
        if (test_port_connectivity("127.0.0.1", test_ports[i], 1000)) {
            printf("Port %d: Open\n", test_ports[i]);
        } else {
            printf("Port %d: Closed\n", test_ports[i]);
            if (test_ports[i] == config->server_port) {
                strcat(check->recommendations, 
                       "Server port is not accessible. Check firewall settings.\n");
                check->total_score -= 20;
            }
        }
    }
}

static void check_services(SystemCheck* check) {
    SC_HANDLE sc_manager = OpenSCManager(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);
    if (sc_manager) {
        ENUM_SERVICE_STATUS service_status;
        DWORD bytes_needed = 0;
        DWORD services_returned;
        DWORD resume_handle = 0;
        
        // Check Windows Firewall service
        SC_HANDLE service = OpenService(sc_manager, "MpsSvc", SERVICE_QUERY_STATUS);
        if (service) {
            SERVICE_STATUS status;
            if (QueryServiceStatus(service, &status)) {
                printf("Windows Firewall Service: %s\n", 
                       status.dwCurrentState == SERVICE_RUNNING ? "Running" : "Stopped");
                
                if (status.dwCurrentState != SERVICE_RUNNING) {
                    strcat(check->recommendations, 
                           "Windows Firewall service is not running.\n");
                    check->total_score -= 15;
                }
            }
            CloseServiceHandle(service);
        }
        
        CloseServiceHandle(sc_manager);
    }
}

static BOOL write_report(const SystemCheck* check, const char* filename) {
    FILE* file = fopen(filename, "w");
    if (!file) return FALSE;
    
    time_t now;
    time(&now);
    
    fprintf(file, "System Check Report\n");
    fprintf(file, "==================\n");
    fprintf(file, "Date: %s\n", ctime(&now));
    fprintf(file, "System Score: %d/100\n\n", check->total_score);
    fprintf(file, "Meets Requirements: %s\n\n", 
            check->meets_requirements ? "Yes" : "No");
    
    if (strlen(check->recommendations) > 0) {
        fprintf(file, "Recommendations:\n");
        fprintf(file, "%s\n", check->recommendations);
    }
    
    fclose(file);
    return TRUE;
}

int main(void) {
    printf("System Configuration Checker\n");
    printf("===========================\n");
    
    SystemCheck check = {
        .total_score = 100,
        .meets_requirements = TRUE,
        .recommendations = ""
    };
    
    // Run checks
    check_cpu_usage(&check);
    check_memory(&check);
    check_disk_space(&check);
    check_network(&check);
    check_services(&check);
    
    // Determine if system meets requirements
    check.meets_requirements = check.total_score >= 70;
    
    // Print results
    printf("\nSystem Check Results:\n");
    printf("Total Score: %d/100\n", check.total_score);
    printf("Meets Requirements: %s\n", 
           check.meets_requirements ? "Yes" : "No");
    
    if (strlen(check.recommendations) > 0) {
        printf("\nRecommendations:\n%s", check.recommendations);
    }
    
    // Write report
    char report_filename[MAX_PATH];
    SYSTEMTIME st;
    GetLocalTime(&st);
    sprintf(report_filename, "system_check_%04d%02d%02d_%02d%02d%02d.txt",
            st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    
    if (write_report(&check, report_filename)) {
        printf("\nDetailed report written to: %s\n", report_filename);
    }
    
    return check.meets_requirements ? 0 : 1;
}