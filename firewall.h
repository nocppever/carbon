#ifndef FIREWALL_H
#define FIREWALL_H

#include <windows.h>
#include <netfw.h>

// COM interface declarations
#define CLSID_NetFwMgr_STR "{304CE942-6E39-40D8-943A-B913C40C9CD4}"
#define IID_INetFwMgr_STR "{F7898AF5-CAC4-4632-A2EC-DA06E5111AF2}"

DEFINE_GUID(CLSID_NetFwMgr,
    0x304CE942, 0x6E39, 0x40D8,
    0x94, 0x3A, 0xB9, 0x13, 0xC4, 0x0C, 0x9C, 0xD4);

DEFINE_GUID(IID_INetFwMgr,
    0xF7898AF5, 0xCAC4, 0x4632,
    0xA2, 0xEC, 0xDA, 0x06, 0xE5, 0x11, 0x1A, 0xF2);

// Function declarations for firewall operations
HRESULT InitializeFirewall(void);
HRESULT CheckFirewallEnabled(BOOL* enabled);
HRESULT AddFirewallRule(const wchar_t* name, const wchar_t* path, LONG port);

#endif