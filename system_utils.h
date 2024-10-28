#ifndef SYSTEM_UTILS_H
#define SYSTEM_UTILS_H

#include <windows.h>
#include <netfw.h>
#include <oleauto.h>
#include <userenv.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "userenv.lib")

typedef struct {
    BOOL is_admin;
    BOOL firewall_enabled;
    BOOL port_blocked;
    char username[256];
    char computer_name[256];
} SystemInfo;

BOOL check_firewall_enabled(void);
BOOL add_firewall_rule(const char* name, const char* path);
HRESULT get_firewall_profile(INetFwProfile** fwProfile);
SystemInfo get_system_info(void);
BOOL is_admin(void);

#endif // SYSTEM_UTILS_H