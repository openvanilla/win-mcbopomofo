#include <msctf.h>
#include <windows.h>

#include "Globals.h"
#include "McBopomofoTIP.h"
#include "Register.h"

// The actual generated CLSID
// {8C9D652A-9B99-4B77-BA9A-3B0F76923B7B}
const CLSID c_clsidMcBopomofoTIP = {
    0x8c9d652a,
    0x9b99,
    0x4b77,
    {0xba, 0x9a, 0x3b, 0xf, 0x76, 0x92, 0x3b, 0x7b}};

HINSTANCE g_hInst = nullptr;
LONG g_cRefDll = 0;

void DllAddRef() { InterlockedIncrement(&g_cRefDll); }

void DllRelease() { InterlockedDecrement(&g_cRefDll); }

class CClassFactory : public IClassFactory {
 public:
  CClassFactory() : _cRef(1) {}
  ~CClassFactory() {}

  // IUnknown
  STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj) override {
    if (ppvObj == nullptr) return E_INVALIDARG;
    *ppvObj = nullptr;
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory)) {
      *ppvObj = static_cast<IClassFactory*>(this);
    }
    if (*ppvObj) {
      AddRef();
      return S_OK;
    }
    return E_NOINTERFACE;
  }

  STDMETHODIMP_(ULONG) AddRef() override {
    return InterlockedIncrement(&_cRef);
  }

  STDMETHODIMP_(ULONG) Release() override {
    LONG cr = InterlockedDecrement(&_cRef);
    if (cr == 0) delete this;
    return cr;
  }

  // IClassFactory
  STDMETHODIMP CreateInstance(IUnknown* pUnkOuter, REFIID riid,
                              void** ppvObj) override {
    if (ppvObj == nullptr) return E_INVALIDARG;
    *ppvObj = nullptr;
    if (pUnkOuter != nullptr) return CLASS_E_NOAGGREGATION;

    McBopomofoTIP* pTIP = new McBopomofoTIP();
    if (pTIP == nullptr) return E_OUTOFMEMORY;

    HRESULT hr = pTIP->QueryInterface(riid, ppvObj);
    pTIP->Release();  // QueryInterface adds a ref, so we release our initial
                      // one
    return hr;
  }

  STDMETHODIMP LockServer(BOOL fLock) override {
    if (fLock) {
      DllAddRef();
    } else {
      DllRelease();
    }
    return S_OK;
  }

 private:
  LONG _cRef;
};

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call,
                      LPVOID lpReserved) {
  switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
      g_hInst = hModule;
      DisableThreadLibraryCalls(hModule);
      LogMessage("DLL_PROCESS_ATTACH loaded");
      break;
    case DLL_PROCESS_DETACH:
      LogMessage("DLL_PROCESS_DETACH unloaded");
      break;
  }
  return TRUE;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppvObj) {
  if (ppvObj == nullptr) return E_INVALIDARG;
  *ppvObj = nullptr;

  if (IsEqualCLSID(rclsid, c_clsidMcBopomofoTIP)) {
    CClassFactory* pFactory = new CClassFactory();
    if (pFactory == nullptr) return E_OUTOFMEMORY;

    HRESULT hr = pFactory->QueryInterface(riid, ppvObj);
    pFactory->Release();
    return hr;
  }

  return CLASS_E_CLASSNOTAVAILABLE;
}

STDAPI DllCanUnloadNow() { return (g_cRefDll == 0) ? S_OK : S_FALSE; }

STDAPI DllRegisterServer() {
  if (!RegisterServer()) return E_FAIL;
  if (!RegisterProfiles()) return E_FAIL;
  if (!RegisterCategories()) return E_FAIL;
  return S_OK;
}

STDAPI DllUnregisterServer() {
  UnregisterCategories();
  UnregisterProfiles();
  UnregisterServer();
  return S_OK;
}
