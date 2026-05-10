#include "EditSession.h"

CEditSessionBase::CEditSessionBase(ITfContext* pContext) : _cRef(1) {
  _pContext = pContext;
  if (_pContext) {
    _pContext->AddRef();
  }
}

CEditSessionBase::~CEditSessionBase() {
  if (_pContext) {
    _pContext->Release();
  }
}

STDAPI CEditSessionBase::QueryInterface(REFIID riid, void** ppvObj) {
  if (ppvObj == nullptr) return E_INVALIDARG;
  *ppvObj = nullptr;

  if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfEditSession)) {
    *ppvObj = static_cast<ITfEditSession*>(this);
  }

  if (*ppvObj) {
    AddRef();
    return S_OK;
  }

  return E_NOINTERFACE;
}

STDAPI_(ULONG) CEditSessionBase::AddRef() {
  return InterlockedIncrement(&_cRef);
}

STDAPI_(ULONG) CEditSessionBase::Release() {
  LONG cr = InterlockedDecrement(&_cRef);
  if (cr == 0) {
    delete this;
  }
  return cr;
}
