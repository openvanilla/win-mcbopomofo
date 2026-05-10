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

}  // namespace

McBopomofoTIP::McBopomofoTIP()
    : _cRef(1),
      _ptim(nullptr),
      _tid(TF_CLIENTID_NULL),
      _dwThreadMgrEventSinkCookie(TF_INVALID_COOKIE),
      _dwThreadFocusSinkCookie(TF_INVALID_COOKIE),
      _dwOpenCloseCompartmentEventSinkCookie(TF_INVALID_COOKIE),
      _pComposition(nullptr),
      _pModeIconButton(nullptr),
      _pSwitchLangButton(nullptr) {
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

STDAPI_(ULONG) McBopomofoTIP::AddRef() { return InterlockedIncrement(&_cRef); }

STDAPI_(ULONG) McBopomofoTIP::Release() {
  LONG cr = InterlockedDecrement(&_cRef);
  if (cr == 0) {
    delete this;
  }
  return cr;
}

BOOL McBopomofoTIP::_InitKeyEventSink() {
  ITfKeystrokeMgr* pKeystrokeMgr = nullptr;
  HRESULT hr =
      _ptim->QueryInterface(IID_ITfKeystrokeMgr, (void**)&pKeystrokeMgr);
  if (FAILED(hr)) {
    return FALSE;
  }

  hr = pKeystrokeMgr->AdviseKeyEventSink(
      _tid, static_cast<ITfKeyEventSink*>(this), TRUE);
  pKeystrokeMgr->Release();
  return SUCCEEDED(hr);
}

void McBopomofoTIP::_UninitKeyEventSink() {
  if (!_ptim) {
    return;
  }

  ITfKeystrokeMgr* pKeystrokeMgr = nullptr;
  HRESULT hr =
      _ptim->QueryInterface(IID_ITfKeystrokeMgr, (void**)&pKeystrokeMgr);
  if (SUCCEEDED(hr)) {
    pKeystrokeMgr->UnadviseKeyEventSink(_tid);
    pKeystrokeMgr->Release();
  }
}

BOOL McBopomofoTIP::_InitCompartmentEventSink() {
  return AdviseCompartmentSink(_ptim, GUID_COMPARTMENT_KEYBOARD_OPENCLOSE,
                               static_cast<ITfCompartmentEventSink*>(this),
                               &_dwOpenCloseCompartmentEventSinkCookie);
}

void McBopomofoTIP::_UninitCompartmentEventSink() {
  UnadviseCompartmentSink(_ptim, GUID_COMPARTMENT_KEYBOARD_OPENCLOSE,
                          &_dwOpenCloseCompartmentEventSinkCookie);
}

BOOL McBopomofoTIP::_InitThreadMgrEventSink() {
  ITfSource* pSource = nullptr;
  HRESULT hr = _ptim->QueryInterface(IID_ITfSource, (void**)&pSource);
  if (FAILED(hr)) {
    return FALSE;
  }

  hr = pSource->AdviseSink(IID_ITfThreadMgrEventSink,
                           static_cast<ITfThreadMgrEventSink*>(this),
                           &_dwThreadMgrEventSinkCookie);
  pSource->Release();
  return SUCCEEDED(hr);
}

void McBopomofoTIP::_UninitThreadMgrEventSink() {
  if (_dwThreadMgrEventSinkCookie == TF_INVALID_COOKIE || !_ptim) {
    return;
  }

  ITfSource* pSource = nullptr;
  if (SUCCEEDED(_ptim->QueryInterface(IID_ITfSource, (void**)&pSource))) {
    pSource->UnadviseSink(_dwThreadMgrEventSinkCookie);
    pSource->Release();
  }
  _dwThreadMgrEventSinkCookie = TF_INVALID_COOKIE;
}

BOOL McBopomofoTIP::_InitThreadFocusSink() {
  ITfSource* pSource = nullptr;
  HRESULT hr = _ptim->QueryInterface(IID_ITfSource, (void**)&pSource);
  if (FAILED(hr)) {
    return FALSE;
  }

  hr = pSource->AdviseSink(IID_ITfThreadFocusSink,
                           static_cast<ITfThreadFocusSink*>(this),
                           &_dwThreadFocusSinkCookie);
  pSource->Release();
  return SUCCEEDED(hr);
}

void McBopomofoTIP::_UninitThreadFocusSink() {
  if (_dwThreadFocusSinkCookie == TF_INVALID_COOKIE || !_ptim) {
    return;
  }

  ITfSource* pSource = nullptr;
  if (SUCCEEDED(_ptim->QueryInterface(IID_ITfSource, (void**)&pSource))) {
    pSource->UnadviseSink(_dwThreadFocusSinkCookie);
    pSource->Release();
  }
  _dwThreadFocusSinkCookie = TF_INVALID_COOKIE;
}

STDAPI McBopomofoTIP::Activate(ITfThreadMgr* ptim, TfClientId tid) {
  return ActivateEx(ptim, tid, 0);
}

STDAPI McBopomofoTIP::ActivateEx(ITfThreadMgr* ptim, TfClientId tid,
                                 DWORD dwFlags) {
  LogMessage("McBopomofoTIP::ActivateEx called with flags: %u", dwFlags);

  if (ptim == nullptr) {
    return E_INVALIDARG;
  }

  _ptim = ptim;
  _ptim->AddRef();
  _tid = tid;

  if (!_InitKeyEventSink()) {
    LogMessage("Failed to init KeyEventSink");
    return E_FAIL;
  }

  if (!_InitCompartmentEventSink()) {
    LogMessage("Failed to init CompartmentEventSink");
    return E_FAIL;
  }
  if (!_InitThreadMgrEventSink()) {
    LogMessage("Failed to init ThreadMgrEventSink");
    return E_FAIL;
  }
  if (!_InitThreadFocusSink()) {
    LogMessage("Failed to init ThreadFocusSink");
    return E_FAIL;
  }

  ITfCompartmentMgr* pCompMgr = nullptr;
  if (SUCCEEDED(
          _ptim->QueryInterface(IID_ITfCompartmentMgr, (void**)&pCompMgr))) {
    ITfCompartment* pComp = nullptr;
    if (SUCCEEDED(pCompMgr->GetCompartment(GUID_COMPARTMENT_KEYBOARD_OPENCLOSE,
                                           &pComp))) {
      VARIANT var;
      var.vt = VT_I4;
      var.lVal = 1;
      pComp->SetValue(_tid, &var);
      pComp->Release();
      LogMessage("Keyboard compartment set to OPEN");
    }
    pCompMgr->Release();
  }

  extern HINSTANCE g_hInst;
  _candidateWindow.Create(g_hInst);
  _tooltipWindow.Create(g_hInst);

  // Register LangBar button
  ITfLangBarItemMgr* pLangBarItemMgr = nullptr;
  if (SUCCEEDED(_ptim->QueryInterface(IID_ITfLangBarItemMgr,
                                      (void**)&pLangBarItemMgr))) {
    _pModeIconButton = new CLangBarButton(this, GUID_LBI_INPUTMODE,
                                          CLangBarButton::Kind::ModeIcon);
    _pSwitchLangButton = new CLangBarButton(
        this, GUID_LBI_SWITCH_LANG, CLangBarButton::Kind::SwitchLanguageMenu);
    pLangBarItemMgr->AddItem(_pModeIconButton);
    pLangBarItemMgr->AddItem(_pSwitchLangButton);
    pLangBarItemMgr->Release();
  }

  LogMessage("McBopomofoTIP::ActivateEx succeeded");
  return S_OK;
}

STDAPI McBopomofoTIP::Deactivate() {
  LogMessage("McBopomofoTIP::Deactivate called");

  if (_pModeIconButton || _pSwitchLangButton) {
    ITfLangBarItemMgr* pLangBarItemMgr = nullptr;
    if (SUCCEEDED(_ptim->QueryInterface(IID_ITfLangBarItemMgr,
                                        (void**)&pLangBarItemMgr))) {
      if (_pModeIconButton) {
        pLangBarItemMgr->RemoveItem(_pModeIconButton);
      }
      if (_pSwitchLangButton) {
        pLangBarItemMgr->RemoveItem(_pSwitchLangButton);
      }
      pLangBarItemMgr->Release();
    }
    if (_pModeIconButton) {
      _pModeIconButton->Release();
      _pModeIconButton = nullptr;
    }
    if (_pSwitchLangButton) {
      _pSwitchLangButton->Release();
      _pSwitchLangButton = nullptr;
    }
  }

  _candidateWindow.Destroy();

  _UninitCompartmentEventSink();
  _UninitThreadFocusSink();
  _UninitThreadMgrEventSink();
  _UninitKeyEventSink();

  if (_pComposition) {
    _pComposition->Release();
    _pComposition = nullptr;
  }

  if (_ptim) {
    _ptim->Release();
    _ptim = nullptr;
  }
  _tid = TF_CLIENTID_NULL;

  return S_OK;
}

STDAPI McBopomofoTIP::OnSetFocus(BOOL fForeground) {
  UNREFERENCED_PARAMETER(fForeground);
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
  if (!_lastState.composingBuffer.empty() || !_lastState.candidates.empty()) {
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
  LogMessage("Sending IPC request: %s", payload.c_str());

  if (pipe.Call(payload, response)) {
    LogMessage("Received IPC response: %s", response.c_str());
    if (McBopomofo::IPC::DeserializeStateUpdate(response, _lastState)) {
      *pfEaten = _lastState.consumed ? TRUE : FALSE;
      LogMessage(
          "State deserialized. Consumed: %d, CommitStr: '%s', CompStr: '%s'",
          _lastState.consumed, _lastState.commitString.c_str(),
          _lastState.composingBuffer.c_str());

      if (_lastState.consumed) {
        ApplyStateToContext(pic, _lastState, "");
      }
    } else {
      LogMessage("Failed to deserialize state update");
      *pfEaten = FALSE;
    }
  } else {
    LogMessage("IPC Call failed");
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
  UNREFERENCED_PARAMETER(wParam);
  UNREFERENCED_PARAMETER(lParam);
  if (pfEaten == nullptr) {
    return E_INVALIDARG;
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
  LogMessage("GUID_COMPARTMENT_KEYBOARD_OPENCLOSE changed: %s",
             isOpen ? "OPEN" : "CLOSED");
  if (!isOpen) {
    ResetServerState();
  }

  RefreshLangBar();
  return S_OK;
}

STDAPI McBopomofoTIP::OnCompositionTerminated(TfEditCookie ecWrite,
                                              ITfComposition* pComposition) {
  UNREFERENCED_PARAMETER(ecWrite);
  if (_pComposition == pComposition) {
    _pComposition->Release();
    _pComposition = nullptr;
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
  UNREFERENCED_PARAMETER(pDocMgrFocus);
  UNREFERENCED_PARAMETER(pDocMgrPrevFocus);
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

STDAPI McBopomofoTIP::OnSetThreadFocus() { return S_OK; }

STDAPI McBopomofoTIP::OnKillThreadFocus() {
  _candidateWindow.Hide();
  return S_OK;
}

bool McBopomofoTIP::IsOpen() {
  DWORD value = 1;
  if (ReadDWORDCompartmentValue(_ptim, GUID_COMPARTMENT_KEYBOARD_OPENCLOSE,
                                &value)) {
    return value != 0;
  }
  return true;
}

bool McBopomofoTIP::IsDirectCommitWithoutComposition(
    const McBopomofo::IPC::StateUpdatePayload& state) const {
  return !state.commitString.empty() && state.composingBuffer.empty() &&
         _pComposition == nullptr;
}

void McBopomofoTIP::HideAuxiliaryWindowsForDirectCommit(
    const McBopomofo::IPC::StateUpdatePayload& state) {
  if (IsDirectCommitWithoutComposition(state)) {
    _tooltipWindow.Hide();
    _candidateWindow.Hide();
  }
}

void McBopomofoTIP::ApplyStateToContext(
    ITfContext* context, const McBopomofo::IPC::StateUpdatePayload& state,
    const char* logPrefix) {
  if (!context) {
    LogMessage("%sRequestEditSession skipped: null context", logPrefix);
    return;
  }

  HideAuxiliaryWindowsForDirectCommit(state);

  CStateEditSession* pEditSession = new CStateEditSession(context, this, state);
  HRESULT hr = E_FAIL;
  context->RequestEditSession(_tid, pEditSession, TF_ES_SYNC | TF_ES_READWRITE,
                              &hr);
  LogMessage("%sRequestEditSession returned: 0x%08X", logPrefix, hr);
  pEditSession->Release();
}

void McBopomofoTIP::ResetServerState() {
  LogMessage("Sending RESET command to server");
  McBopomofo::IPC::NamedPipeClient pipe(McBopomofo::IPC::PIPE_NAME);
  std::string response;
  if (pipe.Call(McBopomofo::IPC::SerializeReset(), response)) {
    LogMessage("Reset response received");
    McBopomofo::IPC::StateUpdatePayload state;
    if (McBopomofo::IPC::DeserializeStateUpdate(response, state)) {
      _lastState = state;
      LogMessage("Reset state: CommitStr='%s', CompStr='%s'",
                 state.commitString.c_str(), state.composingBuffer.c_str());

      ITfContext* pContext = nullptr;
      if (GetFocusedContext(_ptim, &pContext)) {
        ApplyStateToContext(pContext, state, "Reset ");
        pContext->Release();
      } else {
        LogMessage("Reset could not acquire focused context for edit session");
      }
    }
  } else {
    LogMessage("Failed to send RESET command");
  }

  _candidateWindow.Hide();
  _tooltipWindow.Hide();
}

void McBopomofoTIP::RefreshLangBar() {
  if (_pModeIconButton) {
    LogMessage("Refreshing mode icon button");
    _pModeIconButton->Update();
  }
  if (_pSwitchLangButton) {
    LogMessage("Refreshing switch lang button");
    _pSwitchLangButton->Update();
  }
}

void McBopomofoTIP::ToggleOpenClose() {
  if (_ptim) {
    bool currentOpen = IsOpen();
    LogMessage("ToggleOpenClose: current state = %s, toggling to %s",
               currentOpen ? "OPEN" : "CLOSED",
               currentOpen ? "CLOSED" : "OPEN");

    ITfCompartmentMgr* pCompMgr = nullptr;
    if (SUCCEEDED(
            _ptim->QueryInterface(IID_ITfCompartmentMgr, (void**)&pCompMgr))) {
      ITfCompartment* pComp = nullptr;
      if (SUCCEEDED(pCompMgr->GetCompartment(
              GUID_COMPARTMENT_KEYBOARD_OPENCLOSE, &pComp))) {
        VARIANT var;
        var.vt = VT_I4;
        var.lVal = currentOpen ? 0 : 1;
        pComp->SetValue(_tid, &var);
        pComp->Release();

        LogMessage("Compartment value set to: %d", var.lVal);
        RefreshLangBar();
      }
      pCompMgr->Release();
    }
  }
}
