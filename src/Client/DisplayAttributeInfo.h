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

#pragma once
#include <msctf.h>
#include <windows.h>

// GUID for normal inputting state (Underline)
// {4B688CD4-CFB6-4767-AD80-4D562086FC3B}
extern const GUID c_guidDisplayAttributeInput;

// GUID for marking/highlight state (Highlight)
// {D82C4A26-E0CC-43BB-8CF2-BB0BBFF4FE70}
extern const GUID c_guidDisplayAttributeMarked;

class CDisplayAttributeInfo : public ITfDisplayAttributeInfo {
 public:
  CDisplayAttributeInfo(REFGUID guid, TF_DISPLAYATTRIBUTE da,
                        const WCHAR* desc);
  ~CDisplayAttributeInfo();

  // IUnknown
  STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj) override;
  STDMETHODIMP_(ULONG) AddRef(void) override;
  STDMETHODIMP_(ULONG) Release(void) override;

  // ITfDisplayAttributeInfo
  STDMETHODIMP GetGUID(GUID* pguid) override;
  STDMETHODIMP GetDescription(BSTR* pbstrDesc) override;
  STDMETHODIMP GetAttributeInfo(TF_DISPLAYATTRIBUTE* pda) override;
  STDMETHODIMP SetAttributeInfo(const TF_DISPLAYATTRIBUTE* pda) override;
  STDMETHODIMP Reset() override;

 private:
  LONG cRef_;
  GUID guid_;
  TF_DISPLAYATTRIBUTE da_;
  TF_DISPLAYATTRIBUTE daDefault_;
  const WCHAR* desc_;
};

class CEnumDisplayAttributeInfo : public IEnumTfDisplayAttributeInfo {
 public:
  CEnumDisplayAttributeInfo();
  ~CEnumDisplayAttributeInfo();

  // IUnknown
  STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj) override;
  STDMETHODIMP_(ULONG) AddRef(void) override;
  STDMETHODIMP_(ULONG) Release(void) override;

  // IEnumTfDisplayAttributeInfo
  STDMETHODIMP Clone(IEnumTfDisplayAttributeInfo** ppEnum) override;
  STDMETHODIMP Next(ULONG ulCount, ITfDisplayAttributeInfo** rgInfo,
                    ULONG* pcFetched) override;
  STDMETHODIMP Reset() override;
  STDMETHODIMP Skip(ULONG ulCount) override;

 private:
  LONG cRef_;
  int index_;
};
