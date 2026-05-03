#include "McBopomofoTIP.h"
#include "Globals.h"
#include "NamedPipe.h"
#include "StateEditSession.h"
#include "LangBarButton.h"
McBopomofoTIP::McBopomofoTIP()
    : _cRef(1),
      _ptim(nullptr),
      _tid(TF_CLIENTID_NULL),
      _dwThreadMgrEventSinkCookie(TF_INVALID_COOKIE),
      _dwThreadFocusSinkCookie(TF_INVALID_COOKIE),
      _pComposition(nullptr),
      _pLangBarButton(nullptr) {
    DllAddRef();
}

McBopomofoTIP::~McBopomofoTIP() {
    DllRelease();
}

STDAPI McBopomofoTIP::QueryInterface(REFIID riid, void **ppvObj) {
    if (ppvObj == nullptr) {
        return E_INVALIDARG;
    }

    *ppvObj = nullptr;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfTextInputProcessor) ||
        IsEqualIID(riid, IID_ITfTextInputProcessorEx)) {
        *ppvObj = static_cast<ITfTextInputProcessorEx *>(this);
    } else if (IsEqualIID(riid, IID_ITfKeyEventSink)) {
        *ppvObj = static_cast<ITfKeyEventSink *>(this);
    } else if (IsEqualIID(riid, IID_ITfCompositionSink)) {
        *ppvObj = static_cast<ITfCompositionSink *>(this);
    } else if (IsEqualIID(riid, IID_ITfDisplayAttributeProvider)) {
        *ppvObj = static_cast<ITfDisplayAttributeProvider *>(this);
    } else if (IsEqualIID(riid, IID_ITfThreadMgrEventSink)) {
        *ppvObj = static_cast<ITfThreadMgrEventSink *>(this);
    } else if (IsEqualIID(riid, IID_ITfThreadFocusSink)) {
        *ppvObj = static_cast<ITfThreadFocusSink *>(this);
    }

    if (*ppvObj) {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDAPI_(ULONG) McBopomofoTIP::AddRef() {
    return InterlockedIncrement(&_cRef);
}

STDAPI_(ULONG) McBopomofoTIP::Release() {
    LONG cr = InterlockedDecrement(&_cRef);
    if (cr == 0) {
        delete this;
    }
    return cr;
}

BOOL McBopomofoTIP::_InitKeyEventSink() {
    ITfKeystrokeMgr *pKeystrokeMgr = nullptr;
    HRESULT hr = _ptim->QueryInterface(IID_ITfKeystrokeMgr, (void **)&pKeystrokeMgr);

    if (SUCCEEDED(hr)) {
        hr = pKeystrokeMgr->AdviseKeyEventSink(_tid, static_cast<ITfKeyEventSink *>(this), TRUE);
        pKeystrokeMgr->Release();
    }

    return SUCCEEDED(hr);
}

void McBopomofoTIP::_UninitKeyEventSink() {
    ITfKeystrokeMgr *pKeystrokeMgr = nullptr;
    HRESULT hr = _ptim->QueryInterface(IID_ITfKeystrokeMgr, (void **)&pKeystrokeMgr);

    if (SUCCEEDED(hr)) {
        pKeystrokeMgr->UnadviseKeyEventSink(_tid);
        pKeystrokeMgr->Release();
    }
}

BOOL McBopomofoTIP::_InitThreadMgrEventSink() {
    ITfSource *pSource = nullptr;
    HRESULT hr = _ptim->QueryInterface(IID_ITfSource, (void **)&pSource);
    if (FAILED(hr)) {
        return FALSE;
    }

    hr = pSource->AdviseSink(
        IID_ITfThreadMgrEventSink,
        static_cast<ITfThreadMgrEventSink *>(this),
        &_dwThreadMgrEventSinkCookie);
    pSource->Release();
    return SUCCEEDED(hr);
}

void McBopomofoTIP::_UninitThreadMgrEventSink() {
    if (_dwThreadMgrEventSinkCookie == TF_INVALID_COOKIE || !_ptim) {
        return;
    }

    ITfSource *pSource = nullptr;
    if (SUCCEEDED(_ptim->QueryInterface(IID_ITfSource, (void **)&pSource))) {
        pSource->UnadviseSink(_dwThreadMgrEventSinkCookie);
        pSource->Release();
    }
    _dwThreadMgrEventSinkCookie = TF_INVALID_COOKIE;
}

BOOL McBopomofoTIP::_InitThreadFocusSink() {
    ITfSource *pSource = nullptr;
    HRESULT hr = _ptim->QueryInterface(IID_ITfSource, (void **)&pSource);
    if (FAILED(hr)) {
        return FALSE;
    }

    hr = pSource->AdviseSink(
        IID_ITfThreadFocusSink,
        static_cast<ITfThreadFocusSink *>(this),
        &_dwThreadFocusSinkCookie);
    pSource->Release();
    return SUCCEEDED(hr);
}

void McBopomofoTIP::_UninitThreadFocusSink() {
    if (_dwThreadFocusSinkCookie == TF_INVALID_COOKIE || !_ptim) {
        return;
    }

    ITfSource *pSource = nullptr;
    if (SUCCEEDED(_ptim->QueryInterface(IID_ITfSource, (void **)&pSource))) {
        pSource->UnadviseSink(_dwThreadFocusSinkCookie);
        pSource->Release();
    }
    _dwThreadFocusSinkCookie = TF_INVALID_COOKIE;
}

STDAPI McBopomofoTIP::Activate(ITfThreadMgr *ptim, TfClientId tid) {
    return ActivateEx(ptim, tid, 0);
}

STDAPI McBopomofoTIP::ActivateEx(ITfThreadMgr *ptim, TfClientId tid, DWORD dwFlags) {
    LogMessage("McBopomofoTIP::ActivateEx called with flags: %u", dwFlags);

    if (ptim == nullptr) {
        return E_INVALIDARG;
    }

    _ptim = ptim;
    _ptim->AddRef();
    _tid = tid;

    if (!_InitKeyEventSink()) {
        LogMessage("Failed to init KeyEventSink");
        return E_FAIL;
    }
    if (!_InitThreadMgrEventSink()) {
        LogMessage("Failed to init ThreadMgrEventSink");
        return E_FAIL;
    }
    if (!_InitThreadFocusSink()) {
        LogMessage("Failed to init ThreadFocusSink");
        return E_FAIL;
    }

    ITfCompartmentMgr *pCompMgr = nullptr;
    if (SUCCEEDED(_ptim->QueryInterface(IID_ITfCompartmentMgr, (void **)&pCompMgr))) {
        ITfCompartment *pComp = nullptr;
        if (SUCCEEDED(pCompMgr->GetCompartment(GUID_COMPARTMENT_KEYBOARD_OPENCLOSE, &pComp))) {
            VARIANT var;
            var.vt = VT_I4;
            var.lVal = 1;
            pComp->SetValue(_tid, &var);
            pComp->Release();
            LogMessage("Keyboard compartment set to OPEN");
        }
        pCompMgr->Release();
    }

    extern HINSTANCE g_hInst;
    _candidateWindow.Create(g_hInst);

    // Register LangBar button
    ITfLangBarItemMgr *pLangBarItemMgr = nullptr;
    if (SUCCEEDED(_ptim->QueryInterface(IID_ITfLangBarItemMgr, (void **)&pLangBarItemMgr))) {
        extern const GUID GUID_LBI_INPUTMODE; // defined in LangBarButton.cpp
        _pLangBarButton = new CLangBarButton(this, GUID_LBI_INPUTMODE);
        pLangBarItemMgr->AddItem(_pLangBarButton);
        pLangBarItemMgr->Release();
    }

    LogMessage("McBopomofoTIP::ActivateEx succeeded");
    return S_OK;
}

STDAPI McBopomofoTIP::Deactivate() {
    LogMessage("McBopomofoTIP::Deactivate called");

    if (_pLangBarButton) {
        ITfLangBarItemMgr *pLangBarItemMgr = nullptr;
        if (SUCCEEDED(_ptim->QueryInterface(IID_ITfLangBarItemMgr, (void **)&pLangBarItemMgr))) {
            pLangBarItemMgr->RemoveItem(_pLangBarButton);
            pLangBarItemMgr->Release();
        }
        _pLangBarButton->Release();
        _pLangBarButton = nullptr;
    }

    _candidateWindow.Destroy();

    _UninitThreadFocusSink();
    _UninitThreadMgrEventSink();
    _UninitKeyEventSink();

    if (_pComposition) {
        _pComposition->Release();
        _pComposition = nullptr;
    }

    if (_ptim) {
        _ptim->Release();
        _ptim = nullptr;
    }
    _tid = TF_CLIENTID_NULL;

    return S_OK;
}

STDAPI McBopomofoTIP::OnSetFocus(BOOL fForeground) {
    UNREFERENCED_PARAMETER(fForeground);
    return S_OK;
}

STDAPI McBopomofoTIP::OnTestKeyDown(ITfContext *pic, WPARAM wParam, LPARAM lParam, BOOL *pfEaten) {
    UNREFERENCED_PARAMETER(pic);
    if (pfEaten == nullptr) {
        return E_INVALIDARG;
    }

    BYTE keyboardState[256];
    GetKeyboardState(keyboardState);

    // Ctrl+Space toggle
    if (wParam == VK_SPACE && (keyboardState[VK_CONTROL] & 0x80)) {
        *pfEaten = TRUE;
        return S_OK;
    }

    if (!IsOpen()) {
        *pfEaten = FALSE;
        return S_OK;
    }

    // When candidate window is open, block all keys except ESC
    if (!_lastState.candidates.empty()) {
        *pfEaten = TRUE;
        return S_OK;
    }

    switch (wParam) {
    case VK_BACK:
    case VK_TAB:
    case VK_RETURN:
    case VK_ESCAPE:
    case VK_SPACE:
    case VK_PRIOR:
    case VK_NEXT:
    case VK_END:
    case VK_HOME:
    case VK_LEFT:
    case VK_UP:
    case VK_RIGHT:
    case VK_DOWN:
        *pfEaten = TRUE;
        return S_OK;
    default:
        break;
    }

    WCHAR chars[2] = {0};
    *pfEaten = ToUnicode((UINT)wParam, (lParam >> 16) & 0xFF, keyboardState, chars, 2, 0) == 1;
    return S_OK;
}

STDAPI McBopomofoTIP::OnKeyDown(ITfContext *pic, WPARAM wParam, LPARAM lParam, BOOL *pfEaten) {
    if (pfEaten == nullptr) {
        return E_INVALIDARG;
    }

    BYTE keyboardState[256];
    GetKeyboardState(keyboardState);

    // Ctrl+Space toggle
    if (wParam == VK_SPACE && (keyboardState[VK_CONTROL] & 0x80)) {
        ToggleOpenClose();
        *pfEaten = TRUE;
        return S_OK;
    }

    if (!IsOpen()) {
        *pfEaten = FALSE;
        return S_OK;
    }

    BOOL eaten = FALSE;
    OnTestKeyDown(pic, wParam, lParam, &eaten);

    if (!eaten) {
        *pfEaten = FALSE;
        return S_OK;
    }

    McBopomofo::IPC::KeyEventPayload req;
    req.vk = (unsigned int)wParam;
    req.shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    req.ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;

    GetKeyboardState(keyboardState);
    WCHAR chars[2] = {0};
    if (ToUnicode((UINT)wParam, (lParam >> 16) & 0xFF, keyboardState, chars, 2, 0) == 1) {
        req.ascii = (chars[0] >= 32 && chars[0] <= 126) ? chars[0] : 0;
    } else {
        req.ascii = 0;
    }

    McBopomofo::IPC::NamedPipeClient pipe(McBopomofo::IPC::PIPE_NAME);
    std::string response;

    // Local candidate handling disabled (moved to server)
    if (false && !_lastState.candidates.empty()) {
        int index = -1;
        const int pageSize = 9;
        int pageIndex = _lastState.cursorIndex / pageSize;
        int startIndex = pageIndex * pageSize;
        int totalPages = ((int)_lastState.candidates.size() + pageSize - 1) / pageSize;

        if (wParam >= '1' && wParam <= '9') {
            int num = (int)(wParam - '1');
            if (startIndex + num < (int)_lastState.candidates.size()) {
                index = startIndex + num;
            }
        } else if (wParam == VK_NUMPAD1 || (wParam >= VK_NUMPAD1 && wParam <= VK_NUMPAD9)) {
             int num = (int)(wParam - VK_NUMPAD1);
             if (startIndex + num < (int)_lastState.candidates.size()) {
                 index = startIndex + num;
             }
        } else if (wParam == VK_SPACE || wParam == VK_RETURN) {
            index = _lastState.cursorIndex;
        } else if (wParam == VK_UP) {
            _lastState.cursorIndex = (_lastState.cursorIndex - 1 + (int)_lastState.candidates.size()) % (int)_lastState.candidates.size();
            GetCandidateWindow()->UpdateUI(_lastState.candidates, _lastState.cursorIndex, _lastState.forceVertical);
            *pfEaten = TRUE;
            return S_OK;
        } else if (wParam == VK_DOWN) {
            _lastState.cursorIndex = (_lastState.cursorIndex + 1) % (int)_lastState.candidates.size();
            GetCandidateWindow()->UpdateUI(_lastState.candidates, _lastState.cursorIndex, _lastState.forceVertical);
            *pfEaten = TRUE;
            return S_OK;
        } else if (wParam == VK_PRIOR) { // Page Up
            int newIndex = std::max(0, _lastState.cursorIndex - pageSize);
            _lastState.cursorIndex = newIndex;
            GetCandidateWindow()->UpdateUI(_lastState.candidates, _lastState.cursorIndex, _lastState.forceVertical);
            *pfEaten = TRUE;
            return S_OK;
        } else if (wParam == VK_NEXT) { // Page Down
            int newIndex = std::min((int)_lastState.candidates.size() - 1, _lastState.cursorIndex + pageSize);
            _lastState.cursorIndex = newIndex;
            GetCandidateWindow()->UpdateUI(_lastState.candidates, _lastState.cursorIndex, _lastState.forceVertical);
            *pfEaten = TRUE;
            return S_OK;
        }

        if (index != -1) {
            McBopomofo::IPC::SelectCandidatePayload selReq;
            selReq.index = index;
            std::string selPayload = McBopomofo::IPC::SerializeSelectCandidate(selReq);
            if (pipe.Call(selPayload, response)) {
                if (McBopomofo::IPC::DeserializeStateUpdate(response, _lastState)) {
                    CStateEditSession *pEditSession = new CStateEditSession(pic, this, _lastState);
                    HRESULT hr;
                    pic->RequestEditSession(_tid, pEditSession, TF_ES_SYNC | TF_ES_READWRITE, &hr);
                    pEditSession->Release();
                    *pfEaten = TRUE;
                    return S_OK;
                }
            }
        }
    }

    std::string payload = McBopomofo::IPC::SerializeKeyEvent(req);
    LogMessage("Sending IPC request: %s", payload.c_str());

    if (pipe.Call(payload, response)) {
        LogMessage("Received IPC response: %s", response.c_str());
        if (McBopomofo::IPC::DeserializeStateUpdate(response, _lastState)) {
            *pfEaten = _lastState.consumed ? TRUE : FALSE;
            LogMessage("State deserialized. Consumed: %d, CommitStr: '%s', CompStr: '%s'", 
                        _lastState.consumed, _lastState.commitString.c_str(), _lastState.composingBuffer.c_str());

            if (_lastState.consumed) {
                CStateEditSession *pEditSession = new CStateEditSession(pic, this, _lastState);
                HRESULT hr;
                pic->RequestEditSession(_tid, pEditSession, TF_ES_SYNC | TF_ES_READWRITE, &hr);
                LogMessage("RequestEditSession returned: 0x%08X", hr);
                pEditSession->Release();
            }
        } else {
            LogMessage("Failed to deserialize state update");
            *pfEaten = FALSE;
        }
    } else {
        LogMessage("IPC Call failed");
        *pfEaten = FALSE;
    }

    return S_OK;
}

STDAPI McBopomofoTIP::OnTestKeyUp(ITfContext *pic, WPARAM wParam, LPARAM lParam, BOOL *pfEaten) {
    UNREFERENCED_PARAMETER(pic);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    if (pfEaten == nullptr) {
        return E_INVALIDARG;
    }
    *pfEaten = FALSE;
    return S_OK;
}

STDAPI McBopomofoTIP::OnKeyUp(ITfContext *pic, WPARAM wParam, LPARAM lParam, BOOL *pfEaten) {
    UNREFERENCED_PARAMETER(pic);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    if (pfEaten == nullptr) {
        return E_INVALIDARG;
    }
    *pfEaten = FALSE;
    return S_OK;
}

STDAPI McBopomofoTIP::OnPreservedKey(ITfContext *pic, REFGUID rguid, BOOL *pfEaten) {
    UNREFERENCED_PARAMETER(pic);
    UNREFERENCED_PARAMETER(rguid);
    if (pfEaten == nullptr) {
        return E_INVALIDARG;
    }
    *pfEaten = FALSE;
    return S_OK;
}

STDAPI McBopomofoTIP::OnCompositionTerminated(TfEditCookie ecWrite, ITfComposition *pComposition) {
    UNREFERENCED_PARAMETER(ecWrite);
    if (_pComposition == pComposition) {
        _pComposition->Release();
        _pComposition = nullptr;
    }
    return S_OK;
}

#include "DisplayAttributeInfo.h"

STDAPI McBopomofoTIP::EnumDisplayAttributeInfo(IEnumTfDisplayAttributeInfo **ppEnum) {
    if (ppEnum == nullptr) return E_INVALIDARG;
    *ppEnum = new CEnumDisplayAttributeInfo();
    return (*ppEnum != nullptr) ? S_OK : E_OUTOFMEMORY;
}

STDAPI McBopomofoTIP::GetDisplayAttributeInfo(REFGUID guidInfo, ITfDisplayAttributeInfo **ppInfo) {
    if (ppInfo == nullptr) return E_INVALIDARG;
    *ppInfo = nullptr;

    if (IsEqualGUID(guidInfo, c_guidDisplayAttributeInput)) {
        TF_DISPLAYATTRIBUTE da;
        ZeroMemory(&da, sizeof(da));
        da.lsStyle = TF_LS_SQUIGGLE;
        da.crLine.type = TF_CT_SYSCOLOR;
        da.crLine.nIndex = COLOR_WINDOWTEXT;
        *ppInfo = new CDisplayAttributeInfo(c_guidDisplayAttributeInput, da, L"Win-McBopomofo Input");
    } else if (IsEqualGUID(guidInfo, c_guidDisplayAttributeMarked)) {
        TF_DISPLAYATTRIBUTE da;
        ZeroMemory(&da, sizeof(da));
        da.lsStyle = TF_LS_NONE;
        da.crText.type = TF_CT_SYSCOLOR;
        da.crText.nIndex = COLOR_HIGHLIGHTTEXT;
        da.crBk.type = TF_CT_SYSCOLOR;
        da.crBk.nIndex = COLOR_HIGHLIGHT;
        *ppInfo = new CDisplayAttributeInfo(c_guidDisplayAttributeMarked, da, L"Win-McBopomofo Marked");
    }

    return (*ppInfo != nullptr) ? S_OK : E_INVALIDARG;
}

STDAPI McBopomofoTIP::OnInitDocumentMgr(ITfDocumentMgr *pDocMgr) {
    UNREFERENCED_PARAMETER(pDocMgr);
    return S_OK;
}

STDAPI McBopomofoTIP::OnUninitDocumentMgr(ITfDocumentMgr *pDocMgr) {
    UNREFERENCED_PARAMETER(pDocMgr);
    return S_OK;
}

STDAPI McBopomofoTIP::OnSetFocus(ITfDocumentMgr *pDocMgrFocus, ITfDocumentMgr *pDocMgrPrevFocus) {
    UNREFERENCED_PARAMETER(pDocMgrFocus);
    UNREFERENCED_PARAMETER(pDocMgrPrevFocus);
    return S_OK;
}

STDAPI McBopomofoTIP::OnPushContext(ITfContext *pic) {
    UNREFERENCED_PARAMETER(pic);
    return S_OK;
}

STDAPI McBopomofoTIP::OnPopContext(ITfContext *pic) {
    UNREFERENCED_PARAMETER(pic);
    return S_OK;
}

STDAPI McBopomofoTIP::OnSetThreadFocus() {
    return S_OK;
}

STDAPI McBopomofoTIP::OnKillThreadFocus() {
    _candidateWindow.Hide();
    return S_OK;
}

bool McBopomofoTIP::IsOpen() {
    bool isOpen = true;
    if (_ptim) {
        ITfCompartmentMgr *pCompMgr = nullptr;
        if (SUCCEEDED(_ptim->QueryInterface(IID_ITfCompartmentMgr, (void **)&pCompMgr))) {
            ITfCompartment *pComp = nullptr;
            if (SUCCEEDED(pCompMgr->GetCompartment(GUID_COMPARTMENT_KEYBOARD_OPENCLOSE, &pComp))) {
                VARIANT var;
                VariantInit(&var);
                if (SUCCEEDED(pComp->GetValue(&var))) {
                    if (var.vt == VT_I4) {
                        isOpen = (var.lVal != 0);
                    }
                    VariantClear(&var);
                }
                pComp->Release();
            }
            pCompMgr->Release();
        }
    }
    return isOpen;
}

void McBopomofoTIP::ToggleOpenClose() {
    if (_ptim) {
        bool currentOpen = IsOpen();
        ITfCompartmentMgr *pCompMgr = nullptr;
        if (SUCCEEDED(_ptim->QueryInterface(IID_ITfCompartmentMgr, (void **)&pCompMgr))) {
            ITfCompartment *pComp = nullptr;
            if (SUCCEEDED(pCompMgr->GetCompartment(GUID_COMPARTMENT_KEYBOARD_OPENCLOSE, &pComp))) {
                VARIANT var;
                var.vt = VT_I4;
                var.lVal = currentOpen ? 0 : 1;
                pComp->SetValue(_tid, &var);
                pComp->Release();
                
                if (_pLangBarButton) {
                    _pLangBarButton->Update();
                }
            }
            pCompMgr->Release();
        }
    }
}
