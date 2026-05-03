#include "LangBarButton.h"
#include "McBopomofoTIP.h"
#include "Globals.h"
#include "Register.h"
#include "PathCompat.h"
#include <shellapi.h>
#include <filesystem>

// GUID of the IME mode icon in Windows 8/10
const GUID GUID_LBI_INPUTMODE = { 0x2C77A81E, 0x41CC, 0x4178, { 0xA3, 0xA7, 0x5F, 0x8A, 0x98, 0x75, 0x68, 0xE6 } };

CLangBarButton::CLangBarButton(McBopomofoTIP* pTIP, const GUID& guid)
    : _refCount(1), _pTIP(pTIP), _guid(guid) {
    if (_pTIP) _pTIP->AddRef();
}

CLangBarButton::~CLangBarButton() {
    if (_pTIP) _pTIP->Release();
}

STDMETHODIMP CLangBarButton::QueryInterface(REFIID riid, void **ppvObj) {
    if (ppvObj == nullptr) return E_INVALIDARG;
    *ppvObj = nullptr;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfLangBarItem) || IsEqualIID(riid, IID_ITfLangBarItemButton)) {
        *ppvObj = (ITfLangBarItemButton*)this;
    } else if (IsEqualIID(riid, IID_ITfSource)) {
        *ppvObj = (ITfSource*)this;
    }

    if (*ppvObj) {
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CLangBarButton::AddRef() {
    return InterlockedIncrement(&_refCount);
}

STDMETHODIMP_(ULONG) CLangBarButton::Release() {
    ULONG ref = InterlockedDecrement(&_refCount);
    if (ref == 0) delete this;
    return ref;
}

STDMETHODIMP CLangBarButton::GetInfo(TF_LANGBARITEMINFO *pInfo) {
    if (!pInfo) return E_INVALIDARG;
    pInfo->clsidService = c_clsidMcBopomofoTIP;
    pInfo->guidItem = _guid;
    pInfo->dwStyle = TF_LBI_STYLE_BTN_BUTTON | TF_LBI_STYLE_BTN_MENU | TF_LBI_STYLE_SHOWNINTRAY;
    pInfo->ulSort = 0;
    wcscpy_s(pInfo->szDescription, L"Win-McBopomofo");
    return S_OK;
}

STDMETHODIMP CLangBarButton::GetStatus(DWORD *pdwStatus) {
    if (!pdwStatus) return E_INVALIDARG;
    *pdwStatus = 0;
    return S_OK;
}

STDMETHODIMP CLangBarButton::Show(BOOL fShow) {
    return E_NOTIMPL;
}

STDMETHODIMP CLangBarButton::GetTooltipString(BSTR *pbstrToolTip) {
    if (!pbstrToolTip) return E_INVALIDARG;
    *pbstrToolTip = SysAllocString(L"中/英文模式 (中/En)");
    return S_OK;
}

STDMETHODIMP CLangBarButton::OnClick(TfLBIClick click, POINT pt, const RECT *prcArea) {
    if (click == TF_LBI_CLK_LEFT) {
        _pTIP->ToggleOpenClose();
    } else if (click == TF_LBI_CLK_RIGHT) {
        // Right click is handled by InitMenu if style includes TF_LBI_STYLE_BTN_MENU
    }
    return S_OK;
}

STDMETHODIMP CLangBarButton::InitMenu(ITfMenu *pMenu) {
    if (!pMenu) return E_INVALIDARG;

    pMenu->AddMenuItem(1, 0, nullptr, nullptr, L"設定 (Settings)", (ULONG)wcslen(L"設定 (Settings)"), nullptr);
    pMenu->AddMenuItem(2, 0, nullptr, nullptr, L"編輯使用者詞庫 (Edit User Phrases)", (ULONG)wcslen(L"編輯使用者詞庫 (Edit User Phrases)"), nullptr);
    pMenu->AddMenuItem(3, 0, nullptr, nullptr, L"編輯排除詞庫 (Edit Excluded Phrases)", (ULONG)wcslen(L"編輯排除詞庫 (Edit Excluded Phrases)"), nullptr);
    pMenu->AddMenuItem(4, 0, nullptr, nullptr, L"開啟使用者資料夾 (Open Data Folder)", (ULONG)wcslen(L"開啟使用者資料夾 (Open Data Folder)"), nullptr);

    return S_OK;
}

STDMETHODIMP CLangBarButton::OnMenuSelect(UINT wID) {
    switch (wID) {
    case 1: {
        WCHAR path[MAX_PATH];
        GetModuleFileNameW(GetModuleHandleW(L"McBopomofoTIP_v2.dll"), path, MAX_PATH);
        std::filesystem::path p(path);
        p.replace_filename("McBopomofoConfig.exe");
        ShellExecuteW(NULL, L"open", p.c_str(), NULL, NULL, SW_SHOW);
        break;
    }
    case 2: {
        std::string path = McBopomofo::fcitx5_compat::userDirectory() + "/user.txt";
        ShellExecuteA(NULL, "open", path.c_str(), NULL, NULL, SW_SHOW);
        break;
    }
    case 3: {
        std::string path = McBopomofo::fcitx5_compat::userDirectory() + "/exclude.txt";
        ShellExecuteA(NULL, "open", path.c_str(), NULL, NULL, SW_SHOW);
        break;
    }
    case 4: {
        std::string path = McBopomofo::fcitx5_compat::userDirectory();
        ShellExecuteA(NULL, "open", path.c_str(), NULL, NULL, SW_SHOW);
        break;
    }
    }
    return S_OK;
}

#include "resource.h"
extern HINSTANCE g_hInst;

STDMETHODIMP CLangBarButton::GetIcon(HICON *phIcon) {
    if (!phIcon) return E_INVALIDARG;
    
    bool isOpen = _pTIP->IsOpen();
    
    UINT iconId = isOpen ? IDI_ICON_ZH : IDI_ICON_EN;
    *phIcon = LoadIconW(g_hInst, MAKEINTRESOURCEW(iconId));

    if (!*phIcon) {
        // Fallback drawing if icon fails to load
        HDC hdc = GetDC(NULL);
        HDC hMemDC = CreateCompatibleDC(hdc);
        HBITMAP hBitmap = CreateCompatibleBitmap(hdc, 16, 16);
        HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);
        
        RECT rc = {0, 0, 16, 16};
        FillRect(hMemDC, &rc, (HBRUSH)(COLOR_WINDOW+1));
        SetBkMode(hMemDC, TRANSPARENT);
        SetTextColor(hMemDC, RGB(0,0,0));
        
        HFONT hFont = CreateFontW(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        HFONT hOldFont = (HFONT)SelectObject(hMemDC, hFont);
        
        const wchar_t* txt = isOpen ? L"中" : L"英";
        DrawTextW(hMemDC, txt, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        
        SelectObject(hMemDC, hOldFont);
        DeleteObject(hFont);
        SelectObject(hMemDC, hOldBitmap);
        
        ICONINFO ii = {0};
        ii.fIcon = TRUE;
        ii.hbmMask = hBitmap;
        ii.hbmColor = hBitmap;
        *phIcon = CreateIconIndirect(&ii);
        
        DeleteObject(hBitmap);
        DeleteDC(hMemDC);
        ReleaseDC(NULL, hdc);
    }

    return S_OK;
}

STDMETHODIMP CLangBarButton::GetText(BSTR *pbstrText) {
    if (!pbstrText) return E_INVALIDARG;
    bool isOpen = _pTIP->IsOpen();
    *pbstrText = SysAllocString(isOpen ? L"中" : L"英");
    return S_OK;
}

STDMETHODIMP CLangBarButton::AdviseSink(REFIID riid, IUnknown *punk, DWORD *pdwCookie) {
    if (!IsEqualIID(riid, IID_ITfLangBarItemSink)) return E_INVALIDARG;
    ITfLangBarItemSink* pSink;
    if (FAILED(punk->QueryInterface(IID_ITfLangBarItemSink, (void**)&pSink))) return E_NOINTERFACE;
    
    _sinks.push_back(pSink);
    *pdwCookie = (DWORD)_sinks.size();
    return S_OK;
}

STDMETHODIMP CLangBarButton::UnadviseSink(DWORD dwCookie) {
    if (dwCookie == 0 || dwCookie > _sinks.size()) return E_INVALIDARG;
    ITfLangBarItemSink* pSink = _sinks[dwCookie - 1];
    if (pSink) {
        pSink->Release();
        _sinks[dwCookie - 1] = nullptr;
    }
    return S_OK;
}

void CLangBarButton::Update() {
    for (auto sink : _sinks) {
        if (sink) {
            sink->OnUpdate(TF_LBI_ICON | TF_LBI_TEXT | TF_LBI_TOOLTIP);
        }
    }
}
