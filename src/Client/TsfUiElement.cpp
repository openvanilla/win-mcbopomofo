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

#include "TsfUiElement.h"
#include "Globals.h"
#include "McBopomofoTIP.h"
#include "UTFHelper.h"
#include "NamedPipe.h"
#include "Ipc.h"

namespace {
bool GetFocusedContext(ITfThreadMgr* threadMgr, ITfContext** context) {
  if (!threadMgr || !context) return false;
  *context = nullptr;
  ITfDocumentMgr* docMgr = nullptr;
  if (SUCCEEDED(threadMgr->GetFocus(&docMgr)) && docMgr) {
    docMgr->GetTop(context);
    docMgr->Release();
  }
  return *context != nullptr;
}
}  // namespace

// ==========================================
// CCandidateListUIElement Implementation
// ==========================================

CCandidateListUIElement::CCandidateListUIElement(McBopomofoTIP* pTIP)
    : pTIP_(pTIP) {
  CoCreateGuid(&guid_);
  DllAddRef();
}

CCandidateListUIElement::~CCandidateListUIElement() {
  if (pActiveContext_) {
    pActiveContext_->Release();
  }
  DllRelease();
}

STDMETHODIMP CCandidateListUIElement::QueryInterface(REFIID riid, void** ppvObj) {
  if (ppvObj == nullptr) {
    return E_INVALIDARG;
  }
  *ppvObj = nullptr;

  if (IsEqualIID(riid, IID_IUnknown)) {
    *ppvObj = static_cast<IUnknown*>(static_cast<ITfCandidateListUIElementBehavior*>(this));
  } else if (IsEqualIID(riid, IID_ITfUIElement)) {
    *ppvObj = static_cast<ITfUIElement*>(static_cast<ITfCandidateListUIElementBehavior*>(this));
  } else if (IsEqualIID(riid, IID_ITfCandidateListUIElement)) {
    *ppvObj = static_cast<ITfCandidateListUIElement*>(static_cast<ITfCandidateListUIElementBehavior*>(this));
  } else if (IsEqualIID(riid, IID_ITfCandidateListUIElementBehavior)) {
    *ppvObj = static_cast<ITfCandidateListUIElementBehavior*>(this);
  }

  if (*ppvObj) {
    AddRef();
    return S_OK;
  }
  return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CCandidateListUIElement::AddRef() {
  return InterlockedIncrement(&cRef_);
}

STDMETHODIMP_(ULONG) CCandidateListUIElement::Release() {
  LONG cRef = InterlockedDecrement(&cRef_);
  if (cRef == 0) {
    delete this;
  }
  return cRef;
}

STDMETHODIMP CCandidateListUIElement::GetDescription(BSTR* pbstr) {
  if (pbstr == nullptr) {
    return E_INVALIDARG;
  }
  *pbstr = SysAllocString(L"McBopomofo Candidate List");
  return *pbstr ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP CCandidateListUIElement::GetGUID(GUID* pguid) {
  if (pguid == nullptr) {
    return E_INVALIDARG;
  }
  *pguid = guid_;
  return S_OK;
}

STDMETHODIMP CCandidateListUIElement::IsShown(BOOL* pfShow) {
  if (pfShow == nullptr) {
    return E_INVALIDARG;
  }
  *pfShow = fShown_;
  return S_OK;
}

STDMETHODIMP CCandidateListUIElement::Show(BOOL fShow) {
  fShown_ = fShow;
  return S_OK;
}

STDMETHODIMP CCandidateListUIElement::GetUpdatedFlags(DWORD* pdwFlags) {
  if (pdwFlags == nullptr) {
    return E_INVALIDARG;
  }
  *pdwFlags = TF_CLUIE_DOCUMENTMGR | TF_CLUIE_COUNT | TF_CLUIE_SELECTION |
              TF_CLUIE_STRING | TF_CLUIE_PAGEINDEX | TF_CLUIE_CURRENTPAGE;
  return S_OK;
}

STDMETHODIMP CCandidateListUIElement::GetDocumentMgr(ITfDocumentMgr** ppdim) {
  if (ppdim == nullptr) {
    return E_INVALIDARG;
  }
  *ppdim = nullptr;
  if (pActiveContext_) {
    return pActiveContext_->GetDocumentMgr(ppdim);
  }
  return S_OK;
}

STDMETHODIMP CCandidateListUIElement::GetCount(UINT* puCount) {
  if (puCount == nullptr) {
    return E_INVALIDARG;
  }
  *puCount = static_cast<UINT>(candidates_.size());
  return S_OK;
}

STDMETHODIMP CCandidateListUIElement::GetSelection(UINT* puIndex) {
  if (puIndex == nullptr) {
    return E_INVALIDARG;
  }
  *puIndex = static_cast<UINT>(selectionIndex_);
  return S_OK;
}

STDMETHODIMP CCandidateListUIElement::GetString(UINT uIndex, BSTR* pbstr) {
  if (pbstr == nullptr) {
    return E_INVALIDARG;
  }
  *pbstr = nullptr;
  if (uIndex >= candidates_.size()) {
    return E_INVALIDARG;
  }
  *pbstr = SysAllocString(candidates_[uIndex].c_str());
  return *pbstr ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP CCandidateListUIElement::GetPageIndex(UINT* puIndex, UINT uSize, UINT* puPageCnt) {
  if (puPageCnt == nullptr) {
    return E_INVALIDARG;
  }

  UINT pageSize = (candidateKeysCount_ > 0) ? candidateKeysCount_ : 9;
  UINT totalCount = static_cast<UINT>(candidates_.size());
  UINT totalPages = (totalCount + pageSize - 1) / pageSize;
  if (totalPages == 0 && totalCount > 0) {
    totalPages = 1;
  }

  *puPageCnt = totalPages;

  if (puIndex != nullptr) {
    for (UINT i = 0; i < uSize && i < totalPages; ++i) {
      puIndex[i] = i * pageSize;
    }
  }

  return S_OK;
}

STDMETHODIMP CCandidateListUIElement::SetPageIndex(UINT* puIndex, UINT uPageCnt) {
  UNREFERENCED_PARAMETER(puIndex);
  UNREFERENCED_PARAMETER(uPageCnt);
  return S_OK;
}

STDMETHODIMP CCandidateListUIElement::GetCurrentPage(UINT* puPage) {
  if (puPage == nullptr) {
    return E_INVALIDARG;
  }
  UINT pageSize = (candidateKeysCount_ > 0) ? candidateKeysCount_ : 9;
  *puPage = static_cast<UINT>(selectionIndex_ / pageSize);
  return S_OK;
}

STDMETHODIMP CCandidateListUIElement::SetSelection(UINT uIndex) {
  if (uIndex >= candidates_.size()) {
    return E_INVALIDARG;
  }

  if (!pTIP_) {
    return E_UNEXPECTED;
  }

  McBopomofo::IPC::NamedPipeClient pipe(McBopomofo::IPC::PIPE_NAME);
  McBopomofo::IPC::SelectCandidatePayload payload;
  payload.index = static_cast<int>(uIndex);

  std::string request = McBopomofo::IPC::SerializeSelectCandidate(payload);
  std::string response;

  if (pipe.Call(request, response)) {
    McBopomofo::IPC::StateUpdatePayload newState;
    if (McBopomofo::IPC::DeserializeStateUpdate(response, newState)) {
      ITfContext* pContext = pActiveContext_;
      if (!pContext) {
        GetFocusedContext(pTIP_->GetThreadMgr(), &pContext);
      } else {
        pContext->AddRef();
      }

      if (pContext) {
        pTIP_->applyStateToContext_(pContext, newState, "SetSelection ");
        pContext->Release();
      }
    }
  }

  return S_OK;
}

STDMETHODIMP CCandidateListUIElement::Finalize(void) {
  return SetSelection(static_cast<UINT>(selectionIndex_));
}

STDMETHODIMP CCandidateListUIElement::Abort(void) {
  return S_OK;
}

void CCandidateListUIElement::UpdateData(const std::vector<std::string>& candidates,
                                         int selectionIndex,
                                         const std::string& candidateKeys,
                                         int candidateKeysCount) {
  candidates_.clear();
  for (const auto& cand : candidates) {
    candidates_.push_back(McBopomofo::Utf8ToUtf16(cand));
  }
  selectionIndex_ = (selectionIndex >= 0) ? selectionIndex : 0;
  candidateKeys_ = McBopomofo::Utf8ToUtf16(candidateKeys);
  candidateKeysCount_ = candidateKeysCount;
}

void CCandidateListUIElement::SetActiveContext(ITfContext* pContext) {
  if (pActiveContext_) {
    pActiveContext_->Release();
  }
  pActiveContext_ = pContext;
  if (pActiveContext_) {
    pActiveContext_->AddRef();
  }
}

void CCandidateListUIElement::SetShown(BOOL fShow) {
  fShown_ = fShow;
}

void CCandidateListUIElement::ClearTip() {
  pTIP_ = nullptr;
}

// ==========================================
// CReadingInformationUIElement Implementation
// ==========================================

CReadingInformationUIElement::CReadingInformationUIElement(McBopomofoTIP* pTIP)
    : pTIP_(pTIP) {
  CoCreateGuid(&guid_);
  DllAddRef();
}

CReadingInformationUIElement::~CReadingInformationUIElement() {
  if (pActiveContext_) {
    pActiveContext_->Release();
  }
  DllRelease();
}

STDMETHODIMP CReadingInformationUIElement::QueryInterface(REFIID riid, void** ppvObj) {
  if (ppvObj == nullptr) {
    return E_INVALIDARG;
  }
  *ppvObj = nullptr;

  if (IsEqualIID(riid, IID_IUnknown)) {
    *ppvObj = static_cast<IUnknown*>(static_cast<ITfReadingInformationUIElement*>(this));
  } else if (IsEqualIID(riid, IID_ITfUIElement)) {
    *ppvObj = static_cast<ITfUIElement*>(static_cast<ITfReadingInformationUIElement*>(this));
  } else if (IsEqualIID(riid, IID_ITfReadingInformationUIElement)) {
    *ppvObj = static_cast<ITfReadingInformationUIElement*>(this);
  }

  if (*ppvObj) {
    AddRef();
    return S_OK;
  }
  return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CReadingInformationUIElement::AddRef() {
  return InterlockedIncrement(&cRef_);
}

STDMETHODIMP_(ULONG) CReadingInformationUIElement::Release() {
  LONG cRef = InterlockedDecrement(&cRef_);
  if (cRef == 0) {
    delete this;
  }
  return cRef;
}

STDMETHODIMP CReadingInformationUIElement::GetDescription(BSTR* pbstr) {
  if (pbstr == nullptr) {
    return E_INVALIDARG;
  }
  *pbstr = SysAllocString(L"McBopomofo Reading Information");
  return *pbstr ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP CReadingInformationUIElement::GetGUID(GUID* pguid) {
  if (pguid == nullptr) {
    return E_INVALIDARG;
  }
  *pguid = guid_;
  return S_OK;
}

STDMETHODIMP CReadingInformationUIElement::IsShown(BOOL* pfShow) {
  if (pfShow == nullptr) {
    return E_INVALIDARG;
  }
  *pfShow = fShown_;
  return S_OK;
}

STDMETHODIMP CReadingInformationUIElement::Show(BOOL fShow) {
  fShown_ = fShow;
  return S_OK;
}

STDMETHODIMP CReadingInformationUIElement::GetUpdatedFlags(DWORD* pdwFlags) {
  if (pdwFlags == nullptr) {
    return E_INVALIDARG;
  }
  *pdwFlags = TF_RIUIE_CONTEXT | TF_RIUIE_STRING | TF_RIUIE_MAXREADINGSTRINGLENGTH |
              TF_RIUIE_ERRORINDEX | TF_RIUIE_VERTICALORDER;
  return S_OK;
}

STDMETHODIMP CReadingInformationUIElement::GetContext(ITfContext** ppic) {
  if (ppic == nullptr) {
    return E_INVALIDARG;
  }
  *ppic = pActiveContext_;
  if (*ppic) {
    (*ppic)->AddRef();
  }
  return S_OK;
}

STDMETHODIMP CReadingInformationUIElement::GetString(BSTR* pbstr) {
  if (pbstr == nullptr) {
    return E_INVALIDARG;
  }
  *pbstr = SysAllocString(readingString_.c_str());
  return *pbstr ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP CReadingInformationUIElement::GetMaxReadingStringLength(UINT* puMaxLen) {
  if (puMaxLen == nullptr) {
    return E_INVALIDARG;
  }
  *puMaxLen = static_cast<UINT>(readingString_.length());
  return S_OK;
}

STDMETHODIMP CReadingInformationUIElement::GetErrorIndex(UINT* puErrorIndex) {
  if (puErrorIndex == nullptr) {
    return E_INVALIDARG;
  }
  *puErrorIndex = 0;
  return S_OK;
}

STDMETHODIMP CReadingInformationUIElement::IsVerticalOrderPreferred(BOOL* pfVertical) {
  if (pfVertical == nullptr) {
    return E_INVALIDARG;
  }
  *pfVertical = FALSE;
  return S_OK;
}

void CReadingInformationUIElement::UpdateData(const std::string& readingString) {
  readingString_ = McBopomofo::Utf8ToUtf16(readingString);
}

void CReadingInformationUIElement::SetActiveContext(ITfContext* pContext) {
  if (pActiveContext_) {
    pActiveContext_->Release();
  }
  pActiveContext_ = pContext;
  if (pActiveContext_) {
    pActiveContext_->AddRef();
  }
}

void CReadingInformationUIElement::SetShown(BOOL fShow) {
  fShown_ = fShow;
}

void CReadingInformationUIElement::ClearTip() {
  pTIP_ = nullptr;
}
