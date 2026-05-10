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
  LONG _cRef;
  GUID _guid;
  TF_DISPLAYATTRIBUTE _da;
  TF_DISPLAYATTRIBUTE _daDefault;
  const WCHAR* _desc;
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
  LONG _cRef;
  int _index;
};
