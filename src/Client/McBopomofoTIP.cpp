#include "McBopomofoTIP.h"
#include "Globals.h"

McBopomofoTIP::McBopomofoTIP() : _cRef(1), _ptim(nullptr), _tid(TF_CLIENTID_NULL) {
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

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfTextInputProcessor)) {
        *ppvObj = static_cast<ITfTextInputProcessor *>(this);
    } else if (IsEqualIID(riid, IID_ITfKeyEventSink)) {
        *ppvObj = static_cast<ITfKeyEventSink *>(this);
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

STDAPI McBopomofoTIP::Activate(ITfThreadMgr *ptim, TfClientId tid) {
    _ptim = ptim;
    _ptim->AddRef();
    _tid = tid;

    if (!_InitKeyEventSink()) {
        return E_FAIL;
    }

    return S_OK;
}

STDAPI McBopomofoTIP::Deactivate() {
    _UninitKeyEventSink();

    if (_ptim) {
        _ptim->Release();
        _ptim = nullptr;
    }
    _tid = TF_CLIENTID_NULL;

    return S_OK;
}

STDAPI McBopomofoTIP::OnSetFocus(BOOL fForeground) {
    return S_OK;
}

STDAPI McBopomofoTIP::OnTestKeyDown(ITfContext *pic, WPARAM wParam, LPARAM lParam, BOOL *pfEaten) {
    *pfEaten = FALSE;
    return S_OK;
}

STDAPI McBopomofoTIP::OnKeyDown(ITfContext *pic, WPARAM wParam, LPARAM lParam, BOOL *pfEaten) {
    *pfEaten = FALSE;
    return S_OK;
}

STDAPI McBopomofoTIP::OnTestKeyUp(ITfContext *pic, WPARAM wParam, LPARAM lParam, BOOL *pfEaten) {
    *pfEaten = FALSE;
    return S_OK;
}

STDAPI McBopomofoTIP::OnKeyUp(ITfContext *pic, WPARAM wParam, LPARAM lParam, BOOL *pfEaten) {
    *pfEaten = FALSE;
    return S_OK;
}

STDAPI McBopomofoTIP::OnPreservedKey(ITfContext *pic, REFGUID rguid, BOOL *pfEaten) {
    *pfEaten = FALSE;
    return S_OK;
}
