#pragma once
#include <windows.h>
#include <msctf.h>
#include <atomic>
#include <string>
#include <utility>
#include <vector>

class McBopomofoTIP;

extern const GUID GUID_LBI_INPUTMODE;
extern const GUID GUID_LBI_SWITCH_LANG;

class CLangBarButton : public ITfLangBarItemButton, public ITfSource {
public:
    enum class Kind {
        ModeIcon,
        SwitchLanguageMenu,
    };

    CLangBarButton(McBopomofoTIP* pTIP, const GUID& guid, Kind kind);
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
    Kind _kind;
    std::vector<std::pair<DWORD, ITfLangBarItemSink*>> _sinks;
    static std::atomic<DWORD> _nextCookie;
};
