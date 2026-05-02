#include "StateEditSession.h"
#include "UTFHelper.h"

CStateEditSession::CStateEditSession(ITfContext *pContext, McBopomofoTIP *pTIP, const McBopomofo::IPC::StateUpdatePayload& state)
    : CEditSessionBase(pContext), _pTIP(pTIP), _state(state) {
    if (_pTIP) _pTIP->AddRef();
}

CStateEditSession::~CStateEditSession() {
    if (_pTIP) _pTIP->Release();
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
                if (pRange) pRange->Release();
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
            // TODO: Display Attributes (Underline)
            // TODO: Move caret based on _state.cursorIndex
        }
        if (pRange) pRange->Release();
        
    } else if (commitStr.empty() && _pTIP->GetComposition()) {
        // 3. Handle clearing the composition (e.g., user backspaced the last character)
        ITfRange* pRange = nullptr;
        if (SUCCEEDED(_pTIP->GetComposition()->GetRange(&pRange))) {
            pRange->SetText(ec, 0, L"", 0);
            _pTIP->GetComposition()->EndComposition(ec);
            _pTIP->GetComposition()->Release();
            _pTIP->SetComposition(nullptr);
            pRange->Release();
        }
    }

    // TODO: Display Candidate Window if !_state.candidates.empty()

    return S_OK;
}
