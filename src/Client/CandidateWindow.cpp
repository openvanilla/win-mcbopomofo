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

#include "CandidateWindow.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <dwmapi.h>

#include "PathCompat.h"
#include "UTFHelper.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "dwmapi.lib")

const wchar_t* const CANDIDATE_WINDOW_CLASS = L"WinMcBopomofoCandidateWindow";

CandidateWindow::CandidateWindow()
    : hwnd_(nullptr),
      dpiScale_(1.0f),
      cursorIndex_(0),
      candidateKeys_(L"123456789"),
      candidateKeysCount_(9),
      isVertical_(false),
      forceVertical_(false),
      selectionStyle_(McBopomofo::IPC::CandidateSelectionStyle::kStandard),
      isDarkMode_(false),
      pD2DFactory_(nullptr),
      pRenderTarget_(nullptr),
      pDWriteFactory_(nullptr),
      pTextFormat_(nullptr),
      pHintFormat_(nullptr),
      pTextLayout_(nullptr),
      pHintLayout_(nullptr),
      pTextBrush_(nullptr),
      pBgBrush_(nullptr),
      pBorderBrush_(nullptr),
      pHighlightBgBrush_(nullptr),
      pHighlightTextBrush_(nullptr) {
  updateTheme_();
  createDeviceIndependentResources_();
}


CandidateWindow::~CandidateWindow() {
  Destroy();
  discardDeviceResources_();
  if (pHintLayout_) {
    pHintLayout_->Release();
  }
  if (pTextLayout_) {
    pTextLayout_->Release();
  }
  if (pHintFormat_) {
    pHintFormat_->Release();
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

float CandidateWindow::getDpiScale_() {
  if (!hwnd_) return 1.0f;
  UINT dpi = 96;
  HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
  auto pGetDpiForWindow =
      (UINT(WINAPI*)(HWND))GetProcAddress(hUser32, "GetDpiForWindow");
  if (pGetDpiForWindow) {
    dpi = pGetDpiForWindow(hwnd_);
  } else {
    HDC hdc = GetDC(hwnd_);
    if (hdc) {
      dpi = GetDeviceCaps(hdc, LOGPIXELSX);
      ReleaseDC(hwnd_, hdc);
    }
  }
  return (float)dpi / 96.0f;
}

void CandidateWindow::createDeviceIndependentResources_() {
  if (!pD2DFactory_) {
    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pD2DFactory_);
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                        reinterpret_cast<IUnknown**>(&pDWriteFactory_));

    pDWriteFactory_->CreateTextFormat(
        L"Microsoft JhengHei UI",  // Good UI font for Traditional Chinese with
                                   // Emoji support
        NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 18.0f, L"zh-TW", &pTextFormat_);

    pDWriteFactory_->CreateTextFormat(
        L"Microsoft JhengHei UI", NULL, DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 10.0f, L"zh-TW",
        &pHintFormat_);
  }
}

void CandidateWindow::createDeviceResources_() {
  if (!pRenderTarget_ && hwnd_) {
    RECT rc;
    GetClientRect(hwnd_, &rc);
    D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

    pD2DFactory_->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(),
        D2D1::HwndRenderTargetProperties(hwnd_, size), &pRenderTarget_);

    if (pRenderTarget_) {
      pRenderTarget_->CreateSolidColorBrush(
          isDarkMode_ ? D2D1::ColorF(0xF0F0F0) : D2D1::ColorF(0x101010),
          &pTextBrush_);
      pRenderTarget_->CreateSolidColorBrush(
          isDarkMode_ ? D2D1::ColorF(0x202020) : D2D1::ColorF(0xFFFFFF),
          &pBgBrush_);
      pRenderTarget_->CreateSolidColorBrush(
          isDarkMode_ ? D2D1::ColorF(0x404040) : D2D1::ColorF(0xCCCCCC),
          &pBorderBrush_);
      pRenderTarget_->CreateSolidColorBrush(
          D2D1::ColorF(0x0078D7),  // Windows Blue
          &pHighlightBgBrush_);
      pRenderTarget_->CreateSolidColorBrush(
          D2D1::ColorF(0xFFFFFF),  // White text on highlight
          &pHighlightTextBrush_);
    }
  }
}

void CandidateWindow::enableDropShadow_() {
  if (!hwnd_) {
    return;
  }
  BOOL enabled = FALSE;
  if (FAILED(DwmIsCompositionEnabled(&enabled)) || !enabled) {
    return;
  }

  const DWMNCRENDERINGPOLICY policy = DWMNCRP_ENABLED;
  DwmSetWindowAttribute(hwnd_, DWMWA_NCRENDERING_POLICY, &policy,
                        sizeof(policy));
}

void CandidateWindow::discardDeviceResources_() {
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
  if (pHighlightBgBrush_) {
    pHighlightBgBrush_->Release();
    pHighlightBgBrush_ = nullptr;
  }
  if (pHighlightTextBrush_) {
    pHighlightTextBrush_->Release();
    pHighlightTextBrush_ = nullptr;
  }
}

void CandidateWindow::updateTheme_() {
  DWORD useLightTheme = 1;
  DWORD size = sizeof(useLightTheme);
  HKEY hKey;
  if (RegOpenKeyExW(
          HKEY_CURRENT_USER,
          L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
          0, KEY_READ, &hKey) == ERROR_SUCCESS) {
    RegQueryValueExW(hKey, L"AppsUseLightTheme", NULL, NULL,
                     (LPBYTE)&useLightTheme, &size);
    RegCloseKey(hKey);
  }
  isDarkMode_ = (useLightTheme == 0);

  if (pRenderTarget_) {
    discardDeviceResources_();  // Force recreate brushes
    InvalidateRect(hwnd_, nullptr, FALSE);
  }
}

void CandidateWindow::onSettingChange_() {
  updateTheme_();
  // Re-trigger layout read
  std::string dir = McBopomofo::fcitx5_compat::userDirectory();
  std::wstring iniPath = McBopomofo::Utf8ToUtf16(dir) + L"\\mcbopomofo.ini";
  isVertical_ = GetPrivateProfileIntW(L"UI", L"CandidateWindowVertical", 0,
                                      iniPath.c_str()) != 0;
  wchar_t keys[32] = {};
  GetPrivateProfileStringW(L"General", L"CandidateKeys", L"123456789", keys, 32,
                           iniPath.c_str());
  candidateKeys_ = keys;
  if (candidateKeys_ != L"123456789" && candidateKeys_ != L"asdfghjkl" &&
      candidateKeys_ != L"asdfzxcvb") {
    candidateKeys_ = L"123456789";
  }
  candidateKeysCount_ = GetPrivateProfileIntW(L"General", L"CandidateKeysCount",
                                              9, iniPath.c_str());
  if (candidateKeysCount_ < 4 || candidateKeysCount_ > 9) {
    candidateKeysCount_ = 9;
  }
}

bool CandidateWindow::Create(HINSTANCE hInstance) {
  if (hwnd_) return true;

  WNDCLASSEXW wcex = {0};
  wcex.cbSize = sizeof(WNDCLASSEXW);
  wcex.style = CS_IME;
  wcex.lpfnWndProc = wndProc_;
  wcex.hInstance = hInstance;
  wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
  wcex.hbrBackground = NULL;  // Handled by D2D
  wcex.lpszClassName = CANDIDATE_WINDOW_CLASS;

  RegisterClassExW(
      &wcex);  // Ignore failure as it might be registered by another instance

  hwnd_ = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE,
                          CANDIDATE_WINDOW_CLASS, L"",
                          WS_POPUP,       // D2D will draw the border
                          0, 0, 100, 30,  // Initial dummy size
                          nullptr, nullptr, hInstance, this);

  onSettingChange_();  // Load initial settings
  enableDropShadow_();

  return hwnd_ != nullptr;
}


void CandidateWindow::Destroy() {
  if (hwnd_) {
    DestroyWindow(hwnd_);
    hwnd_ = nullptr;
  }
}

void CandidateWindow::UpdateUI(const std::vector<std::string>& candidates,
                               int cursorIndex, bool forceVertical,
                               McBopomofo::IPC::CandidateSelectionStyle selectionStyle,
                               const std::string& hint) {
  if (!hwnd_) return;

  dpiScale_ = getDpiScale_();

  candidates_.clear();
  for (const auto& c : candidates) {
    candidates_.push_back(McBopomofo::Utf8ToUtf16(c));
  }
  cursorIndex_ = cursorIndex;
  hint_ = McBopomofo::Utf8ToUtf16(hint);

  if (candidates_.empty()) {
    Hide();
    return;
  }

  // Be defensive about stale or invalid cursor indexes from IPC/state
  // transitions.
  if (cursorIndex_ < 0 ||
      cursorIndex_ >= static_cast<int>(candidates_.size())) {
    cursorIndex_ = 0;
  }

  forceVertical_ = forceVertical;
  selectionStyle_ = selectionStyle;
  rebuildLayoutAndResize_();
}

void CandidateWindow::rebuildLayoutAndResize_() {
  bool drawVertical = isVertical_ || forceVertical_;
  if (candidates_.empty()) {
    return;
  }

  const int pageSize = candidateKeysCount_;
  int pageIndex = cursorIndex_ / pageSize;
  int startIndex = pageIndex * pageSize;
  int endIndex = std::min((int)candidates_.size(), startIndex + pageSize);

  std::wstringstream ss;
  keyRanges_.clear();
  selectedRange_ = {0, 0};
  UINT32 currentPos = 0;

  for (int i = startIndex; i < endIndex; ++i) {
    int displayIndex = i - startIndex;
    std::wstring keyStr;
    if (selectionStyle_ ==
        McBopomofo::IPC::CandidateSelectionStyle::kShiftReturn) {
      keyStr = L"\u21e7\u23ce ";
    } else if (selectionStyle_ ==
               McBopomofo::IPC::CandidateSelectionStyle::kShiftDigits) {
      keyStr = L"\u21e7";
      keyStr += static_cast<wchar_t>(L'1' + displayIndex);
      keyStr += L". ";
    } else {
      wchar_t key = displayIndex < static_cast<int>(candidateKeys_.length())
                        ? candidateKeys_[displayIndex]
                        : L'?';
      keyStr.assign(1, key);
      keyStr += L". ";
    }
    std::wstring candStr = candidates_[i];

    if (i == cursorIndex_) {
      selectedRange_.start = currentPos;
    }

    keyRanges_.push_back({currentPos, (UINT32)keyStr.length()});
    ss << keyStr;
    currentPos += (UINT32)keyStr.length();

    ss << candStr;
    currentPos += (UINT32)candStr.length();

    if (i == cursorIndex_) {
      selectedRange_.length = currentPos - selectedRange_.start;
    }

    if (i < endIndex - 1) {
      std::wstring sep = (drawVertical ? L"\n" : L"   ");
      ss << sep;
      currentPos += (UINT32)sep.length();
    }
  }

  // Add page indicator if there are multiple pages
  if (static_cast<int>(candidates_.size()) > pageSize) {
    int totalPages =
        (static_cast<int>(candidates_.size()) + pageSize - 1) / pageSize;
    std::wstring indStr = (drawVertical ? L"\n(" : L"  (");
    indStr += std::to_wstring(pageIndex + 1) + L"/" +
              std::to_wstring(totalPages) + L")";

    keyRanges_.push_back({currentPos, (UINT32)indStr.length()});
    ss << indStr;
    currentPos += (UINT32)indStr.length();
  }

  displayString_ = ss.str();

  if (pTextLayout_) {
    pTextLayout_->Release();
    pTextLayout_ = nullptr;
  }
  if (pHintLayout_) {
    pHintLayout_->Release();
    pHintLayout_ = nullptr;
  }

  if (pDWriteFactory_ && pTextFormat_) {
    pDWriteFactory_->CreateTextLayout(
        displayString_.c_str(), (UINT32)displayString_.length(), pTextFormat_,
        10000.0f, 10000.0f, &pTextLayout_);

    if (pTextLayout_) {
      for (const auto& range : keyRanges_) {
        DWRITE_TEXT_RANGE dwriteRange = {range.start, range.length};
        pTextLayout_->SetFontFamilyName(L"Segoe UI", dwriteRange);
        pTextLayout_->SetFontSize(15.0f, dwriteRange);
      }
    }
  }

  if (pDWriteFactory_ && pHintFormat_ && !hint_.empty()) {
    pDWriteFactory_->CreateTextLayout(hint_.c_str(), (UINT32)hint_.length(),
                                      pHintFormat_, 10000.0f, 10000.0f,
                                      &pHintLayout_);
  }

  float textWidth = 0, textHeight = 0;
  if (pTextLayout_) {
    DWRITE_TEXT_METRICS metrics;
    pTextLayout_->GetMetrics(&metrics);
    textWidth = metrics.width;
    textHeight = metrics.height;
  }

  float hintWidth = 0, hintHeight = 0;
  if (pHintLayout_) {
    DWRITE_TEXT_METRICS metrics;
    pHintLayout_->GetMetrics(&metrics);
    hintWidth = metrics.width;
    hintHeight = metrics.height;
  }

  int width = (int)std::ceil(std::max(textWidth, hintWidth) * dpiScale_) +
              (int)(24 * dpiScale_);
  int height = (int)std::ceil((textHeight + hintHeight) * dpiScale_) +
               (int)(16 * dpiScale_);

  if (pHintLayout_) {
    height += (int)(4 * dpiScale_);  // Gap between hint and candidates
  }

  // Enforce a minimum size to prevent the window from collapsing or being
  // rejected by the OS
  width = std::max(width, (int)(50 * dpiScale_));
  height = std::max(height, (int)(24 * dpiScale_));

  SetWindowPos(hwnd_, HWND_TOPMOST, 0, 0, width, height,
               SWP_NOMOVE | SWP_NOACTIVATE);
  if (pRenderTarget_) {
    pRenderTarget_->Resize(D2D1::SizeU(width, height));
  }
  ShowWindow(hwnd_, SW_SHOWNOACTIVATE);
  InvalidateRect(hwnd_, nullptr, FALSE);
}

void CandidateWindow::Move(int x, int y) {
  if (hwnd_) {
    const float oldScale = dpiScale_;
    SetWindowPos(hwnd_, HWND_TOPMOST, x, y, 0, 0,
                 SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
    dpiScale_ = getDpiScale_();
    if (std::abs(dpiScale_ - oldScale) > 0.001f) {
      rebuildLayoutAndResize_();
    } else {
      InvalidateRect(hwnd_, nullptr, FALSE);
    }
  }
}

void CandidateWindow::Hide() {
  if (hwnd_) {
    ShowWindow(hwnd_, SW_HIDE);
  }
}

LRESULT CALLBACK CandidateWindow::wndProc_(HWND hwnd, UINT uMsg, WPARAM wParam,
                                          LPARAM lParam) {
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
      return pThis->onPaint_(hwnd);
    } else if (uMsg == WM_SETTINGCHANGE) {
      pThis->onSettingChange_();
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

LRESULT CandidateWindow::onPaint_(HWND hwnd) {
  PAINTSTRUCT ps;
  BeginPaint(hwnd, &ps);


  createDeviceResources_();
  if (pRenderTarget_) {
    pRenderTarget_->BeginDraw();
    pRenderTarget_->SetTransform(D2D1::Matrix3x2F::Scale(dpiScale_, dpiScale_));
    pRenderTarget_->Clear(pBgBrush_->GetColor());

    if (pTextBrush_) {
      float currentY = 8.0f;

      if (pHintLayout_) {
        pRenderTarget_->DrawTextLayout(D2D1::Point2F(12.0f, currentY),
                                       pHintLayout_, pTextBrush_);

        DWRITE_TEXT_METRICS hintMetrics;
        pHintLayout_->GetMetrics(&hintMetrics);
        currentY += hintMetrics.height + 4.0f;
      }

      if (pTextLayout_) {
        // Draw highlight background if we have a selected range
        if (selectedRange_.length > 0 && pHighlightBgBrush_) {
          UINT32 actualHitTestCount = 0;
          pTextLayout_->HitTestTextRange(selectedRange_.start,
                                         selectedRange_.length, 0, 0, nullptr, 0,
                                         &actualHitTestCount);

          if (actualHitTestCount > 0) {
            std::vector<DWRITE_HIT_TEST_METRICS> hitTestMetrics(
                actualHitTestCount);
            pTextLayout_->HitTestTextRange(
                selectedRange_.start, selectedRange_.length, 12.0f, currentY,
                hitTestMetrics.data(), actualHitTestCount, &actualHitTestCount);

            float layoutWidth = 0;
            bool isVerticalLayout = isVertical_ || forceVertical_;
            if (isVerticalLayout) {
              DWRITE_TEXT_METRICS textMetrics;
              pTextLayout_->GetMetrics(&textMetrics);
              layoutWidth = textMetrics.width;
            }

            for (const auto& metrics : hitTestMetrics) {
              float right = metrics.left + metrics.width;
              if (isVerticalLayout) {
                right = 12.0f + layoutWidth;
              }

              D2D1_RECT_F rect = D2D1::RectF(
                  metrics.left - 4.0f, metrics.top - 2.0f, right + 4.0f,
                  metrics.top + metrics.height + 2.0f);
              pRenderTarget_->FillRectangle(rect, pHighlightBgBrush_);
            }
          }

          // Apply highlight text color effect
          DWRITE_TEXT_RANGE dwriteRange = {selectedRange_.start,
                                           selectedRange_.length};
          pTextLayout_->SetDrawingEffect(pHighlightTextBrush_, dwriteRange);
        }

        pRenderTarget_->DrawTextLayout(
            D2D1::Point2F(12.0f, currentY), pTextLayout_, pTextBrush_,
            D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);

        // Revert drawing effect
        if (selectedRange_.length > 0) {
          DWRITE_TEXT_RANGE dwriteRange = {selectedRange_.start,
                                           selectedRange_.length};
          pTextLayout_->SetDrawingEffect(nullptr, dwriteRange);
        }
      }
    }

    // Draw border
    D2D1_SIZE_F size = pRenderTarget_->GetSize();
    // border should be in pixels, but SetTransform is active.
    // We should probably draw the border without transform or compensate.
    pRenderTarget_->SetTransform(D2D1::Matrix3x2F::Identity());
    pRenderTarget_->DrawRectangle(
        D2D1::RectF(0.5f, 0.5f, size.width - 0.5f, size.height - 0.5f),
        pBorderBrush_, 1.0f);

    HRESULT hr = pRenderTarget_->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) {
      discardDeviceResources_();
    }
  }

  EndPaint(hwnd, &ps);
  return 0;
}
