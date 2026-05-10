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

#include "TooltipWindow.h"

#include <algorithm>
#include <cmath>

#include "UTFHelper.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

const wchar_t* const TOOLTIP_WINDOW_CLASS = L"WinMcBopomofoTooltipWindow";

TooltipWindow::TooltipWindow()
    : _hwnd(nullptr),
      _dpiScale(1.0f),
      _pD2DFactory(nullptr),
      _pRenderTarget(nullptr),
      _pDWriteFactory(nullptr),
      _pTextFormat(nullptr),
      _pTextLayout(nullptr),
      _pTextBrush(nullptr),
      _pBgBrush(nullptr),
      _pBorderBrush(nullptr) {
  CreateDeviceIndependentResources();
}

TooltipWindow::~TooltipWindow() {
  Destroy();
  DiscardDeviceResources();
  if (_pTextLayout) {
    _pTextLayout->Release();
  }
  if (_pTextFormat) {
    _pTextFormat->Release();
  }
  if (_pDWriteFactory) {
    _pDWriteFactory->Release();
  }
  if (_pD2DFactory) {
    _pD2DFactory->Release();
  }
}

float TooltipWindow::GetDpiScale() {
  if (!_hwnd) return 1.0f;
  UINT dpi = 96;
  HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
  auto pGetDpiForWindow =
      (UINT(WINAPI*)(HWND))GetProcAddress(hUser32, "GetDpiForWindow");
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

void TooltipWindow::CreateDeviceIndependentResources() {
  if (!_pD2DFactory) {
    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &_pD2DFactory);
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                        reinterpret_cast<IUnknown**>(&_pDWriteFactory));

    _pDWriteFactory->CreateTextFormat(
        L"Microsoft JhengHei UI",  // Good UI font for Traditional Chinese
        NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        15.0f,  // Slightly smaller than candidate window
        L"zh-TW", &_pTextFormat);
  }
}

void TooltipWindow::CreateDeviceResources() {
  if (!_pRenderTarget && _hwnd) {
    RECT rc;
    GetClientRect(_hwnd, &rc);
    D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

    _pD2DFactory->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(),
        D2D1::HwndRenderTargetProperties(_hwnd, size), &_pRenderTarget);

    if (_pRenderTarget) {
      _pRenderTarget->CreateSolidColorBrush(
          D2D1::ColorF(0x000000),  // Black text
          &_pTextBrush);
      _pRenderTarget->CreateSolidColorBrush(
          D2D1::ColorF(0xFFFFFFE0),  // Light yellow background
          &_pBgBrush);
      _pRenderTarget->CreateSolidColorBrush(
          D2D1::ColorF(0x000000),  // Black border
          &_pBorderBrush);
    }
  }
}

void TooltipWindow::DiscardDeviceResources() {
  if (_pRenderTarget) {
    _pRenderTarget->Release();
    _pRenderTarget = nullptr;
  }
  if (_pTextBrush) {
    _pTextBrush->Release();
    _pTextBrush = nullptr;
  }
  if (_pBgBrush) {
    _pBgBrush->Release();
    _pBgBrush = nullptr;
  }
  if (_pBorderBrush) {
    _pBorderBrush->Release();
    _pBorderBrush = nullptr;
  }
}

bool TooltipWindow::Create(HINSTANCE hInstance) {
  if (_hwnd) return true;

  WNDCLASSEXW wcex = {0};
  wcex.cbSize = sizeof(WNDCLASSEXW);
  wcex.style = CS_DROPSHADOW | CS_IME;
  wcex.lpfnWndProc = WndProc;
  wcex.hInstance = hInstance;
  wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
  wcex.hbrBackground = NULL;  // Handled by D2D
  wcex.lpszClassName = TOOLTIP_WINDOW_CLASS;

  RegisterClassExW(
      &wcex);  // Ignore failure as it might be registered by another instance

  _hwnd = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE,
                          TOOLTIP_WINDOW_CLASS, L"",
                          WS_POPUP,       // D2D will draw the border
                          0, 0, 100, 30,  // Initial dummy size
                          nullptr, nullptr, hInstance, this);

  return _hwnd != nullptr;
}

void TooltipWindow::Destroy() {
  if (_hwnd) {
    DestroyWindow(_hwnd);
    _hwnd = nullptr;
  }
}

void TooltipWindow::UpdateUI(const std::string& tooltipText) {
  if (!_hwnd) return;

  if (tooltipText.empty()) {
    Hide();
    return;
  }

  _dpiScale = GetDpiScale();
  _displayString = McBopomofo::Utf8ToUtf16(tooltipText);

  if (_pTextLayout) {
    _pTextLayout->Release();
    _pTextLayout = nullptr;
  }

  if (_pDWriteFactory && _pTextFormat) {
    _pDWriteFactory->CreateTextLayout(
        _displayString.c_str(), (UINT32)_displayString.length(), _pTextFormat,
        10000.0f, 10000.0f, &_pTextLayout);
  }

  float textWidth = 0, textHeight = 0;
  if (_pTextLayout) {
    DWRITE_TEXT_METRICS metrics;
    _pTextLayout->GetMetrics(&metrics);
    textWidth = metrics.width;
    textHeight = metrics.height;
  }

  int width = (int)std::ceil(textWidth * _dpiScale) + (int)(16 * _dpiScale);
  int height = (int)std::ceil(textHeight * _dpiScale) + (int)(8 * _dpiScale);

  // Enforce a minimum size
  width = std::max(width, (int)(20 * _dpiScale));
  height = std::max(height, (int)(20 * _dpiScale));

  SetWindowPos(_hwnd, HWND_TOPMOST, 0, 0, width, height,
               SWP_NOMOVE | SWP_NOACTIVATE);
  if (_pRenderTarget) {
    _pRenderTarget->Resize(D2D1::SizeU(width, height));
  }
  ShowWindow(_hwnd, SW_SHOWNOACTIVATE);
  InvalidateRect(_hwnd, nullptr, FALSE);
}

void TooltipWindow::Move(int x, int y) {
  if (_hwnd) {
    SetWindowPos(_hwnd, HWND_TOPMOST, x, y, 0, 0,
                 SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
  }
}

void TooltipWindow::Hide() {
  if (_hwnd) {
    ShowWindow(_hwnd, SW_HIDE);
  }
}

LRESULT CALLBACK TooltipWindow::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                        LPARAM lParam) {
  TooltipWindow* pThis = nullptr;

  if (uMsg == WM_NCCREATE) {
    CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
    pThis = (TooltipWindow*)pCreate->lpCreateParams;
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
  } else {
    pThis = (TooltipWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
  }

  if (pThis) {
    if (uMsg == WM_PAINT) {
      return pThis->OnPaint(hwnd);
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

LRESULT TooltipWindow::OnPaint(HWND hwnd) {
  PAINTSTRUCT ps;
  BeginPaint(hwnd, &ps);

  CreateDeviceResources();
  if (_pRenderTarget) {
    _pRenderTarget->BeginDraw();
    _pRenderTarget->SetTransform(D2D1::Matrix3x2F::Scale(_dpiScale, _dpiScale));
    _pRenderTarget->Clear(_pBgBrush->GetColor());

    if (_pTextLayout && _pTextBrush) {
      _pRenderTarget->DrawTextLayout(D2D1::Point2F(8.0f, 4.0f), _pTextLayout,
                                     _pTextBrush,
                                     D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
    }

    // Draw border
    D2D1_SIZE_F size = _pRenderTarget->GetSize();
    _pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
    _pRenderTarget->DrawRectangle(
        D2D1::RectF(0.5f, 0.5f, size.width - 0.5f, size.height - 0.5f),
        _pBorderBrush, 1.0f);

    HRESULT hr = _pRenderTarget->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) {
      DiscardDeviceResources();
    }
  }

  EndPaint(hwnd, &ps);
  return 0;
}
