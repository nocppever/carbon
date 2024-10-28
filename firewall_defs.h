#ifndef FIREWALL_DEFS_H
#define FIREWALL_DEFS_H

#include <windows.h>
#include <objbase.h>
#include <oleauto.h>
#include <stdio.h>
#include <netfw.h>  // Include netfw.h to use the predefined GUIDs

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

#endif // FIREWALL_DEFS_H