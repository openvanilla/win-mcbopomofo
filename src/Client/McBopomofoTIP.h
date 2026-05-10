#pragma once
#include <msctf.h>
#include <windows.h>

#include "CandidateWindow.h"
#include "Ipc.h"
#include "TooltipWindow.h"

class McBopomofoTIP : public ITfTextInputProcessorEx,
                      public ITfKeyEventSink,
                      public ITfCompositionSink,
                      public ITfDisplayAttributeProvider,
                      public ITfThreadMgrEventSink,
                      public ITfThreadFocusSink,
                      public ITfCompartmentEventSink {
 public:
  McBopomofoTIP();
  ~McBopomofoTIP();

  // IUnknown methods
  STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj) override;
  STDMETHODIMP_(ULONG) AddRef(void) override;
  STDMETHODIMP_(ULONG) Release(void) override;

  // ITfTextInputProcessor methods
  STDMETHODIMP Activate(ITfThreadMgr* ptim, TfClientId tid) override;
  STDMETHODIMP Deactivate() override;

  // ITfTextInputProcessorEx methods
  STDMETHODIMP ActivateEx(ITfThreadMgr* ptim, TfClientId tid,
                          DWORD dwFlags) override;

  // ITfKeyEventSink methods
  STDMETHODIMP OnSetFocus(BOOL fForeground) override;
  STDMETHODIMP OnTestKeyDown(ITfContext* pic, WPARAM wParam, LPARAM lParam,
                             BOOL* pfEaten) override;
  STDMETHODIMP OnKeyDown(ITfContext* pic, WPARAM wParam, LPARAM lParam,
                         BOOL* pfEaten) override;
  STDMETHODIMP OnTestKeyUp(ITfContext* pic, WPARAM wParam, LPARAM lParam,
                           BOOL* pfEaten) override;
  STDMETHODIMP OnKeyUp(ITfContext* pic, WPARAM wParam, LPARAM lParam,
                       BOOL* pfEaten) override;
  STDMETHODIMP OnPreservedKey(ITfContext* pic, REFGUID rguid,
                              BOOL* pfEaten) override;

  // ITfCompositionSink methods
  STDMETHODIMP OnCompositionTerminated(TfEditCookie ecWrite,
                                       ITfComposition* pComposition) override;

  // ITfDisplayAttributeProvider methods
  STDMETHODIMP EnumDisplayAttributeInfo(
      IEnumTfDisplayAttributeInfo** ppEnum) override;
  STDMETHODIMP GetDisplayAttributeInfo(
      REFGUID guidInfo, ITfDisplayAttributeInfo** ppInfo) override;

  // ITfThreadMgrEventSink methods
  STDMETHODIMP OnInitDocumentMgr(ITfDocumentMgr* pDocMgr) override;
  STDMETHODIMP OnUninitDocumentMgr(ITfDocumentMgr* pDocMgr) override;
  STDMETHODIMP OnSetFocus(ITfDocumentMgr* pDocMgrFocus,
                          ITfDocumentMgr* pDocMgrPrevFocus) override;
  STDMETHODIMP OnPushContext(ITfContext* pic) override;
  STDMETHODIMP OnPopContext(ITfContext* pic) override;

  // ITfThreadFocusSink methods
  STDMETHODIMP OnSetThreadFocus() override;
  STDMETHODIMP OnKillThreadFocus() override;

  // ITfCompartmentEventSink methods
  STDMETHODIMP OnChange(REFGUID rguid) override;

 private:
  BOOL _InitKeyEventSink();
  void _UninitKeyEventSink();
  BOOL _InitCompartmentEventSink();
  void _UninitCompartmentEventSink();
  BOOL _InitThreadMgrEventSink();
  void _UninitThreadMgrEventSink();
  BOOL _InitThreadFocusSink();
  void _UninitThreadFocusSink();
  bool IsDirectCommitWithoutComposition(
      const McBopomofo::IPC::StateUpdatePayload& state) const;
  void HideAuxiliaryWindowsForDirectCommit(
      const McBopomofo::IPC::StateUpdatePayload& state);
  void ApplyStateToContext(ITfContext* context,
                           const McBopomofo::IPC::StateUpdatePayload& state,
                           const char* logPrefix);
  void ResetServerState();

  LONG _cRef;
  ITfThreadMgr* _ptim;
  TfClientId _tid;

  DWORD _dwThreadMgrEventSinkCookie;
  DWORD _dwThreadFocusSinkCookie;
  DWORD _dwOpenCloseCompartmentEventSinkCookie;

  // Track the server state locally to decide whether to eat keys in
  // OnTestKeyDown
  McBopomofo::IPC::StateUpdatePayload _lastState;

  ITfComposition* _pComposition;
  CandidateWindow _candidateWindow;
  TooltipWindow _tooltipWindow;
  class CLangBarButton* _pModeIconButton;
  class CLangBarButton* _pSwitchLangButton;

 public:
  void ToggleOpenClose();
  bool IsOpen();
  void RefreshLangBar();

 public:
  ITfComposition* GetComposition() const { return _pComposition; }
  void SetComposition(ITfComposition* pComp) { _pComposition = pComp; }
  CandidateWindow* GetCandidateWindow() { return &_candidateWindow; }
  TooltipWindow* GetTooltipWindow() { return &_tooltipWindow; }
};
