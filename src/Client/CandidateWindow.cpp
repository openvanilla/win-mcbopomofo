#include "CandidateWindow.h"
#include "UTFHelper.h"
#include "PathCompat.h"
#include <sstream>
#include <cmath>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

const wchar_t* const CANDIDATE_WINDOW_CLASS = L"WinMcBopomofoCandidateWindow";

CandidateWindow::CandidateWindow() 
    : _hwnd(nullptr), _cursorIndex(0), _isVertical(false), _isDarkMode(false),
      _pD2DFactory(nullptr), _pRenderTarget(nullptr), _pDWriteFactory(nullptr), 
      _pTextFormat(nullptr), _pTextLayout(nullptr),
      _pTextBrush(nullptr), _pBgBrush(nullptr), _pBorderBrush(nullptr) {
    UpdateTheme();
    CreateDeviceIndependentResources();
}

CandidateWindow::~CandidateWindow() {
    Destroy();
    DiscardDeviceResources();
    if (_pTextLayout) { _pTextLayout->Release(); }
    if (_pTextFormat) { _pTextFormat->Release(); }
    if (_pDWriteFactory) { _pDWriteFactory->Release(); }
    if (_pD2DFactory) { _pD2DFactory->Release(); }
}

void CandidateWindow::CreateDeviceIndependentResources() {
    if (!_pD2DFactory) {
        D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &_pD2DFactory);
        DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&_pDWriteFactory));
        
        _pDWriteFactory->CreateTextFormat(
            L"Microsoft JhengHei UI", // Good UI font for Traditional Chinese with Emoji support
            NULL,
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            18.0f,
            L"zh-TW",
            &_pTextFormat
        );
    }
}

void CandidateWindow::CreateDeviceResources() {
    if (!_pRenderTarget && _hwnd) {
        RECT rc;
        GetClientRect(_hwnd, &rc);
        D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

        _pD2DFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(_hwnd, size),
            &_pRenderTarget
        );

        if (_pRenderTarget) {
            _pRenderTarget->CreateSolidColorBrush(
                _isDarkMode ? D2D1::ColorF(0xF0F0F0) : D2D1::ColorF(0x101010),
                &_pTextBrush
            );
            _pRenderTarget->CreateSolidColorBrush(
                _isDarkMode ? D2D1::ColorF(0x202020) : D2D1::ColorF(0xFFFFFF),
                &_pBgBrush
            );
            _pRenderTarget->CreateSolidColorBrush(
                _isDarkMode ? D2D1::ColorF(0x404040) : D2D1::ColorF(0xCCCCCC),
                &_pBorderBrush
            );
        }
    }
}

void CandidateWindow::DiscardDeviceResources() {
    if (_pRenderTarget) { _pRenderTarget->Release(); _pRenderTarget = nullptr; }
    if (_pTextBrush) { _pTextBrush->Release(); _pTextBrush = nullptr; }
    if (_pBgBrush) { _pBgBrush->Release(); _pBgBrush = nullptr; }
    if (_pBorderBrush) { _pBorderBrush->Release(); _pBorderBrush = nullptr; }
}

void CandidateWindow::UpdateTheme() {
    DWORD useLightTheme = 1;
    DWORD size = sizeof(useLightTheme);
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegQueryValueExW(hKey, L"AppsUseLightTheme", NULL, NULL, (LPBYTE)&useLightTheme, &size);
        RegCloseKey(hKey);
    }
    _isDarkMode = (useLightTheme == 0);

    if (_pRenderTarget) {
        DiscardDeviceResources(); // Force recreate brushes
        InvalidateRect(_hwnd, nullptr, FALSE);
    }
}

void CandidateWindow::OnSettingChange() {
    UpdateTheme();
    // Re-trigger layout read
    std::string dir = McBopomofo::fcitx5_compat::userDirectory();
    std::wstring iniPath = McBopomofo::Utf8ToUtf16(dir) + L"\\mcbopomofo.ini";
    _isVertical = GetPrivateProfileIntW(L"UI", L"CandidateWindowVertical", 0, iniPath.c_str()) != 0;
}

bool CandidateWindow::Create(HINSTANCE hInstance) {
    if (_hwnd) return true;

    WNDCLASSEXW wcex = {0};
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.style = CS_DROPSHADOW | CS_IME;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = NULL; // Handled by D2D
    wcex.lpszClassName = CANDIDATE_WINDOW_CLASS;

    RegisterClassExW(&wcex); // Ignore failure as it might be registered by another instance

    _hwnd = CreateWindowExW(
        WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE,
        CANDIDATE_WINDOW_CLASS,
        L"",
        WS_POPUP, // D2D will draw the border
        0, 0, 100, 30, // Initial dummy size
        nullptr, nullptr, hInstance, this
    );

    OnSettingChange(); // Load initial settings

    return _hwnd != nullptr;
}

void CandidateWindow::Destroy() {
    if (_hwnd) {
        DestroyWindow(_hwnd);
        _hwnd = nullptr;
    }
}

void CandidateWindow::UpdateUI(const std::vector<std::string>& candidates, int cursorIndex, bool forceVertical) {
    if (!_hwnd) return;

    _candidates.clear();
    for (const auto& c : candidates) {
        _candidates.push_back(McBopomofo::Utf8ToUtf16(c));
    }
    _cursorIndex = cursorIndex;

    if (_candidates.empty()) {
        Hide();
        return;
    }

    bool drawVertical = _isVertical || forceVertical;

    std::wstringstream ss;
    for (size_t i = 0; i < _candidates.size(); ++i) {
        ss << (i + 1) << L"." << _candidates[i];
        if (i < _candidates.size() - 1) {
            ss << (drawVertical ? L"\n" : L"   ");
        }
    }
    _displayString = ss.str();

    if (_pTextLayout) {
        _pTextLayout->Release();
        _pTextLayout = nullptr;
    }

    if (_pDWriteFactory && _pTextFormat) {
        _pDWriteFactory->CreateTextLayout(
            _displayString.c_str(),
            (UINT32)_displayString.length(),
            _pTextFormat,
            10000.0f,
            10000.0f,
            &_pTextLayout
        );
    }

    float textWidth = 0, textHeight = 0;
    if (_pTextLayout) {
        DWRITE_TEXT_METRICS metrics;
        _pTextLayout->GetMetrics(&metrics);
        textWidth = metrics.width;
        textHeight = metrics.height;
    }

    int width = (int)std::ceil(textWidth) + 24;
    int height = (int)std::ceil(textHeight) + 16;

    SetWindowPos(_hwnd, HWND_TOPMOST, 0, 0, width, height, SWP_NOMOVE | SWP_NOACTIVATE);
    if (_pRenderTarget) {
        _pRenderTarget->Resize(D2D1::SizeU(width, height));
    }
    InvalidateRect(_hwnd, nullptr, FALSE);
}

void CandidateWindow::Move(int x, int y) {
    if (_hwnd && !_candidates.empty()) {
        SetWindowPos(_hwnd, HWND_TOPMOST, x, y, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
    }
}

void CandidateWindow::Hide() {
    if (_hwnd) {
        ShowWindow(_hwnd, SW_HIDE);
    }
}

LRESULT CALLBACK CandidateWindow::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    CandidateWindow* pThis = nullptr;

    if (uMsg == WM_NCCREATE) {
        CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
        pThis = (CandidateWindow*)pCreate->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
    } else {
        pThis = (CandidateWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }

    if (pThis) {
        if (uMsg == WM_PAINT) {
            return pThis->OnPaint(hwnd);
        } else if (uMsg == WM_SETTINGCHANGE) {
            pThis->OnSettingChange();
        } else if (uMsg == WM_DISPLAYCHANGE) {
            ::InvalidateRect(hwnd, nullptr, FALSE);
        }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CandidateWindow::OnPaint(HWND hwnd) {
    PAINTSTRUCT ps;
    BeginPaint(hwnd, &ps);

    CreateDeviceResources();
    if (_pRenderTarget) {
        _pRenderTarget->BeginDraw();
        _pRenderTarget->Clear(_pBgBrush->GetColor());

        if (_pTextLayout && _pTextBrush) {
            _pRenderTarget->DrawTextLayout(
                D2D1::Point2F(12.0f, 8.0f),
                _pTextLayout,
                _pTextBrush,
                D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT
            );
        }

        // Draw border
        D2D1_SIZE_F size = _pRenderTarget->GetSize();
        _pRenderTarget->DrawRectangle(D2D1::RectF(0.5f, 0.5f, size.width - 0.5f, size.height - 0.5f), _pBorderBrush, 1.0f);

        HRESULT hr = _pRenderTarget->EndDraw();
        if (hr == D2DERR_RECREATE_TARGET) {
            DiscardDeviceResources();
        }
    }

    EndPaint(hwnd, &ps);
    return 0;
}
