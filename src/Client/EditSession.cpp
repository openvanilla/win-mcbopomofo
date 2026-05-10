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

#include "EditSession.h"

CEditSessionBase::CEditSessionBase(ITfContext* pContext) : cRef_(1) {
  pContext_ = pContext;
  if (pContext_) {
    pContext_->AddRef();
  }
}

CEditSessionBase::~CEditSessionBase() {
  if (pContext_) {
    pContext_->Release();
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
  return InterlockedIncrement(&cRef_);
}

STDAPI_(ULONG) CEditSessionBase::Release() {
  LONG cr = InterlockedDecrement(&cRef_);
  if (cr == 0) {
    delete this;
  }
  return cr;
}

