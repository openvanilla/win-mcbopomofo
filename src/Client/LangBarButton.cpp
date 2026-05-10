// Copyright (c) 2026 and onwards The McBopomofo Authors.
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

#include "LangBarButton.h"

#include <shellapi.h>

#include <algorithm>
#include <filesystem>

#include "Globals.h"
#include "Ipc.h"
#include "McBopomofoTIP.h"
#include "NamedPipe.h"
#include "PathCompat.h"
#include "Register.h"
#include "resource.h"
#include "UTFHelper.h"

// GUID of the IME mode icon in Windows 8/10
extern const GUID GUID_LBI_INPUTMODE = {
    0x2C77A81E,
    0x41CC,
    0x4178,
    {0xA3, 0xA7, 0x5F, 0x8A, 0x98, 0x75, 0x68, 0xE6}};
// Regular language bar button, equivalent to PIME's "switch-lang" button.
extern const GUID GUID_LBI_SWITCH_LANG = {
    0x5C7D0E31,
    0x28C0,
    0x4D1F,
    {0xB3, 0xD5, 0x91, 0x6D, 0x57, 0xC9, 0x11, 0x7A}};

namespace {
constexpr UINT MENU_TOGGLE_OPEN_CLOSE = 100;
constexpr UINT MENU_TOGGLE_ASSOCIATED_PHRASES = 101;
constexpr UINT MENU_TOGGLE_HALF_WIDTH_PUNCTUATION = 102;
constexpr UINT MENU_TOGGLE_CHINESE_CONVERSION = 103;
constexpr UINT MENU_TOGGLE_BOPOMOFO_FONT_ANNOTATION = 104;
constexpr UINT MENU_OPEN_SETTINGS = 1;
constexpr UINT MENU_EDIT_USER_PHRASES = 2;
constexpr UINT MENU_EDIT_EXCLUDED_PHRASES = 3;
constexpr UINT MENU_OPEN_USER_DATA_FOLDER = 4;

struct MenuItem {
  UINT id;
  const wchar_t* text;
  bool checked;
  bool separator;
};

std::wstring SettingsPath() {
  std::filesystem::path path(McBopomofo::fcitx5_compat::userDirectory());
  path /= "mcbopomofo.ini";
  return path.wstring();
}

bool ReadBoolSetting(const wchar_t* key, bool defaultValue) {
  return GetPrivateProfileIntW(L"General", key, defaultValue ? 1 : 0,
                               SettingsPath().c_str()) != 0;
}

const wchar_t* CurrentModeLabel(McBopomofoTIP* tip) {
  if (!tip->IsOpen()) {
    return L"英";
  }
  return ReadBoolSetting(L"ChineseConversionEnabled", false) ? L"简" : L"中";
}

void WriteBoolSetting(const wchar_t* key, bool value) {
  WritePrivateProfileStringW(L"General", key, value ? L"1" : L"0",
                             SettingsPath().c_str());
}

void NotifySettingsChanged() {
  McBopomofo::IPC::NamedPipeClient client(McBopomofo::IPC::PIPE_NAME);
  std::string response;
  client.Call(McBopomofo::IPC::SerializeReloadSettings(), response);
  SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, 0, SMTO_ABORTIFHUNG,
                      100, nullptr);
}

std::vector<MenuItem> BuildLangBarMenuItems(McBopomofoTIP* tip) {
  const bool isOpen = tip->IsOpen();
  const bool associatedPhrasesEnabled =
      ReadBoolSetting(L"AssociatedPhrasesEnabled", false);
  const bool halfWidthPunctuationEnabled =
      ReadBoolSetting(L"HalfWidthPunctuationEnabled", false);
  const bool chineseConversionEnabled =
      ReadBoolSetting(L"ChineseConversionEnabled", false);
  const bool bopomofoFontAnnotationEnabled =
      ReadBoolSetting(L"BopomofoFontAnnotationSupportEnabled", false);

  static std::wstring switchEnglish =
      McBopomofo::LoadLocalizedStringW(g_hInst, IDS_SWITCH_TO_ENGLISH);
  static std::wstring switchChinese =
      McBopomofo::LoadLocalizedStringW(g_hInst, IDS_SWITCH_TO_CHINESE);
  static std::wstring outputSimplified =
      McBopomofo::LoadLocalizedStringW(g_hInst, IDS_OUTPUT_SIMPLIFIED);
  static std::wstring outputTraditional =
      McBopomofo::LoadLocalizedStringW(g_hInst, IDS_OUTPUT_TRADITIONAL);
  static std::wstring punctuationHalf =
      McBopomofo::LoadLocalizedStringW(g_hInst, IDS_PUNCTUATION_HALF);
  static std::wstring punctuationFull =
      McBopomofo::LoadLocalizedStringW(g_hInst, IDS_PUNCTUATION_FULL);
  static std::wstring enableAssoc =
      McBopomofo::LoadLocalizedStringW(g_hInst, IDS_ENABLE_ASSOCIATED_PHRASES);
  static std::wstring enableAnnot =
      McBopomofo::LoadLocalizedStringW(g_hInst, IDS_ENABLE_BOPOMOFO_ANNOTATION);
  static std::wstring settingsStr =
      McBopomofo::LoadLocalizedStringW(g_hInst, IDS_SETTINGS);
  static std::wstring editUser =
      McBopomofo::LoadLocalizedStringW(g_hInst, IDS_EDIT_USER_PHRASES);
  static std::wstring editExcluded =
      McBopomofo::LoadLocalizedStringW(g_hInst, IDS_EDIT_EXCLUDED_PHRASES);
  static std::wstring openUserData =
      McBopomofo::LoadLocalizedStringW(g_hInst, IDS_OPEN_USER_DATA_FOLDER);

  return {
      {MENU_TOGGLE_OPEN_CLOSE,
       isOpen ? switchEnglish.c_str() : switchChinese.c_str(), false, false},
      {0, nullptr, false, true},
      {MENU_TOGGLE_CHINESE_CONVERSION,
       chineseConversionEnabled ? outputSimplified.c_str()
                                : outputTraditional.c_str(),
       false, false},
      {MENU_TOGGLE_HALF_WIDTH_PUNCTUATION,
       halfWidthPunctuationEnabled ? punctuationHalf.c_str()
                                   : punctuationFull.c_str(),
       false, false},
      {MENU_TOGGLE_ASSOCIATED_PHRASES, enableAssoc.c_str(),
       associatedPhrasesEnabled, false},
      {MENU_TOGGLE_BOPOMOFO_FONT_ANNOTATION, enableAnnot.c_str(),
       bopomofoFontAnnotationEnabled, false},
      {0, nullptr, false, true},
      {MENU_OPEN_SETTINGS, settingsStr.c_str(), false, false},
      {MENU_EDIT_USER_PHRASES, editUser.c_str(), false, false},
      {MENU_EDIT_EXCLUDED_PHRASES, editExcluded.c_str(), false, false},
      {MENU_OPEN_USER_DATA_FOLDER, openUserData.c_str(), false, false},
  };
}

void AppendPopupMenuItems(HMENU menu, const std::vector<MenuItem>& items) {
  for (const auto& item : items) {
    if (item.separator) {
      AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
      continue;
    }

    UINT flags = MF_STRING | (item.checked ? MF_CHECKED : MF_UNCHECKED);
    AppendMenuW(menu, flags, item.id, item.text);
  }
}

HRESULT AppendTfMenuItems(ITfMenu* menu, const std::vector<MenuItem>& items) {
  for (const auto& item : items) {
    if (item.separator) {
      HRESULT hr = menu->AddMenuItem(0, TF_LBMENUF_SEPARATOR, nullptr, nullptr,
                                     nullptr, 0, nullptr);
      if (FAILED(hr)) {
        return hr;
      }
      continue;
    }

    HRESULT hr = menu->AddMenuItem(
        item.id, item.checked ? TF_LBMENUF_CHECKED : 0, nullptr, nullptr,
        item.text, static_cast<ULONG>(wcslen(item.text)), nullptr);
    if (FAILED(hr)) {
      return hr;
    }
  }
  return S_OK;
}
}  // namespace

std::atomic<DWORD> CLangBarButton::_nextCookie = 1;

CLangBarButton::CLangBarButton(McBopomofoTIP* pTIP, const GUID& guid, Kind kind)
    : _refCount(1), _pTIP(pTIP), _guid(guid), _kind(kind) {
  if (_pTIP) _pTIP->AddRef();
}

CLangBarButton::~CLangBarButton() {
  for (auto& sink : _sinks) {
    if (sink.second) {
      sink.second->Release();
      sink.second = nullptr;
    }
  }
  if (_pTIP) _pTIP->Release();
}

STDMETHODIMP CLangBarButton::QueryInterface(REFIID riid, void** ppvObj) {
  if (ppvObj == nullptr) return E_INVALIDARG;
  *ppvObj = nullptr;

  if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfLangBarItem) ||
      IsEqualIID(riid, IID_ITfLangBarItemButton)) {
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

STDMETHODIMP CLangBarButton::GetInfo(TF_LANGBARITEMINFO* pInfo) {
  if (!pInfo) return E_INVALIDARG;
  pInfo->clsidService = c_clsidMcBopomofoTIP;
  pInfo->guidItem = _guid;
  pInfo->dwStyle = (_kind == Kind::ModeIcon)
                       ? (TF_LBI_STYLE_BTN_BUTTON | TF_LBI_STYLE_SHOWNINTRAY)
                       : TF_LBI_STYLE_BTN_MENU;
  pInfo->ulSort = 0;
  wcscpy_s(pInfo->szDescription, L" ");
  return S_OK;
}

STDMETHODIMP CLangBarButton::GetStatus(DWORD* pdwStatus) {
  if (!pdwStatus) return E_INVALIDARG;
  *pdwStatus = 0;
  return S_OK;
}

STDMETHODIMP CLangBarButton::Show(BOOL fShow) {
  UNREFERENCED_PARAMETER(fShow);
  return E_NOTIMPL;
}

STDMETHODIMP CLangBarButton::GetTooltipString(BSTR* pbstrToolTip) {
  if (!pbstrToolTip) return E_INVALIDARG;
  *pbstrToolTip = SysAllocString(
      McBopomofo::LoadLocalizedStringW(g_hInst, IDS_IME_MODE_TOOLTIP).c_str());
  return S_OK;
}

STDMETHODIMP CLangBarButton::OnClick(TfLBIClick click, POINT pt,
                                     const RECT* prcArea) {
  UNREFERENCED_PARAMETER(prcArea);

  if (_kind == Kind::SwitchLanguageMenu) {
    return S_OK;
  }

  if (click == TF_LBI_CLK_LEFT) {
    _pTIP->ToggleOpenClose();
  } else if (click == TF_LBI_CLK_RIGHT) {
    HMENU menu = CreatePopupMenu();
    if (menu) {
      AppendPopupMenuItems(menu, BuildLangBarMenuItems(_pTIP));

      HWND hwnd = CreateWindowExW(0, L"STATIC", L"", WS_POPUP, 0, 0, 0, 0,
                                  HWND_DESKTOP, nullptr, g_hInst, nullptr);
      if (!hwnd) {
        hwnd = GetDesktopWindow();
      }

      UINT command = TrackPopupMenu(
          menu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_LEFTALIGN | TPM_BOTTOMALIGN,
          pt.x, pt.y, 0, hwnd, nullptr);
      if (command != 0) {
        OnMenuSelect(command);
      }
      if (hwnd && hwnd != GetDesktopWindow()) {
        DestroyWindow(hwnd);
      }
      DestroyMenu(menu);
    }
  }
  return S_OK;
}

STDMETHODIMP CLangBarButton::InitMenu(ITfMenu* pMenu) {
  if (!pMenu) return E_INVALIDARG;
  return AppendTfMenuItems(pMenu, BuildLangBarMenuItems(_pTIP));
}

STDMETHODIMP CLangBarButton::OnMenuSelect(UINT wID) {
  switch (wID) {
    case MENU_TOGGLE_OPEN_CLOSE:
      _pTIP->ToggleOpenClose();
      break;
    case MENU_TOGGLE_ASSOCIATED_PHRASES: {
      bool enabled = !ReadBoolSetting(L"AssociatedPhrasesEnabled", false);
      WriteBoolSetting(L"AssociatedPhrasesEnabled", enabled);
      NotifySettingsChanged();
      _pTIP->RefreshLangBar();
      break;
    }
    case MENU_TOGGLE_HALF_WIDTH_PUNCTUATION: {
      bool enabled = !ReadBoolSetting(L"HalfWidthPunctuationEnabled", false);
      WriteBoolSetting(L"HalfWidthPunctuationEnabled", enabled);
      NotifySettingsChanged();
      _pTIP->RefreshLangBar();
      break;
    }
    case MENU_TOGGLE_CHINESE_CONVERSION: {
      bool enabled = !ReadBoolSetting(L"ChineseConversionEnabled", false);
      WriteBoolSetting(L"ChineseConversionEnabled", enabled);
      NotifySettingsChanged();
      _pTIP->RefreshLangBar();
      break;
    }
    case MENU_TOGGLE_BOPOMOFO_FONT_ANNOTATION: {
      bool enabled =
          !ReadBoolSetting(L"BopomofoFontAnnotationSupportEnabled", false);
      WriteBoolSetting(L"BopomofoFontAnnotationSupportEnabled", enabled);
      NotifySettingsChanged();
      _pTIP->RefreshLangBar();
      break;
    }
    case MENU_OPEN_SETTINGS: {
      McBopomofo::IPC::NamedPipeClient client(McBopomofo::IPC::PIPE_NAME);
      std::string response;
      client.Call(McBopomofo::IPC::SerializeOpenSettings(), response);
      break;
    }
    case MENU_EDIT_USER_PHRASES: {
      std::string path =
          McBopomofo::fcitx5_compat::userDirectory() + "/user.txt";
      ShellExecuteA(NULL, "open", path.c_str(), NULL, NULL, SW_SHOW);
      break;
    }
    case MENU_EDIT_EXCLUDED_PHRASES: {
      std::string path =
          McBopomofo::fcitx5_compat::userDirectory() + "/exclude.txt";
      ShellExecuteA(NULL, "open", path.c_str(), NULL, NULL, SW_SHOW);
      break;
    }
    case MENU_OPEN_USER_DATA_FOLDER: {
      std::string path = McBopomofo::fcitx5_compat::userDirectory();
      ShellExecuteA(NULL, "open", path.c_str(), NULL, NULL, SW_SHOW);
      break;
    }
  }
  return S_OK;
}

#include "resource.h"
extern HINSTANCE g_hInst;

STDMETHODIMP CLangBarButton::GetIcon(HICON* phIcon) {
  if (!phIcon) return E_INVALIDARG;
  *phIcon = nullptr;

  const wchar_t* label = CurrentModeLabel(_pTIP);
  LogMessage("CLangBarButton::GetIcon called with label: %ls", label);

  HDC hdc = GetDC(NULL);
  HDC hMemDC = CreateCompatibleDC(hdc);
  HBITMAP hBitmap = CreateCompatibleBitmap(hdc, 16, 16);
  HBITMAP hMaskBitmap = CreateBitmap(16, 16, 1, 1, NULL);

  HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);

  RECT rc = {0, 0, 16, 16};
  FillRect(hMemDC, &rc, (HBRUSH)(COLOR_WINDOW + 1));
  SetBkMode(hMemDC, TRANSPARENT);
  SetTextColor(hMemDC, RGB(0, 0, 0));

  HFONT hFont =
      CreateFontW(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                  OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                  DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
  HFONT hOldFont = (HFONT)SelectObject(hMemDC, hFont);

  DrawTextW(hMemDC, label, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

  SelectObject(hMemDC, hOldFont);
  DeleteObject(hFont);
  SelectObject(hMemDC, hOldBitmap);

  ICONINFO ii = {0};
  ii.fIcon = TRUE;
  ii.hbmMask = hMaskBitmap;
  ii.hbmColor = hBitmap;
  *phIcon = CreateIconIndirect(&ii);

  DeleteObject(hMaskBitmap);
  DeleteObject(hBitmap);
  DeleteDC(hMemDC);
  ReleaseDC(NULL, hdc);

  LogMessage("CLangBarButton::GetIcon created icon: %p", *phIcon);

  return S_OK;
}

STDMETHODIMP CLangBarButton::GetText(BSTR* pbstrText) {
  if (!pbstrText) return E_INVALIDARG;
  *pbstrText = SysAllocString(CurrentModeLabel(_pTIP));
  return S_OK;
}

STDMETHODIMP CLangBarButton::AdviseSink(REFIID riid, IUnknown* punk,
                                        DWORD* pdwCookie) {
  if (!pdwCookie || !punk) return E_INVALIDARG;
  *pdwCookie = TF_INVALID_COOKIE;
  if (!IsEqualIID(riid, IID_ITfLangBarItemSink)) return E_NOINTERFACE;

  ITfLangBarItemSink* pSink = nullptr;
  if (FAILED(punk->QueryInterface(IID_ITfLangBarItemSink, (void**)&pSink)))
    return E_NOINTERFACE;

  *pdwCookie = _nextCookie++;
  _sinks.emplace_back(*pdwCookie, pSink);
  return S_OK;
}

STDMETHODIMP CLangBarButton::UnadviseSink(DWORD dwCookie) {
  auto it = std::find_if(
      _sinks.begin(), _sinks.end(),
      [dwCookie](const auto& item) { return item.first == dwCookie; });
  if (it == _sinks.end()) {
    return E_INVALIDARG;
  }

  if (it->second) {
    it->second->Release();
  }
  _sinks.erase(it);
  return S_OK;
}

void CLangBarButton::Update() {
  LogMessage("CLangBarButton::Update called, sink count: %zu", _sinks.size());

  // Notify all registered sinks
  for (const auto& sink : _sinks) {
    if (sink.second) {
      HRESULT hr =
          sink.second->OnUpdate(TF_LBI_ICON | TF_LBI_TEXT | TF_LBI_TOOLTIP);
      LogMessage("Sink OnUpdate returned: 0x%08X", hr);
    }
  }
}
