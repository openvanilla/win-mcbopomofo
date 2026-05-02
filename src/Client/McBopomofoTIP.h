#pragma once
#include <windows.h>
#include <msctf.h>

class McBopomofoTIP : public ITfTextInputProcessor {
public:
    McBopomofoTIP();
    ~McBopomofoTIP();

    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef(void) override;
    STDMETHODIMP_(ULONG) Release(void) override;

    // ITfTextInputProcessor methods
    STDMETHODIMP Activate(ITfThreadMgr *ptim, TfClientId tid) override;
    STDMETHODIMP Deactivate() override;

private:
    LONG _cRef;
    ITfThreadMgr *_ptim;
    TfClientId _tid;
};
