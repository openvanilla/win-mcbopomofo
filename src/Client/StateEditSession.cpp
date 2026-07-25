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

#include "StateEditSession.h"

#include <algorithm>

#include "DisplayAttributeInfo.h"
#include "Globals.h"
#include "UTFHelper.h"

namespace {

HWND GetContextWindow(ITfContext* context);

void LogContextWindowInfo(const char* prefix, ITfContext* context) {
  UNREFERENCED_PARAMETER(prefix);
  HWND hwnd = GetContextWindow(context);
  char className[128] = {};
  char title[128] = {};
  DWORD pid = 0;

  if (hwnd) {
    GetClassNameA(hwnd, className, static_cast<int>(sizeof(className)));
    GetWindowTextA(hwnd, title, static_cast<int>(sizeof(title)));
    GetWindowThreadProcessId(hwnd, &pid);
  }

  // LogMessage("%s hwnd=%p pid=%lu class=%s title=%s", prefix, hwnd,
  //            static_cast<unsigned long>(pid), className[0] ? className : "-",
  //            title[0] ? title : "-");
}

HWND GetContextWindow(ITfContext* context) {
  if (context) {
    ITfContextView* pView = nullptr;
    if (SUCCEEDED(context->GetActiveView(&pView)) && pView) {
      HWND hwnd = nullptr;
      if (SUCCEEDED(pView->GetWnd(&hwnd)) && hwnd) {
        pView->Release();
        return hwnd;
      }
      pView->Release();
    }
  }
  return GetFocus();
}

}  // namespace

CStateEditSession::CStateEditSession(
    ITfContext* pContext, McBopomofoTIP* pTIP,
    const McBopomofo::IPC::StateUpdatePayload& state)
    : CEditSessionBase(pContext), pTIP_(pTIP), state_(state) {
  if (pTIP_) pTIP_->AddRef();
}

CStateEditSession::~CStateEditSession() {
  if (pTIP_) pTIP_->Release();
}

void SetDisplayAttribute(TfEditCookie ec, ITfContext* pContext,
                         ITfRange* pRange, TfGuidAtom guidAtom) {
  ITfProperty* pProp = nullptr;
  if (SUCCEEDED(pContext->GetProperty(GUID_PROP_ATTRIBUTE, &pProp))) {
    VARIANT var;
    var.vt = VT_I4;
    var.lVal = guidAtom;
    pProp->SetValue(ec, pRange, &var);
    pProp->Release();
  }
}

STDAPI CStateEditSession::DoEditSession(TfEditCookie ec) {
  std::wstring commitStr = McBopomofo::Utf8ToUtf16(state_.commitString);
  std::wstring compStr = McBopomofo::Utf8ToUtf16(state_.composingBuffer);
  const bool directCommitWithoutComposition =
      !commitStr.empty() && compStr.empty() &&
      pTIP_->GetComposition() == nullptr;

  // 1. Handle Committing Text
  if (!commitStr.empty()) {
    if (pTIP_->GetComposition()) {
      ITfRange* pRange = nullptr;
      if (SUCCEEDED(pTIP_->GetComposition()->GetRange(&pRange))) {
        pRange->SetText(ec, 0, commitStr.c_str(), (LONG)commitStr.length());

        // Clear display attributes when committing
        ITfProperty* pProp = nullptr;
        if (SUCCEEDED(pContext_->GetProperty(GUID_PROP_ATTRIBUTE, &pProp))) {
          pProp->Clear(ec, pRange);
          pProp->Release();
        }

        pRange->Collapse(ec, TF_ANCHOR_END);
        TF_SELECTION sel;
        sel.range = pRange;
        sel.style.ase = TF_AE_NONE;
        sel.style.fInterimChar = FALSE;
        pContext_->SetSelection(ec, 1, &sel);

        pTIP_->GetComposition()->EndComposition(ec);
        pTIP_->GetComposition()->Release();
        pTIP_->SetComposition(nullptr);
        pRange->Release();
      }
    } else {
      // No composition active, create a temporary one for insertion.
      // This approach is safer than calling
      // InsertTextAtSelection(TF_IAS_NOQUERY, ...) directly on some TSF hosts
      // (e.g., Notepad), which can cause access violations.
      //
      // Strategy:
      // 1. Query the current selection position safely (TF_IAS_QUERYONLY)
      // 2. Create a temporary composition at that position
      // 3. Insert text into the composition range
      // 4. Move cursor to the end of inserted text
      // 5. End the composition to commit all changes atomically
      ITfInsertAtSelection* pInsert = nullptr;
      if (SUCCEEDED(pContext_->QueryInterface(IID_ITfInsertAtSelection,
                                              (void**)&pInsert))) {
        ITfRange* pRange = nullptr;
        // First, query the selection position without modifying anything
        // (TF_IAS_QUERYONLY flag)
        if (SUCCEEDED(pInsert->InsertTextAtSelection(ec, TF_IAS_QUERYONLY, NULL,
                                                     0, &pRange)) &&
            pRange) {
          // Now we have a valid range at the current selection
          // Create a composition at this position
          ITfContextComposition* pContextComp = nullptr;
          if (SUCCEEDED(pContext_->QueryInterface(IID_ITfContextComposition,
                                                  (void**)&pContextComp))) {
            ITfComposition* pComp = nullptr;
            if (SUCCEEDED(pContextComp->StartComposition(ec, pRange, pTIP_,
                                                         &pComp)) &&
                pComp) {
              // Insert text into the composition range
              pRange->SetText(ec, 0, commitStr.c_str(),
                              (LONG)commitStr.length());

              // Move cursor to the end of inserted text
              pRange->Collapse(ec, TF_ANCHOR_END);
              TF_SELECTION sel;
              sel.range = pRange;
              sel.style.ase = TF_AE_NONE;
              sel.style.fInterimChar = FALSE;
              pContext_->SetSelection(ec, 1, &sel);

              // Immediately end the composition to commit all changes
              pComp->EndComposition(ec);
              pComp->Release();
            }
            pContextComp->Release();
          }
          pRange->Release();
        }
        pInsert->Release();
      }
    }

    if (directCommitWithoutComposition) {
      if (pTIP_->GetUIElementMgr()) {
        auto* pUIElementMgr = pTIP_->GetUIElementMgr();
        DWORD dwCandId = pTIP_->GetCandidateUIElementId();
        if (dwCandId != 0) {
          pUIElementMgr->EndUIElement(dwCandId);
          pTIP_->SetCandidateUIElementId(0);
        }
        if (pTIP_->GetCandidateUIElement()) {
          pTIP_->GetCandidateUIElement()->SetShown(FALSE);
        }

        DWORD dwReadingId = pTIP_->GetReadingUIElementId();
        if (dwReadingId != 0) {
          pUIElementMgr->EndUIElement(dwReadingId);
          pTIP_->SetReadingUIElementId(0);
        }
        if (pTIP_->GetReadingUIElement()) {
          pTIP_->GetReadingUIElement()->SetShown(FALSE);
        }
      }
      return S_OK;
    }
  }

  // 2. Handle Composing Text
  if (!compStr.empty()) {
    ITfRange* pRange = nullptr;
    if (!pTIP_->GetComposition()) {
      // Start composition
      ITfInsertAtSelection* pInsert = nullptr;
      if (SUCCEEDED(pContext_->QueryInterface(IID_ITfInsertAtSelection,
                                              (void**)&pInsert))) {
        pInsert->InsertTextAtSelection(ec, TF_IAS_QUERYONLY, NULL, 0, &pRange);
        pInsert->Release();
      }
      if (pRange) {
        ITfContextComposition* pContextComp = nullptr;
        if (SUCCEEDED(pContext_->QueryInterface(IID_ITfContextComposition,
                                                (void**)&pContextComp))) {
          ITfComposition* pComp = nullptr;
          if (SUCCEEDED(
                  pContextComp->StartComposition(ec, pRange, pTIP_, &pComp)) &&
              pComp) {
            pTIP_->SetComposition(pComp);
          }
          pContextComp->Release();
        }
      }
    } else {
      pTIP_->GetComposition()->GetRange(&pRange);
    }

    if (pRange && pTIP_->GetComposition()) {
      pRange->SetText(ec, 0, compStr.c_str(), (LONG)compStr.length());

      // Apply Display Attributes
      ITfCategoryMgr* pCategoryMgr = nullptr;
      if (SUCCEEDED(CoCreateInstance(CLSID_TF_CategoryMgr, nullptr,
                                     CLSCTX_INPROC_SERVER, IID_ITfCategoryMgr,
                                     (void**)&pCategoryMgr))) {
        TfGuidAtom gaInput = TF_INVALID_GUIDATOM;
        TfGuidAtom gaMarked = TF_INVALID_GUIDATOM;
        pCategoryMgr->RegisterGUID(c_guidDisplayAttributeInput, &gaInput);
        pCategoryMgr->RegisterGUID(c_guidDisplayAttributeMarked, &gaMarked);

        // Apply input attribute to the entire composing string first
        SetDisplayAttribute(ec, pContext_, pRange, gaInput);

        if (state_.markStart >= 0 && state_.markEnd >= 0) {
          // Apply marking attribute to the marked portion
          size_t startOffset = McBopomofo::Utf8OffsetToUtf16Offset(
              state_.composingBuffer, state_.markStart);
          size_t endOffset = McBopomofo::Utf8OffsetToUtf16Offset(
              state_.composingBuffer, state_.markEnd);
          size_t markLength =
              endOffset >= startOffset ? endOffset - startOffset : 0;

          ITfRange* pMarkRange = nullptr;
          if (SUCCEEDED(pRange->Clone(&pMarkRange))) {
            LONG cch = 0;
            // Collapse to start, then shift to the marked range
            pMarkRange->Collapse(ec, TF_ANCHOR_START);
            pMarkRange->ShiftStart(ec, (LONG)startOffset, &cch, nullptr);
            pMarkRange->ShiftEnd(ec, (LONG)markLength, &cch, nullptr);
            SetDisplayAttribute(ec, pContext_, pMarkRange, gaMarked);
            pMarkRange->Release();
          }
        }

        pCategoryMgr->Release();
      }

      // Set caret at cursorIndex
      ITfRange* pCursorRange = nullptr;
      if (SUCCEEDED(pRange->Clone(&pCursorRange))) {
        LONG cch = 0;
        size_t utf16CursorIndex = McBopomofo::Utf8OffsetToUtf16Offset(
            state_.composingBuffer, state_.cursorIndex);
        pCursorRange->Collapse(ec, TF_ANCHOR_START);
        pCursorRange->ShiftEnd(ec, (LONG)utf16CursorIndex, &cch, nullptr);
        pCursorRange->Collapse(ec, TF_ANCHOR_END);

        TF_SELECTION sel;
        sel.range = pCursorRange;
        sel.style.ase = TF_AE_NONE;
        sel.style.fInterimChar = FALSE;
        pContext_->SetSelection(ec, 1, &sel);

        // Update UI content first so windows have correct sizes
        if (pTIP_->GetUIElementMgr() && pTIP_->GetReadingUIElement()) {
          auto* pUIElementMgr = pTIP_->GetUIElementMgr();
          auto* pReadingElement = pTIP_->GetReadingUIElement();
          pReadingElement->SetActiveContext(pContext_);

          if (!state_.tooltip.empty()) {
            pReadingElement->UpdateData(state_.tooltip);
            pReadingElement->SetShown(TRUE);

            DWORD dwId = pTIP_->GetReadingUIElementId();
            if (dwId == 0) {
              BOOL bShow = TRUE;
              if (SUCCEEDED(pUIElementMgr->BeginUIElement(pReadingElement,
                                                          &bShow, &dwId))) {
                pTIP_->SetReadingUIElementId(dwId);
              }
            } else {
              pUIElementMgr->UpdateUIElement(dwId);
            }
          } else {
            pReadingElement->SetShown(FALSE);
            DWORD dwId = pTIP_->GetReadingUIElementId();
            if (dwId != 0) {
              pUIElementMgr->EndUIElement(dwId);
              pTIP_->SetReadingUIElementId(0);
            }
          }
        }

        if (pTIP_->GetUIElementMgr() && pTIP_->GetCandidateUIElement()) {
          auto* pUIElementMgr = pTIP_->GetUIElementMgr();
          auto* pCandElement = pTIP_->GetCandidateUIElement();
          pCandElement->SetActiveContext(pContext_);

          if (!state_.candidates.empty()) {
            LogContextWindowInfo("CandidateUI composition host", pContext_);
            pCandElement->UpdateData(state_.candidates, state_.candidateIndex,
                                     state_.candidateKeys,
                                     state_.candidateKeysCount);
            pCandElement->SetShown(TRUE);

            DWORD dwId = pTIP_->GetCandidateUIElementId();
            if (dwId == 0) {
              pCandElement->ResetDiagnostics();
              BOOL bShow = TRUE;
              HRESULT hr =
                  pUIElementMgr->BeginUIElement(pCandElement, &bShow, &dwId);
              if (SUCCEEDED(hr)) {
                pTIP_->SetCandidateUIElementId(dwId);
              }
            } else {
              pUIElementMgr->UpdateUIElement(dwId);
            }
          } else {
            pCandElement->SetShown(FALSE);
            DWORD dwId = pTIP_->GetCandidateUIElementId();
            if (dwId != 0) {
              pUIElementMgr->EndUIElement(dwId);
              pTIP_->SetCandidateUIElementId(0);
            }
          }
        }

        pCursorRange->Release();
      }
    }
    if (pRange) pRange->Release();

  } else if (commitStr.empty() && pTIP_->GetComposition()) {
    // 3. Handle clearing the composition (e.g., user backspaced the last
    // character)
    ITfRange* pRange = nullptr;
    if (SUCCEEDED(pTIP_->GetComposition()->GetRange(&pRange))) {
      pRange->SetText(ec, 0, L"", 0);

      ITfProperty* pProp = nullptr;
      if (SUCCEEDED(pContext_->GetProperty(GUID_PROP_ATTRIBUTE, &pProp))) {
        pProp->Clear(ec, pRange);
        pProp->Release();
      }

      pTIP_->GetComposition()->EndComposition(ec);
      pTIP_->GetComposition()->Release();
      pTIP_->SetComposition(nullptr);
      pRange->Release();
    }
  }

  // Handle case where we have candidates or tooltip but no active composition
  // (e.g. from ChoosingPunctuationList triggered from Empty state)
  if (!directCommitWithoutComposition && pTIP_->GetComposition() == nullptr &&
      (!state_.candidates.empty() || !state_.tooltip.empty())) {
    if (pTIP_->GetUIElementMgr() && pTIP_->GetReadingUIElement()) {
      auto* pUIElementMgr = pTIP_->GetUIElementMgr();
      auto* pReadingElement = pTIP_->GetReadingUIElement();
      pReadingElement->SetActiveContext(pContext_);

      if (!state_.tooltip.empty()) {
        pReadingElement->UpdateData(state_.tooltip);
        pReadingElement->SetShown(TRUE);

        DWORD dwId = pTIP_->GetReadingUIElementId();
        if (dwId == 0) {
          BOOL bShow = TRUE;
          if (SUCCEEDED(pUIElementMgr->BeginUIElement(pReadingElement, &bShow,
                                                      &dwId))) {
            pTIP_->SetReadingUIElementId(dwId);
          }
        } else {
          pUIElementMgr->UpdateUIElement(dwId);
        }
      } else {
        pReadingElement->SetShown(FALSE);
        DWORD dwId = pTIP_->GetReadingUIElementId();
        if (dwId != 0) {
          pUIElementMgr->EndUIElement(dwId);
          pTIP_->SetReadingUIElementId(0);
        }
      }
    }

    if (pTIP_->GetUIElementMgr() && pTIP_->GetCandidateUIElement()) {
      auto* pUIElementMgr = pTIP_->GetUIElementMgr();
      auto* pCandElement = pTIP_->GetCandidateUIElement();
      pCandElement->SetActiveContext(pContext_);

      if (!state_.candidates.empty()) {
        LogContextWindowInfo("CandidateUI no-composition host", pContext_);
        pCandElement->UpdateData(state_.candidates, state_.candidateIndex,
                                 state_.candidateKeys,
                                 state_.candidateKeysCount);
        pCandElement->SetShown(TRUE);

        DWORD dwId = pTIP_->GetCandidateUIElementId();
        if (dwId == 0) {
          pCandElement->ResetDiagnostics();
          BOOL bShow = TRUE;
          HRESULT hr =
              pUIElementMgr->BeginUIElement(pCandElement, &bShow, &dwId);
          if (SUCCEEDED(hr)) {
            pTIP_->SetCandidateUIElementId(dwId);
          }
        } else {
          pUIElementMgr->UpdateUIElement(dwId);
        }
      } else {
        pCandElement->SetShown(FALSE);
        DWORD dwId = pTIP_->GetCandidateUIElementId();
        if (dwId != 0) {
          pUIElementMgr->EndUIElement(dwId);
          pTIP_->SetCandidateUIElementId(0);
        }
      }
    }
  }

  if (state_.candidates.empty()) {
    if (pTIP_->GetUIElementMgr() && pTIP_->GetCandidateUIElement()) {
      DWORD dwId = pTIP_->GetCandidateUIElementId();
      if (dwId != 0) {
        pTIP_->GetUIElementMgr()->EndUIElement(dwId);
        pTIP_->SetCandidateUIElementId(0);
      }
      pTIP_->GetCandidateUIElement()->SetShown(FALSE);
    }
  }
  if (state_.tooltip.empty()) {
    if (pTIP_->GetUIElementMgr() && pTIP_->GetReadingUIElement()) {
      DWORD dwId = pTIP_->GetReadingUIElementId();
      if (dwId != 0) {
        pTIP_->GetUIElementMgr()->EndUIElement(dwId);
        pTIP_->SetReadingUIElementId(0);
      }
      pTIP_->GetReadingUIElement()->SetShown(FALSE);
    }
  }

  return S_OK;
}
