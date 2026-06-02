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
#include <vector>

#include "Ipc.h"

// CandidateWindow is a per-monitor-DPI-aware popup rendered with
// Direct2D/DirectWrite.
//
//  It tracks the current scale with GetDpiScaleForWindow(), refreshes the scale
// on Move() and WM_DPICHANGED, rebuilds text layouts in DIPs, then converts the
// measured size back to physical pixels before resizing the HWND. Painting
// happens in onPaint_(): create/recreate the HWND render target and brushes,
// apply a DPI scale transform, clear the background, draw the optional hint and
// candidate text layouts, paint the selected range highlight, then draw the
// 1-pixel border in device space.
class CandidateWindow {
 public:
  // Some hosts, notably CoreWindow-based apps and Chromium browsers, do not
  // reliably show the popup when it is rendered with our D2D path even though
  // the HWND is created, sized, and moved correctly. Use GDI as a compatibility
  // renderer for those hosts and keep D2D for normal desktop windows.
  enum class RenderMode {
    kD2D,
    kGDI,
  };

  struct UpdateUIRequest {
    std::vector<std::string> candidates;
    std::string hint;

    int cursorIndex = 0;
    McBopomofo::IPC::CandidateSelectionStyle selectionStyle =
        McBopomofo::IPC::CandidateSelectionStyle::kStandard;

    bool forceVertical = false;
    bool candidateWindowVertical = false;

    int candidateFontSize = 16;
    std::string candidateKeys = "123456789";
    int candidateKeysCount = 9;

    McBopomofo::IPC::CandidateWindowColors colors;
  };

  CandidateWindow();
  ~CandidateWindow();

  bool Create(HINSTANCE hInstance);
  void Destroy();
  void SetOwnerWindow(HWND ownerHwnd);

  void UpdateUI(const UpdateUIRequest& request);
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

  // For testing purposes
  std::wstring GetDisplayString() const { return displayString_; }
  void SetVertical(bool vertical) { isVertical_ = vertical; }

 private:
  static LRESULT CALLBACK wndProc_(HWND hwnd, UINT uMsg, WPARAM wParam,
                                   LPARAM lParam);
  LRESULT onPaint_(HWND hwnd);
  void onSettingChange_();

  void createDeviceIndependentResources_();
  void createDeviceResources_();
  void discardDeviceResources_();
  void rebuildLayoutAndResize_();
  bool recreateWindow_();
  void enableSystemRoundedCorners_();
  void updateRoundedRegion_();
  void applyCandidateWindowSettings_(
      bool vertical, const std::string& candidateKeys, int candidateKeysCount,
      const McBopomofo::IPC::CandidateWindowColors& colors);
  void reloadServerControlledSettings_();
  void updateRenderMode_();

  struct TextRange {
    UINT32 start;
    UINT32 length;
  };

  HWND hwnd_;
  HWND ownerHwnd_;
  HINSTANCE hInstance_;
  RenderMode renderMode_;
  float dpiScale_;
  std::vector<std::wstring> candidates_;
  int cursorIndex_;
  std::wstring displayString_;
  std::wstring hint_;
  std::wstring candidateKeys_;
  int candidateKeysCount_;
  bool isVertical_;
  bool forceVertical_;
  McBopomofo::IPC::CandidateSelectionStyle selectionStyle_;
  int candidateFontSize_;
  McBopomofo::IPC::CandidateWindowColors colors_;

  TextRange selectedRange_;
  std::vector<TextRange> keyRanges_;

  ID2D1Factory* pD2DFactory_;
  ID2D1HwndRenderTarget* pRenderTarget_;
  IDWriteFactory* pDWriteFactory_;
  IDWriteTextFormat* pTextFormat_;
  IDWriteTextFormat* pHintFormat_;
  IDWriteTextLayout* pTextLayout_;
  IDWriteTextLayout* pHintLayout_;

  ID2D1SolidColorBrush* pTextBrush_;
  ID2D1SolidColorBrush* pBgBrush_;
  ID2D1SolidColorBrush* pBorderBrush_;
  ID2D1SolidColorBrush* pHighlightBgBrush_;
  ID2D1SolidColorBrush* pHighlightTextBrush_;
};
