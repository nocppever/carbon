#include "system_utils.h"

// Define the GUIDs
DEFINE_GUID(CLSID_NetFwMgr, 0x304CE942, 0x6E39, 0x40D8, 0x94, 0x3A, 0xB9, 0x13, 0xC4, 0x0C, 0x9C, 0xD4);
DEFINE_GUID(IID_INetFwMgr, 0xF7898AF5, 0xCAC4, 0x4632, 0xA2, 0xEC, 0xDA, 0x06, 0xE5, 0x11, 0x1A, 0xF2);
DEFINE_GUID(CLSID_NetFwAuthorizedApplication, 0xEC9846B3, 0x2762, 0x4A6B, 0xA2, 0x90, 0x4E, 0xA3, 0xED, 0x7E, 0xDC, 0x3D);
DEFINE_GUID(IID_INetFwAuthorizedApplication, 0xE0483BA0, 0x47FF, 0x4D9C, 0xA6, 0xD6, 0x77, 0xA2, 0xFC, 0xC8, 0x90, 0xF4);

HRESULT get_firewall_profile(INetFwProfile** fwProfile) {
    HRESULT hr;
    INetFwMgr* fwMgr = NULL;
    INetFwPolicy* fwPolicy = NULL;

    // Create an instance of the firewall settings manager.
    hr = CoCreateInstance(
        &CLSID_NetFwMgr,
        NULL,
        CLSCTX_INPROC_SERVER,
        &IID_INetFwMgr,
        (void**)&fwMgr
    );
    if (FAILED(hr)) {
        goto cleanup;
    }

    // Retrieve the local firewall policy.
    hr = fwMgr->lpVtbl->get_LocalPolicy(fwMgr, &fwPolicy);
    if (FAILED(hr)) {
        goto cleanup;
    }

    // Retrieve the firewall profile currently in effect.
    hr = fwPolicy->lpVtbl->get_CurrentProfile(fwPolicy, fwProfile);
    if (FAILED(hr)) {
        goto cleanup;
    }

cleanup:
    if (fwPolicy) {
        fwPolicy->lpVtbl->Release(fwPolicy);
    }
    if (fwMgr) {
        fwMgr->lpVtbl->Release(fwMgr);
    }
    return hr;
}

SystemInfo get_system_info(void) {
    SystemInfo info = {0};
    DWORD size = sizeof(info.computer_name);
    
    info.is_admin = is_admin();
    GetComputerNameA(info.computer_name, &size);
    
    size = sizeof(info.username);
    GetUserNameA(info.username, &size);
    
    info.firewall_enabled = check_firewall_enabled();
    
    return info;
}
BOOL is_admin(void) {
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

BOOL check_firewall_enabled(void) {
    HRESULT hr = S_OK;
    INetFwMgr* fw_mgr = NULL;
    INetFwPolicy* fw_policy = NULL;
    INetFwProfile* fw_profile = NULL;
    VARIANT_BOOL fw_enabled = VARIANT_FALSE;
    BOOL result = FALSE;
    
    // Initialize COM
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) return TRUE; // Assume enabled if can't check
    
    // Create FirewallManager instance
    hr = CoCreateInstance(&CLSID_NetFwMgr, NULL, CLSCTX_INPROC_SERVER,
                         &IID_INetFwMgr, (void**)&fw_mgr);
    if (SUCCEEDED(hr) && fw_mgr) {
        hr = fw_mgr->lpVtbl->get_LocalPolicy(fw_mgr, &fw_policy);
        if (SUCCEEDED(hr) && fw_policy) {
            hr = fw_policy->lpVtbl->get_CurrentProfile(fw_policy, &fw_profile);
            if (SUCCEEDED(hr) && fw_profile) {
                hr = fw_profile->lpVtbl->get_FirewallEnabled(fw_profile, &fw_enabled);
                if (SUCCEEDED(hr)) {
                    result = (fw_enabled == VARIANT_TRUE);
                }
                fw_profile->lpVtbl->Release(fw_profile);
            }
            fw_policy->lpVtbl->Release(fw_policy);
        }
        fw_mgr->lpVtbl->Release(fw_mgr);
    }
    CoUninitialize();
    return result;
}

BOOL add_firewall_rule(const char* name, const char* path) {
    HRESULT hr;
    INetFwProfile* fwProfile = NULL;
    INetFwAuthorizedApplication* fwApp = NULL;
    BSTR bstrName = NULL;
    BSTR bstrPath = NULL;
    BOOL result = FALSE; // Declare and initialize the result variable

    // Initialize COM.
    hr = CoInitializeEx(0, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        goto cleanup;
    }

    // Retrieve the firewall profile.
    hr = get_firewall_profile(&fwProfile);
    if (FAILED(hr)) {
        goto cleanup;
    }

    // Create an instance of an authorized application.
    hr = CoCreateInstance(
        &CLSID_NetFwAuthorizedApplication,
        NULL,
        CLSCTX_INPROC_SERVER,
        &IID_INetFwAuthorizedApplication,
        (void**)&fwApp
    );
    if (FAILED(hr)) {
        goto cleanup;
    }

    // Convert path to OLECHAR
    int path_len = MultiByteToWideChar(CP_ACP, 0, path, -1, NULL, 0);
    OLECHAR* ole_path = (OLECHAR*)malloc(path_len * sizeof(OLECHAR));
    MultiByteToWideChar(CP_ACP, 0, path, -1, ole_path, path_len);
    bstrPath = SysAllocString(ole_path);
    free(ole_path);
    if (!bstrPath) {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }
    hr = fwApp->lpVtbl->put_ProcessImageFileName(fwApp, bstrPath);
    if (FAILED(hr)) {
        goto cleanup;
    }

    // Convert name to OLECHAR
    int name_len = MultiByteToWideChar(CP_ACP, 0, name, -1, NULL, 0);
    OLECHAR* ole_name = (OLECHAR*)malloc(name_len * sizeof(OLECHAR));
    MultiByteToWideChar(CP_ACP, 0, name, -1, ole_name, name_len);
    bstrName = SysAllocString(ole_name);
    free(ole_name);
    if (!bstrName) {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }
    hr = fwApp->lpVtbl->put_Name(fwApp, bstrName);
    if (FAILED(hr)) {
        goto cleanup;
    }

    // Retrieve the authorized applications collection.
    INetFwAuthorizedApplications* fwApps = NULL;
    hr = fwProfile->lpVtbl->get_AuthorizedApplications(fwProfile, &fwApps);
    if (FAILED(hr)) {
        goto cleanup;
    }

    // Add the application to the authorized application collection.
    hr = fwApps->lpVtbl->Add(fwApps, fwApp);
    if (FAILED(hr)) {
        goto cleanup;
    }

    result = TRUE; // Set result to TRUE if everything succeeded

cleanup:
    if (bstrName) {
        SysFreeString(bstrName);
    }
    if (bstrPath) {
        SysFreeString(bstrPath);
    }
    if (fwApp) {
        fwApp->lpVtbl->Release(fwApp);
    }
    if (fwProfile) {
        fwProfile->lpVtbl->Release(fwProfile);
    }
    CoUninitialize();

    return result;
}