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
#include <string>
#include <vector>

class McBopomofoTIP;

// CCandidateListUIElement is the TSF-side candidate list object exposed through
// ITfUIElementMgr. It is used when the text service has candidate data to show
// and calls BeginUIElement/UpdateUIElement so the host application or the system
// can present the candidate list via the standard TSF UIElement mechanism,
// instead of relying only on this project's custom CandidateWindow popup.
class CCandidateListUIElement : public ITfCandidateListUIElementBehavior {
 public:
  CCandidateListUIElement(McBopomofoTIP* pTIP);
  virtual ~CCandidateListUIElement();

  // IUnknown methods
  STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj) override;
  STDMETHODIMP_(ULONG) AddRef(void) override;
  STDMETHODIMP_(ULONG) Release(void) override;

  // ITfUIElement methods
  STDMETHODIMP GetDescription(BSTR* pbstr) override;
  STDMETHODIMP GetGUID(GUID* pguid) override;
  STDMETHODIMP IsShown(BOOL* pfShow) override;
  STDMETHODIMP Show(BOOL fShow) override;

  // ITfCandidateListUIElement methods
  STDMETHODIMP GetUpdatedFlags(DWORD* pdwFlags) override;
  STDMETHODIMP GetDocumentMgr(ITfDocumentMgr** ppdim) override;
  STDMETHODIMP GetCount(UINT* puCount) override;
  STDMETHODIMP GetSelection(UINT* puIndex) override;
  STDMETHODIMP GetString(UINT uIndex, BSTR* pbstr) override;
  STDMETHODIMP GetPageIndex(UINT* puIndex, UINT uSize, UINT* puPageCnt) override;
  STDMETHODIMP SetPageIndex(UINT* puIndex, UINT uPageCnt) override;
  STDMETHODIMP GetCurrentPage(UINT* puPage) override;

  // ITfCandidateListUIElementBehavior methods
  STDMETHODIMP SetSelection(UINT uIndex) override;
  STDMETHODIMP Finalize(void) override;
  STDMETHODIMP Abort(void) override;

  // State Management
  void UpdateData(const std::vector<std::string>& candidates, int selectionIndex,
                  const std::string& candidateKeys, int candidateKeysCount);
  void SetActiveContext(ITfContext* pContext);
  void SetShown(BOOL fShow);
  void ClearTip();
  void ResetDiagnostics();
  bool HasHostInteraction() const;
  const char* LastHostMethod() const;
  unsigned long HostInteractionCount() const;

 private:
  void noteHostInteraction_(const char* methodName);

  LONG cRef_ = 1;
  McBopomofoTIP* pTIP_;
  GUID guid_;
  BOOL fShown_ = FALSE;

  std::vector<std::wstring> candidates_;
  int selectionIndex_ = 0;
  std::wstring candidateKeys_ = L"123456789";
  int candidateKeysCount_ = 9;

  ITfContext* pActiveContext_ = nullptr;
  unsigned long hostInteractionCount_ = 0;
  const char* lastHostMethod_ = "none";
};

// CReadingInformationUIElement class
class CReadingInformationUIElement : public ITfReadingInformationUIElement {
 public:
  CReadingInformationUIElement(McBopomofoTIP* pTIP);
  virtual ~CReadingInformationUIElement();

  // IUnknown methods
  STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj) override;
  STDMETHODIMP_(ULONG) AddRef(void) override;
  STDMETHODIMP_(ULONG) Release(void) override;

  // ITfUIElement methods
  STDMETHODIMP GetDescription(BSTR* pbstr) override;
  STDMETHODIMP GetGUID(GUID* pguid) override;
  STDMETHODIMP IsShown(BOOL* pfShow) override;
  STDMETHODIMP Show(BOOL fShow) override;

  // ITfReadingInformationUIElement methods
  STDMETHODIMP GetUpdatedFlags(DWORD* pdwFlags) override;
  STDMETHODIMP GetContext(ITfContext** ppic) override;
  STDMETHODIMP GetString(BSTR* pbstr) override;
  STDMETHODIMP GetMaxReadingStringLength(UINT* puMaxLen) override;
  STDMETHODIMP GetErrorIndex(UINT* puErrorIndex) override;
  STDMETHODIMP IsVerticalOrderPreferred(BOOL* pfVertical) override;

  // State Management
  void UpdateData(const std::string& readingString);
  void SetActiveContext(ITfContext* pContext);
  void SetShown(BOOL fShow);
  void ClearTip();

 private:
  LONG cRef_ = 1;
  McBopomofoTIP* pTIP_;
  GUID guid_;
  BOOL fShown_ = FALSE;

  std::wstring readingString_;
  ITfContext* pActiveContext_ = nullptr;
};
