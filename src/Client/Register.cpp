#include "Register.h"
#include "Globals.h"
#include <strsafe.h>

// Profile GUID for McBopomofo (Genereted a new random one)
// {A3668853-2ED4-4D4B-A951-DE1C8B4C0A29}
const GUID c_guidMcBopomofoProfile = 
{ 0xa3668853, 0x2ed4, 0x4d4b, { 0xa9, 0x51, 0xde, 0x1c, 0x8b, 0x4c, 0xa, 0x29 } };

static const WCHAR c_szInfoKeyPrefix[] = L"CLSID\\";
static const WCHAR c_szInProcSvr32[] = L"InProcServer32";
static const WCHAR c_szModelName[] = L"ThreadingModel";

BOOL SetRegString(HKEY hKey, LPCWSTR lpSubKey, LPCWSTR lpValueName, LPCWSTR lpData) {
    HKEY hSubKey = nullptr;
    LONG lRes = RegCreateKeyExW(hKey, lpSubKey, 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hSubKey, nullptr);
    if (lRes != ERROR_SUCCESS) return FALSE;

    lRes = RegSetValueExW(hSubKey, lpValueName, 0, REG_SZ, (const BYTE*)lpData, (lstrlenW(lpData) + 1) * sizeof(WCHAR));
    RegCloseKey(hSubKey);
    return (lRes == ERROR_SUCCESS);
}

BOOL RegisterServer() {
    WCHAR szModulePath[MAX_PATH];
    if (GetModuleFileNameW(g_hInst, szModulePath, ARRAYSIZE(szModulePath)) == 0) return FALSE;

    WCHAR szCLSID[128];
    StringFromGUID2(c_clsidMcBopomofoTIP, szCLSID, ARRAYSIZE(szCLSID));

    WCHAR szKey[256];
    StringCchPrintfW(szKey, ARRAYSIZE(szKey), L"%s%s", c_szInfoKeyPrefix, szCLSID);

    if (!SetRegString(HKEY_CLASSES_ROOT, szKey, nullptr, L"Win-McBopomofo TIP")) return FALSE;

    WCHAR szInProcKey[256];
    StringCchPrintfW(szInProcKey, ARRAYSIZE(szInProcKey), L"%s\\%s", szKey, c_szInProcSvr32);
    if (!SetRegString(HKEY_CLASSES_ROOT, szInProcKey, nullptr, szModulePath)) return FALSE;
    if (!SetRegString(HKEY_CLASSES_ROOT, szInProcKey, c_szModelName, L"Apartment")) return FALSE;

    return TRUE;
}

void UnregisterServer() {
    WCHAR szCLSID[128];
    StringFromGUID2(c_clsidMcBopomofoTIP, szCLSID, ARRAYSIZE(szCLSID));

    WCHAR szKey[256];
    StringCchPrintfW(szKey, ARRAYSIZE(szKey), L"%s%s\\%s", c_szInfoKeyPrefix, szCLSID, c_szInProcSvr32);
    RegDeleteKeyW(HKEY_CLASSES_ROOT, szKey);

    StringCchPrintfW(szKey, ARRAYSIZE(szKey), L"%s%s", c_szInfoKeyPrefix, szCLSID);
    RegDeleteKeyW(HKEY_CLASSES_ROOT, szKey);
}

BOOL RegisterProfiles() {
    ITfInputProcessorProfileMgr *pProfileMgr = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_ITfInputProcessorProfileMgr, (void**)&pProfileMgr);
    if (FAILED(hr)) return FALSE;

    WCHAR szModulePath[MAX_PATH];
    GetModuleFileNameW(g_hInst, szModulePath, ARRAYSIZE(szModulePath));
    
    // Register for Traditional Chinese (Taiwan)
    LANGID langid = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL);

    hr = pProfileMgr->RegisterProfile(
        c_clsidMcBopomofoTIP,
        langid,
        c_guidMcBopomofoProfile,
        L"Win-McBopomofo",
        (ULONG)wcslen(L"Win-McBopomofo"),
        szModulePath,
        (ULONG)wcslen(szModulePath),
        0, // Icon index
        0, // hkl substitute
        0, // Preferred layout
        TRUE, // Enabled by default
        0  // Flags
    );

    pProfileMgr->Release();
    return SUCCEEDED(hr);
}

void UnregisterProfiles() {
    ITfInputProcessorProfileMgr *pProfileMgr = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_ITfInputProcessorProfileMgr, (void**)&pProfileMgr);
    if (FAILED(hr)) return;

    LANGID langid = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL);
    pProfileMgr->UnregisterProfile(c_clsidMcBopomofoTIP, langid, c_guidMcBopomofoProfile, 0);
    pProfileMgr->Release();
}

BOOL RegisterCategories() {
    ITfCategoryMgr *pCategoryMgr = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TF_CategoryMgr, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_ITfCategoryMgr, (void**)&pCategoryMgr);
    if (FAILED(hr)) return FALSE;

    // Register as a Keyboard TIP
    hr = pCategoryMgr->RegisterCategory(c_clsidMcBopomofoTIP, GUID_TFCAT_TIP_KEYBOARD, c_clsidMcBopomofoTIP);
    // Register as a Display Attribute Provider
    if (SUCCEEDED(hr)) {
        hr = pCategoryMgr->RegisterCategory(c_clsidMcBopomofoTIP, GUID_TFCAT_DISPLAYATTRIBUTEPROVIDER, c_clsidMcBopomofoTIP);
        hr = pCategoryMgr->RegisterCategory(c_clsidMcBopomofoTIP, GUID_TFCAT_TIPCAP_SECUREMODE, c_clsidMcBopomofoTIP);
        hr = pCategoryMgr->RegisterCategory(c_clsidMcBopomofoTIP, GUID_TFCAT_TIPCAP_UIELEMENTENABLED, c_clsidMcBopomofoTIP);
        hr = pCategoryMgr->RegisterCategory(c_clsidMcBopomofoTIP, GUID_TFCAT_TIPCAP_INPUTMODECOMPARTMENT, c_clsidMcBopomofoTIP);
        hr = pCategoryMgr->RegisterCategory(c_clsidMcBopomofoTIP, GUID_TFCAT_TIPCAP_COMLESS, c_clsidMcBopomofoTIP);
    }

    pCategoryMgr->Release();
    return SUCCEEDED(hr);
}

void UnregisterCategories() {
    ITfCategoryMgr *pCategoryMgr = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TF_CategoryMgr, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_ITfCategoryMgr, (void**)&pCategoryMgr);
    if (FAILED(hr)) return;

    pCategoryMgr->UnregisterCategory(c_clsidMcBopomofoTIP, GUID_TFCAT_TIP_KEYBOARD, c_clsidMcBopomofoTIP);
    pCategoryMgr->UnregisterCategory(c_clsidMcBopomofoTIP, GUID_TFCAT_DISPLAYATTRIBUTEPROVIDER, c_clsidMcBopomofoTIP);

    pCategoryMgr->Release();
}
