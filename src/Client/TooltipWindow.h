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

#pragma once
#include <d2d1.h>
#include <dwrite.h>
#include <windows.h>

#include <string>

class TooltipWindow {
 public:
  TooltipWindow();
  ~TooltipWindow();

  bool Create(HINSTANCE hInstance);
  void Destroy();
  void SetOwnerWindow(HWND ownerHwnd);

  void UpdateUI(const std::string& tooltipText);
  void Move(int x, int y);
  void Hide();

  bool IsVisible() const { return hwnd_ && IsWindowVisible(hwnd_); }
  int GetHeight() const {
    if (!hwnd_) return 0;
    RECT rc;
    GetWindowRect(hwnd_, &rc);
    return rc.bottom - rc.top;
  }
  int GetWidth() const {
    if (!hwnd_) return 0;
    RECT rc;
    GetWindowRect(hwnd_, &rc);
    return rc.right - rc.left;
  }

 private:
  static LRESULT CALLBACK wndProc_(HWND hwnd, UINT uMsg, WPARAM wParam,
                                   LPARAM lParam);
  LRESULT onPaint_(HWND hwnd);

  void createDeviceIndependentResources_();
  void createDeviceResources_();
  void discardDeviceResources_();
  void rebuildLayoutAndResize_();

  HWND hwnd_;
  HWND ownerHwnd_;
  float dpiScale_;
  std::wstring displayString_;

  ID2D1Factory* pD2DFactory_;
  ID2D1HwndRenderTarget* pRenderTarget_;
  IDWriteFactory* pDWriteFactory_;
  IDWriteTextFormat* pTextFormat_;
  IDWriteTextLayout* pTextLayout_;

  ID2D1SolidColorBrush* pTextBrush_;
  ID2D1SolidColorBrush* pBgBrush_;
  ID2D1SolidColorBrush* pBorderBrush_;
};
