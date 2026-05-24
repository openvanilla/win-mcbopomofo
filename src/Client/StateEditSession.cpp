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
  HWND hwnd = GetContextWindow(context);
  char className[128] = {};
  char title[128] = {};
  DWORD pid = 0;

  if (hwnd) {
    GetClassNameA(hwnd, className, static_cast<int>(sizeof(className)));
    GetWindowTextA(hwnd, title, static_cast<int>(sizeof(title)));
    GetWindowThreadProcessId(hwnd, &pid);
  }

  LogMessage("%s hwnd=%p pid=%lu class=%s title=%s", prefix, hwnd,
             static_cast<unsigned long>(pid), className[0] ? className : "-",
             title[0] ? title : "-");
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

void MoveAuxiliaryWindowsInternal(McBopomofoTIP* tip, const RECT& rc) {
  auto* candWin = tip->GetCandidateWindow();
  auto* tooltipWin = tip->GetTooltipWindow();

  const bool candVisible = candWin->IsVisible();
  const bool tooltipVisible = tooltipWin->IsVisible();

  if (!candVisible && !tooltipVisible) {
    return;
  }

  // Determine the monitor where the text is being typed
  POINT ptTopLeft = {rc.left, rc.top};
  HMONITOR hMonitor = MonitorFromPoint(ptTopLeft, MONITOR_DEFAULTTONEAREST);
  MONITORINFO mi = {0};
  mi.cbSize = sizeof(MONITORINFO);
  GetMonitorInfoW(hMonitor, &mi);
  
  const int screenBottom = mi.rcWork.bottom;
  const int screenRight = mi.rcWork.right;
  const int screenLeft = mi.rcWork.left;

  int candHeight = candVisible ? candWin->GetHeight() : 0;
  int candWidth = candVisible ? candWin->GetWidth() : 0;
  int tooltipHeight = tooltipVisible ? tooltipWin->GetHeight() : 0;
  int tooltipWidth = tooltipVisible ? tooltipWin->GetWidth() : 0;

  int totalRequiredHeight = tooltipHeight + (tooltipVisible && candVisible ? 4 : 0) + candHeight;
  int yBelow = rc.bottom + 10;
  
  int finalCandY = 0;
  int finalTooltipY = 0;
  bool showAbove = false;

  if (yBelow + totalRequiredHeight > screenBottom) {
    showAbove = true;
  }

  if (showAbove) {
    // If going above, candidate window is at the bottom, tooltip above it.
    // cand_win.y = input_text_rect.top - cand_win.height - 10
    finalCandY = rc.top - candHeight - 10;
    finalTooltipY = finalCandY - tooltipHeight - (tooltipVisible && candVisible ? 4 : 0);
  } else {
    // Default below
    finalTooltipY = yBelow;
    finalCandY = yBelow + (tooltipVisible ? tooltipHeight + 4 : 0);
  }

  int x = rc.left;

  // Prevent going off the right edge of the screen
  int maxWidth = std::max(candWidth, tooltipWidth);
  if (x + maxWidth > screenRight) {
    x = screenRight - maxWidth;
  }
  // Prevent going off the left edge of the screen
  if (x < screenLeft) {
    x = screenLeft;
  }

  if (tooltipVisible) {
    tooltipWin->Move(x, finalTooltipY);
  }
  if (candVisible) {
    candWin->Move(x, finalCandY);
  }
}

bool MoveWindowsToRange(TfEditCookie ec, ITfContext* context, ITfRange* range,
                        McBopomofoTIP* tip) {
  if (!range || !tip) {
    return false;
  }

  ITfContextView* pView = nullptr;
  HRESULT hr = context->GetActiveView(&pView);
  if (FAILED(hr)) {
    LogMessage("MoveWindowsToRange GetActiveView failed hr=0x%08X", hr);
    return false;
  }

  RECT rc = {0};
  BOOL fClipped = FALSE;
  bool moved = false;
  hr = pView->GetTextExt(ec, range, &rc, &fClipped);
  if (SUCCEEDED(hr)) {
    LogMessage(
        "MoveWindowsToRange GetTextExt ok clipped=%d rect=(%ld,%ld,%ld,%ld)",
        fClipped ? 1 : 0, rc.left, rc.top, rc.right, rc.bottom);
    MoveAuxiliaryWindowsInternal(tip, rc);
    moved = true;
  } else {
    LogMessage("MoveWindowsToRange GetTextExt failed hr=0x%08X", hr);
  }
  pView->Release();
  return moved;
}

bool MoveWindowsToSelection(TfEditCookie ec, ITfContext* context,
                            McBopomofoTIP* tip) {
  TF_SELECTION selection = {};
  ULONG fetched = 0;
  HRESULT hr =
      context->GetSelection(ec, TF_DEFAULT_SELECTION, 1, &selection, &fetched);
  if (FAILED(hr) || fetched != 1 || !selection.range) {
    LogMessage(
        "MoveWindowsToSelection GetSelection failed hr=0x%08X fetched=%lu range=%p",
        hr, static_cast<unsigned long>(fetched), selection.range);
    return false;
  }

  bool moved = MoveWindowsToRange(ec, context, selection.range, tip);
  selection.range->Release();
  return moved;
}

void MoveWindowsToCaretFallback(McBopomofoTIP* tip) {
  if (!tip) {
    return;
  }

  GUITHREADINFO gti = {0};
  gti.cbSize = sizeof(GUITHREADINFO);
  if (GetGUIThreadInfo(GetCurrentThreadId(), &gti) && gti.hwndCaret) {
    RECT caretRect = gti.rcCaret;
    POINT ptTopLeft = {caretRect.left, caretRect.top};
    POINT ptBottomRight = {caretRect.right, caretRect.bottom};
    ClientToScreen(gti.hwndCaret, &ptTopLeft);
    ClientToScreen(gti.hwndCaret, &ptBottomRight);
    
    RECT screenRect;
    screenRect.left = ptTopLeft.x;
    screenRect.top = ptTopLeft.y;
    screenRect.right = ptBottomRight.x;
    screenRect.bottom = ptBottomRight.y;

    LogMessage("MoveWindowsToCaretFallback caret rect=(%ld,%ld,%ld,%ld)",
               screenRect.left, screenRect.top, screenRect.right,
               screenRect.bottom);
    MoveAuxiliaryWindowsInternal(tip, screenRect);
  } else {
    LogMessage("MoveWindowsToCaretFallback no caret info");
  }
}

void MoveAuxiliaryWindows(TfEditCookie ec, ITfContext* context, ITfRange* range,
                          McBopomofoTIP* tip) {
  bool moved = MoveWindowsToRange(ec, context, range, tip);
  if (!moved) {
    moved = MoveWindowsToSelection(ec, context, tip);
  }
  if (!moved) {
    MoveWindowsToCaretFallback(tip);
  }
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

    // For direct commits without an active composition, stop here after
    // insertion. Some TSF hosts, including recent Notepad builds, can
    // crash if we continue touching the document in the same edit session.
    // We still need to explicitly hide auxiliary UI first, otherwise the
    // candidate window can be left visible after a direct commit.
    if (directCommitWithoutComposition) {
      pTIP_->GetCandidateWindow()->Hide();
      pTIP_->GetTooltipWindow()->Hide();

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
        bool showCustomTooltip = true;
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
              if (SUCCEEDED(pUIElementMgr->BeginUIElement(pReadingElement, &bShow, &dwId))) {
                pTIP_->SetReadingUIElementId(dwId);
                pTIP_->SetShowCustomTooltipWindow(bShow ? true : false);
              }
            } else {
              pUIElementMgr->UpdateUIElement(dwId);
            }
            showCustomTooltip = pTIP_->IsShowCustomTooltipWindow();
          } else {
            pReadingElement->SetShown(FALSE);
            DWORD dwId = pTIP_->GetReadingUIElementId();
            if (dwId != 0) {
              pUIElementMgr->EndUIElement(dwId);
              pTIP_->SetReadingUIElementId(0);
            }
            showCustomTooltip = false;
          }
        }

        if (showCustomTooltip && !state_.tooltip.empty()) {
          pTIP_->GetTooltipWindow()->SetOwnerWindow(GetContextWindow(pContext_));
          pTIP_->GetTooltipWindow()->UpdateUI(state_.tooltip);
        } else {
          pTIP_->GetTooltipWindow()->Hide();
        }

        bool showCustomCand = true;
        if (pTIP_->GetUIElementMgr() && pTIP_->GetCandidateUIElement()) {
          auto* pUIElementMgr = pTIP_->GetUIElementMgr();
          auto* pCandElement = pTIP_->GetCandidateUIElement();
          pCandElement->SetActiveContext(pContext_);

          if (!state_.candidates.empty()) {
            LogContextWindowInfo("CandidateUI composition host", pContext_);
            pCandElement->UpdateData(state_.candidates, state_.candidateIndex,
                                     state_.candidateKeys, state_.candidateKeysCount);
            pCandElement->SetShown(TRUE);

            DWORD dwId = pTIP_->GetCandidateUIElementId();
            if (dwId == 0) {
              pCandElement->ResetDiagnostics();
              BOOL bShow = TRUE;
              HRESULT hr =
                  pUIElementMgr->BeginUIElement(pCandElement, &bShow, &dwId);
              LogMessage(
                  "CandidateUI BeginUIElement hr=0x%08X requestCustomUI=%d dwId=%lu count=%llu hostInteractions=%lu lastHostMethod=%s",
                  hr, bShow ? 1 : 0, static_cast<unsigned long>(dwId),
                  static_cast<unsigned long long>(state_.candidates.size()),
                  pCandElement->HostInteractionCount(),
                  pCandElement->LastHostMethod());
              if (SUCCEEDED(hr)) {
                pTIP_->SetCandidateUIElementId(dwId);
                pTIP_->SetShowCustomCandidateWindow(bShow ? true : false);
              }
            } else {
              HRESULT hr = pUIElementMgr->UpdateUIElement(dwId);
              LogMessage(
                  "CandidateUI UpdateUIElement hr=0x%08X dwId=%lu count=%llu hostInteractions=%lu lastHostMethod=%s",
                  hr, static_cast<unsigned long>(dwId),
                  static_cast<unsigned long long>(state_.candidates.size()),
                  pCandElement->HostInteractionCount(),
                  pCandElement->LastHostMethod());
            }
            showCustomCand = pTIP_->IsShowCustomCandidateWindow();
            LogMessage(
                "CandidateUI customFallback=%d hostInteracted=%d hostInteractions=%lu lastHostMethod=%s after TSF routing",
                showCustomCand ? 1 : 0,
                pCandElement->HasHostInteraction() ? 1 : 0,
                pCandElement->HostInteractionCount(),
                pCandElement->LastHostMethod());
          } else {
            pCandElement->SetShown(FALSE);
            DWORD dwId = pTIP_->GetCandidateUIElementId();
            if (dwId != 0) {
              pUIElementMgr->EndUIElement(dwId);
              pTIP_->SetCandidateUIElementId(0);
            }
            showCustomCand = false;
          }
        }

        if (showCustomCand && !state_.candidates.empty()) {
          pTIP_->GetCandidateWindow()->SetOwnerWindow(GetContextWindow(pContext_));
          CandidateWindow::UpdateUIRequest request;
          request.candidates = state_.candidates;
          request.cursorIndex = state_.candidateIndex;
          request.forceVertical = state_.forceVertical;
          request.selectionStyle = state_.selectionStyle;
          request.candidateFontSize = state_.candidateFontSize;
          request.hint = state_.hint;
          request.candidateWindowVertical = state_.candidateWindowVertical;
          request.candidateKeys = state_.candidateKeys;
          request.candidateKeysCount = state_.candidateKeysCount;
          request.colors = state_.candidateWindowColors;
          pTIP_->GetCandidateWindow()->UpdateUI(request);
        } else {
          pTIP_->GetCandidateWindow()->Hide();
        }

        // Now move the auxiliary windows
        if (!directCommitWithoutComposition &&
            ((showCustomCand && !state_.candidates.empty()) || (showCustomTooltip && !state_.tooltip.empty()))) {
          MoveAuxiliaryWindows(ec, pContext_, pCursorRange, pTIP_);
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
    
    bool showCustomTooltip = true;
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
          if (SUCCEEDED(pUIElementMgr->BeginUIElement(pReadingElement, &bShow, &dwId))) {
            pTIP_->SetReadingUIElementId(dwId);
            pTIP_->SetShowCustomTooltipWindow(bShow ? true : false);
          }
        } else {
          pUIElementMgr->UpdateUIElement(dwId);
        }
        showCustomTooltip = pTIP_->IsShowCustomTooltipWindow();
      } else {
        pReadingElement->SetShown(FALSE);
        DWORD dwId = pTIP_->GetReadingUIElementId();
        if (dwId != 0) {
          pUIElementMgr->EndUIElement(dwId);
          pTIP_->SetReadingUIElementId(0);
        }
        showCustomTooltip = false;
      }
    }

    if (showCustomTooltip && !state_.tooltip.empty()) {
      pTIP_->GetTooltipWindow()->SetOwnerWindow(GetContextWindow(pContext_));
      pTIP_->GetTooltipWindow()->UpdateUI(state_.tooltip);
    } else {
      pTIP_->GetTooltipWindow()->Hide();
    }

    bool showCustomCand = true;
    if (pTIP_->GetUIElementMgr() && pTIP_->GetCandidateUIElement()) {
      auto* pUIElementMgr = pTIP_->GetUIElementMgr();
      auto* pCandElement = pTIP_->GetCandidateUIElement();
      pCandElement->SetActiveContext(pContext_);

      if (!state_.candidates.empty()) {
        LogContextWindowInfo("CandidateUI no-composition host", pContext_);
        pCandElement->UpdateData(state_.candidates, state_.candidateIndex,
                                 state_.candidateKeys, state_.candidateKeysCount);
        pCandElement->SetShown(TRUE);

        DWORD dwId = pTIP_->GetCandidateUIElementId();
        if (dwId == 0) {
          pCandElement->ResetDiagnostics();
          BOOL bShow = TRUE;
          HRESULT hr =
              pUIElementMgr->BeginUIElement(pCandElement, &bShow, &dwId);
          LogMessage(
              "CandidateUI BeginUIElement hr=0x%08X requestCustomUI=%d dwId=%lu count=%llu hostInteractions=%lu lastHostMethod=%s (no composition)",
              hr, bShow ? 1 : 0, static_cast<unsigned long>(dwId),
              static_cast<unsigned long long>(state_.candidates.size()),
              pCandElement->HostInteractionCount(),
              pCandElement->LastHostMethod());
          if (SUCCEEDED(hr)) {
            pTIP_->SetCandidateUIElementId(dwId);
            pTIP_->SetShowCustomCandidateWindow(bShow ? true : false);
          }
        } else {
          HRESULT hr = pUIElementMgr->UpdateUIElement(dwId);
          LogMessage(
              "CandidateUI UpdateUIElement hr=0x%08X dwId=%lu count=%llu hostInteractions=%lu lastHostMethod=%s (no composition)",
              hr, static_cast<unsigned long>(dwId),
              static_cast<unsigned long long>(state_.candidates.size()),
              pCandElement->HostInteractionCount(),
              pCandElement->LastHostMethod());
        }
        showCustomCand = pTIP_->IsShowCustomCandidateWindow();
        LogMessage(
            "CandidateUI customFallback=%d hostInteracted=%d hostInteractions=%lu lastHostMethod=%s after TSF routing (no composition)",
            showCustomCand ? 1 : 0,
            pCandElement->HasHostInteraction() ? 1 : 0,
            pCandElement->HostInteractionCount(),
            pCandElement->LastHostMethod());
      } else {
        pCandElement->SetShown(FALSE);
        DWORD dwId = pTIP_->GetCandidateUIElementId();
        if (dwId != 0) {
          pUIElementMgr->EndUIElement(dwId);
          pTIP_->SetCandidateUIElementId(0);
        }
        showCustomCand = false;
      }
    }

    if (showCustomCand && !state_.candidates.empty()) {
      pTIP_->GetCandidateWindow()->SetOwnerWindow(GetContextWindow(pContext_));
      CandidateWindow::UpdateUIRequest request;
      request.candidates = state_.candidates;
      request.cursorIndex = state_.candidateIndex;
      request.forceVertical = state_.forceVertical;
      request.selectionStyle = state_.selectionStyle;
      request.candidateFontSize = state_.candidateFontSize;
      request.hint = state_.hint;
      request.candidateWindowVertical = state_.candidateWindowVertical;
      request.candidateKeys = state_.candidateKeys;
      request.candidateKeysCount = state_.candidateKeysCount;
      request.colors = state_.candidateWindowColors;
      pTIP_->GetCandidateWindow()->UpdateUI(request);
    } else {
      pTIP_->GetCandidateWindow()->Hide();
    }

    if ((showCustomCand && !state_.candidates.empty()) || (showCustomTooltip && !state_.tooltip.empty())) {
      MoveAuxiliaryWindows(ec, pContext_, nullptr, pTIP_);
    }
  }

  if (state_.candidates.empty()) {
    pTIP_->GetCandidateWindow()->Hide();
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
    pTIP_->GetTooltipWindow()->Hide();
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
