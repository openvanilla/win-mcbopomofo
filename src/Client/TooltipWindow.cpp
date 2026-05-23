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

#include "Globals.h"

#include <algorithm>
#include <cmath>
#include <dwmapi.h>

#include "UTFHelper.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "dwmapi.lib")

const wchar_t* const TOOLTIP_WINDOW_CLASS = L"WinMcBopomofoTooltipWindow";



TooltipWindow::TooltipWindow()
    : hwnd_(nullptr),
      dpiScale_(1.0f),
      pD2DFactory_(nullptr),
      pRenderTarget_(nullptr),
      pDWriteFactory_(nullptr),
      pTextFormat_(nullptr),
      pTextLayout_(nullptr),
      pTextBrush_(nullptr),
      pBgBrush_(nullptr),
      pBorderBrush_(nullptr) {
  createDeviceIndependentResources_();
}

TooltipWindow::~TooltipWindow() {
  Destroy();
  discardDeviceResources_();
  if (pTextLayout_) {
    pTextLayout_->Release();
  }
  if (pTextFormat_) {
    pTextFormat_->Release();
  }
  if (pDWriteFactory_) {
    pDWriteFactory_->Release();
  }
  if (pD2DFactory_) {
    pD2DFactory_->Release();
  }
}

void TooltipWindow::createDeviceIndependentResources_() {
  if (!pD2DFactory_) {
    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pD2DFactory_);
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                        reinterpret_cast<IUnknown**>(&pDWriteFactory_));

    pDWriteFactory_->CreateTextFormat(
        L"Microsoft JhengHei UI",  // Good UI font for Traditional Chinese
        NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        15.0f,  // Slightly smaller than candidate window
        L"zh-TW", &pTextFormat_);
  }
}

void TooltipWindow::createDeviceResources_() {
  if (!pRenderTarget_ && hwnd_) {
    RECT rc;
    GetClientRect(hwnd_, &rc);
    D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

    pD2DFactory_->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_DEFAULT,
            D2D1::PixelFormat(),
            96.0f,
            96.0f
        ),
        D2D1::HwndRenderTargetProperties(hwnd_, size), &pRenderTarget_);

    if (pRenderTarget_) {
      pRenderTarget_->CreateSolidColorBrush(
          D2D1::ColorF(0x000000),  // Black text
          &pTextBrush_);
      pRenderTarget_->CreateSolidColorBrush(
          D2D1::ColorF(0xFFFFFFE0),  // Light yellow background
          &pBgBrush_);
      pRenderTarget_->CreateSolidColorBrush(
          D2D1::ColorF(0x000000),  // Black border
          &pBorderBrush_);
    }
  }
}

void TooltipWindow::discardDeviceResources_() {
  if (pRenderTarget_) {
    pRenderTarget_->Release();
    pRenderTarget_ = nullptr;
  }
  if (pTextBrush_) {
    pTextBrush_->Release();
    pTextBrush_ = nullptr;
  }
  if (pBgBrush_) {
    pBgBrush_->Release();
    pBgBrush_ = nullptr;
  }
  if (pBorderBrush_) {
    pBorderBrush_->Release();
    pBorderBrush_ = nullptr;
  }
}

bool TooltipWindow::Create(HINSTANCE hInstance) {
  if (hwnd_) return true;

  WNDCLASSEXW wcex = {0};
  wcex.cbSize = sizeof(WNDCLASSEXW);
  wcex.style = 0;
  wcex.lpfnWndProc = wndProc_;
  wcex.hInstance = hInstance;
  wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
  wcex.hbrBackground = NULL;  // Handled by D2D
  wcex.lpszClassName = TOOLTIP_WINDOW_CLASS;

  RegisterClassExW(
      &wcex);  // Ignore failure as it might be registered by another instance

  hwnd_ = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE,
                          TOOLTIP_WINDOW_CLASS, L"",
                          WS_POPUP,       // D2D will draw the border
                          0, 0, 100, 30,  // Initial dummy size
                          nullptr, nullptr, hInstance, this);

  EnableWindowDropShadow(hwnd_);

  return hwnd_ != nullptr;
}

void TooltipWindow::Destroy() {
  if (hwnd_) {
    DestroyWindow(hwnd_);
    hwnd_ = nullptr;
  }
}

void TooltipWindow::UpdateUI(const std::string& tooltipText) {
  if (!hwnd_) return;

  if (tooltipText.empty()) {
    Hide();
    return;
  }

  dpiScale_ = GetDpiScaleForWindow(hwnd_);
  displayString_ = McBopomofo::Utf8ToUtf16(tooltipText);
  rebuildLayoutAndResize_();
}

void TooltipWindow::rebuildLayoutAndResize_() {
  if (displayString_.empty()) {
    return;
  }

  if (pTextLayout_) {
    pTextLayout_->Release();
    pTextLayout_ = nullptr;
  }

  if (pDWriteFactory_ && pTextFormat_) {
    pDWriteFactory_->CreateTextLayout(
        displayString_.c_str(), (UINT32)displayString_.length(), pTextFormat_,
        10000.0f, 10000.0f, &pTextLayout_);
  }

  float textWidth = 0, textHeight = 0;
  if (pTextLayout_) {
    DWRITE_TEXT_METRICS metrics;
    pTextLayout_->GetMetrics(&metrics);
    textWidth = metrics.width;
    textHeight = metrics.height;
  }

  int width = (int)std::ceil(textWidth * dpiScale_) + (int)(16 * dpiScale_);
  int height = (int)std::ceil(textHeight * dpiScale_) + (int)(8 * dpiScale_);

  // Enforce a minimum size
  width = std::max(width, (int)(20 * dpiScale_));
  height = std::max(height, (int)(20 * dpiScale_));

  SetWindowPos(hwnd_, HWND_TOPMOST, 0, 0, width, height,
               SWP_NOMOVE | SWP_NOACTIVATE);
  if (pRenderTarget_) {
    pRenderTarget_->Resize(D2D1::SizeU(width, height));
  }
  ShowWindow(hwnd_, SW_SHOWNOACTIVATE);
  InvalidateRect(hwnd_, nullptr, FALSE);
}

void TooltipWindow::Move(int x, int y) {
  if (hwnd_) {
    const float oldScale = dpiScale_;
    SetWindowPos(hwnd_, HWND_TOPMOST, x, y, 0, 0,
                 SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
    dpiScale_ = GetDpiScaleForWindow(hwnd_);
    if (std::abs(dpiScale_ - oldScale) > 0.001f) {
      rebuildLayoutAndResize_();
    } else {
      InvalidateRect(hwnd_, nullptr, FALSE);
    }
  }
}

void TooltipWindow::Hide() {
  if (hwnd_) {
    ShowWindow(hwnd_, SW_HIDE);
  }
}

LRESULT CALLBACK TooltipWindow::wndProc_(HWND hwnd, UINT uMsg, WPARAM wParam,
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
      return pThis->onPaint_(hwnd);
    } else if (uMsg == WM_DPICHANGED) {
      pThis->dpiScale_ = (float)LOWORD(wParam) / 96.0f;
      RECT* prcNewWindow = (RECT*)lParam;
      SetWindowPos(hwnd, NULL, prcNewWindow->left, prcNewWindow->top,
                   prcNewWindow->right - prcNewWindow->left,
                   prcNewWindow->bottom - prcNewWindow->top,
                   SWP_NOZORDER | SWP_NOACTIVATE);
      pThis->rebuildLayoutAndResize_();
      return 0;
    } else if (uMsg == WM_DISPLAYCHANGE) {
      ::InvalidateRect(hwnd, nullptr, FALSE);
    }
  }

  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT TooltipWindow::onPaint_(HWND hwnd) {
  PAINTSTRUCT ps;
  BeginPaint(hwnd, &ps);

  createDeviceResources_();
  if (pRenderTarget_) {
    pRenderTarget_->BeginDraw();
    pRenderTarget_->SetTransform(D2D1::Matrix3x2F::Scale(dpiScale_, dpiScale_));
    if (pBgBrush_) {
      pRenderTarget_->Clear(pBgBrush_->GetColor());
    } else {
      pRenderTarget_->Clear(D2D1::ColorF(D2D1::ColorF::White));
    }

    if (pTextLayout_ && pTextBrush_) {
      pRenderTarget_->DrawTextLayout(D2D1::Point2F(8.0f, 4.0f), pTextLayout_,
                                     pTextBrush_,
                                     D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);
    }

    // Draw border
    D2D1_SIZE_F size = pRenderTarget_->GetSize();
    pRenderTarget_->SetTransform(D2D1::Matrix3x2F::Identity());
    if (pBorderBrush_) {
      pRenderTarget_->DrawRectangle(
          D2D1::RectF(0.5f, 0.5f, size.width - 0.5f, size.height - 0.5f),
          pBorderBrush_, 1.0f);
    }

    HRESULT hr = pRenderTarget_->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) {
      discardDeviceResources_();
      InvalidateRect(hwnd, nullptr, FALSE);
    }
  }

  EndPaint(hwnd, &ps);
  return 0;
}
