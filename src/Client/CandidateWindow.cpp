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

#include <dwmapi.h>

#include <algorithm>
#include <cmath>
#include <cwchar>
#include <sstream>
#include <string_view>

#include "Globals.h"
#include "NamedPipe.h"
#include "UTFHelper.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "dwmapi.lib")

const wchar_t* const CANDIDATE_WINDOW_CLASS = L"WinMcBopomofoCandidateWindow";

#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif

#ifndef DWMWCP_ROUND
#define DWMWCP_DEFAULT 0
#define DWMWCP_DONOTROUND 1
#define DWMWCP_ROUND 2
#define DWMWCP_ROUNDSMALL 3
#endif

namespace {

#ifndef DWMWA_BORDER_COLOR
#define DWMWA_BORDER_COLOR 34
#endif

D2D1_COLOR_F D2DColorFromRgb(uint32_t rgb) { return D2D1::ColorF(rgb); }

COLORREF ToColorRef(uint32_t rgb) {
  return RGB((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
}

bool IsCoreWindowHost(HWND hwnd) {
  if (!hwnd) {
    return false;
  }
  wchar_t className[128] = {};
  if (!GetClassNameW(hwnd, className, static_cast<int>(std::size(className)))) {
    return false;
  }
  return wcscmp(className, L"Windows.UI.Core.CoreWindow") == 0;
}

bool IsEmojiCodePoint(char32_t cp) {
  return (cp >= 0x1F300 && cp <= 0x1FAFF) || (cp >= 0x2600 && cp <= 0x27BF) ||
         (cp >= 0xFE00 && cp <= 0xFE0F);
}

struct GdiFontSet {
  HFONT textFont = nullptr;
  HFONT keyFont = nullptr;
  HFONT hintFont = nullptr;
  HFONT emojiFont = nullptr;
  HFONT keyEmojiFont = nullptr;
  HFONT hintEmojiFont = nullptr;
};

HFONT CreateUiFont(const wchar_t* faceName, LONG height,
                   LONG weight = FW_NORMAL) {
  LOGFONTW lf = {};
  lf.lfHeight = height;
  lf.lfWeight = weight;
  lf.lfQuality = CLEARTYPE_QUALITY;
  wcscpy_s(lf.lfFaceName, faceName);
  return CreateFontIndirectW(&lf);
}

GdiFontSet CreateGdiFontSet(float dpiScale, int candidateFontSize) {
  const LONG textHeight = -std::max(
      12L, static_cast<LONG>(std::lround(candidateFontSize * dpiScale)));
  const LONG keyHeight = -std::max(
      10L, static_cast<LONG>(std::lround((candidateFontSize - 3) * dpiScale)));
  const LONG hintHeight = -std::max(
      11L, static_cast<LONG>(std::lround((candidateFontSize - 3) * dpiScale)));

  GdiFontSet fonts;
  fonts.textFont = CreateUiFont(L"Microsoft JhengHei UI", textHeight);
  fonts.keyFont = CreateUiFont(L"Segoe UI", keyHeight);
  fonts.hintFont = CreateUiFont(L"Microsoft JhengHei UI", hintHeight);
  fonts.emojiFont = CreateUiFont(L"Segoe UI Emoji", textHeight);
  fonts.keyEmojiFont = CreateUiFont(L"Segoe UI Emoji", keyHeight);
  fonts.hintEmojiFont = CreateUiFont(L"Segoe UI Emoji", hintHeight);
  return fonts;
}

void DestroyGdiFontSet(GdiFontSet& fonts) {
  if (fonts.textFont) DeleteObject(fonts.textFont);
  if (fonts.keyFont) DeleteObject(fonts.keyFont);
  if (fonts.hintFont) DeleteObject(fonts.hintFont);
  if (fonts.emojiFont) DeleteObject(fonts.emojiFont);
  if (fonts.keyEmojiFont) DeleteObject(fonts.keyEmojiFont);
  if (fonts.hintEmojiFont) DeleteObject(fonts.hintEmojiFont);
}

int DrawOrMeasureTextRun(HDC hdc, std::wstring_view text, int x, int y,
                         HFONT primaryFont, HFONT emojiFont, COLORREF color,
                         bool draw) {
  int cursorX = x;
  SetBkMode(hdc, TRANSPARENT);
  SetTextColor(hdc, color);

  size_t i = 0;
  while (i < text.size()) {
    size_t start = i;
    char32_t cp = text[i];
    size_t cpLen = 1;
    if (i + 1 < text.size() && IS_HIGH_SURROGATE(text[i]) &&
        IS_LOW_SURROGATE(text[i + 1])) {
      cp = (((text[i] - 0xD800) << 10) | (text[i + 1] - 0xDC00)) + 0x10000;
      cpLen = 2;
    }
    const bool useEmoji = IsEmojiCodePoint(cp);
    i += cpLen;
    while (i < text.size()) {
      char32_t nextCp = text[i];
      size_t nextLen = 1;
      if (i + 1 < text.size() && IS_HIGH_SURROGATE(text[i]) &&
          IS_LOW_SURROGATE(text[i + 1])) {
        nextCp =
            (((text[i] - 0xD800) << 10) | (text[i + 1] - 0xDC00)) + 0x10000;
        nextLen = 2;
      }
      if (IsEmojiCodePoint(nextCp) != useEmoji) {
        break;
      }
      i += nextLen;
    }

    std::wstring_view run = text.substr(start, i - start);
    HFONT runFont = useEmoji ? emojiFont : primaryFont;
    HGDIOBJ oldFont = SelectObject(hdc, runFont ? runFont : primaryFont);
    SIZE size = {};
    GetTextExtentPoint32W(hdc, run.data(), static_cast<int>(run.size()), &size);
    if (draw) {
      TextOutW(hdc, cursorX, y, run.data(), static_cast<int>(run.size()));
    }
    SelectObject(hdc, oldFont);
    cursorX += size.cx;
  }
  return cursorX - x;
}

std::vector<std::pair<UINT32, std::wstring_view>> SplitDisplayLines(
    const std::wstring& text) {
  std::vector<std::pair<UINT32, std::wstring_view>> lines;
  size_t lineStart = 0;
  while (lineStart <= text.size()) {
    size_t lineEnd = text.find(L'\n', lineStart);
    if (lineEnd == std::wstring::npos) {
      lineEnd = text.size();
    }
    lines.emplace_back(
        static_cast<UINT32>(lineStart),
        std::wstring_view(text).substr(lineStart, lineEnd - lineStart));
    if (lineEnd == text.size()) {
      break;
    }
    lineStart = lineEnd + 1;
  }
  return lines;
}

}  // namespace

CandidateWindow::CandidateWindow()
    : hwnd_(nullptr),
      ownerHwnd_(nullptr),
      hInstance_(nullptr),
      renderMode_(RenderMode::kD2D),
      dpiScale_(1.0f),
      cursorIndex_(0),
      candidateKeys_(L"123456789"),
      candidateKeysCount_(9),
      isVertical_(false),
      forceVertical_(false),
      selectionStyle_(McBopomofo::IPC::CandidateSelectionStyle::kStandard),
      candidateFontSize_(16),
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

void CandidateWindow::createDeviceIndependentResources_() {
  if (!pD2DFactory_) {
    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pD2DFactory_);
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_ISOLATED, __uuidof(IDWriteFactory),
                        reinterpret_cast<IUnknown**>(&pDWriteFactory_));
  }

  if (pTextFormat_) {
    pTextFormat_->Release();
    pTextFormat_ = nullptr;
  }
  if (pHintFormat_) {
    pHintFormat_->Release();
    pHintFormat_ = nullptr;
  }

  if (pDWriteFactory_) {
    pDWriteFactory_->CreateTextFormat(
        L"Microsoft JhengHei UI",  // Good UI font for Traditional Chinese with
                                   // Emoji support
        NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, static_cast<FLOAT>(candidateFontSize_),
        L"zh-TW", &pTextFormat_);

    pDWriteFactory_->CreateTextFormat(
        L"Microsoft JhengHei UI", NULL, DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        std::max(12.0f, static_cast<float>(candidateFontSize_) - 4.0f),
        L"zh-TW", &pHintFormat_);
  }
}

void CandidateWindow::createDeviceResources_() {
  if (!pRenderTarget_ && hwnd_) {
    RECT rc;
    GetClientRect(hwnd_, &rc);
    D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

    pD2DFactory_->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT,
                                     D2D1::PixelFormat(), 96.0f, 96.0f),
        D2D1::HwndRenderTargetProperties(hwnd_, size), &pRenderTarget_);

    if (pRenderTarget_) {
      pRenderTarget_->CreateSolidColorBrush(D2DColorFromRgb(colors_.text),
                                            &pTextBrush_);
      pRenderTarget_->CreateSolidColorBrush(D2DColorFromRgb(colors_.background),
                                            &pBgBrush_);
      pRenderTarget_->CreateSolidColorBrush(D2DColorFromRgb(colors_.border),
                                            &pBorderBrush_);
      pRenderTarget_->CreateSolidColorBrush(
          D2DColorFromRgb(colors_.highlightBackground), &pHighlightBgBrush_);
      pRenderTarget_->CreateSolidColorBrush(
          D2DColorFromRgb(colors_.highlightText), &pHighlightTextBrush_);
    }
  }
}

void CandidateWindow::enableSystemRoundedCorners_() {
  if (!hwnd_) {
    return;
  }

  const auto cornerPreference =
      static_cast<DWM_WINDOW_CORNER_PREFERENCE>(DWMWCP_ROUND);
  DwmSetWindowAttribute(hwnd_, DWMWA_WINDOW_CORNER_PREFERENCE,
                        &cornerPreference, sizeof(cornerPreference));
}

void CandidateWindow::updateRoundedRegion_() {
  if (!hwnd_) {
    return;
  }

  RECT rc;
  GetClientRect(hwnd_, &rc);
  const int width = rc.right - rc.left;
  const int height = rc.bottom - rc.top;
  if (width <= 0 || height <= 0) {
    return;
  }

  const int radius =
      std::max(6, static_cast<int>(std::lround(8.0f * dpiScale_)));
  HRGN region =
      CreateRoundRectRgn(0, 0, width + 1, height + 1, radius * 2, radius * 2);
  if (region) {
    SetWindowRgn(hwnd_, region, TRUE);
  }
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

void CandidateWindow::onSettingChange_() {
  InvalidateRect(hwnd_, nullptr, FALSE);
}

void CandidateWindow::applyCandidateWindowSettings_(
    bool vertical, const std::string& candidateKeys, int candidateKeysCount,
    const McBopomofo::IPC::CandidateWindowColors& colors) {
  isVertical_ = vertical;

  std::wstring keys = McBopomofo::Utf8ToUtf16(candidateKeys);
  if (keys != L"123456789" && keys != L"asdfghjkl" && keys != L"asdfzxcvb") {
    keys = L"123456789";
  }
  candidateKeys_ = keys;

  candidateKeysCount_ = candidateKeysCount >= 4 && candidateKeysCount <= 9
                            ? candidateKeysCount
                            : 9;

  if (colors_.text != colors.text || colors_.background != colors.background ||
      colors_.border != colors.border ||
      colors_.highlightBackground != colors.highlightBackground ||
      colors_.highlightText != colors.highlightText) {
    colors_ = colors;
    discardDeviceResources_();
  }
}

void CandidateWindow::reloadServerControlledSettings_() {
  McBopomofo::IPC::NamedPipeClient pipe(McBopomofo::IPC::PIPE_NAME);
  std::string response;
  if (!pipe.Call(McBopomofo::IPC::SerializeReloadSettings(), response)) {
    return;
  }

  McBopomofo::IPC::StateUpdatePayload state;
  if (!McBopomofo::IPC::DeserializeStateUpdate(response, state)) {
    return;
  }

  applyCandidateWindowSettings_(state.candidateWindowVertical,
                                state.candidateKeys, state.candidateKeysCount,
                                state.candidateWindowColors);
  InvalidateRect(hwnd_, nullptr, FALSE);
}

bool CandidateWindow::Create(HINSTANCE hInstance) {
  if (hInstance) {
    hInstance_ = hInstance;
  }
  if (hwnd_) return true;
  if (!hInstance_) return false;

  WNDCLASSEXW wcex = {0};
  wcex.cbSize = sizeof(WNDCLASSEXW);
  wcex.style = CS_IME | CS_DROPSHADOW;
  wcex.lpfnWndProc = wndProc_;
  wcex.hInstance = hInstance_;
  wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
  wcex.hbrBackground = NULL;  // Handled by D2D
  wcex.lpszClassName = CANDIDATE_WINDOW_CLASS;

  RegisterClassExW(
      &wcex);  // Ignore failure as it might be registered by another instance

  hwnd_ = CreateWindowExW(
      WS_EX_TOOLWINDOW | WS_EX_TOPMOST, CANDIDATE_WINDOW_CLASS, L"",
      WS_POPUP | WS_CLIPCHILDREN, 0, 0, 100, 30,  // Initial dummy size
      ownerHwnd_, nullptr, hInstance_, this);

  return hwnd_ != nullptr;
}

bool CandidateWindow::recreateWindow_() {
  if (!hInstance_) {
    return false;
  }

  bool wasVisible = IsVisible();
  RECT rc = {0};
  if (hwnd_) {
    GetWindowRect(hwnd_, &rc);
    Destroy();
  }

  if (!Create(hInstance_)) {
    // LogMessage("CandidateWindow recreate failed owner=%p", ownerHwnd_);
    return false;
  }

  if (rc.right > rc.left && rc.bottom > rc.top) {
    SetWindowPos(hwnd_, HWND_TOPMOST, rc.left, rc.top, rc.right - rc.left,
                 rc.bottom - rc.top, SWP_NOACTIVATE);
  }

  // LogMessage("CandidateWindow recreated hwnd=%p owner=%p wasVisible=%d",
  // hwnd_,
  //            ownerHwnd_, wasVisible ? 1 : 0);
  return true;
}

void CandidateWindow::updateRenderMode_() {
  const RenderMode newMode =
      IsCoreWindowHost(ownerHwnd_) ? RenderMode::kGDI : RenderMode::kD2D;
  if (renderMode_ != newMode) {
    renderMode_ = newMode;
    // LogMessage("CandidateWindow renderer=%s owner=%p",
    //            renderMode_ == RenderMode::kGDI ? "GDI" : "D2D", ownerHwnd_);
  }
}

void CandidateWindow::SetOwnerWindow(HWND ownerHwnd) {
  if (ownerHwnd && !IsWindow(ownerHwnd)) {
    ownerHwnd = nullptr;
  }
  if (ownerHwnd == hwnd_) {
    ownerHwnd = nullptr;
  }
  if (ownerHwnd_ == ownerHwnd) {
    return;
  }

  const HWND previousOwner = ownerHwnd_;
  ownerHwnd_ = ownerHwnd;
  updateRenderMode_();
  if (!hwnd_) {
    return;
  }

  if (previousOwner != ownerHwnd_ && recreateWindow_()) {
    return;
  }

  SetWindowLongPtrW(hwnd_, GWLP_HWNDPARENT,
                    reinterpret_cast<LONG_PTR>(ownerHwnd_));
  // LogMessage("CandidateWindow owner updated hwnd=%p owner=%p", hwnd_,
  //            ownerHwnd_);
}

void CandidateWindow::Destroy() {
  if (hwnd_) {
    DestroyWindow(hwnd_);
    hwnd_ = nullptr;
  }
}

void CandidateWindow::UpdateUI(const UpdateUIRequest& request) {
  if (!hwnd_) return;

  // LogMessage(
  //     "CandidateWindow UpdateUI hwnd=%p owner=%p candidates=%llu "
  //     "cursorIndex=%d forceVertical=%d vertical=%d hintLen=%llu",
  //     hwnd_, ownerHwnd_,
  //     static_cast<unsigned long long>(request.candidates.size()),
  //     request.cursorIndex, request.forceVertical ? 1 : 0,
  //     request.candidateWindowVertical ? 1 : 0,
  //     static_cast<unsigned long long>(request.hint.size()));

  applyCandidateWindowSettings_(request.candidateWindowVertical,
                                request.candidateKeys,
                                request.candidateKeysCount, request.colors);

  dpiScale_ = GetDpiScaleForWindow(hwnd_);

  candidates_.clear();
  for (const auto& c : request.candidates) {
    candidates_.push_back(McBopomofo::Utf8ToUtf16(c));
  }
  cursorIndex_ = request.cursorIndex;
  hint_ = McBopomofo::Utf8ToUtf16(request.hint);

  if (request.candidates.empty()) {
    // LogMessage("CandidateWindow UpdateUI empty candidates -> hide");
    Hide();
    return;
  }

  // Be defensive about stale or invalid cursor indexes from IPC/state
  // transitions.
  if (cursorIndex_ < 0 ||
      cursorIndex_ >= static_cast<int>(candidates_.size())) {
    cursorIndex_ = 0;
  }

  forceVertical_ = request.forceVertical;
  selectionStyle_ = request.selectionStyle;
  if (candidateFontSize_ != request.candidateFontSize) {
    candidateFontSize_ = request.candidateFontSize;
    createDeviceIndependentResources_();
  }
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
        pTextLayout_->SetFontSize(
            std::max(10.0f, static_cast<float>(candidateFontSize_) - 3.0f),
            dwriteRange);
      }
    }
  }

  if (pDWriteFactory_ && pHintFormat_ && !hint_.empty()) {
    pDWriteFactory_->CreateTextLayout(hint_.c_str(), (UINT32)hint_.length(),
                                      pHintFormat_, 10000.0f, 10000.0f,
                                      &pHintLayout_);
  }

  int width = 0;
  int height = 0;
  if (renderMode_ == RenderMode::kGDI) {
    HDC screenDc = GetDC(nullptr);
    if (screenDc) {
      GdiFontSet fonts = CreateGdiFontSet(dpiScale_, candidateFontSize_);
      const int horizontalPadding =
          static_cast<int>(std::lround(24.0f * dpiScale_));
      const int verticalPadding =
          static_cast<int>(std::lround(16.0f * dpiScale_));
      const int hintGap = static_cast<int>(std::lround(4.0f * dpiScale_));

      auto fontLineHeight = [screenDc](HFONT font) -> int {
        if (!font) {
          return 0;
        }
        HGDIOBJ oldFont = SelectObject(screenDc, font);
        TEXTMETRICW tm = {};
        GetTextMetricsW(screenDc, &tm);
        SelectObject(screenDc, oldFont);
        return tm.tmHeight + tm.tmExternalLeading;
      };

      auto measureCandidateLine = [&](UINT32 globalStart,
                                      std::wstring_view lineText) {
        int measuredWidth = 0;
        size_t localPos = 0;
        while (localPos < lineText.size()) {
          const UINT32 globalPos = globalStart + static_cast<UINT32>(localPos);
          bool inKeyRange = false;
          size_t nextBoundary = lineText.size();
          for (const auto& range : keyRanges_) {
            const UINT32 rangeStart = range.start;
            const UINT32 rangeEnd = range.start + range.length;
            if (globalPos >= rangeStart && globalPos < rangeEnd) {
              inKeyRange = true;
              nextBoundary = std::min(
                  nextBoundary, static_cast<size_t>(rangeEnd - globalStart));
              break;
            }
            if (rangeStart > globalPos) {
              nextBoundary = std::min(
                  nextBoundary, static_cast<size_t>(rangeStart - globalStart));
            }
          }

          if (nextBoundary <= localPos) {
            nextBoundary = localPos + 1;
          }

          std::wstring_view segment =
              lineText.substr(localPos, nextBoundary - localPos);
          measuredWidth += DrawOrMeasureTextRun(
              screenDc, segment, 0, 0,
              inKeyRange ? fonts.keyFont : fonts.textFont,
              inKeyRange ? fonts.keyEmojiFont : fonts.emojiFont,
              ToColorRef(colors_.text), false);
          localPos = nextBoundary;
        }
        return measuredWidth;
      };

      const int textLineHeight = std::max(fontLineHeight(fonts.textFont),
                                          fontLineHeight(fonts.keyFont));
      const int hintLineHeight = fontLineHeight(fonts.hintFont);
      const int lineGap = static_cast<int>(std::lround(4.0f * dpiScale_));

      int contentWidth = 0;
      int contentHeight = 0;
      if (!hint_.empty()) {
        contentWidth =
            std::max(contentWidth,
                     DrawOrMeasureTextRun(screenDc, hint_, 0, 0, fonts.hintFont,
                                          fonts.hintEmojiFont,
                                          ToColorRef(colors_.text), false));
        contentHeight += std::max(hintLineHeight, 14);
      }

      const auto lines = SplitDisplayLines(displayString_);
      if (!lines.empty() && !hint_.empty()) {
        contentHeight += hintGap;
      }
      for (size_t i = 0; i < lines.size(); ++i) {
        const auto& [globalStart, lineText] = lines[i];
        contentWidth =
            std::max(contentWidth, measureCandidateLine(globalStart, lineText));
        contentHeight += textLineHeight;
        if (i + 1 < lines.size()) {
          contentHeight += lineGap;
        }
      }

      width = contentWidth + horizontalPadding;
      height = contentHeight + verticalPadding;
      DestroyGdiFontSet(fonts);
      ReleaseDC(nullptr, screenDc);
    }
  }

  if (width == 0 || height == 0) {
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

    width = (int)std::ceil(std::max(textWidth, hintWidth) * dpiScale_) +
            (int)(24 * dpiScale_);
    height = (int)std::ceil((textHeight + hintHeight) * dpiScale_) +
             (int)(16 * dpiScale_);

    if (pHintLayout_) {
      height += (int)(4 * dpiScale_);
    }
  }

  // Enforce a minimum size to prevent the window from collapsing or being
  // rejected by the OS
  width = std::max(width, (int)(50 * dpiScale_));
  height = std::max(height, (int)(24 * dpiScale_));

  // LogMessage(
  //     "CandidateWindow layout hwnd=%p width=%d height=%d dpi=%.3f
  //     pageIndex=%d " "pageSize=%d totalCandidates=%llu selectedStart=%lu
  //     selectedLength=%lu", hwnd_, width, height, dpiScale_, pageIndex,
  //     pageSize, static_cast<unsigned long long>(candidates_.size()),
  //     static_cast<unsigned long>(selectedRange_.start),
  //     static_cast<unsigned long>(selectedRange_.length));

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
    // LogMessage("CandidateWindow Move hwnd=%p owner=%p x=%d y=%d", hwnd_,
    //            ownerHwnd_, x, y);
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

void CandidateWindow::Hide() {
  if (hwnd_) {
    LogMessage("CandidateWindow Hide hwnd=%p owner=%p", hwnd_, ownerHwnd_);
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
    } else if (uMsg == WM_ERASEBKGND) {
      return 1;
    } else if (uMsg == WM_SETTINGCHANGE) {
      if (lParam != 0 && wcscmp(reinterpret_cast<LPCWSTR>(lParam),
                                L"ImmersiveColorSet") == 0) {
        pThis->reloadServerControlledSettings_();
        return 0;
      }
      pThis->onSettingChange_();
    } else if (uMsg == WM_DWMCOLORIZATIONCOLORCHANGED) {
      pThis->reloadServerControlledSettings_();
      return 0;
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
  HDC hdc = BeginPaint(hwnd, &ps);

  RenderMode activeMode = renderMode_;
  if (activeMode == RenderMode::kD2D) {
    createDeviceResources_();
    // Fall back to GDI rendering if Direct2D initialization fails.
    // This is crucial in sandboxed processes (like Microsoft Edge or Chrome)
    // where GPU/Direct3D device recreation can be blocked after device loss.
    if (!pRenderTarget_) {
      activeMode = RenderMode::kGDI;
    }
  }

  if (activeMode == RenderMode::kGDI) {
    RECT rc;
    GetClientRect(hwnd, &rc);

    HBRUSH bgBrush = CreateSolidBrush(ToColorRef(colors_.background));
    FillRect(hdc, &rc, bgBrush);
    DeleteObject(bgBrush);

    HPEN borderPen = CreatePen(PS_SOLID, 1, ToColorRef(colors_.border));
    HGDIOBJ oldPen = SelectObject(hdc, borderPen);
    HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(borderPen);

    GdiFontSet fonts = CreateGdiFontSet(dpiScale_, candidateFontSize_);
    const int originX = static_cast<int>(std::lround(12.0f * dpiScale_));
    int currentY = static_cast<int>(std::lround(8.0f * dpiScale_));
    const int lineGap = static_cast<int>(std::lround(4.0f * dpiScale_));
    const int highlightPaddingX =
        static_cast<int>(std::lround(4.0f * dpiScale_));
    const int highlightPaddingY =
        static_cast<int>(std::lround(2.0f * dpiScale_));
    const bool drawVertical = isVertical_ || forceVertical_;

    auto fontLineHeight = [hdc](HFONT font) -> int {
      if (!font) {
        return 0;
      }
      HGDIOBJ oldFont = SelectObject(hdc, font);
      TEXTMETRICW tm = {};
      GetTextMetricsW(hdc, &tm);
      SelectObject(hdc, oldFont);
      return tm.tmHeight + tm.tmExternalLeading;
    };

    auto drawCandidateLine = [&](UINT32 globalStart, std::wstring_view lineText,
                                 int x, int y, COLORREF color) {
      int cursorX = x;
      size_t localPos = 0;
      while (localPos < lineText.size()) {
        const UINT32 globalPos = globalStart + static_cast<UINT32>(localPos);
        bool inKeyRange = false;
        size_t nextBoundary = lineText.size();
        for (const auto& range : keyRanges_) {
          const UINT32 rangeStart = range.start;
          const UINT32 rangeEnd = range.start + range.length;
          if (globalPos >= rangeStart && globalPos < rangeEnd) {
            inKeyRange = true;
            nextBoundary = std::min(
                nextBoundary, static_cast<size_t>(rangeEnd - globalStart));
            break;
          }
          if (rangeStart > globalPos) {
            nextBoundary = std::min(
                nextBoundary, static_cast<size_t>(rangeStart - globalStart));
          }
        }

        if (nextBoundary <= localPos) {
          nextBoundary = localPos + 1;
        }

        std::wstring_view segment =
            lineText.substr(localPos, nextBoundary - localPos);
        cursorX += DrawOrMeasureTextRun(
            hdc, segment, cursorX, y,
            inKeyRange ? fonts.keyFont : fonts.textFont,
            inKeyRange ? fonts.keyEmojiFont : fonts.emojiFont, color, true);
        localPos = nextBoundary;
      }
      return cursorX - x;
    };

    auto measureCandidateLine = [&](UINT32 globalStart,
                                    std::wstring_view lineText) {
      int width = 0;
      size_t localPos = 0;
      while (localPos < lineText.size()) {
        const UINT32 globalPos = globalStart + static_cast<UINT32>(localPos);
        bool inKeyRange = false;
        size_t nextBoundary = lineText.size();
        for (const auto& range : keyRanges_) {
          const UINT32 rangeStart = range.start;
          const UINT32 rangeEnd = range.start + range.length;
          if (globalPos >= rangeStart && globalPos < rangeEnd) {
            inKeyRange = true;
            nextBoundary = std::min(
                nextBoundary, static_cast<size_t>(rangeEnd - globalStart));
            break;
          }
          if (rangeStart > globalPos) {
            nextBoundary = std::min(
                nextBoundary, static_cast<size_t>(rangeStart - globalStart));
          }
        }

        if (nextBoundary <= localPos) {
          nextBoundary = localPos + 1;
        }

        std::wstring_view segment =
            lineText.substr(localPos, nextBoundary - localPos);
        width += DrawOrMeasureTextRun(
            hdc, segment, 0, 0, inKeyRange ? fonts.keyFont : fonts.textFont,
            inKeyRange ? fonts.keyEmojiFont : fonts.emojiFont,
            ToColorRef(colors_.text), false);
        localPos = nextBoundary;
      }
      return width;
    };

    const int lineHeight =
        std::max(fontLineHeight(fonts.textFont), fontLineHeight(fonts.keyFont));
    const int hintLineHeight = fontLineHeight(fonts.hintFont);

    if (!hint_.empty()) {
      DrawOrMeasureTextRun(hdc, hint_, originX, currentY, fonts.hintFont,
                           fonts.hintEmojiFont, ToColorRef(colors_.text), true);
      currentY += std::max(hintLineHeight, 14) + lineGap;
    }

    const auto lines = SplitDisplayLines(displayString_);
    for (const auto& [globalStart, lineText] : lines) {
      const UINT32 lineEnd = globalStart + static_cast<UINT32>(lineText.size());
      const UINT32 selStart = selectedRange_.start;
      const UINT32 selEnd = selectedRange_.start + selectedRange_.length;

      const bool intersects = selectedRange_.length > 0 && selStart < lineEnd &&
                              selEnd > globalStart;

      int cursorX = originX;
      if (intersects) {
        const UINT32 localStart = std::max(selStart, globalStart) - globalStart;
        const UINT32 localEnd = std::min(selEnd, lineEnd) - globalStart;

        std::wstring_view prefix = lineText.substr(0, localStart);
        std::wstring_view selected =
            lineText.substr(localStart, localEnd - localStart);
        std::wstring_view suffix = lineText.substr(localEnd);

        cursorX += drawCandidateLine(globalStart, prefix, cursorX, currentY,
                                     ToColorRef(colors_.text));

        const int selectedWidth =
            measureCandidateLine(globalStart + localStart, selected);
        const int fullLineWidth = measureCandidateLine(globalStart, lineText);
        RECT highlightRect = {
            cursorX - highlightPaddingX, currentY - highlightPaddingY,
            drawVertical ? rc.right - (originX - highlightPaddingX)
                         : cursorX + selectedWidth + highlightPaddingX,
            currentY + lineHeight + highlightPaddingY};
        if (drawVertical) {
          highlightRect.left = originX - highlightPaddingX;
          highlightRect.right = std::max(
              highlightRect.left + fullLineWidth + highlightPaddingX * 2,
              rc.right - (originX - highlightPaddingX));
        }
        HBRUSH highlightBrush =
            CreateSolidBrush(ToColorRef(colors_.highlightBackground));
        FillRect(hdc, &highlightRect, highlightBrush);
        DeleteObject(highlightBrush);

        cursorX +=
            drawCandidateLine(globalStart + localStart, selected, cursorX,
                              currentY, ToColorRef(colors_.highlightText));
        drawCandidateLine(globalStart + localEnd, suffix, cursorX, currentY,
                          ToColorRef(colors_.text));
      } else {
        drawCandidateLine(globalStart, lineText, cursorX, currentY,
                          ToColorRef(colors_.text));
      }
      currentY += lineHeight + lineGap;
    }

    DestroyGdiFontSet(fonts);
    EndPaint(hwnd, &ps);
    return 0;
  }

  createDeviceResources_();
  if (pRenderTarget_) {
    pRenderTarget_->BeginDraw();
    pRenderTarget_->SetTransform(D2D1::Matrix3x2F::Scale(dpiScale_, dpiScale_));
    if (pBgBrush_) {
      pRenderTarget_->Clear(pBgBrush_->GetColor());
    } else {
      pRenderTarget_->Clear(D2D1::ColorF(D2D1::ColorF::White));
    }

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
                                         selectedRange_.length, 0, 0, nullptr,
                                         0, &actualHitTestCount);

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
