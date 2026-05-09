#include "McBopomofoTIP.h"
#include "Globals.h"
#include "NamedPipe.h"
#include "StateEditSession.h"
#include "LangBarButton.h"

namespace {

bool IsServerHandledShortcutKey(WPARAM wParam, const BYTE keyboardState[256]) {
    const bool ctrlPressed = (keyboardState[VK_CONTROL] & 0x80) != 0;
    const bool shiftPressed = (keyboardState[VK_SHIFT] & 0x80) != 0;

    if (ctrlPressed && !shiftPressed && wParam == VK_OEM_5) {
        return true;
    }
    return false;
}

}

McBopomofoTIP::McBopomofoTIP()
    : _cRef(1),
      _ptim(nullptr),
      _tid(TF_CLIENTID_NULL),
      _dwThreadMgrEventSinkCookie(TF_INVALID_COOKIE),
      _dwThreadFocusSinkCookie(TF_INVALID_COOKIE),
      _pComposition(nullptr),
      _pModeIconButton(nullptr),
      _pSwitchLangButton(nullptr) {
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
    _tooltipWindow.Create(g_hInst);

    // Register LangBar button
    ITfLangBarItemMgr *pLangBarItemMgr = nullptr;
    if (SUCCEEDED(_ptim->QueryInterface(IID_ITfLangBarItemMgr, (void **)&pLangBarItemMgr))) {
        _pModeIconButton = new CLangBarButton(this, GUID_LBI_INPUTMODE, CLangBarButton::Kind::ModeIcon);
        _pSwitchLangButton = new CLangBarButton(this, GUID_LBI_SWITCH_LANG, CLangBarButton::Kind::SwitchLanguageMenu);
        pLangBarItemMgr->AddItem(_pModeIconButton);
        pLangBarItemMgr->AddItem(_pSwitchLangButton);
        pLangBarItemMgr->Release();
    }

    LogMessage("McBopomofoTIP::ActivateEx succeeded");
    return S_OK;
}

STDAPI McBopomofoTIP::Deactivate() {
    LogMessage("McBopomofoTIP::Deactivate called");

    if (_pModeIconButton || _pSwitchLangButton) {
        ITfLangBarItemMgr *pLangBarItemMgr = nullptr;
        if (SUCCEEDED(_ptim->QueryInterface(IID_ITfLangBarItemMgr, (void **)&pLangBarItemMgr))) {
            if (_pModeIconButton) {
                pLangBarItemMgr->RemoveItem(_pModeIconButton);
            }
            if (_pSwitchLangButton) {
                pLangBarItemMgr->RemoveItem(_pSwitchLangButton);
            }
            pLangBarItemMgr->Release();
        }
        if (_pModeIconButton) {
            _pModeIconButton->Release();
            _pModeIconButton = nullptr;
        }
        if (_pSwitchLangButton) {
            _pSwitchLangButton->Release();
            _pSwitchLangButton = nullptr;
        }
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

    // Ctrl+Space toggle (system-level operation, handle here)
    if (wParam == VK_SPACE && (keyboardState[VK_CONTROL] & 0x80)) {
        ToggleOpenClose();
        *pfEaten = TRUE;
        return S_OK;
    }

    // If IME is closed, don't eat any keys
    if (!IsOpen()) {
        *pfEaten = FALSE;
        return S_OK;
    }

    // If we have active composing buffer or candidates, let server handle all keys
    if (!_lastState.composingBuffer.empty() || !_lastState.candidates.empty()) {
        *pfEaten = TRUE;
        return S_OK;
    }

    if (IsServerHandledShortcutKey(wParam, keyboardState)) {
        *pfEaten = TRUE;
        return S_OK;
    }

    // No active state: only eat printable characters, let server decide if it wants to start composing
    WCHAR chars[2] = {0};
    if (ToUnicode((UINT)wParam, (lParam >> 16) & 0xFF, keyboardState, chars, 2, 0) == 1) {
        // Only eat if it's a printable ASCII character (let server handle Bopomofo keys)
        *pfEaten = (chars[0] >= 32 && chars[0] <= 126);
    } else {
        *pfEaten = FALSE;
    }

    return S_OK;
}

//
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

void McBopomofoTIP::RefreshLangBar() {
    if (_pModeIconButton) {
        LogMessage("Refreshing mode icon button");
        _pModeIconButton->Update();
    }
    if (_pSwitchLangButton) {
        LogMessage("Refreshing switch lang button");
        _pSwitchLangButton->Update();
    }
}

void McBopomofoTIP::ToggleOpenClose() {
    if (_ptim) {
        bool currentOpen = IsOpen();
        LogMessage("ToggleOpenClose: current state = %s, toggling to %s", 
                   currentOpen ? "OPEN" : "CLOSED", 
                   currentOpen ? "CLOSED" : "OPEN");

        ITfCompartmentMgr *pCompMgr = nullptr;
        if (SUCCEEDED(_ptim->QueryInterface(IID_ITfCompartmentMgr, (void **)&pCompMgr))) {
            ITfCompartment *pComp = nullptr;
            if (SUCCEEDED(pCompMgr->GetCompartment(GUID_COMPARTMENT_KEYBOARD_OPENCLOSE, &pComp))) {
                VARIANT var;
                var.vt = VT_I4;
                var.lVal = currentOpen ? 0 : 1;
                pComp->SetValue(_tid, &var);
                pComp->Release();

                LogMessage("Compartment value set to: %d", var.lVal);
                RefreshLangBar();
            }
            pCompMgr->Release();
        }
    }
}
