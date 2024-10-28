#ifndef SYSTEM_UTILS_H
#define SYSTEM_UTILS_H

#include <windows.h>
#include <userenv.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <netfw.h>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "userenv.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

DEFINE_GUID(CLSID_NetFwMgr, 0x304CE942, 0x6E39, 0x40D8, 0x94, 0x3A, 0xB9, 0x13, 0xC4, 0x0C, 0x9C, 0xD4);
DEFINE_GUID(IID_INetFwMgr, 0xF7898AF5, 0xCAC4, 0x4632, 0xA2, 0xEC, 0xDA, 0x06, 0xE5, 0x11, 0x1A, 0xF2);
DEFINE_GUID(CLSID_NetFwAuthorizedApplication, 0xEC9846B3, 0x2762, 0x4A6B, 0xA2, 0x90, 0x4E, 0xA3, 0xED, 0x7E, 0xDC, 0x3D);
DEFINE_GUID(IID_INetFwAuthorizedApplication, 0xE0483BA0, 0x47FF, 0x4D9C, 0xA6, 0xD6, 0x77, 0xA2, 0xFC, 0xC8, 0x90, 0xF4);

typedef struct {
    BOOL is_admin;
    BOOL firewall_enabled;
    BOOL port_blocked;
    char username[256];
    char computer_name[256];
} SystemInfo;

// Check if running as administrator
static inline BOOL is_admin(void) {
    BOOL is_admin = FALSE;
    HANDLE token = NULL;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
        TOKEN_ELEVATION elevation;
        DWORD size;
        if (GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size)) {
            is_admin = elevation.TokenIsElevated;
        }
        CloseHandle(token);
    }
    return is_admin;
}

// Get system information
static inline SystemInfo get_system_info(void) {
    SystemInfo info = {0};
    DWORD size = sizeof(info.computer_name);
    
    info.is_admin = is_admin();
    GetComputerNameA(info.computer_name, &size);
    
    size = sizeof(info.username);
    GetUserNameA(info.username, &size);
    
    return info;
}

// Check if Windows Firewall is enabled
BOOL check_firewall_enabled(void);

// Add firewall rule for our application
BOOL add_firewall_rule(const char* name, const char* path);

#endif // SYSTEM_UTILS_H