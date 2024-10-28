#ifndef FIREWALL_DEFS_H
#define FIREWALL_DEFS_H

#include <windows.h>
#include <objbase.h>
#include <oleauto.h>
#include <stdio.h>

// Firewall COM interface definitions
#define CLSID_NetFwMgr_STR "{304CE942-6E39-40D8-943A-B913C40C9CD4}"
#define IID_INetFwMgr_STR "{F7898AF5-CAC4-4632-A2EC-DA06E5111AF2}"

DEFINE_GUID(CLSID_NetFwMgr,
    0x304CE942, 0x6E39, 0x40D8,
    0x94, 0x3A, 0xB9, 0x13, 0xC4, 0x0C, 0x9C, 0xD4);

DEFINE_GUID(IID_INetFwMgr,
    0xF7898AF5, 0xCAC4, 0x4632,
    0xA2, 0xEC, 0xDA, 0x06, 0xE5, 0x11, 0x1A, 0xF2);

// Helper functions for string conversion
static inline BSTR str_to_bstr(const char* str) {
    int len = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
    if (len <= 0) return NULL;
    
    wchar_t* wstr = (wchar_t*)calloc(len, sizeof(wchar_t));
    if (!wstr) return NULL;
    
    MultiByteToWideChar(CP_UTF8, 0, str, -1, wstr, len);
    BSTR bstr = SysAllocString(wstr);
    free(wstr);
    return bstr;
}

#endif