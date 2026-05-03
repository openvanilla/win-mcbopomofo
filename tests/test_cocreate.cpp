#include <windows.h>
#include <iostream>
#include <msctf.h>

const CLSID c_clsidMcBopomofoTIP = 
{ 0x810b8d97, 0xdab, 0x4e87, { 0x95, 0x51, 0x76, 0xa3, 0xd4, 0x9d, 0xe, 0x76 } };

int main() {
    CoInitialize(NULL);
    IUnknown* pUnk = nullptr;
    HRESULT hr = CoCreateInstance(c_clsidMcBopomofoTIP, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (void**)&pUnk);
    if (FAILED(hr)) {
        std::cerr << "CoCreateInstance failed: 0x" << std::hex << hr << std::endl;
    } else {
        std::cout << "CoCreateInstance succeeded!" << std::endl;
        pUnk->Release();
    }
    CoUninitialize();
    return 0;
}
