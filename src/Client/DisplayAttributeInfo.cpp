#include "DisplayAttributeInfo.h"

const GUID c_guidDisplayAttributeInput = 
{ 0x4b688cd4, 0xcfb6, 0x4767, { 0xad, 0x80, 0x4d, 0x56, 0x20, 0x86, 0xfc, 0x3b } };

const GUID c_guidDisplayAttributeMarked = 
{ 0xd82c4a26, 0xe0cc, 0x43bb, { 0x8c, 0xf2, 0xbb, 0xb, 0xbf, 0xf4, 0xfe, 0x70 } };

// ----------------------------------------------------------------------------
// CDisplayAttributeInfo
// ----------------------------------------------------------------------------
CDisplayAttributeInfo::CDisplayAttributeInfo(REFGUID guid, TF_DISPLAYATTRIBUTE da, const WCHAR* desc)
    : _cRef(1), _guid(guid), _da(da), _daDefault(da), _desc(desc) {
}

CDisplayAttributeInfo::~CDisplayAttributeInfo() {}

STDAPI CDisplayAttributeInfo::QueryInterface(REFIID riid, void **ppvObj) {
    if (ppvObj == nullptr) return E_INVALIDARG;
    *ppvObj = nullptr;
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfDisplayAttributeInfo)) {
        *ppvObj = static_cast<ITfDisplayAttributeInfo *>(this);
    }
    if (*ppvObj) {
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

STDAPI_(ULONG) CDisplayAttributeInfo::AddRef(void) {
    return InterlockedIncrement(&_cRef);
}

STDAPI_(ULONG) CDisplayAttributeInfo::Release(void) {
    LONG cr = InterlockedDecrement(&_cRef);
    if (cr == 0) delete this;
    return cr;
}

STDAPI CDisplayAttributeInfo::GetGUID(GUID *pguid) {
    if (pguid == nullptr) return E_INVALIDARG;
    *pguid = _guid;
    return S_OK;
}

STDAPI CDisplayAttributeInfo::GetDescription(BSTR *pbstrDesc) {
    if (pbstrDesc == nullptr) return E_INVALIDARG;
    *pbstrDesc = SysAllocString(_desc);
    return (*pbstrDesc != nullptr) ? S_OK : E_OUTOFMEMORY;
}

STDAPI CDisplayAttributeInfo::GetAttributeInfo(TF_DISPLAYATTRIBUTE *pda) {
    if (pda == nullptr) return E_INVALIDARG;
    *pda = _da;
    return S_OK;
}

STDAPI CDisplayAttributeInfo::SetAttributeInfo(const TF_DISPLAYATTRIBUTE *pda) {
    if (pda == nullptr) return E_INVALIDARG;
    _da = *pda;
    return S_OK;
}

STDAPI CDisplayAttributeInfo::Reset() {
    _da = _daDefault;
    return S_OK;
}

// ----------------------------------------------------------------------------
// CEnumDisplayAttributeInfo
// ----------------------------------------------------------------------------
CEnumDisplayAttributeInfo::CEnumDisplayAttributeInfo() : _cRef(1), _index(0) {}
CEnumDisplayAttributeInfo::~CEnumDisplayAttributeInfo() {}

STDAPI CEnumDisplayAttributeInfo::QueryInterface(REFIID riid, void **ppvObj) {
    if (ppvObj == nullptr) return E_INVALIDARG;
    *ppvObj = nullptr;
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IEnumTfDisplayAttributeInfo)) {
        *ppvObj = static_cast<IEnumTfDisplayAttributeInfo *>(this);
    }
    if (*ppvObj) {
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

STDAPI_(ULONG) CEnumDisplayAttributeInfo::AddRef(void) {
    return InterlockedIncrement(&_cRef);
}

STDAPI_(ULONG) CEnumDisplayAttributeInfo::Release(void) {
    LONG cr = InterlockedDecrement(&_cRef);
    if (cr == 0) delete this;
    return cr;
}

STDAPI CEnumDisplayAttributeInfo::Clone(IEnumTfDisplayAttributeInfo **ppEnum) {
    if (ppEnum == nullptr) return E_INVALIDARG;
    *ppEnum = new CEnumDisplayAttributeInfo();
    if (*ppEnum == nullptr) return E_OUTOFMEMORY;
    ((CEnumDisplayAttributeInfo*)*ppEnum)->_index = _index;
    return S_OK;
}

STDAPI CEnumDisplayAttributeInfo::Next(ULONG ulCount, ITfDisplayAttributeInfo **rgInfo, ULONG *pcFetched) {
    if (pcFetched) *pcFetched = 0;
    if (ulCount == 0 || rgInfo == nullptr) return E_INVALIDARG;

    ULONG fetched = 0;
    while (fetched < ulCount && _index < 2) {
        if (_index == 0) {
            TF_DISPLAYATTRIBUTE da;
            ZeroMemory(&da, sizeof(da));
            da.lsStyle = TF_LS_SQUIGGLE;
            da.crLine.type = TF_CT_SYSCOLOR;
            da.crLine.nIndex = COLOR_WINDOWTEXT;
            rgInfo[fetched] = new CDisplayAttributeInfo(c_guidDisplayAttributeInput, da, L"Win-McBopomofo Input");
        } else if (_index == 1) {
            TF_DISPLAYATTRIBUTE da;
            ZeroMemory(&da, sizeof(da));
            da.lsStyle = TF_LS_NONE;
            da.crText.type = TF_CT_SYSCOLOR;
            da.crText.nIndex = COLOR_HIGHLIGHTTEXT;
            da.crBk.type = TF_CT_SYSCOLOR;
            da.crBk.nIndex = COLOR_HIGHLIGHT;
            rgInfo[fetched] = new CDisplayAttributeInfo(c_guidDisplayAttributeMarked, da, L"Win-McBopomofo Marked");
        }
        if (rgInfo[fetched] == nullptr) return E_OUTOFMEMORY;
        _index++;
        fetched++;
    }

    if (pcFetched) *pcFetched = fetched;
    return (fetched == ulCount) ? S_OK : S_FALSE;
}

STDAPI CEnumDisplayAttributeInfo::Reset() {
    _index = 0;
    return S_OK;
}

STDAPI CEnumDisplayAttributeInfo::Skip(ULONG ulCount) {
    _index += ulCount;
    if (_index > 2) _index = 2;
    return S_OK;
}