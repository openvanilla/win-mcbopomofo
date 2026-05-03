#include <windows.h>
#include <msctf.h>

const CLSID c_clsidMcBopomofoTIP = 
{ 0x810b8d97, 0xdab, 0x4e87, { 0x95, 0x51, 0x76, 0xa3, 0xd4, 0x9d, 0xe, 0x76 } };

int main() {
    CoInitialize(NULL);
    ITfCategoryMgr *pCategoryMgr = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_TF_CategoryMgr, nullptr, CLSCTX_INPROC_SERVER, IID_ITfCategoryMgr, (void**)&pCategoryMgr))) {
        pCategoryMgr->RegisterCategory(c_clsidMcBopomofoTIP, GUID_TFCAT_TIPCAP_SECUREMODE, c_clsidMcBopomofoTIP);
        pCategoryMgr->RegisterCategory(c_clsidMcBopomofoTIP, GUID_TFCAT_TIPCAP_UIELEMENTENABLED, c_clsidMcBopomofoTIP);
        pCategoryMgr->RegisterCategory(c_clsidMcBopomofoTIP, GUID_TFCAT_TIPCAP_INPUTMODECOMPARTMENT, c_clsidMcBopomofoTIP);
        pCategoryMgr->RegisterCategory(c_clsidMcBopomofoTIP, GUID_TFCAT_TIPCAP_COMLESS, c_clsidMcBopomofoTIP);
        pCategoryMgr->Release();
    }
    CoUninitialize();
    return 0;
}
