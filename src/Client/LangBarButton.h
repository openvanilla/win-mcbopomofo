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

#pragma once
#include <msctf.h>
#include <windows.h>

#include <atomic>
#include <string>
#include <utility>
#include <vector>

class McBopomofoTIP;

extern const GUID GUID_LBI_INPUTMODE;
extern const GUID GUID_LBI_SWITCH_LANG;
extern const GUID GUID_LBI_FULL_HALF;
extern const GUID GUID_LBI_SETTINGS;

class CLangBarButton : public ITfLangBarItemButton, public ITfSource {
 public:
  enum class Kind {
    ImeModeMenu,
    FullHalfToggle,
    SwitchLanguageToggle,
    SettingsMenu,
  };

  CLangBarButton(McBopomofoTIP* pTIP, const GUID& guid, Kind kind);
  ~CLangBarButton();

  // IUnknown
  STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj) override;
  STDMETHODIMP_(ULONG) AddRef(void) override;
  STDMETHODIMP_(ULONG) Release(void) override;

  // ITfLangBarItem
  STDMETHODIMP GetInfo(TF_LANGBARITEMINFO* pInfo) override;
  STDMETHODIMP GetStatus(DWORD* pdwStatus) override;
  STDMETHODIMP Show(BOOL fShow) override;
  STDMETHODIMP GetTooltipString(BSTR* pbstrToolTip) override;

  // ITfLangBarItemButton
  STDMETHODIMP OnClick(TfLBIClick click, POINT pt,
                       const RECT* prcArea) override;
  STDMETHODIMP InitMenu(ITfMenu* pMenu) override;
  STDMETHODIMP OnMenuSelect(UINT wID) override;
  STDMETHODIMP GetIcon(HICON* phIcon) override;
  STDMETHODIMP GetText(BSTR* pbstrText) override;

  // ITfSource
  STDMETHODIMP AdviseSink(REFIID riid, IUnknown* punk,
                          DWORD* pdwCookie) override;
  STDMETHODIMP UnadviseSink(DWORD dwCookie) override;

  void Update();

 private:
  long refCount_;
  McBopomofoTIP* pTIP_;
  GUID guid_;
  Kind kind_;
  std::vector<std::pair<DWORD, ITfLangBarItemSink*>> sinks_;
  static std::atomic<DWORD> nextCookie_;
};
