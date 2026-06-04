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

#include "Ipc.h"
#include "TsfUiElement.h"

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
  BOOL initKeyEventSink_();
  void uninitKeyEventSink_();
  BOOL initCompartmentEventSink_();
  void uninitCompartmentEventSink_();
  BOOL initThreadMgrEventSink_();
  void uninitThreadMgrEventSink_();
  BOOL initThreadFocusSink_();
  void uninitThreadFocusSink_();


  void resetServerState_();
  bool shouldToggleOpenCloseWithShift_() const;
  bool handleStandaloneShiftKeyDown_(WPARAM wParam,
                                     const BYTE keyboardState[256]);
  bool handleStandaloneShiftKeyUp_(WPARAM wParam,
                                   const BYTE keyboardState[256]);

  LONG cRef_;
  ITfThreadMgr* ptim_;
  TfClientId tid_;

  DWORD dwThreadMgrEventSinkCookie_;
  DWORD dwThreadFocusSinkCookie_;
  DWORD dwOpenCloseCompartmentEventSinkCookie_;

  // Track the server state locally to decide whether to eat keys in
  // OnTestKeyDown
  McBopomofo::IPC::StateUpdatePayload lastState_;

  ITfComposition* pComposition_;

  // Full-width / half-width punctuation toggle shown in the language bar.
  class CLangBarButton* pModeIconButton_;

  // Toggle button that appears in the legacy Windows Language Bar
  // to switch between Chinese and English mode.
  class CLangBarButton* pSwitchLangButton_;

  // Settings menu button that appears in the legacy Windows Language Bar.
  class CLangBarButton* pSettingsButton_;

  bool shiftToggleKeyPending_ = false;

  public:
  void ToggleOpenClose();
  bool IsOpen();
  void RefreshLangBar();
  void applyStateToContext_(ITfContext* context,
                            const McBopomofo::IPC::StateUpdatePayload& state,
                            const char* logPrefix);

 public:
  ITfComposition* GetComposition() const { return pComposition_; }
  void SetComposition(ITfComposition* pComp) { pComposition_ = pComp; }


  ITfUIElementMgr* GetUIElementMgr() const { return pUIElementMgr_; }
  CCandidateListUIElement* GetCandidateUIElement() const {
    return pCandidateUIElement_;
  }
  CReadingInformationUIElement* GetReadingUIElement() const {
    return pReadingUIElement_;
  }

  DWORD GetCandidateUIElementId() const { return dwCandidateUIElementId_; }
  DWORD GetReadingUIElementId() const { return dwReadingUIElementId_; }
  void SetCandidateUIElementId(DWORD id) { dwCandidateUIElementId_ = id; }
  void SetReadingUIElementId(DWORD id) { dwReadingUIElementId_ = id; }

  ITfThreadMgr* GetThreadMgr() const { return ptim_; }
  TfClientId GetClientId() const { return tid_; }

 private:
  ITfUIElementMgr* pUIElementMgr_ = nullptr;
  CCandidateListUIElement* pCandidateUIElement_ = nullptr;
  CReadingInformationUIElement* pReadingUIElement_ = nullptr;
  DWORD dwCandidateUIElementId_ = 0;
  DWORD dwReadingUIElementId_ = 0;
};
