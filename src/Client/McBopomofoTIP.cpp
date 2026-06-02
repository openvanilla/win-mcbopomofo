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

#include "McBopomofoTIP.h"

#include "EditSession.h"
#include "Globals.h"
#include "LangBarButton.h"
#include "NamedPipe.h"
#include "StateEditSession.h"

namespace {

bool ReadDWORDCompartmentValue(ITfThreadMgr* threadMgr, REFGUID compartmentGuid,
                               DWORD* value) {
  if (!threadMgr || !value) {
    return false;
  }

  ITfCompartmentMgr* pCompMgr = nullptr;
  HRESULT hr =
      threadMgr->QueryInterface(IID_ITfCompartmentMgr, (void**)&pCompMgr);
  if (FAILED(hr)) {
    return false;
  }

  ITfCompartment* pComp = nullptr;
  hr = pCompMgr->GetCompartment(compartmentGuid, &pComp);
  pCompMgr->Release();
  if (FAILED(hr)) {
    return false;
  }

  VARIANT var;
  VariantInit(&var);
  hr = pComp->GetValue(&var);
  pComp->Release();
  if (FAILED(hr) || var.vt != VT_I4) {
    VariantClear(&var);
    return false;
  }

  *value = static_cast<DWORD>(var.lVal);
  VariantClear(&var);
  return true;
}

bool AdviseCompartmentSink(ITfThreadMgr* threadMgr, REFGUID compartmentGuid,
                           ITfCompartmentEventSink* sink, DWORD* cookie) {
  if (!threadMgr || !sink || !cookie) {
    return false;
  }

  ITfCompartmentMgr* pCompMgr = nullptr;
  HRESULT hr =
      threadMgr->QueryInterface(IID_ITfCompartmentMgr, (void**)&pCompMgr);
  if (FAILED(hr)) {
    return false;
  }

  ITfCompartment* pCompartment = nullptr;
  hr = pCompMgr->GetCompartment(compartmentGuid, &pCompartment);
  pCompMgr->Release();
  if (FAILED(hr)) {
    return false;
  }

  ITfSource* pSource = nullptr;
  hr = pCompartment->QueryInterface(IID_ITfSource, (void**)&pSource);
  if (SUCCEEDED(hr)) {
    hr = pSource->AdviseSink(IID_ITfCompartmentEventSink, sink, cookie);
    pSource->Release();
  }
  pCompartment->Release();
  return SUCCEEDED(hr);
}

void UnadviseCompartmentSink(ITfThreadMgr* threadMgr, REFGUID compartmentGuid,
                             DWORD* cookie) {
  if (!threadMgr || !cookie || *cookie == TF_INVALID_COOKIE) {
    return;
  }

  ITfCompartmentMgr* pCompMgr = nullptr;
  HRESULT hr =
      threadMgr->QueryInterface(IID_ITfCompartmentMgr, (void**)&pCompMgr);
  if (SUCCEEDED(hr)) {
    ITfCompartment* pCompartment = nullptr;
    hr = pCompMgr->GetCompartment(compartmentGuid, &pCompartment);
    pCompMgr->Release();
    if (SUCCEEDED(hr)) {
      ITfSource* pSource = nullptr;
      hr = pCompartment->QueryInterface(IID_ITfSource, (void**)&pSource);
      if (SUCCEEDED(hr)) {
        pSource->UnadviseSink(*cookie);
        pSource->Release();
      }
      pCompartment->Release();
    }
  }

  *cookie = TF_INVALID_COOKIE;
}

bool IsVirtualKeyDown(int vk) {
  return (GetKeyState(vk) & 0x8000) != 0 ||
         (GetAsyncKeyState(vk) & 0x8000) != 0;
}

bool IsCtrlPressed(const BYTE keyboardState[256]) {
  return (keyboardState[VK_CONTROL] & 0x80) != 0 ||
         (keyboardState[VK_LCONTROL] & 0x80) != 0 ||
         (keyboardState[VK_RCONTROL] & 0x80) != 0 ||
         IsVirtualKeyDown(VK_CONTROL) || IsVirtualKeyDown(VK_LCONTROL) ||
         IsVirtualKeyDown(VK_RCONTROL);
}

bool IsShiftPressed(const BYTE keyboardState[256]) {
  return (keyboardState[VK_SHIFT] & 0x80) != 0 ||
         (keyboardState[VK_LSHIFT] & 0x80) != 0 ||
         (keyboardState[VK_RSHIFT] & 0x80) != 0 || IsVirtualKeyDown(VK_SHIFT) ||
         IsVirtualKeyDown(VK_LSHIFT) || IsVirtualKeyDown(VK_RSHIFT);
}

bool IsServerHandledCtrlShortcutKey(WPARAM wParam) {
  switch (wParam) {
    case VK_OEM_COMMA:
    case VK_OEM_PERIOD:
    case '1':
    case VK_OEM_2:
    case VK_OEM_1:
    case VK_OEM_7:
    case VK_OEM_5:
      return true;
    default:
      return false;
  }
}

bool IsServerHandledShortcutKey(WPARAM wParam, const BYTE keyboardState[256]) {
  const bool ctrlPressed = IsCtrlPressed(keyboardState);
  return ctrlPressed && IsServerHandledCtrlShortcutKey(wParam);
}

bool IsStandaloneModifierKey(WPARAM wParam) {
  switch (wParam) {
    case VK_SHIFT:
    case VK_LSHIFT:
    case VK_RSHIFT:
    case VK_CONTROL:
    case VK_LCONTROL:
    case VK_RCONTROL:
      return true;
    default:
      return false;
  }
}

bool IsShiftKey(WPARAM wParam) {
  switch (wParam) {
    case VK_SHIFT:
    case VK_LSHIFT:
    case VK_RSHIFT:
      return true;
    default:
      return false;
  }
}

bool IsAltPressed(const BYTE keyboardState[256]) {
  return (keyboardState[VK_MENU] & 0x80) != 0 ||
         (keyboardState[VK_LMENU] & 0x80) != 0 ||
         (keyboardState[VK_RMENU] & 0x80) != 0 || IsVirtualKeyDown(VK_MENU) ||
         IsVirtualKeyDown(VK_LMENU) || IsVirtualKeyDown(VK_RMENU);
}

bool IsOnlyShiftKeyEvent(WPARAM wParam, const BYTE keyboardState[256]) {
  return IsShiftKey(wParam) && !IsCtrlPressed(keyboardState) &&
         !IsAltPressed(keyboardState);
}

bool GetFocusedContext(ITfThreadMgr* threadMgr, ITfContext** context) {
  if (!threadMgr || !context) {
    return false;
  }

  *context = nullptr;

  ITfDocumentMgr* pDocMgr = nullptr;
  if (FAILED(threadMgr->GetFocus(&pDocMgr)) || !pDocMgr) {
    return false;
  }

  HRESULT hr = pDocMgr->GetTop(context);
  pDocMgr->Release();
  return SUCCEEDED(hr) && *context != nullptr;
}

HWND GetContextWindow(ITfContext* context) {
  if (context) {
    ITfContextView* view = nullptr;
    if (SUCCEEDED(context->GetActiveView(&view)) && view) {
      HWND hwnd = nullptr;
      if (SUCCEEDED(view->GetWnd(&hwnd)) && hwnd) {
        view->Release();
        return hwnd;
      }
      view->Release();
    }
  }
  return GetFocus();
}

bool IsUsableLayoutRect(const RECT& rc) {
  return rc.bottom > rc.top && (rc.left != 0 || rc.top != 0 || rc.right != 0 ||
                               rc.bottom != 0);
}

bool GetCaretFallbackRect(RECT* rect) {
  if (!rect) {
    return false;
  }

  GUITHREADINFO gti = {0};
  gti.cbSize = sizeof(GUITHREADINFO);
  if (!GetGUIThreadInfo(GetCurrentThreadId(), &gti) || !gti.hwndCaret) {
    return false;
  }

  RECT caretRect = gti.rcCaret;
  POINT topLeft = {caretRect.left, caretRect.top};
  POINT bottomRight = {caretRect.right, caretRect.bottom};
  ClientToScreen(gti.hwndCaret, &topLeft);
  ClientToScreen(gti.hwndCaret, &bottomRight);

  rect->left = topLeft.x;
  rect->top = topLeft.y;
  rect->right = bottomRight.x;
  rect->bottom = bottomRight.y;
  return IsUsableLayoutRect(*rect);
}

class CKeyLayoutEditSession : public CEditSessionBase {
 public:
  CKeyLayoutEditSession(ITfContext* context, RECT* rect, bool* hasRect)
      : CEditSessionBase(context), rect_(rect), hasRect_(hasRect) {}

  STDMETHODIMP DoEditSession(TfEditCookie ec) override {
    if (!rect_ || !hasRect_) {
      return E_INVALIDARG;
    }

    *hasRect_ = false;

    TF_SELECTION selection = {};
    ULONG fetched = 0;
    HRESULT hr = pContext_->GetSelection(ec, TF_DEFAULT_SELECTION, 1,
                                         &selection, &fetched);
    if (SUCCEEDED(hr) && fetched == 1 && selection.range) {
      ITfContextView* view = nullptr;
      if (SUCCEEDED(pContext_->GetActiveView(&view)) && view) {
        RECT rc = {0};
        BOOL clipped = FALSE;
        hr = view->GetTextExt(ec, selection.range, &rc, &clipped);
        if (SUCCEEDED(hr) && IsUsableLayoutRect(rc)) {
          *rect_ = rc;
          *hasRect_ = true;
        }
        view->Release();
      }
      selection.range->Release();
    }

    if (!*hasRect_) {
      *hasRect_ = GetCaretFallbackRect(rect_);
    }
    return S_OK;
  }

 private:
  RECT* rect_;
  bool* hasRect_;
};

bool GetKeyDownLayout(ITfContext* context, TfClientId clientId, RECT* rect) {
  if (!context || !rect) {
    return false;
  }

  bool hasRect = false;
  CKeyLayoutEditSession* session =
      new CKeyLayoutEditSession(context, rect, &hasRect);
  HRESULT editSessionResult = E_FAIL;
  HRESULT hr = context->RequestEditSession(
      clientId, session, TF_ES_SYNC | TF_ES_READ, &editSessionResult);
  session->Release();
  return SUCCEEDED(hr) && SUCCEEDED(editSessionResult) && hasRect;
}

}  // namespace

McBopomofoTIP::McBopomofoTIP()
    : cRef_(1),
      ptim_(nullptr),
      tid_(TF_CLIENTID_NULL),
      dwThreadMgrEventSinkCookie_(TF_INVALID_COOKIE),
      dwThreadFocusSinkCookie_(TF_INVALID_COOKIE),
      dwOpenCloseCompartmentEventSinkCookie_(TF_INVALID_COOKIE),
      pComposition_(nullptr),
      pModeIconButton_(nullptr),
      pSwitchLangButton_(nullptr) {
  DllAddRef();
}

McBopomofoTIP::~McBopomofoTIP() { DllRelease(); }

STDAPI McBopomofoTIP::QueryInterface(REFIID riid, void** ppvObj) {
  if (ppvObj == nullptr) {
    return E_INVALIDARG;
  }

  *ppvObj = nullptr;

  if (IsEqualIID(riid, IID_IUnknown) ||
      IsEqualIID(riid, IID_ITfTextInputProcessor) ||
      IsEqualIID(riid, IID_ITfTextInputProcessorEx)) {
    *ppvObj = static_cast<ITfTextInputProcessorEx*>(this);
  } else if (IsEqualIID(riid, IID_ITfKeyEventSink)) {
    *ppvObj = static_cast<ITfKeyEventSink*>(this);
  } else if (IsEqualIID(riid, IID_ITfCompositionSink)) {
    *ppvObj = static_cast<ITfCompositionSink*>(this);
  } else if (IsEqualIID(riid, IID_ITfDisplayAttributeProvider)) {
    *ppvObj = static_cast<ITfDisplayAttributeProvider*>(this);
  } else if (IsEqualIID(riid, IID_ITfThreadMgrEventSink)) {
    *ppvObj = static_cast<ITfThreadMgrEventSink*>(this);
  } else if (IsEqualIID(riid, IID_ITfThreadFocusSink)) {
    *ppvObj = static_cast<ITfThreadFocusSink*>(this);
  } else if (IsEqualIID(riid, IID_ITfCompartmentEventSink)) {
    *ppvObj = static_cast<ITfCompartmentEventSink*>(this);
  }

  if (*ppvObj) {
    AddRef();
    return S_OK;
  }

  return E_NOINTERFACE;
}

STDAPI_(ULONG) McBopomofoTIP::AddRef() { return InterlockedIncrement(&cRef_); }

STDAPI_(ULONG) McBopomofoTIP::Release() {
  LONG cr = InterlockedDecrement(&cRef_);
  if (cr == 0) {
    delete this;
  }
  return cr;
}

BOOL McBopomofoTIP::initKeyEventSink_() {
  ITfKeystrokeMgr* pKeystrokeMgr = nullptr;
  HRESULT hr =
      ptim_->QueryInterface(IID_ITfKeystrokeMgr, (void**)&pKeystrokeMgr);
  if (FAILED(hr)) {
    return FALSE;
  }

  hr = pKeystrokeMgr->AdviseKeyEventSink(
      tid_, static_cast<ITfKeyEventSink*>(this), TRUE);
  pKeystrokeMgr->Release();
  return SUCCEEDED(hr);
}

void McBopomofoTIP::uninitKeyEventSink_() {
  if (!ptim_) {
    return;
  }

  ITfKeystrokeMgr* pKeystrokeMgr = nullptr;
  HRESULT hr =
      ptim_->QueryInterface(IID_ITfKeystrokeMgr, (void**)&pKeystrokeMgr);
  if (SUCCEEDED(hr)) {
    pKeystrokeMgr->UnadviseKeyEventSink(tid_);
    pKeystrokeMgr->Release();
  }
}

BOOL McBopomofoTIP::initCompartmentEventSink_() {
  return AdviseCompartmentSink(ptim_, GUID_COMPARTMENT_KEYBOARD_OPENCLOSE,
                               static_cast<ITfCompartmentEventSink*>(this),
                               &dwOpenCloseCompartmentEventSinkCookie_);
}

void McBopomofoTIP::uninitCompartmentEventSink_() {
  UnadviseCompartmentSink(ptim_, GUID_COMPARTMENT_KEYBOARD_OPENCLOSE,
                          &dwOpenCloseCompartmentEventSinkCookie_);
}

BOOL McBopomofoTIP::initThreadMgrEventSink_() {
  ITfSource* pSource = nullptr;
  HRESULT hr = ptim_->QueryInterface(IID_ITfSource, (void**)&pSource);
  if (FAILED(hr)) {
    return FALSE;
  }

  hr = pSource->AdviseSink(IID_ITfThreadMgrEventSink,
                           static_cast<ITfThreadMgrEventSink*>(this),
                           &dwThreadMgrEventSinkCookie_);
  pSource->Release();
  return SUCCEEDED(hr);
}

void McBopomofoTIP::uninitThreadMgrEventSink_() {
  if (dwThreadMgrEventSinkCookie_ == TF_INVALID_COOKIE || !ptim_) {
    return;
  }

  ITfSource* pSource = nullptr;
  if (SUCCEEDED(ptim_->QueryInterface(IID_ITfSource, (void**)&pSource))) {
    pSource->UnadviseSink(dwThreadMgrEventSinkCookie_);
    pSource->Release();
  }
  dwThreadMgrEventSinkCookie_ = TF_INVALID_COOKIE;
}

BOOL McBopomofoTIP::initThreadFocusSink_() {
  ITfSource* pSource = nullptr;
  HRESULT hr = ptim_->QueryInterface(IID_ITfSource, (void**)&pSource);
  if (FAILED(hr)) {
    return FALSE;
  }

  hr = pSource->AdviseSink(IID_ITfThreadFocusSink,
                           static_cast<ITfThreadFocusSink*>(this),
                           &dwThreadFocusSinkCookie_);
  pSource->Release();
  return SUCCEEDED(hr);
}

void McBopomofoTIP::uninitThreadFocusSink_() {
  if (dwThreadFocusSinkCookie_ == TF_INVALID_COOKIE || !ptim_) {
    return;
  }

  ITfSource* pSource = nullptr;
  if (SUCCEEDED(ptim_->QueryInterface(IID_ITfSource, (void**)&pSource))) {
    pSource->UnadviseSink(dwThreadFocusSinkCookie_);
    pSource->Release();
  }
  dwThreadFocusSinkCookie_ = TF_INVALID_COOKIE;
}

STDAPI McBopomofoTIP::Activate(ITfThreadMgr* ptim, TfClientId tid) {
  return ActivateEx(ptim, tid, 0);
}

STDAPI McBopomofoTIP::ActivateEx(ITfThreadMgr* ptim, TfClientId tid,
                                 DWORD dwFlags) {
  UNREFERENCED_PARAMETER(dwFlags);
  // LogMessage("McBopomofoTIP::ActivateEx called with flags: %u", dwFlags);

  if (ptim == nullptr) {
    return E_INVALIDARG;
  }

  ptim_ = ptim;
  ptim_->AddRef();
  tid_ = tid;

  if (!initKeyEventSink_()) {
    // LogMessage("Failed to init KeyEventSink");
    return E_FAIL;
  }

  if (!initCompartmentEventSink_()) {
    // LogMessage("Failed to init CompartmentEventSink");
    return E_FAIL;
  }
  if (!initThreadMgrEventSink_()) {
    // LogMessage("Failed to init ThreadMgrEventSink");
    return E_FAIL;
  }
  if (!initThreadFocusSink_()) {
    // LogMessage("Failed to init ThreadFocusSink");
    return E_FAIL;
  }

  // Register LangBar button - must be created BEFORE setting compartment value
  // because SetValue triggers OnChange callback synchronously
  ITfLangBarItemMgr* pLangBarItemMgr = nullptr;
  if (SUCCEEDED(ptim_->QueryInterface(IID_ITfLangBarItemMgr,
                                      (void**)&pLangBarItemMgr))) {
    pModeIconButton_ = new CLangBarButton(this, GUID_LBI_INPUTMODE,
                                          CLangBarButton::Kind::ModeIcon);
    pSwitchLangButton_ = new CLangBarButton(
        this, GUID_LBI_SWITCH_LANG, CLangBarButton::Kind::SwitchLanguageToggle);
    pSettingsButton_ = new CLangBarButton(this, GUID_LBI_SETTINGS,
                                          CLangBarButton::Kind::SettingsMenu);
    pLangBarItemMgr->AddItem(pModeIconButton_);
    pLangBarItemMgr->AddItem(pSwitchLangButton_);
    pLangBarItemMgr->AddItem(pSettingsButton_);
    pLangBarItemMgr->Release();
  }

  ITfCompartmentMgr* pCompMgr = nullptr;
  if (SUCCEEDED(
          ptim_->QueryInterface(IID_ITfCompartmentMgr, (void**)&pCompMgr))) {
    ITfCompartment* pComp = nullptr;
    if (SUCCEEDED(pCompMgr->GetCompartment(GUID_COMPARTMENT_KEYBOARD_OPENCLOSE,
                                           &pComp))) {
      VARIANT var;
      var.vt = VT_I4;
      var.lVal = 1;
      pComp->SetValue(tid_, &var);
      pComp->Release();
      LogMessage("Keyboard compartment set to OPEN");
    }
    pCompMgr->Release();
  }

  if (SUCCEEDED(ptim_->QueryInterface(IID_ITfUIElementMgr,
                                      (void**)&pUIElementMgr_))) {
    pCandidateUIElement_ = new CCandidateListUIElement(this);
    pReadingUIElement_ = new CReadingInformationUIElement(this);
    // LogMessage("ITfUIElementMgr successfully acquired.");
  } else {
    // LogMessage("Failed to acquire ITfUIElementMgr.");
  }

  // LogMessage("McBopomofoTIP::ActivateEx succeeded");
  return S_OK;
}

STDAPI McBopomofoTIP::Deactivate() {
  // LogMessage("McBopomofoTIP::Deactivate called");

  if (pUIElementMgr_) {
    if (pCandidateUIElement_) {
      if (dwCandidateUIElementId_ != 0) {
        pUIElementMgr_->EndUIElement(dwCandidateUIElementId_);
        dwCandidateUIElementId_ = 0;
      }
      pCandidateUIElement_->ClearTip();
      pCandidateUIElement_->Release();
      pCandidateUIElement_ = nullptr;
    }
    if (pReadingUIElement_) {
      if (dwReadingUIElementId_ != 0) {
        pUIElementMgr_->EndUIElement(dwReadingUIElementId_);
        dwReadingUIElementId_ = 0;
      }
      pReadingUIElement_->ClearTip();
      pReadingUIElement_->Release();
      pReadingUIElement_ = nullptr;
    }
    pUIElementMgr_->Release();
    pUIElementMgr_ = nullptr;
  }

  if (pModeIconButton_ || pSwitchLangButton_ || pSettingsButton_) {
    ITfLangBarItemMgr* pLangBarItemMgr = nullptr;
    if (SUCCEEDED(ptim_->QueryInterface(IID_ITfLangBarItemMgr,
                                        (void**)&pLangBarItemMgr))) {
      if (pModeIconButton_) {
        pLangBarItemMgr->RemoveItem(pModeIconButton_);
      }
      if (pSwitchLangButton_) {
        pLangBarItemMgr->RemoveItem(pSwitchLangButton_);
      }
      if (pSettingsButton_) {
        pLangBarItemMgr->RemoveItem(pSettingsButton_);
      }
      pLangBarItemMgr->Release();
    }
    if (pModeIconButton_) {
      pModeIconButton_->Release();
      pModeIconButton_ = nullptr;
    }
    if (pSwitchLangButton_) {
      pSwitchLangButton_->Release();
      pSwitchLangButton_ = nullptr;
    }
    if (pSettingsButton_) {
      pSettingsButton_->Release();
      pSettingsButton_ = nullptr;
    }
  }

  uninitCompartmentEventSink_();
  uninitThreadFocusSink_();
  uninitThreadMgrEventSink_();
  uninitKeyEventSink_();

  if (pComposition_) {
    pComposition_->Release();
    pComposition_ = nullptr;
  }

  if (ptim_) {
    ptim_->Release();
    ptim_ = nullptr;
  }
  tid_ = TF_CLIENTID_NULL;

  return S_OK;
}

STDAPI McBopomofoTIP::OnSetFocus(BOOL fForeground) {
  shiftToggleKeyPending_ = false;
  if (!fForeground) {
    resetServerState_();
  }
  return S_OK;
}

STDAPI McBopomofoTIP::OnTestKeyDown(ITfContext* pic, WPARAM wParam,
                                    LPARAM lParam, BOOL* pfEaten) {
  UNREFERENCED_PARAMETER(pic);
  if (pfEaten == nullptr) {
    return E_INVALIDARG;
  }

  BYTE keyboardState[256];
  GetKeyboardState(keyboardState);

  if (!IsOpen()) {
    *pfEaten = FALSE;
    return S_OK;
  }

  if (IsStandaloneModifierKey(wParam)) {
    *pfEaten = FALSE;
    return S_OK;
  }

  // If we have active composing buffer or candidates, let server handle all
  // keys
  if (!lastState_.composingBuffer.empty() || !lastState_.candidates.empty()) {
    *pfEaten = TRUE;
    return S_OK;
  }

  if (IsServerHandledShortcutKey(wParam, keyboardState)) {
    *pfEaten = TRUE;
    return S_OK;
  }

  // No active state: only eat printable characters, let server decide if it
  // wants to start composing
  WCHAR chars[2] = {0};
  if (ToUnicode((UINT)wParam, (lParam >> 16) & 0xFF, keyboardState, chars, 2,
                0) == 1) {
    // Only eat if it's a printable ASCII character (let server handle Bopomofo
    // keys)
    *pfEaten = (chars[0] >= 32 && chars[0] <= 126);
  } else {
    *pfEaten = FALSE;
  }

  return S_OK;
}

//
STDAPI McBopomofoTIP::OnKeyDown(ITfContext* pic, WPARAM wParam, LPARAM lParam,
                                BOOL* pfEaten) {
  if (pfEaten == nullptr) {
    return E_INVALIDARG;
  }

  BYTE keyboardState[256];
  GetKeyboardState(keyboardState);

  if (handleStandaloneShiftKeyDown_(wParam, keyboardState)) {
    *pfEaten = FALSE;
    return S_OK;
  }

  if (!IsOpen()) {
    *pfEaten = FALSE;
    return S_OK;
  }

  BOOL eaten = FALSE;
  OnTestKeyDown(pic, wParam, lParam, &eaten);

  if (!eaten) {
    *pfEaten = FALSE;
    return S_OK;
  }

  McBopomofo::IPC::KeyEventPayload req;
  req.vk = (unsigned int)wParam;
  req.shift = IsShiftPressed(keyboardState);
  req.ctrl = IsCtrlPressed(keyboardState);
  RECT keyLayout = {0};
  if (GetKeyDownLayout(pic, tid_, &keyLayout)) {
    req.hasLayout = true;
    req.ownerHwnd =
        static_cast<uint64_t>(reinterpret_cast<uintptr_t>(GetContextWindow(pic)));
    req.anchorLeft = static_cast<int>(keyLayout.left);
    req.anchorTop = static_cast<int>(keyLayout.top);
    req.anchorRight = static_cast<int>(keyLayout.right);
    req.anchorBottom = static_cast<int>(keyLayout.bottom);
  }

  GetKeyboardState(keyboardState);
  WCHAR chars[2] = {0};
  if (ToUnicode((UINT)wParam, (lParam >> 16) & 0xFF, keyboardState, chars, 2,
                0) == 1) {
    req.ascii = (chars[0] >= 32 && chars[0] <= 126) ? chars[0] : 0;
  } else {
    req.ascii = 0;
  }

  McBopomofo::IPC::NamedPipeClient pipe(McBopomofo::IPC::PIPE_NAME);
  std::string response;

  std::string payload = McBopomofo::IPC::SerializeKeyEvent(req);
  // LogMessage("Sending IPC request: %s", payload.c_str());

  if (pipe.Call(payload, response)) {
    // LogMessage("Received IPC response: %s", response.c_str());
    if (McBopomofo::IPC::DeserializeStateUpdate(response, lastState_)) {
      const bool hasCommit = !lastState_.commitString.empty();
      *pfEaten = (lastState_.consumed || hasCommit) ? TRUE : FALSE;
      // LogMessage(
      //     "State deserialized. Consumed: %d, CommitStr: '%s', CompStr: '%s'",
      //     lastState_.consumed, lastState_.commitString.c_str(),
      //     lastState_.composingBuffer.c_str());

      if (lastState_.consumed || hasCommit) {
        applyStateToContext_(pic, lastState_, "");
      }
    } else {
      // LogMessage("Failed to deserialize state update");
      *pfEaten = FALSE;
    }
  } else {
    // LogMessage("IPC Call failed");
    *pfEaten = FALSE;
  }

  return S_OK;
}

STDAPI McBopomofoTIP::OnTestKeyUp(ITfContext* pic, WPARAM wParam, LPARAM lParam,
                                  BOOL* pfEaten) {
  UNREFERENCED_PARAMETER(pic);
  UNREFERENCED_PARAMETER(wParam);
  UNREFERENCED_PARAMETER(lParam);
  if (pfEaten == nullptr) {
    return E_INVALIDARG;
  }
  *pfEaten = FALSE;
  return S_OK;
}

STDAPI McBopomofoTIP::OnKeyUp(ITfContext* pic, WPARAM wParam, LPARAM lParam,
                              BOOL* pfEaten) {
  UNREFERENCED_PARAMETER(pic);
  UNREFERENCED_PARAMETER(lParam);
  if (pfEaten == nullptr) {
    return E_INVALIDARG;
  }

  BYTE keyboardState[256];
  GetKeyboardState(keyboardState);
  if (handleStandaloneShiftKeyUp_(wParam, keyboardState)) {
    *pfEaten = TRUE;
    return S_OK;
  }

  *pfEaten = FALSE;
  return S_OK;
}

STDAPI McBopomofoTIP::OnPreservedKey(ITfContext* pic, REFGUID rguid,
                                     BOOL* pfEaten) {
  UNREFERENCED_PARAMETER(pic);
  UNREFERENCED_PARAMETER(rguid);
  if (pfEaten == nullptr) {
    return E_INVALIDARG;
  }

  *pfEaten = FALSE;
  return S_OK;
}

STDAPI McBopomofoTIP::OnChange(REFGUID rguid) {
  if (!IsEqualGUID(rguid, GUID_COMPARTMENT_KEYBOARD_OPENCLOSE)) {
    return S_OK;
  }

  const bool isOpen = IsOpen();
  // LogMessage("GUID_COMPARTMENT_KEYBOARD_OPENCLOSE changed: %s",
  //            isOpen ? "OPEN" : "CLOSED");
  if (!isOpen) {
    resetServerState_();
  }

  RefreshLangBar();
  return S_OK;
}

STDAPI McBopomofoTIP::OnCompositionTerminated(TfEditCookie ecWrite,
                                              ITfComposition* pComposition) {
  UNREFERENCED_PARAMETER(ecWrite);
  if (pComposition_ == pComposition) {
    pComposition_->Release();
    pComposition_ = nullptr;
  }
  return S_OK;
}

#include "DisplayAttributeInfo.h"

STDAPI McBopomofoTIP::EnumDisplayAttributeInfo(
    IEnumTfDisplayAttributeInfo** ppEnum) {
  if (ppEnum == nullptr) return E_INVALIDARG;
  *ppEnum = new CEnumDisplayAttributeInfo();
  return (*ppEnum != nullptr) ? S_OK : E_OUTOFMEMORY;
}

STDAPI McBopomofoTIP::GetDisplayAttributeInfo(
    REFGUID guidInfo, ITfDisplayAttributeInfo** ppInfo) {
  if (ppInfo == nullptr) return E_INVALIDARG;
  *ppInfo = nullptr;

  if (IsEqualGUID(guidInfo, c_guidDisplayAttributeInput)) {
    TF_DISPLAYATTRIBUTE da;
    ZeroMemory(&da, sizeof(da));
    da.lsStyle = TF_LS_DOT;
    da.crLine.type = TF_CT_SYSCOLOR;
    da.crLine.nIndex = COLOR_WINDOWTEXT;
    *ppInfo = new CDisplayAttributeInfo(c_guidDisplayAttributeInput, da,
                                        L"Win-McBopomofo Input");
  } else if (IsEqualGUID(guidInfo, c_guidDisplayAttributeMarked)) {
    TF_DISPLAYATTRIBUTE da;
    ZeroMemory(&da, sizeof(da));
    da.lsStyle = TF_LS_SOLID;
    da.crText.type = TF_CT_SYSCOLOR;
    da.crText.nIndex = COLOR_HIGHLIGHTTEXT;
    da.crBk.type = TF_CT_SYSCOLOR;
    da.crBk.nIndex = COLOR_HIGHLIGHT;
    *ppInfo = new CDisplayAttributeInfo(c_guidDisplayAttributeMarked, da,
                                        L"Win-McBopomofo Marked");
  }

  return (*ppInfo != nullptr) ? S_OK : E_INVALIDARG;
}

STDAPI McBopomofoTIP::OnInitDocumentMgr(ITfDocumentMgr* pDocMgr) {
  UNREFERENCED_PARAMETER(pDocMgr);
  return S_OK;
}

STDAPI McBopomofoTIP::OnUninitDocumentMgr(ITfDocumentMgr* pDocMgr) {
  UNREFERENCED_PARAMETER(pDocMgr);
  return S_OK;
}

STDAPI McBopomofoTIP::OnSetFocus(ITfDocumentMgr* pDocMgrFocus,
                                 ITfDocumentMgr* pDocMgrPrevFocus) {
  UNREFERENCED_PARAMETER(pDocMgrPrevFocus);
  shiftToggleKeyPending_ = false;
  if (pDocMgrFocus == nullptr) {
    resetServerState_();
  }
  return S_OK;
}

STDAPI McBopomofoTIP::OnPushContext(ITfContext* pic) {
  UNREFERENCED_PARAMETER(pic);
  return S_OK;
}

STDAPI McBopomofoTIP::OnPopContext(ITfContext* pic) {
  UNREFERENCED_PARAMETER(pic);
  return S_OK;
}

STDAPI McBopomofoTIP::OnSetThreadFocus() {
  shiftToggleKeyPending_ = false;
  return S_OK;
}

STDAPI McBopomofoTIP::OnKillThreadFocus() {
  shiftToggleKeyPending_ = false;
  hideServerAuxiliaryUI_();
  resetServerState_();
  return S_OK;
}

bool McBopomofoTIP::IsOpen() {
  DWORD value = 1;
  if (ReadDWORDCompartmentValue(ptim_, GUID_COMPARTMENT_KEYBOARD_OPENCLOSE,
                                &value)) {
    return value != 0;
  }
  return true;
}

bool McBopomofoTIP::isDirectCommitWithoutComposition_(
    const McBopomofo::IPC::StateUpdatePayload& state) const {
  return !state.commitString.empty() && state.composingBuffer.empty() &&
         pComposition_ == nullptr;
}

void McBopomofoTIP::hideAuxiliaryWindowsForDirectCommit_(
    const McBopomofo::IPC::StateUpdatePayload& state) {
  if (isDirectCommitWithoutComposition_(state)) {
    hideServerAuxiliaryUI_();
  }
}

void McBopomofoTIP::hideServerAuxiliaryUI_() {
  RECT empty = {};
  UpdateServerUILayout(nullptr, empty, false, false);
}

void McBopomofoTIP::UpdateServerUILayout(HWND ownerHwnd, const RECT& anchor,
                                         bool showCandidateWindow,
                                         bool showTooltipWindow) {
  McBopomofo::IPC::ClientUILayoutPayload payload;
  payload.showCandidateWindow = showCandidateWindow;
  payload.showTooltipWindow = showTooltipWindow;
  payload.ownerHwnd =
      static_cast<uint64_t>(reinterpret_cast<uintptr_t>(ownerHwnd));
  payload.anchorLeft = static_cast<int>(anchor.left);
  payload.anchorTop = static_cast<int>(anchor.top);
  payload.anchorRight = static_cast<int>(anchor.right);
  payload.anchorBottom = static_cast<int>(anchor.bottom);

  McBopomofo::IPC::NamedPipeClient pipe(McBopomofo::IPC::PIPE_NAME);
  std::string response;
  pipe.Call(McBopomofo::IPC::SerializeClientUILayout(payload), response);
}

void McBopomofoTIP::applyStateToContext_(
    ITfContext* context, const McBopomofo::IPC::StateUpdatePayload& state,
    const char* logPrefix) {
  UNREFERENCED_PARAMETER(logPrefix);
  if (!context) {
    // LogMessage("%sRequestEditSession skipped: null context", logPrefix);
    return;
  }

  hideAuxiliaryWindowsForDirectCommit_(state);

  CStateEditSession* pEditSession = new CStateEditSession(context, this, state);
  HRESULT hr = E_FAIL;
  context->RequestEditSession(tid_, pEditSession, TF_ES_SYNC | TF_ES_READWRITE,
                              &hr);
  // LogMessage("%sRequestEditSession returned: 0x%08X", logPrefix, hr);
  pEditSession->Release();
}

void McBopomofoTIP::resetServerState_() {
  // LogMessage("Sending RESET command to server");
  McBopomofo::IPC::NamedPipeClient pipe(McBopomofo::IPC::PIPE_NAME);
  std::string response;
  if (pipe.Call(McBopomofo::IPC::SerializeReset(), response)) {
    // LogMessage("Reset response received");
    McBopomofo::IPC::StateUpdatePayload state;
    if (McBopomofo::IPC::DeserializeStateUpdate(response, state)) {
      lastState_ = state;
      // LogMessage("Reset state: CommitStr='%s', CompStr='%s'",
      //            state.commitString.c_str(), state.composingBuffer.c_str());

      ITfContext* pContext = nullptr;
      if (GetFocusedContext(ptim_, &pContext)) {
        applyStateToContext_(pContext, state, "Reset ");
        pContext->Release();
      } else {
        // LogMessage("Reset could not acquire focused context for edit
        // session");
      }
    }
  } else {
    // LogMessage("Failed to send RESET command");
  }

  hideServerAuxiliaryUI_();
}

void McBopomofoTIP::RefreshLangBar() {
  if (pModeIconButton_) {
    // LogMessage("Refreshing mode icon button");
    pModeIconButton_->Update();
  }
  if (pSwitchLangButton_) {
    // LogMessage("Refreshing switch lang button");
    pSwitchLangButton_->Update();
  }
  if (pSettingsButton_) {
    // LogMessage("Refreshing settings button");
    pSettingsButton_->Update();
  }
}

bool McBopomofoTIP::shouldToggleOpenCloseWithShift_() const {
  McBopomofo::IPC::NamedPipeClient pipe(McBopomofo::IPC::PIPE_NAME);
  std::string response;
  if (!pipe.Call(McBopomofo::IPC::SerializeGetSettings(), response)) {
    // LogMessage("GET_SETTINGS IPC Call failed, fallback to enabled");
    return true;
  }

  McBopomofo::IPC::ClientSettingsPayload payload;
  if (!McBopomofo::IPC::DeserializeClientSettings(response, payload)) {
    // LogMessage("GET_SETTINGS response deserialize failed, fallback to
    // enabled");
    return true;
  }

  return payload.shiftToggleOpenClose;
}

bool McBopomofoTIP::handleStandaloneShiftKeyDown_(
    WPARAM wParam, const BYTE keyboardState[256]) {
  if (IsOnlyShiftKeyEvent(wParam, keyboardState)) {
    shiftToggleKeyPending_ = true;
    return true;
  }

  shiftToggleKeyPending_ = false;
  return false;
}

bool McBopomofoTIP::handleStandaloneShiftKeyUp_(WPARAM wParam,
                                                const BYTE keyboardState[256]) {
  const bool shouldToggle = IsOnlyShiftKeyEvent(wParam, keyboardState) &&
                            shiftToggleKeyPending_ &&
                            shouldToggleOpenCloseWithShift_();
  shiftToggleKeyPending_ = false;
  if (!shouldToggle) {
    return false;
  }

  ToggleOpenClose();
  return true;
}

void McBopomofoTIP::ToggleOpenClose() {
  if (ptim_) {
    bool currentOpen = IsOpen();
    // LogMessage("ToggleOpenClose: current state = %s, toggling to %s",
    //            currentOpen ? "OPEN" : "CLOSED",
    //            currentOpen ? "CLOSED" : "OPEN");

    ITfCompartmentMgr* pCompMgr = nullptr;
    if (SUCCEEDED(
            ptim_->QueryInterface(IID_ITfCompartmentMgr, (void**)&pCompMgr))) {
      ITfCompartment* pComp = nullptr;
      if (SUCCEEDED(pCompMgr->GetCompartment(
              GUID_COMPARTMENT_KEYBOARD_OPENCLOSE, &pComp))) {
        VARIANT var;
        var.vt = VT_I4;
        var.lVal = currentOpen ? 0 : 1;
        pComp->SetValue(tid_, &var);
        pComp->Release();

        // LogMessage("Compartment value set to: %d", var.lVal);
        RefreshLangBar();
      }
      pCompMgr->Release();
    }
  }
}
