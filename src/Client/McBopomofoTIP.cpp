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

STDAPI McBopomofoTIP::Activate(ITfThreadMgr *ptim, TfClientId tid) {
    _ptim = ptim;
    _ptim->AddRef();
    _tid = tid;

    // TODO: Initialize key event sink and other TSF components here

    return S_OK;
}

STDAPI McBopomofoTIP::Deactivate() {
    // TODO: Cleanup TSF components here

    if (_ptim) {
        _ptim->Release();
        _ptim = nullptr;
    }
    _tid = TF_CLIENTID_NULL;

    return S_OK;
}
