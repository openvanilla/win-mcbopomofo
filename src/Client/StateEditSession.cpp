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
#include "UTFHelper.h"

namespace {

POINT ComputeAuxiliaryWindowPoint(const RECT& rc) {
  const int rectHeight = std::max<int>(0, static_cast<int>(rc.bottom - rc.top));
  const int verticalGap = std::max(8, rectHeight + 4);
  return POINT{rc.left, rc.bottom + verticalGap};
}

void MoveAuxiliaryWindowsInternal(McBopomofoTIP* tip, POINT pt) {
  auto* candWin = tip->GetCandidateWindow();
  auto* tooltipWin = tip->GetTooltipWindow();

  const bool candVisible = candWin->IsVisible();
  const bool tooltipVisible = tooltipWin->IsVisible();

  if (candVisible && tooltipVisible) {
    tooltipWin->Move(pt.x, pt.y);
    candWin->Move(pt.x, pt.y + tooltipWin->GetHeight() + 4);
  } else if (tooltipVisible) {
    tooltipWin->Move(pt.x, pt.y);
  } else if (candVisible) {
    candWin->Move(pt.x, pt.y);
  }
}

bool MoveWindowsToRange(TfEditCookie ec, ITfContext* context, ITfRange* range,
                        McBopomofoTIP* tip) {
  if (!range || !tip) {
    return false;
  }

  ITfContextView* pView = nullptr;
  if (FAILED(context->GetActiveView(&pView))) {
    return false;
  }

  RECT rc = {0};
  BOOL fClipped = FALSE;
  bool moved = false;
  if (SUCCEEDED(pView->GetTextExt(ec, range, &rc, &fClipped))) {
    POINT pt = ComputeAuxiliaryWindowPoint(rc);
    MoveAuxiliaryWindowsInternal(tip, pt);
    moved = true;
  }
  pView->Release();
  return moved;
}

bool MoveWindowsToSelection(TfEditCookie ec, ITfContext* context,
                            McBopomofoTIP* tip) {
  TF_SELECTION selection = {};
  ULONG fetched = 0;
  if (FAILED(context->GetSelection(ec, TF_DEFAULT_SELECTION, 1, &selection,
                                   &fetched)) ||
      fetched != 1 || !selection.range) {
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
    POINT pt = ComputeAuxiliaryWindowPoint(caretRect);
    ClientToScreen(gti.hwndCaret, &pt);
    MoveAuxiliaryWindowsInternal(tip, pt);
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
    : CEditSessionBase(pContext), _pTIP(pTIP), _state(state) {
  if (_pTIP) _pTIP->AddRef();
}

CStateEditSession::~CStateEditSession() {
  if (_pTIP) _pTIP->Release();
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
  std::wstring commitStr = McBopomofo::Utf8ToUtf16(_state.commitString);
  std::wstring compStr = McBopomofo::Utf8ToUtf16(_state.composingBuffer);
  const bool directCommitWithoutComposition =
      !commitStr.empty() && compStr.empty() &&
      _pTIP->GetComposition() == nullptr;

  // 1. Handle Committing Text
  if (!commitStr.empty()) {
    if (_pTIP->GetComposition()) {
      ITfRange* pRange = nullptr;
      if (SUCCEEDED(_pTIP->GetComposition()->GetRange(&pRange))) {
        pRange->SetText(ec, 0, commitStr.c_str(), (LONG)commitStr.length());

        // Clear display attributes when committing
        ITfProperty* pProp = nullptr;
        if (SUCCEEDED(_pContext->GetProperty(GUID_PROP_ATTRIBUTE, &pProp))) {
          pProp->Clear(ec, pRange);
          pProp->Release();
        }

        pRange->Collapse(ec, TF_ANCHOR_END);
        TF_SELECTION sel;
        sel.range = pRange;
        sel.style.ase = TF_AE_NONE;
        sel.style.fInterimChar = FALSE;
        _pContext->SetSelection(ec, 1, &sel);

        _pTIP->GetComposition()->EndComposition(ec);
        _pTIP->GetComposition()->Release();
        _pTIP->SetComposition(nullptr);
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
      if (SUCCEEDED(_pContext->QueryInterface(IID_ITfInsertAtSelection,
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
          if (SUCCEEDED(_pContext->QueryInterface(IID_ITfContextComposition,
                                                  (void**)&pContextComp))) {
            ITfComposition* pComp = nullptr;
            if (SUCCEEDED(pContextComp->StartComposition(ec, pRange, _pTIP,
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
              _pContext->SetSelection(ec, 1, &sel);

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
      _pTIP->GetCandidateWindow()->Hide();
      _pTIP->GetTooltipWindow()->Hide();
      return S_OK;
    }
  }

  // 2. Handle Composing Text
  if (!compStr.empty()) {
    ITfRange* pRange = nullptr;
    if (!_pTIP->GetComposition()) {
      // Start composition
      ITfInsertAtSelection* pInsert = nullptr;
      if (SUCCEEDED(_pContext->QueryInterface(IID_ITfInsertAtSelection,
                                              (void**)&pInsert))) {
        pInsert->InsertTextAtSelection(ec, TF_IAS_QUERYONLY, NULL, 0, &pRange);
        pInsert->Release();
      }
      if (pRange) {
        ITfContextComposition* pContextComp = nullptr;
        if (SUCCEEDED(_pContext->QueryInterface(IID_ITfContextComposition,
                                                (void**)&pContextComp))) {
          ITfComposition* pComp = nullptr;
          if (SUCCEEDED(
                  pContextComp->StartComposition(ec, pRange, _pTIP, &pComp)) &&
              pComp) {
            _pTIP->SetComposition(pComp);
          }
          pContextComp->Release();
        }
      }
    } else {
      _pTIP->GetComposition()->GetRange(&pRange);
    }

    if (pRange && _pTIP->GetComposition()) {
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
        SetDisplayAttribute(ec, _pContext, pRange, gaInput);

        if (_state.markStart >= 0 && _state.markEnd >= 0) {
          // Apply marking attribute to the marked portion
          size_t startOffset = McBopomofo::Utf8OffsetToUtf16Offset(
              _state.composingBuffer, _state.markStart);
          size_t endOffset = McBopomofo::Utf8OffsetToUtf16Offset(
              _state.composingBuffer, _state.markEnd);
          size_t markLength =
              endOffset >= startOffset ? endOffset - startOffset : 0;

          ITfRange* pMarkRange = nullptr;
          if (SUCCEEDED(pRange->Clone(&pMarkRange))) {
            LONG cch = 0;
            // Collapse to start, then shift to the marked range
            pMarkRange->Collapse(ec, TF_ANCHOR_START);
            pMarkRange->ShiftStart(ec, (LONG)startOffset, &cch, nullptr);
            pMarkRange->ShiftEnd(ec, (LONG)markLength, &cch, nullptr);
            SetDisplayAttribute(ec, _pContext, pMarkRange, gaMarked);
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
            _state.composingBuffer, _state.cursorIndex);
        pCursorRange->Collapse(ec, TF_ANCHOR_START);
        pCursorRange->ShiftEnd(ec, (LONG)utf16CursorIndex, &cch, nullptr);
        pCursorRange->Collapse(ec, TF_ANCHOR_END);

        TF_SELECTION sel;
        sel.range = pCursorRange;
        sel.style.ase = TF_AE_NONE;
        sel.style.fInterimChar = FALSE;
        _pContext->SetSelection(ec, 1, &sel);

        // Update UI content first so windows have correct sizes
        if (!_state.tooltip.empty()) {
          _pTIP->GetTooltipWindow()->UpdateUI(_state.tooltip);
        } else {
          _pTIP->GetTooltipWindow()->Hide();
        }

        if (!_state.candidates.empty()) {
          _pTIP->GetCandidateWindow()->UpdateUI(
              _state.candidates, _state.candidateIndex, _state.forceVertical,
              _state.useShiftKeySelection, _state.hint);
        } else {
          _pTIP->GetCandidateWindow()->Hide();
        }

        // Now move the auxiliary windows
        if (!directCommitWithoutComposition &&
            (!_state.candidates.empty() || !_state.tooltip.empty())) {
          MoveAuxiliaryWindows(ec, _pContext, pCursorRange, _pTIP);
        }
        pCursorRange->Release();
      }
    }
    if (pRange) pRange->Release();

  } else if (commitStr.empty() && _pTIP->GetComposition()) {
    // 3. Handle clearing the composition (e.g., user backspaced the last
    // character)
    ITfRange* pRange = nullptr;
    if (SUCCEEDED(_pTIP->GetComposition()->GetRange(&pRange))) {
      pRange->SetText(ec, 0, L"", 0);

      ITfProperty* pProp = nullptr;
      if (SUCCEEDED(_pContext->GetProperty(GUID_PROP_ATTRIBUTE, &pProp))) {
        pProp->Clear(ec, pRange);
        pProp->Release();
      }

      _pTIP->GetComposition()->EndComposition(ec);
      _pTIP->GetComposition()->Release();
      _pTIP->SetComposition(nullptr);
      pRange->Release();
    }
  }

  // Handle case where we have candidates or tooltip but no active composition
  // (e.g. from ChoosingPunctuationList triggered from Empty state)
  if (!directCommitWithoutComposition && _pTIP->GetComposition() == nullptr &&
      (!_state.candidates.empty() || !_state.tooltip.empty())) {
    if (!_state.tooltip.empty()) {
      _pTIP->GetTooltipWindow()->UpdateUI(_state.tooltip);
    } else {
      _pTIP->GetTooltipWindow()->Hide();
    }

    if (!_state.candidates.empty()) {
      _pTIP->GetCandidateWindow()->UpdateUI(
          _state.candidates, _state.candidateIndex, _state.forceVertical,
          _state.useShiftKeySelection, _state.hint);
    } else {
      _pTIP->GetCandidateWindow()->Hide();
    }

    MoveAuxiliaryWindows(ec, _pContext, nullptr, _pTIP);
  }

  return S_OK;
}
