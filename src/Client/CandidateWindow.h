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

class CandidateWindow {
 public:
  CandidateWindow();
  ~CandidateWindow();

  bool Create(HINSTANCE hInstance);
  void Destroy();

  void UpdateUI(const std::vector<std::string>& candidates, int cursorIndex,
                bool forceVertical = false, bool useShiftKeySelection = false,
                const std::string& hint = "");
  void Move(int x, int y);
  void Hide();

  bool IsVisible() const { return _hwnd && IsWindowVisible(_hwnd); }
  int GetHeight() const {
    if (!_hwnd) return 0;
    RECT rc;
    GetWindowRect(_hwnd, &rc);
    return rc.bottom - rc.top;
  }

  // For testing purposes
  std::wstring GetDisplayString() const { return _displayString; }
  void SetVertical(bool vertical) { _isVertical = vertical; }

 private:
  static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                  LPARAM lParam);
  LRESULT OnPaint(HWND hwnd);
  void OnSettingChange();

  void CreateDeviceIndependentResources();
  void CreateDeviceResources();
  void DiscardDeviceResources();
  void UpdateTheme();
  float GetDpiScale();

  struct TextRange {
    UINT32 start;
    UINT32 length;
  };

  HWND _hwnd;
  float _dpiScale;
  std::vector<std::wstring> _candidates;
  int _cursorIndex;
  std::wstring _displayString;
  std::wstring _hint;
  std::wstring _candidateKeys;
  int _candidateKeysCount;
  bool _isVertical;
  bool _forceVertical;
  bool _useShiftKeySelection;
  bool _isDarkMode;

  TextRange _selectedRange;
  std::vector<TextRange> _keyRanges;

  ID2D1Factory* _pD2DFactory;
  ID2D1HwndRenderTarget* _pRenderTarget;
  IDWriteFactory* _pDWriteFactory;
  IDWriteTextFormat* _pTextFormat;
  IDWriteTextFormat* _pHintFormat;
  IDWriteTextLayout* _pTextLayout;
  IDWriteTextLayout* _pHintLayout;

  ID2D1SolidColorBrush* _pTextBrush;
  ID2D1SolidColorBrush* _pBgBrush;
  ID2D1SolidColorBrush* _pBorderBrush;
  ID2D1SolidColorBrush* _pHighlightBgBrush;
  ID2D1SolidColorBrush* _pHighlightTextBrush;
};
