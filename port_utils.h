#ifndef PORT_UTILS_H
#define PORT_UTILS_H

#include <windows.h>
#include <tlhelp32.h>
#include <iphlpapi.h>
#include <tcpmib.h>
#include "error.h"

#define MIN_PORT 1024
#define MAX_PORT 65535
#define DEFAULT_PORT 8080


typedef struct {
    uint16_t port;
    BOOL is_available;
    BOOL is_blocked;
    DWORD process_id;
    char process_name[MAX_PATH];
} PortInfo;

// Get process name from PID
static inline BOOL get_process_name(DWORD pid, char* name, size_t name_size) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(pe32);

    if (Process32First(snapshot, &pe32)) {
        do {
            if (pe32.th32ProcessID == pid) {
                strncpy(name, pe32.szExeFile, name_size - 1);
                name[name_size - 1] = '\0';
                CloseHandle(snapshot);
                return TRUE;
            }
        } while (Process32Next(snapshot, &pe32));
    }

    CloseHandle(snapshot);
    return FALSE;
}
// Check if port is in use
static inline PortInfo check_port(uint16_t port) {
    PortInfo info = {0};
    info.port = port;
    info.is_available = TRUE;
    
    // Get TCP table
    PMIB_TCPTABLE_OWNER_PID tcp_table;
    DWORD size = 0;
    DWORD result;
    
    // Get required size
    result = GetExtendedTcpTable(NULL, &size, TRUE, AF_INET, 
                                TCP_TABLE_OWNER_PID_ALL, 0);
    
    tcp_table = (PMIB_TCPTABLE_OWNER_PID)malloc(size);
    if (!tcp_table) return info;
    
    result = GetExtendedTcpTable(tcp_table, &size, TRUE, AF_INET,
                                TCP_TABLE_OWNER_PID_ALL, 0);
    
    if (result == NO_ERROR) {
        for (DWORD i = 0; i < tcp_table->dwNumEntries; i++) {
            if (ntohs(tcp_table->table[i].dwLocalPort) == port) {
                info.is_available = FALSE;
                info.process_id = tcp_table->table[i].dwOwningPid;
                get_process_name(info.process_id, info.process_name, 
                               sizeof(info.process_name));
                break;
            }
        }
    }
    
    free(tcp_table);
    return info;
}

// Find available port in range
static inline uint16_t find_port(uint16_t start_port, uint16_t end_port) {
    for (uint16_t port = start_port; port <= end_port; port++) {
        PortInfo info = check_port(port);
        if (info.is_available) {
            return port;
        }
    }
    return 0;
}

// Test port connectivity
static inline BOOL test_port_connectivity(const char* host, uint16_t port, 
                                        int timeout_ms) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) return FALSE;
    
    // Set socket to non-blocking
    u_long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);
    
    // Setup address
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, host, &addr.sin_addr);
    
    // Try to connect
    connect(sock, (struct sockaddr*)&addr, sizeof(addr));
    
    // Setup for select
    fd_set write_fds;
    FD_ZERO(&write_fds);
    FD_SET(sock, &write_fds);
    
    struct timeval timeout;
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;
    
    int result = select(0, NULL, &write_fds, NULL, &timeout);
    closesocket(sock);
    
    return result > 0;
}

// Port scanning utilities
typedef struct {
    uint16_t port;
    const char* service_name;
    BOOL is_available;
} PortScanResult;

static inline PortScanResult* scan_common_ports(size_t* count) {
    static const struct {
        uint16_t port;
        const char* name;
    } common_ports[] = {
        {80, "HTTP"},
        {443, "HTTPS"},
        {8080, "HTTP-ALT"},
        {3389, "RDP"},
        {22, "SSH"},
        {21, "FTP"},
        {25, "SMTP"},
        {110, "POP3"},
        {143, "IMAP"},
        {3306, "MySQL"},
        {5432, "PostgreSQL"},
        {27017, "MongoDB"},
        {6379, "Redis"},
        {5672, "AMQP"},
        {5671, "AMQP/SSL"}
    };
    
    *count = sizeof(common_ports) / sizeof(common_ports[0]);
    PortScanResult* results = malloc(sizeof(PortScanResult) * (*count));
    
    if (!results) {
        *count = 0;
        return NULL;
    }
    
    for (size_t i = 0; i < *count; i++) {
        results[i].port = common_ports[i].port;
        results[i].service_name = common_ports[i].name;
        PortInfo info = check_port(common_ports[i].port);
        results[i].is_available = info.is_available;
    }
    
    return results;
}

#endif