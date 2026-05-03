#pragma once
#include <windows.h>
#include <msctf.h>
#include <string>
#include <vector>

class McBopomofoTIP;

class CLangBarButton : public ITfLangBarItemButton, public ITfSource {
public:
    CLangBarButton(McBopomofoTIP* pTIP, const GUID& guid);
    ~CLangBarButton();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef(void) override;
    STDMETHODIMP_(ULONG) Release(void) override;

    // ITfLangBarItem
    STDMETHODIMP GetInfo(TF_LANGBARITEMINFO *pInfo) override;
    STDMETHODIMP GetStatus(DWORD *pdwStatus) override;
    STDMETHODIMP Show(BOOL fShow) override;
    STDMETHODIMP GetTooltipString(BSTR *pbstrToolTip) override;

    // ITfLangBarItemButton
    STDMETHODIMP OnClick(TfLBIClick click, POINT pt, const RECT *prcArea) override;
    STDMETHODIMP InitMenu(ITfMenu *pMenu) override;
    STDMETHODIMP OnMenuSelect(UINT wID) override;
    STDMETHODIMP GetIcon(HICON *phIcon) override;
    STDMETHODIMP GetText(BSTR *pbstrText) override;

    // ITfSource
    STDMETHODIMP AdviseSink(REFIID riid, IUnknown *punk, DWORD *pdwCookie) override;
    STDMETHODIMP UnadviseSink(DWORD dwCookie) override;

    void Update();

private:
    long _refCount;
    McBopomofoTIP* _pTIP;
    GUID _guid;
    std::vector<ITfLangBarItemSink*> _sinks;
};
