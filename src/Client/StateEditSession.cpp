#include "StateEditSession.h"
#include "UTFHelper.h"
#include "DisplayAttributeInfo.h"

CStateEditSession::CStateEditSession(ITfContext *pContext, McBopomofoTIP *pTIP, const McBopomofo::IPC::StateUpdatePayload& state)
    : CEditSessionBase(pContext), _pTIP(pTIP), _state(state) {
    if (_pTIP) _pTIP->AddRef();
}

CStateEditSession::~CStateEditSession() {
    if (_pTIP) _pTIP->Release();
}

void SetDisplayAttribute(TfEditCookie ec, ITfContext* pContext, ITfRange* pRange, TfGuidAtom guidAtom) {
    ITfProperty *pProp = nullptr;
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

    // 1. Handle Committing Text
    if (!commitStr.empty()) {
        if (_pTIP->GetComposition()) {
            ITfRange* pRange = nullptr;
            if (SUCCEEDED(_pTIP->GetComposition()->GetRange(&pRange))) {
                pRange->SetText(ec, 0, commitStr.c_str(), (LONG)commitStr.length());
                
                // Clear display attributes when committing
                ITfProperty *pProp = nullptr;
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
            // No composition active, insert directly at selection
            ITfInsertAtSelection* pInsert = nullptr;
            if (SUCCEEDED(_pContext->QueryInterface(IID_ITfInsertAtSelection, (void**)&pInsert))) {
                ITfRange* pRange = nullptr;
                pInsert->InsertTextAtSelection(ec, TF_IAS_NOQUERY, commitStr.c_str(), (LONG)commitStr.length(), &pRange);
                if (pRange) {
                    pRange->Collapse(ec, TF_ANCHOR_END);
                    TF_SELECTION sel;
                    sel.range = pRange;
                    sel.style.ase = TF_AE_NONE;
                    sel.style.fInterimChar = FALSE;
                    _pContext->SetSelection(ec, 1, &sel);
                    pRange->Release();
                }
                pInsert->Release();
            }
        }
    }

    // 2. Handle Composing Text
    if (!compStr.empty()) {
        ITfRange* pRange = nullptr;
        if (!_pTIP->GetComposition()) {
            // Start composition
            ITfInsertAtSelection* pInsert = nullptr;
            if (SUCCEEDED(_pContext->QueryInterface(IID_ITfInsertAtSelection, (void**)&pInsert))) {
                pInsert->InsertTextAtSelection(ec, TF_IAS_QUERYONLY, NULL, 0, &pRange);
                pInsert->Release();
            }
            if (pRange) {
                ITfContextComposition* pContextComp = nullptr;
                if (SUCCEEDED(_pContext->QueryInterface(IID_ITfContextComposition, (void**)&pContextComp))) {
                    ITfComposition* pComp = nullptr;
                    if (SUCCEEDED(pContextComp->StartComposition(ec, pRange, _pTIP, &pComp)) && pComp) {
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
            ITfCategoryMgr *pCategoryMgr = nullptr;
            if (SUCCEEDED(CoCreateInstance(CLSID_TF_CategoryMgr, nullptr, CLSCTX_INPROC_SERVER, IID_ITfCategoryMgr, (void**)&pCategoryMgr))) {
                TfGuidAtom gaInput = TF_INVALID_GUIDATOM;
                TfGuidAtom gaMarked = TF_INVALID_GUIDATOM;
                pCategoryMgr->RegisterGUID(c_guidDisplayAttributeInput, &gaInput);
                pCategoryMgr->RegisterGUID(c_guidDisplayAttributeMarked, &gaMarked);
                
                if (_state.markStart >= 0 && _state.markEnd >= 0) {
                    // Apply marking attribute
                    size_t startOffset = McBopomofo::Utf8OffsetToUtf16Offset(_state.composingBuffer, _state.markStart);
                    size_t endOffset = McBopomofo::Utf8OffsetToUtf16Offset(_state.composingBuffer, _state.markEnd);
                    
                    ITfRange* pMarkRange = nullptr;
                    if (SUCCEEDED(pRange->Clone(&pMarkRange))) {
                        LONG cch = 0;
                        pMarkRange->Collapse(ec, TF_ANCHOR_START);
                        pMarkRange->ShiftEnd(ec, (LONG)endOffset, &cch, nullptr);
                        pMarkRange->ShiftStart(ec, (LONG)startOffset, &cch, nullptr);
                        SetDisplayAttribute(ec, _pContext, pMarkRange, gaMarked);
                        pMarkRange->Release();
                    }
                    
                    // Also apply input attribute to the rest? 
                    // TSF usually allows multiple attributes. Let's apply input to the whole thing first, then mark over it.
                    SetDisplayAttribute(ec, _pContext, pRange, gaInput);
                } else {
                    SetDisplayAttribute(ec, _pContext, pRange, gaInput);
                }
                
                pCategoryMgr->Release();
            }

            // Set caret at cursorIndex
            ITfRange* pCursorRange = nullptr;
            if (SUCCEEDED(pRange->Clone(&pCursorRange))) {
                LONG cch = 0;
                size_t utf16CursorIndex = McBopomofo::Utf8OffsetToUtf16Offset(_state.composingBuffer, _state.cursorIndex);
                pCursorRange->Collapse(ec, TF_ANCHOR_START);
                pCursorRange->ShiftEnd(ec, (LONG)utf16CursorIndex, &cch, nullptr);
                pCursorRange->Collapse(ec, TF_ANCHOR_END);

                TF_SELECTION sel;
                sel.range = pCursorRange;
                sel.style.ase = TF_AE_NONE;
                sel.style.fInterimChar = FALSE;
                _pContext->SetSelection(ec, 1, &sel);
            
                // Try to find the coordinates for the candidate window
                if (!_state.candidates.empty()) {
                    ITfContextView* pView = nullptr;
                    if (SUCCEEDED(_pContext->GetActiveView(&pView))) {
                        RECT rc = {0};
                        BOOL fClipped = FALSE;
                        
                        if (SUCCEEDED(pView->GetTextExt(ec, pCursorRange, &rc, &fClipped))) {
                            _pTIP->GetCandidateWindow()->Move(rc.left, rc.bottom + 2);
                        }
                        pView->Release();
                    }
                }
                pCursorRange->Release();
            }
        }
        if (pRange) pRange->Release();
        
    } else if (commitStr.empty() && _pTIP->GetComposition()) {
        // 3. Handle clearing the composition (e.g., user backspaced the last character)
        ITfRange* pRange = nullptr;
        if (SUCCEEDED(_pTIP->GetComposition()->GetRange(&pRange))) {
            pRange->SetText(ec, 0, L"", 0);
            
            ITfProperty *pProp = nullptr;
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

    // 4. Update Candidate Window UI
    _pTIP->GetCandidateWindow()->UpdateUI(_state.candidates, _state.cursorIndex, _state.forceVertical);

    return S_OK;
}
