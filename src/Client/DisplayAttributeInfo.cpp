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

#include "DisplayAttributeInfo.h"

const GUID c_guidDisplayAttributeInput = {
    0x4b688cd4,
    0xcfb6,
    0x4767,
    {0xad, 0x80, 0x4d, 0x56, 0x20, 0x86, 0xfc, 0x3b}};

const GUID c_guidDisplayAttributeMarked = {
    0xd82c4a26,
    0xe0cc,
    0x43bb,
    {0x8c, 0xf2, 0xbb, 0xb, 0xbf, 0xf4, 0xfe, 0x70}};

// ----------------------------------------------------------------------------
// CDisplayAttributeInfo
// ----------------------------------------------------------------------------
CDisplayAttributeInfo::CDisplayAttributeInfo(REFGUID guid,
                                             TF_DISPLAYATTRIBUTE da,
                                             const WCHAR* desc)
    : cRef_(1), guid_(guid), da_(da), daDefault_(da), desc_(desc) {}

CDisplayAttributeInfo::~CDisplayAttributeInfo() {}

STDAPI CDisplayAttributeInfo::QueryInterface(REFIID riid, void** ppvObj) {
  if (ppvObj == nullptr) return E_INVALIDARG;
  *ppvObj = nullptr;
  if (IsEqualIID(riid, IID_IUnknown) ||
      IsEqualIID(riid, IID_ITfDisplayAttributeInfo)) {
    *ppvObj = static_cast<ITfDisplayAttributeInfo*>(this);
  }
  if (*ppvObj) {
    AddRef();
    return S_OK;
  }
  return E_NOINTERFACE;
}

STDAPI_(ULONG) CDisplayAttributeInfo::AddRef(void) {
  return InterlockedIncrement(&cRef_);
}

STDAPI_(ULONG) CDisplayAttributeInfo::Release(void) {
  LONG cr = InterlockedDecrement(&cRef_);
  if (cr == 0) delete this;
  return cr;
}

STDAPI CDisplayAttributeInfo::GetGUID(GUID* pguid) {
  if (pguid == nullptr) return E_INVALIDARG;
  *pguid = guid_;
  return S_OK;
}

STDAPI CDisplayAttributeInfo::GetDescription(BSTR* pbstrDesc) {
  if (pbstrDesc == nullptr) return E_INVALIDARG;
  *pbstrDesc = SysAllocString(desc_);
  return (*pbstrDesc != nullptr) ? S_OK : E_OUTOFMEMORY;
}

STDAPI CDisplayAttributeInfo::GetAttributeInfo(TF_DISPLAYATTRIBUTE* pda) {
  if (pda == nullptr) return E_INVALIDARG;
  *pda = da_;
  return S_OK;
}

STDAPI CDisplayAttributeInfo::SetAttributeInfo(const TF_DISPLAYATTRIBUTE* pda) {
  if (pda == nullptr) return E_INVALIDARG;
  da_ = *pda;
  return S_OK;
}

STDAPI CDisplayAttributeInfo::Reset() {
  da_ = daDefault_;
  return S_OK;
}

// ----------------------------------------------------------------------------
// CEnumDisplayAttributeInfo
// ----------------------------------------------------------------------------
CEnumDisplayAttributeInfo::CEnumDisplayAttributeInfo() : cRef_(1), index_(0) {}
CEnumDisplayAttributeInfo::~CEnumDisplayAttributeInfo() {}

STDAPI CEnumDisplayAttributeInfo::QueryInterface(REFIID riid, void** ppvObj) {
  if (ppvObj == nullptr) return E_INVALIDARG;
  *ppvObj = nullptr;
  if (IsEqualIID(riid, IID_IUnknown) ||
      IsEqualIID(riid, IID_IEnumTfDisplayAttributeInfo)) {
    *ppvObj = static_cast<IEnumTfDisplayAttributeInfo*>(this);
  }
  if (*ppvObj) {
    AddRef();
    return S_OK;
  }
  return E_NOINTERFACE;
}

STDAPI_(ULONG) CEnumDisplayAttributeInfo::AddRef(void) {
  return InterlockedIncrement(&cRef_);
}

STDAPI_(ULONG) CEnumDisplayAttributeInfo::Release(void) {
  LONG cr = InterlockedDecrement(&cRef_);
  if (cr == 0) delete this;
  return cr;
}

STDAPI CEnumDisplayAttributeInfo::Clone(IEnumTfDisplayAttributeInfo** ppEnum) {
  if (ppEnum == nullptr) return E_INVALIDARG;
  *ppEnum = new CEnumDisplayAttributeInfo();
  if (*ppEnum == nullptr) return E_OUTOFMEMORY;
  ((CEnumDisplayAttributeInfo*)*ppEnum)->index_ = index_;
  return S_OK;
}

STDAPI CEnumDisplayAttributeInfo::Next(ULONG ulCount,
                                       ITfDisplayAttributeInfo** rgInfo,
                                       ULONG* pcFetched) {
  if (pcFetched) *pcFetched = 0;
  if (ulCount == 0 || rgInfo == nullptr) return E_INVALIDARG;

  ULONG fetched = 0;
  while (fetched < ulCount && index_ < 2) {
    if (index_ == 0) {
      TF_DISPLAYATTRIBUTE da;
      ZeroMemory(&da, sizeof(da));
      da.lsStyle = TF_LS_SQUIGGLE;
      da.crLine.type = TF_CT_SYSCOLOR;
      da.crLine.nIndex = COLOR_WINDOWTEXT;
      rgInfo[fetched] = new CDisplayAttributeInfo(c_guidDisplayAttributeInput,
                                                  da, L"Win-McBopomofo Input");
    } else if (index_ == 1) {
      TF_DISPLAYATTRIBUTE da;
      ZeroMemory(&da, sizeof(da));
      da.lsStyle = TF_LS_NONE;
      da.crText.type = TF_CT_SYSCOLOR;
      da.crText.nIndex = COLOR_HIGHLIGHTTEXT;
      da.crBk.type = TF_CT_SYSCOLOR;
      da.crBk.nIndex = COLOR_HIGHLIGHT;
      rgInfo[fetched] = new CDisplayAttributeInfo(c_guidDisplayAttributeMarked,
                                                  da, L"Win-McBopomofo Marked");
    }
    if (rgInfo[fetched] == nullptr) return E_OUTOFMEMORY;
    index_++;
    fetched++;
  }

  if (pcFetched) *pcFetched = fetched;
  return (fetched == ulCount) ? S_OK : S_FALSE;
}

STDAPI CEnumDisplayAttributeInfo::Reset() {
  index_ = 0;
  return S_OK;
}

STDAPI CEnumDisplayAttributeInfo::Skip(ULONG ulCount) {
  index_ += ulCount;
  if (index_ > 2) index_ = 2;
  return S_OK;
}