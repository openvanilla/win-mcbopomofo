#include "CandidateWindow.h"
#include "UTFHelper.h"
#include "PathCompat.h"
#include <sstream>
#include <cmath>
#include <algorithm>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

const wchar_t* const CANDIDATE_WINDOW_CLASS = L"WinMcBopomofoCandidateWindow";

CandidateWindow::CandidateWindow() 
    : _hwnd(nullptr), _dpiScale(1.0f), _cursorIndex(0), _candidateKeys(L"123456789"),
      _candidateKeysCount(9), _isVertical(false), _forceVertical(false), _isDarkMode(false),
      _pD2DFactory(nullptr), _pRenderTarget(nullptr), _pDWriteFactory(nullptr), 
      _pTextFormat(nullptr), _pTextLayout(nullptr),
      _pTextBrush(nullptr), _pBgBrush(nullptr), _pBorderBrush(nullptr),
      _pHighlightBgBrush(nullptr), _pHighlightTextBrush(nullptr) {
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

float CandidateWindow::GetDpiScale() {
    if (!_hwnd) return 1.0f;
    UINT dpi = 96;
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    auto pGetDpiForWindow = (UINT(WINAPI*)(HWND))GetProcAddress(hUser32, "GetDpiForWindow");
    if (pGetDpiForWindow) {
        dpi = pGetDpiForWindow(_hwnd);
    } else {
        HDC hdc = GetDC(_hwnd);
        if (hdc) {
            dpi = GetDeviceCaps(hdc, LOGPIXELSX);
            ReleaseDC(_hwnd, hdc);
        }
    }
    return (float)dpi / 96.0f;
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
            _pRenderTarget->CreateSolidColorBrush(
                D2D1::ColorF(0x0078D7), // Windows Blue
                &_pHighlightBgBrush
            );
            _pRenderTarget->CreateSolidColorBrush(
                D2D1::ColorF(0xFFFFFF), // White text on highlight
                &_pHighlightTextBrush
            );
        }
    }
}

void CandidateWindow::DiscardDeviceResources() {
    if (_pRenderTarget) { _pRenderTarget->Release(); _pRenderTarget = nullptr; }
    if (_pTextBrush) { _pTextBrush->Release(); _pTextBrush = nullptr; }
    if (_pBgBrush) { _pBgBrush->Release(); _pBgBrush = nullptr; }
    if (_pBorderBrush) { _pBorderBrush->Release(); _pBorderBrush = nullptr; }
    if (_pHighlightBgBrush) { _pHighlightBgBrush->Release(); _pHighlightBgBrush = nullptr; }
    if (_pHighlightTextBrush) { _pHighlightTextBrush->Release(); _pHighlightTextBrush = nullptr; }
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
    wchar_t keys[32] = {};
    GetPrivateProfileStringW(L"General", L"CandidateKeys", L"123456789", keys, 32, iniPath.c_str());
    _candidateKeys = keys;
    if (_candidateKeys != L"123456789" && _candidateKeys != L"asdfghjkl" && _candidateKeys != L"asdfzxcvb") {
        _candidateKeys = L"123456789";
    }
    _candidateKeysCount = GetPrivateProfileIntW(L"General", L"CandidateKeysCount", 9, iniPath.c_str());
    if (_candidateKeysCount < 4 || _candidateKeysCount > 9) {
        _candidateKeysCount = 9;
    }
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

    _dpiScale = GetDpiScale();

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
    _forceVertical = forceVertical;

    const int pageSize = _candidateKeysCount;
    int pageIndex = _cursorIndex / pageSize;
    int startIndex = pageIndex * pageSize;
    int endIndex = std::min((int)_candidates.size(), startIndex + pageSize);

    std::wstringstream ss;
    _keyRanges.clear();
    _selectedRange = {0, 0};
    UINT32 currentPos = 0;

    for (int i = startIndex; i < endIndex; ++i) {
        int displayIndex = i - startIndex;
        wchar_t key = displayIndex < static_cast<int>(_candidateKeys.length()) ? _candidateKeys[displayIndex] : L'?';
        std::wstring keyStr(1, key);
        keyStr += L". ";
        std::wstring candStr = _candidates[i];

        if (i == _cursorIndex) {
            _selectedRange.start = currentPos;
        }

        _keyRanges.push_back({currentPos, (UINT32)keyStr.length()});
        ss << keyStr;
        currentPos += (UINT32)keyStr.length();

        ss << candStr;
        currentPos += (UINT32)candStr.length();

        if (i == _cursorIndex) {
            _selectedRange.length = currentPos - _selectedRange.start;
        }

        if (i < endIndex - 1) {
            std::wstring sep = (drawVertical ? L"\n" : L"   ");
            ss << sep;
            currentPos += (UINT32)sep.length();
        }
    }
    
    // Add page indicator if there are multiple pages
    if (static_cast<int>(_candidates.size()) > pageSize) {
        int totalPages = (static_cast<int>(_candidates.size()) + pageSize - 1) / pageSize;
        std::wstring indStr = (drawVertical ? L"\n(" : L"  (");
        indStr += std::to_wstring(pageIndex + 1) + L"/" + std::to_wstring(totalPages) + L")";
        
        _keyRanges.push_back({currentPos, (UINT32)indStr.length()});
        ss << indStr;
        currentPos += (UINT32)indStr.length();
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

        if (_pTextLayout) {
            for (const auto& range : _keyRanges) {
                DWRITE_TEXT_RANGE dwriteRange = { range.start, range.length };
                _pTextLayout->SetFontFamilyName(L"Segoe UI", dwriteRange);
                _pTextLayout->SetFontSize(15.0f, dwriteRange);
            }
        }
    }

    float textWidth = 0, textHeight = 0;
    if (_pTextLayout) {
        DWRITE_TEXT_METRICS metrics;
        _pTextLayout->GetMetrics(&metrics);
        textWidth = metrics.width;
        textHeight = metrics.height;
    }

    int width = (int)std::ceil(textWidth * _dpiScale) + (int)(24 * _dpiScale);
    int height = (int)std::ceil(textHeight * _dpiScale) + (int)(16 * _dpiScale);

    // Enforce a minimum size to prevent the window from collapsing or being rejected by the OS
    width = std::max(width, (int)(50 * _dpiScale));
    height = std::max(height, (int)(24 * _dpiScale));

    SetWindowPos(_hwnd, HWND_TOPMOST, 0, 0, width, height, SWP_NOMOVE | SWP_NOACTIVATE);
    if (_pRenderTarget) {
        _pRenderTarget->Resize(D2D1::SizeU(width, height));
    }
    ShowWindow(_hwnd, SW_SHOWNOACTIVATE);
    InvalidateRect(_hwnd, nullptr, FALSE);
}

void CandidateWindow::Move(int x, int y) {
    if (_hwnd) {
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
        } else if (uMsg == WM_DPICHANGED) {
            pThis->_dpiScale = (float)LOWORD(wParam) / 96.0f;
            RECT* prcNewWindow = (RECT*)lParam;
            SetWindowPos(hwnd, NULL, prcNewWindow->left, prcNewWindow->top, 
                         prcNewWindow->right - prcNewWindow->left, 
                         prcNewWindow->bottom - prcNewWindow->top, 
                         SWP_NOZORDER | SWP_NOACTIVATE);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
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
        _pRenderTarget->SetTransform(D2D1::Matrix3x2F::Scale(_dpiScale, _dpiScale));
        _pRenderTarget->Clear(_pBgBrush->GetColor());

        if (_pTextLayout && _pTextBrush) {
            // Draw highlight background if we have a selected range
            if (_selectedRange.length > 0 && _pHighlightBgBrush) {
                UINT32 actualHitTestCount = 0;
                _pTextLayout->HitTestTextRange(
                    _selectedRange.start, _selectedRange.length,
                    0, 0, nullptr, 0, &actualHitTestCount
                );
                
                if (actualHitTestCount > 0) {
                    std::vector<DWRITE_HIT_TEST_METRICS> hitTestMetrics(actualHitTestCount);
                    _pTextLayout->HitTestTextRange(
                        _selectedRange.start, _selectedRange.length,
                        12.0f, 8.0f, hitTestMetrics.data(), actualHitTestCount, &actualHitTestCount
                    );

                    float layoutWidth = 0;
                    bool isVerticalLayout = _isVertical || _forceVertical;
                    if (isVerticalLayout) {
                        DWRITE_TEXT_METRICS textMetrics;
                        _pTextLayout->GetMetrics(&textMetrics);
                        layoutWidth = textMetrics.width;
                    }

                    for (const auto& metrics : hitTestMetrics) {
                        float right = metrics.left + metrics.width;
                        if (isVerticalLayout) {
                            right = 12.0f + layoutWidth;
                        }

                        D2D1_RECT_F rect = D2D1::RectF(
                            metrics.left - 4.0f, 
                            metrics.top - 2.0f, 
                            right + 4.0f, 
                            metrics.top + metrics.height + 2.0f
                        );
                        _pRenderTarget->FillRectangle(rect, _pHighlightBgBrush);
                    }
                }

                // Apply highlight text color effect
                DWRITE_TEXT_RANGE dwriteRange = { _selectedRange.start, _selectedRange.length };
                _pTextLayout->SetDrawingEffect(_pHighlightTextBrush, dwriteRange);
            }

            _pRenderTarget->DrawTextLayout(
                D2D1::Point2F(12.0f, 8.0f),
                _pTextLayout,
                _pTextBrush,
                D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT
            );

            // Revert drawing effect so it doesn't leak into subsequent frames if brush is destroyed
            if (_selectedRange.length > 0) {
                DWRITE_TEXT_RANGE dwriteRange = { _selectedRange.start, _selectedRange.length };
                _pTextLayout->SetDrawingEffect(nullptr, dwriteRange);
            }
        }

        // Draw border
        D2D1_SIZE_F size = _pRenderTarget->GetSize();
        // border should be in pixels, but SetTransform is active. 
        // We should probably draw the border without transform or compensate.
        _pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
        _pRenderTarget->DrawRectangle(D2D1::RectF(0.5f, 0.5f, size.width - 0.5f, size.height - 0.5f), _pBorderBrush, 1.0f);

        HRESULT hr = _pRenderTarget->EndDraw();
        if (hr == D2DERR_RECREATE_TARGET) {
            DiscardDeviceResources();
        }
    }

    EndPaint(hwnd, &ps);
    return 0;
}
