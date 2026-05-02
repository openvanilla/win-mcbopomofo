#pragma once
#include <windows.h>
#include <msctf.h>

class CEditSessionBase : public ITfEditSession {
public:
    CEditSessionBase(ITfContext *pContext);
    virtual ~CEditSessionBase();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef(void) override;
    STDMETHODIMP_(ULONG) Release(void) override;

    // ITfEditSession
    virtual STDMETHODIMP DoEditSession(TfEditCookie ec) = 0;

protected:
    ITfContext *_pContext;
private:
    LONG _cRef;
};
