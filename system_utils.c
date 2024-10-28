#include "system_utils.h"
#include "firewall_defs.h"
#include <userenv.h>
#include <iphlpapi.h>
#include "error.h"



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
                result = (fw_enabled == VARIANT_TRUE);
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
    HRESULT hr = S_OK;
    INetFwMgr* fw_mgr = NULL;
    INetFwPolicy* fw_policy = NULL;
    INetFwProfile* fw_profile = NULL;
    INetFwAuthorizedApplications* fw_apps = NULL;
    BOOL success = FALSE;
    
    // Initialize COM
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) return FALSE;
    
    // Convert strings to BSTR
    BSTR bstr_path = SysAllocString((const OLECHAR*)path);
    BSTR bstr_name = SysAllocString((const OLECHAR*)name);
    
    if (bstr_path && bstr_name) {
        // Create FirewallManager instance
        hr = CoCreateInstance(&CLSID_NetFwMgr, NULL, CLSCTX_INPROC_SERVER,
                             &IID_INetFwMgr, (void**)&fw_mgr);
        if (SUCCEEDED(hr) && fw_mgr) {
            hr = fw_mgr->lpVtbl->get_LocalPolicy(fw_mgr, &fw_policy);
            if (SUCCEEDED(hr) && fw_policy) {
                hr = fw_policy->lpVtbl->get_CurrentProfile(fw_policy, &fw_profile);
                if (SUCCEEDED(hr) && fw_profile) {
                    hr = fw_profile->lpVtbl->get_AuthorizedApplications(fw_profile, &fw_apps);
                    if (SUCCEEDED(hr) && fw_apps) {
                        INetFwAuthorizedApplication* fw_app = NULL;
                        hr = CoCreateInstance(&CLSID_NetFwAuthorizedApplication, NULL, CLSCTX_INPROC_SERVER,
                                             &IID_INetFwAuthorizedApplication, (void**)&fw_app);
                        if (SUCCEEDED(hr) && fw_app) {
                            fw_app->lpVtbl->put_ProcessImageFileName(fw_app, bstr_path);
                            fw_app->lpVtbl->put_Name(fw_app, bstr_name);
                            hr = fw_apps->lpVtbl->Add(fw_apps, fw_app);
                            success = SUCCEEDED(hr);
                            fw_app->lpVtbl->Release(fw_app);
                        }
                        fw_apps->lpVtbl->Release(fw_apps);
                    }
                    fw_profile->lpVtbl->Release(fw_profile);
                }
                fw_policy->lpVtbl->Release(fw_policy);
            }
            fw_mgr->lpVtbl->Release(fw_mgr);
        }
    }
    
    SysFreeString(bstr_path);
    SysFreeString(bstr_name);
    CoUninitialize();
    return success;
}