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

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <dwmapi.h>
#include <shellapi.h>
#include <uxtheme.h>

#include <algorithm>
#include <array>
#include <string>
#include <utility>
#include <vector>

#include "../Common/UTFHelper.h"
#include "Ipc.h"
#include "NamedPipe.h"
#include "Settings.h"
#include "resource.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment( \
    linker,      \
    "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

using namespace McBopomofo;

namespace {

constexpr const wchar_t* kSingleInstanceMutexName =
    L"Local\\WinMcBopomofoConfigSingleInstance";
constexpr int kReloadCommand = IDC_RELOAD_BUTTON;
constexpr int kManualLinkCommand = IDC_MANUAL_LINK;
constexpr int kProjectHomepageCommand = IDC_PROJECT_HOMEPAGE_LINK;
constexpr int kScrollLineHeight = 20;  // pixels per scroll line
constexpr const wchar_t* kManualUrl =
    L"https://github.com/openvanilla/McBopomofo/wiki/"
    L"%E4%BD%BF%E7%94%A8%E6%89%8B%E5%86%8A";
constexpr const wchar_t* kProjectHomepageUrl =
    L"https://github.com/openvanilla/win-mcbopomofo";

// Light Mode Colors
constexpr COLORREF kLightWindowColor = RGB(246, 247, 249);
constexpr COLORREF kLightTextColor = RGB(32, 33, 36);
constexpr COLORREF kLightControlColor = RGB(255, 255, 255);

// Dark Mode Colors
constexpr COLORREF kDarkWindowColor = RGB(32, 33, 36);
constexpr COLORREF kDarkTextColor = RGB(232, 234, 237);
constexpr COLORREF kDarkControlColor = RGB(45, 46, 50);

COLORREF g_WindowColor = kLightWindowColor;
COLORREF g_TextColor = kLightTextColor;
COLORREF g_ControlColor = kLightControlColor;
HBRUSH g_WindowBrush = nullptr;
HBRUSH g_ControlBrush = nullptr;
bool g_DarkMode = false;
int g_ScrollPos = 0;  // Current vertical scroll position
int g_ContentHeight = 0;
std::vector<std::pair<HWND, RECT>> g_ChildBaseRects;

bool IsDarkModeEnabled() {
  HKEY hKey;
  DWORD value = 0;
  DWORD size = sizeof(value);
  if (RegOpenKeyExW(
          HKEY_CURRENT_USER,
          L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
          0, KEY_READ, &hKey) == ERROR_SUCCESS) {
    RegQueryValueExW(hKey, L"AppsUseLightTheme", nullptr, nullptr,
                     reinterpret_cast<LPBYTE>(&value), &size);
    RegCloseKey(hKey);
  }
  return value == 0;
}

void UpdateThemeColors() {
  g_DarkMode = IsDarkModeEnabled();
  if (g_DarkMode) {
    g_WindowColor = kDarkWindowColor;
    g_TextColor = kDarkTextColor;
    g_ControlColor = kDarkControlColor;
  } else {
    g_WindowColor = kLightWindowColor;
    g_TextColor = kLightTextColor;
    g_ControlColor = kLightControlColor;
  }
  if (g_WindowBrush) DeleteObject(g_WindowBrush);
  if (g_ControlBrush) DeleteObject(g_ControlBrush);
  g_WindowBrush = CreateSolidBrush(g_WindowColor);
  g_ControlBrush = CreateSolidBrush(g_ControlColor);
}

void ApplyThemeToWindow(HWND hwnd) {
  BOOL dark = g_DarkMode;
  DwmSetWindowAttribute(hwnd, 20 /* DWMWA_USE_IMMERSIVE_DARK_MODE */, &dark,
                        sizeof(dark));
  SetWindowTheme(hwnd, g_DarkMode ? L"DarkMode_Explorer" : L"Explorer",
                 nullptr);
}

struct ComboOption {
  UINT labelId;
  const char* value;
};

struct CtrlEnterOption {
  UINT labelId;
  KeyHandlerCtrlEnter value;
};

const std::array<ComboOption, 6> kLayoutOptions = {{
    {IDS_LAYOUT_STANDARD, "Standard"},
    {IDS_LAYOUT_ETEN, "ETen"},
    {IDS_LAYOUT_HSU, "Hsu"},
    {IDS_LAYOUT_ETEN26, "ETen26"},
    {IDS_LAYOUT_HANYUPINYIN, "HanyuPinyin"},
    {IDS_LAYOUT_IBM, "IBM"},
}};

const std::array<UINT, 2> kInputModeLabels = {{
    IDS_MODE_MCBOPOMOFO_AUTO,
    IDS_MODE_PLAIN_BOPOMOFO_MANUAL,
}};

const std::array<ComboOption, 3> kCandidateKeyOptions = {{
    {0, "123456789"},  // Not localized
    {0, "asdfghjkl"},  // Not localized
    {0, "asdfzxcvb"},  // Not localized
}};

const std::array<CtrlEnterOption, 6> kCtrlEnterOptions = {{
    {IDS_CTRL_ENTER_DISABLED, KeyHandlerCtrlEnter::Disabled},
    {IDS_CTRL_ENTER_BPMF_READING, KeyHandlerCtrlEnter::OutputBpmfReadings},
    {IDS_CTRL_ENTER_HTML_RUBY, KeyHandlerCtrlEnter::OutputHTMLRubyText},
    {IDS_CTRL_ENTER_HANYU_PINYIN, KeyHandlerCtrlEnter::OutputHanyuPinyin},
    {IDS_CTRL_ENTER_TAIWAN_BRAILLE_UNICODE,
     KeyHandlerCtrlEnter::OutputTaiwanBrailleUnicode},
    {IDS_CTRL_ENTER_TAIWAN_BRAILLE_ASCII,
     KeyHandlerCtrlEnter::OutputTaiwanBrailleAscii},
}};

const std::array<ComboOption, 3> kSelectionActionOptions = {{
    {IDS_SELECTION_ACTION_NONE, "None"},
    {IDS_SELECTION_ACTION_JK, "JK"},
    {IDS_SELECTION_ACTION_HL, "HL"},
}};

const std::array<int, 8> kCandidateFontSizes = {
    {10, 12, 14, 16, 18, 20, 24, 28}};

HWND hLayoutCombo = nullptr;
HWND hModeCombo = nullptr;
HWND hVerticalRadio = nullptr;
HWND hHorizontalRadio = nullptr;
HWND hCandidateKeysCombo = nullptr;
HWND hSelectBeforeRadio = nullptr;
HWND hSelectAfterRadio = nullptr;
HWND hMoveCursorCheck = nullptr;
HWND hLowercaseRadio = nullptr;
HWND hUppercaseRadio = nullptr;
HWND hEscClearCheck = nullptr;
HWND hShiftEnterCheck = nullptr;
HWND hCtrlEnterCombo = nullptr;
HWND hRepeatedPunctuationCheck = nullptr;
HWND hChooseSpaceCheck = nullptr;
HWND hSelectionActionCombo = nullptr;
HWND hCandidateFontSizeCombo = nullptr;
HWND hShiftToggleCheck = nullptr;
HWND hErrorBeepCheck = nullptr;
HWND hManualLink = nullptr;
HWND hProjectHomepageLink = nullptr;
HWND hReloadBtn = nullptr;
HWND hCandidateKeysCountCombo = nullptr;
HFONT hUiFont = nullptr;
HFONT hTitleFont = nullptr;
HFONT hLinkFont = nullptr;
Settings settings;
std::vector<HWND> g_ThemedControls;
std::vector<HWND> g_TextControls;
std::vector<HWND> g_GroupBoxes;
std::vector<HWND> g_LinkLabels;
std::vector<HWND> g_ComboBoxes;
std::vector<HWND> g_Separators;

bool IsRadioButton(HWND hwnd) {
  return hwnd == hVerticalRadio || hwnd == hHorizontalRadio ||
         hwnd == hSelectBeforeRadio || hwnd == hSelectAfterRadio ||
         hwnd == hLowercaseRadio || hwnd == hUppercaseRadio;
}

int Scale(int value);
bool ContainsControl(const std::vector<HWND>& controls, HWND hwnd);
void ApplyThemeToCombo(HWND combo);

int MaxScrollPos(const SCROLLINFO& si) {
  return std::max(0, si.nMax - static_cast<int>(si.nPage) + 1);
}

void CacheChildBaseRects(HWND hwnd) {
  g_ChildBaseRects.clear();
  g_ContentHeight = 0;
  for (HWND child = GetWindow(hwnd, GW_CHILD); child != nullptr;
       child = GetWindow(child, GW_HWNDNEXT)) {
    RECT rect{};
    GetWindowRect(child, &rect);
    POINT topLeft{rect.left, rect.top};
    POINT bottomRight{rect.right, rect.bottom};
    ScreenToClient(hwnd, &topLeft);
    ScreenToClient(hwnd, &bottomRight);
    rect.left = topLeft.x;
    rect.top = topLeft.y + g_ScrollPos;
    rect.right = bottomRight.x;
    rect.bottom = bottomRight.y + g_ScrollPos;
    g_ChildBaseRects.emplace_back(child, rect);
    g_ContentHeight =
        std::max(g_ContentHeight, static_cast<int>(rect.bottom));
  }
  g_ContentHeight += Scale(20);
}

void ReflowChildControls(int scrollPos) {
  HDWP hdwp = BeginDeferWindowPos(static_cast<int>(g_ChildBaseRects.size()));
  if (!hdwp) {
    return;
  }

  for (const auto& [child, rect] : g_ChildBaseRects) {
    if (!IsWindow(child)) {
      continue;
    }
    const int width = rect.right - rect.left;
    const int height = rect.bottom - rect.top;
    hdwp = DeferWindowPos(hdwp, child, nullptr, rect.left,
                          rect.top - scrollPos, width, height,
                          SWP_NOZORDER | SWP_NOACTIVATE);
    if (!hdwp) {
      return;
    }
  }

  EndDeferWindowPos(hdwp);
}

void ApplyVerticalScroll(HWND hwnd, int requestedPos) {
  SCROLLINFO si = {sizeof(SCROLLINFO), SIF_ALL};
  GetScrollInfo(hwnd, SB_VERT, &si);

  int newPos = std::max(0, std::min(requestedPos, MaxScrollPos(si)));
  if (newPos == g_ScrollPos) {
    return;
  }

  si.nPos = newPos;
  SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
  g_ScrollPos = newPos;
  ReflowChildControls(g_ScrollPos);
  InvalidateRect(hwnd, nullptr, TRUE);
}

void TrackContentBottom(int y, int height) {
  g_ContentHeight = std::max(g_ContentHeight, Scale(y + height));
}

int Scale(int value) {
  HDC hdc = GetDC(nullptr);
  int dpi = hdc ? GetDeviceCaps(hdc, LOGPIXELSX) : 96;
  if (hdc) {
    ReleaseDC(nullptr, hdc);
  }
  return MulDiv(value, dpi, 96);
}

HFONT CreateUIFont(int pointSize, int weight, bool underline = false) {
  HDC hdc = GetDC(nullptr);
  int dpi = hdc ? GetDeviceCaps(hdc, LOGPIXELSY) : 96;
  if (hdc) {
    ReleaseDC(nullptr, hdc);
  }

  const wchar_t* fontName = L"Microsoft JhengHei";
  LANGID langId = GetUserDefaultUILanguage();
  if (PRIMARYLANGID(langId) == LANG_ENGLISH) {
    fontName = L"Arial";
    pointSize = 10;
  } else {
    pointSize = 11;
  }

  return CreateFontW(-MulDiv(pointSize, dpi, 72), 0, 0, 0, weight, FALSE,
                     underline, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                     CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                     DEFAULT_PITCH | FF_DONTCARE, fontName);
}

void ApplyFont(HWND hwnd, HFONT font = nullptr) {
  SendMessageW(hwnd, WM_SETFONT,
               reinterpret_cast<WPARAM>(font ? font : hUiFont), TRUE);
}

HWND TrackControl(HWND hwnd, HFONT font = nullptr) {
  ApplyFont(hwnd, font);
  g_ThemedControls.push_back(hwnd);
  return hwnd;
}

HWND TrackTextControl(HWND hwnd, HFONT font = nullptr) {
  ApplyFont(hwnd, font);
  g_TextControls.push_back(hwnd);
  return hwnd;
}

void ApplyThemeToControls() {
  const wchar_t* theme = g_DarkMode ? L"DarkMode_Explorer" : L"Explorer";
  for (HWND control : g_ThemedControls) {
    if (control && IsWindow(control)) {
      if (ContainsControl(g_ComboBoxes, control)) {
        ApplyThemeToCombo(control);
      } else if (IsRadioButton(control)) {
        SetWindowTheme(control, L"", L"");
      } else {
        SetWindowTheme(control, theme, nullptr);
      }
      InvalidateRect(control, nullptr, TRUE);
    }
  }
  for (HWND control : g_TextControls) {
    if (control && IsWindow(control)) {
      SetWindowTheme(control, L"", L"");
      InvalidateRect(control, nullptr, TRUE);
    }
  }
}

void AddComboString(HWND combo, const wchar_t* text) {
  SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(text));
}

int ComboSelection(HWND combo, int fallback) {
  LRESULT selection = SendMessageW(combo, CB_GETCURSEL, 0, 0);
  return selection == CB_ERR ? fallback : static_cast<int>(selection);
}

bool IsChecked(HWND control) {
  if (IsRadioButton(control)) {
    return GetWindowLongPtrW(control, GWLP_USERDATA) != 0;
  }
  return SendMessageW(control, BM_GETCHECK, 0, 0) == BST_CHECKED;
}

void SetChecked(HWND control, bool checked) {
  if (IsRadioButton(control)) {
    SetWindowLongPtrW(control, GWLP_USERDATA, checked ? 1 : 0);
    return;
  }
  SendMessageW(control, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
}

HWND CreateLabel(HWND parent, const wchar_t* text, int x, int y, int width) {
  TrackContentBottom(y, 26);
  return TrackControl(CreateWindowW(
      L"Static", text, WS_VISIBLE | WS_CHILD | SS_LEFTNOWORDWRAP, Scale(x),
      Scale(y), Scale(width), Scale(26), parent, nullptr, nullptr, nullptr));
}

HWND CreateSectionTitle(HWND parent, const wchar_t* text, int x, int y,
                        int width) {
  TrackContentBottom(y, 24);
  return TrackControl(
      CreateWindowW(L"Static", text, WS_VISIBLE | WS_CHILD | SS_LEFTNOWORDWRAP,
                    Scale(x), Scale(y), Scale(width), Scale(24), parent,
                    nullptr, nullptr, nullptr),
      hTitleFont);
}

HWND CreateGroup(HWND parent, int x, int y, int width, int height) {
  TrackContentBottom(y, height);
  HWND group = CreateWindowW(
      L"Button", L"", WS_VISIBLE | WS_CHILD | BS_GROUPBOX | BS_OWNERDRAW,
      Scale(x), Scale(y), Scale(width), Scale(height), parent, nullptr, nullptr,
      nullptr);
  g_GroupBoxes.push_back(group);
  return TrackControl(group);
}

HWND CreateCombo(HWND parent, int x, int y, int width) {
  TrackContentBottom(y, 24);
  HWND combo = CreateWindowW(
      L"ComboBox", L"", WS_VISIBLE | WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST,
      Scale(x), Scale(y), Scale(width), Scale(180), parent, nullptr, nullptr,
      nullptr);
  g_ComboBoxes.push_back(combo);
  ApplyThemeToCombo(combo);
  return TrackControl(combo);
}

HWND CreateCheck(HWND parent, const wchar_t* text, int x, int y, int width) {
  TrackContentBottom(y, 24);
  HWND check = CreateWindowW(L"Button", text,
                             WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX,
                             Scale(x), Scale(y), Scale(width), Scale(24), parent,
                             nullptr, nullptr, nullptr);
  return TrackControl(check);
}

HWND CreateRadio(HWND parent, const wchar_t* text, int x, int y, int width,
                 bool startsGroup) {
  TrackContentBottom(y, 24);
  DWORD style =
      WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_AUTORADIOBUTTON;
  if (startsGroup) {
    style |= WS_GROUP;
  }
  HWND radio =
      CreateWindowW(L"Button", text, style, Scale(x), Scale(y), Scale(width),
                    Scale(24), parent, nullptr, nullptr, nullptr);
  return TrackControl(radio);
}

HWND CreateLink(HWND parent, const wchar_t* text, int x, int y, int width,
                int commandId) {
  TrackContentBottom(y, 22);
  HWND link = CreateWindowW(
      L"Static", text, WS_VISIBLE | WS_CHILD | WS_TABSTOP | SS_NOTIFY, Scale(x),
      Scale(y), Scale(width), Scale(22), parent,
      reinterpret_cast<HMENU>(static_cast<INT_PTR>(commandId)), nullptr,
      nullptr);
  ApplyFont(link, hLinkFont);
  g_LinkLabels.push_back(link);
  return link;
}

HWND CreateSeparator(HWND parent, int x, int y, int width) {
  TrackContentBottom(y, 8);
  HWND separator = CreateWindowW(L"Static", L"",
                                 WS_VISIBLE | WS_CHILD | SS_OWNERDRAW,
                                 Scale(x), Scale(y), Scale(width), Scale(8),
                                 parent, nullptr, nullptr, nullptr);
  g_Separators.push_back(separator);
  return separator;
}

bool ContainsControl(const std::vector<HWND>& controls, HWND hwnd) {
  return std::find(controls.begin(), controls.end(), hwnd) != controls.end();
}

void ApplyThemeToCombo(HWND combo) {
  const wchar_t* theme = g_DarkMode ? L"DarkMode_CFD" : L"CFD";
  SetWindowTheme(combo, theme, nullptr);

  COMBOBOXINFO info = {sizeof(COMBOBOXINFO)};
  if (!GetComboBoxInfo(combo, &info)) {
    return;
  }
  if (info.hwndList) {
    SetWindowTheme(info.hwndList, theme, nullptr);
  }
  if (info.hwndItem) {
    SetWindowTheme(info.hwndItem, theme, nullptr);
  }
}

void DrawControlText(HDC hdc, HWND hwnd, RECT rect, UINT format) {
  wchar_t text[256] = {};
  GetWindowTextW(hwnd, text, static_cast<int>(std::size(text)));
  HFONT font = reinterpret_cast<HFONT>(SendMessageW(hwnd, WM_GETFONT, 0, 0));
  HFONT oldFont =
      font ? reinterpret_cast<HFONT>(SelectObject(hdc, font)) : nullptr;
  SetBkMode(hdc, TRANSPARENT);
  SetTextColor(hdc, g_TextColor);
  DrawTextW(hdc, text, -1, &rect, format);
  if (oldFont) {
    SelectObject(hdc, oldFont);
  }
}

void DrawCheckGlyph(HDC hdc, RECT rect, bool checked) {
  COLORREF fillColor = g_DarkMode ? RGB(45, 46, 50) : RGB(255, 255, 255);
  COLORREF borderColor = g_DarkMode ? RGB(154, 160, 166) : RGB(95, 99, 104);
  COLORREF checkColor = g_DarkMode ? RGB(138, 180, 248) : RGB(0, 102, 204);

  HBRUSH fillBrush = CreateSolidBrush(fillColor);
  FillRect(hdc, &rect, fillBrush);
  DeleteObject(fillBrush);

  HPEN borderPen = CreatePen(PS_SOLID, Scale(1), borderColor);
  HGDIOBJ oldPen = SelectObject(hdc, borderPen);
  HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
  Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
  SelectObject(hdc, oldBrush);
  SelectObject(hdc, oldPen);
  DeleteObject(borderPen);

  if (!checked) {
    return;
  }

  HPEN checkPen = CreatePen(PS_SOLID, std::max(Scale(2), 2), checkColor);
  oldPen = SelectObject(hdc, checkPen);
  MoveToEx(hdc, rect.left + Scale(4), rect.top + Scale(8), nullptr);
  LineTo(hdc, rect.left + Scale(7), rect.top + Scale(11));
  LineTo(hdc, rect.right - Scale(4), rect.top + Scale(4));
  SelectObject(hdc, oldPen);
  DeleteObject(checkPen);
}

void DrawRadioGlyph(HDC hdc, RECT rect, bool checked) {
  COLORREF fillColor = g_DarkMode ? RGB(45, 46, 50) : RGB(255, 255, 255);
  COLORREF borderColor = g_DarkMode ? RGB(154, 160, 166) : RGB(95, 99, 104);
  COLORREF dotColor = g_DarkMode ? RGB(138, 180, 248) : RGB(0, 102, 204);

  HBRUSH fillBrush = CreateSolidBrush(fillColor);
  HPEN borderPen = CreatePen(PS_SOLID, Scale(1), borderColor);
  HGDIOBJ oldBrush = SelectObject(hdc, fillBrush);
  HGDIOBJ oldPen = SelectObject(hdc, borderPen);
  Ellipse(hdc, rect.left, rect.top, rect.right, rect.bottom);
  SelectObject(hdc, oldPen);
  SelectObject(hdc, oldBrush);
  DeleteObject(borderPen);
  DeleteObject(fillBrush);

  if (!checked) {
    return;
  }

  RECT dotRect = rect;
  InflateRect(&dotRect, -Scale(5), -Scale(5));
  HBRUSH dotBrush = CreateSolidBrush(dotColor);
  HGDIOBJ oldDotBrush = SelectObject(hdc, dotBrush);
  HGDIOBJ oldDotPen = SelectObject(hdc, GetStockObject(NULL_PEN));
  Ellipse(hdc, dotRect.left, dotRect.top, dotRect.right, dotRect.bottom);
  SelectObject(hdc, oldDotPen);
  SelectObject(hdc, oldDotBrush);
  DeleteObject(dotBrush);
}

void DrawOwnerDrawButton(const DRAWITEMSTRUCT* item) {
  HDC hdc = item->hDC;
  RECT rect = item->rcItem;
  FillRect(hdc, &rect, g_WindowBrush);

  if (IsRadioButton(item->hwndItem)) {
    RECT glyphRect = rect;
    glyphRect.right = glyphRect.left + Scale(16);
    int glyphHeight = Scale(16);
    int controlHeight = rect.bottom - rect.top;
    glyphRect.top = rect.top + (controlHeight - glyphHeight) / 2;
    glyphRect.bottom = glyphRect.top + glyphHeight;

    bool checked = IsChecked(item->hwndItem);
    DrawRadioGlyph(hdc, glyphRect, checked);

    RECT textRect = rect;
    textRect.left = glyphRect.right + Scale(8);
    DrawControlText(hdc, item->hwndItem, textRect,
                    DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_END_ELLIPSIS);

    if ((item->itemState & ODS_FOCUS) != 0) {
      DrawFocusRect(hdc, &textRect);
    }
    return;
  }

  if (ContainsControl(g_Separators, item->hwndItem)) {
    RECT lineRect = rect;
    lineRect.top = rect.top + ((rect.bottom - rect.top) / 2);
    lineRect.bottom = lineRect.top + 1;
    HBRUSH lineBrush = CreateSolidBrush(g_DarkMode ? RGB(92, 94, 99)
                                                   : RGB(210, 214, 220));
    FillRect(hdc, &lineRect, lineBrush);
    DeleteObject(lineBrush);
    return;
  }

  if (ContainsControl(g_GroupBoxes, item->hwndItem)) {
    HPEN pen = CreatePen(PS_SOLID, 1,
                         g_DarkMode ? RGB(92, 94, 99) : RGB(210, 214, 220));
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
    Rectangle(hdc, rect.left, rect.top + Scale(8), rect.right, rect.bottom);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(pen);
    return;
  }

  COLORREF buttonColor =
      g_DarkMode ? RGB(55, 57, 62) : RGB(255, 255, 255);
  HBRUSH buttonBrush = CreateSolidBrush(buttonColor);
  FillRect(hdc, &rect, buttonBrush);
  DeleteObject(buttonBrush);

  HPEN pen = CreatePen(PS_SOLID, 1,
                       g_DarkMode ? RGB(105, 107, 112) : RGB(196, 200, 207));
  HGDIOBJ oldPen = SelectObject(hdc, pen);
  HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
  Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
  SelectObject(hdc, oldBrush);
  SelectObject(hdc, oldPen);
  DeleteObject(pen);

  DrawControlText(hdc, item->hwndItem, rect,
                  DT_SINGLELINE | DT_VCENTER | DT_CENTER | DT_END_ELLIPSIS);
  if ((item->itemState & ODS_FOCUS) != 0) {
    InflateRect(&rect, -Scale(4), -Scale(4));
    DrawFocusRect(hdc, &rect);
  }
}

void SetLayoutSelection() {
  auto layout = settings.keyboardLayout();
  auto it = std::find_if(
      kLayoutOptions.begin(), kLayoutOptions.end(),
      [&](const ComboOption& option) { return layout == option.value; });
  int index = it == kLayoutOptions.end()
                  ? 0
                  : static_cast<int>(std::distance(kLayoutOptions.begin(), it));
  SendMessageW(hLayoutCombo, CB_SETCURSEL, index, 0);
}

void SetCtrlEnterSelection() {
  auto behavior = settings.ctrlEnterKeyBehavior();
  auto it = std::find_if(
      kCtrlEnterOptions.begin(), kCtrlEnterOptions.end(),
      [&](const CtrlEnterOption& option) { return behavior == option.value; });
  int index =
      it == kCtrlEnterOptions.end()
          ? 0
          : static_cast<int>(std::distance(kCtrlEnterOptions.begin(), it));
  SendMessageW(hCtrlEnterCombo, CB_SETCURSEL, index, 0);
}

void SetCandidateKeysSelection() {
  auto keys = settings.candidateKeys();
  auto it = std::find_if(
      kCandidateKeyOptions.begin(), kCandidateKeyOptions.end(),
      [&](const ComboOption& option) { return keys == option.value; });
  int index =
      it == kCandidateKeyOptions.end()
          ? 0
          : static_cast<int>(std::distance(kCandidateKeyOptions.begin(), it));
  SendMessageW(hCandidateKeysCombo, CB_SETCURSEL, index, 0);
}

void SetCandidateKeysCountSelection() {
  int count = settings.candidateKeysCount();
  SendMessageW(hCandidateKeysCountCombo, CB_SETCURSEL,
               count >= 4 && count <= 9 ? count - 4 : 5, 0);
}

void SetSelectionActionSelection() {
  auto action = settings.selectionAction();
  auto it = std::find_if(
      kSelectionActionOptions.begin(), kSelectionActionOptions.end(),
      [&](const ComboOption& option) { return action == option.value; });
  int index = it == kSelectionActionOptions.end()
                  ? 0
                  : static_cast<int>(
                        std::distance(kSelectionActionOptions.begin(), it));
  SendMessageW(hSelectionActionCombo, CB_SETCURSEL, index, 0);
}

void SetCandidateFontSizeSelection() {
  int fontSize = settings.candidateFontSize();
  auto it = std::find(kCandidateFontSizes.begin(), kCandidateFontSizes.end(),
                      fontSize);
  int index =
      it == kCandidateFontSizes.end()
          ? 3
          : static_cast<int>(std::distance(kCandidateFontSizes.begin(), it));
  SendMessageW(hCandidateFontSizeCombo, CB_SETCURSEL, index, 0);
}

void UpdateUI() {
  settings.load();

  SetLayoutSelection();
  int inputMode = static_cast<int>(settings.inputMode());
  SendMessageW(hModeCombo, CB_SETCURSEL, inputMode == 1 ? 1 : 0, 0);

  bool candidateWindowVertical = settings.candidateWindowVertical();
  SetChecked(hVerticalRadio, candidateWindowVertical);
  SetChecked(hHorizontalRadio, !candidateWindowVertical);
  SetCandidateKeysSelection();
  SetCandidateKeysCountSelection();
  SetSelectionActionSelection();

  bool selectAfterCursor = settings.selectPhraseAfterCursorAsCandidate();
  SetChecked(hSelectBeforeRadio, !selectAfterCursor);
  SetChecked(hSelectAfterRadio, selectAfterCursor);
  SetChecked(hMoveCursorCheck, settings.moveCursorAfterSelection());

  bool putLowercase = settings.putLowercaseLettersToComposingBuffer();
  SetChecked(hUppercaseRadio, !putLowercase);
  SetChecked(hLowercaseRadio, putLowercase);

  SetChecked(hEscClearCheck, settings.escKeyClearsEntireComposingBuffer());
  SetChecked(hShiftEnterCheck, settings.shiftEnterEnabled());
  SetCtrlEnterSelection();
  SetCandidateFontSizeSelection();
  SetChecked(hShiftToggleCheck, settings.shiftToggleOpenClose());
  SetChecked(hRepeatedPunctuationCheck,
             settings.repeatedPunctuationToSelectCandidateEnabled());
  SetChecked(hChooseSpaceCheck, settings.chooseCandidateUsingSpace());
  SetChecked(hErrorBeepCheck, settings.beepOnError());
}

void NotifyServer() {
  IPC::NamedPipeClient client(IPC::PIPE_NAME);
  std::string response;
  client.Call(IPC::SerializeReloadSettings(), response);
  SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, 0, SMTO_ABORTIFHUNG,
                      100, nullptr);
}

void SaveAndNotify() {
  int layoutIdx = ComboSelection(hLayoutCombo, 0);
  settings.setKeyboardLayout(kLayoutOptions[layoutIdx].value);

  int modeIdx = ComboSelection(hModeCombo, 0);
  settings.setInputMode(modeIdx == 1 ? InputMode::PlainBopomofo
                                     : InputMode::McBopomofo);

  settings.setCandidateWindowVertical(IsChecked(hVerticalRadio));
  int candidateKeysIdx = ComboSelection(hCandidateKeysCombo, 0);
  settings.setCandidateKeys(kCandidateKeyOptions[candidateKeysIdx].value);
  settings.setCandidateKeysCount(ComboSelection(hCandidateKeysCountCombo, 5) +
                                 4);
  int selectionActionIdx = ComboSelection(hSelectionActionCombo, 0);
  settings.setSelectionAction(
      kSelectionActionOptions[selectionActionIdx].value);

  settings.setSelectPhraseAfterCursorAsCandidate(IsChecked(hSelectAfterRadio));
  settings.setMoveCursorAfterSelection(IsChecked(hMoveCursorCheck));
  settings.setPutLowercaseLettersToComposingBuffer(IsChecked(hLowercaseRadio));
  settings.setEscKeyClearsEntireComposingBuffer(IsChecked(hEscClearCheck));
  settings.setShiftEnterEnabled(IsChecked(hShiftEnterCheck));
  settings.setShiftToggleOpenClose(IsChecked(hShiftToggleCheck));
  int candidateFontIdx = ComboSelection(hCandidateFontSizeCombo, 3);
  settings.setCandidateFontSize(kCandidateFontSizes[candidateFontIdx]);

  int ctrlEnterIdx = ComboSelection(hCtrlEnterCombo, 0);
  settings.setCtrlEnterKeyBehavior(kCtrlEnterOptions[ctrlEnterIdx].value);

  settings.setRepeatedPunctuationToSelectCandidateEnabled(
      IsChecked(hRepeatedPunctuationCheck));
  settings.setChooseCandidateUsingSpace(IsChecked(hChooseSpaceCheck));
  settings.setBeepOnError(IsChecked(hErrorBeepCheck));

  settings.save();
  NotifyServer();
}

HWND BindControl(HWND parent, int id, HFONT font = nullptr) {
  HWND control = GetDlgItem(parent, id);
  if (control == nullptr) {
    return nullptr;
  }
  TrackControl(control, font);
  return control;
}

void LocalizeControls(HWND hwnd) {
  HINSTANCE hInst = GetModuleHandle(nullptr);
  auto set = [&](int id, UINT strId) {
    SetDlgItemTextW(hwnd, id, LoadLocalizedStringW(hInst, strId).c_str());
  };

  set(IDC_RELOAD_BUTTON, IDS_RELOAD);
  set(IDC_INPUT_MODE_LABEL, IDS_INPUT_MODE);
  set(IDC_KEYBOARD_LAYOUT_LABEL, IDS_KEYBOARD_LAYOUT);
  set(IDC_SELECTION_KEYS_LABEL, IDS_CANDIDATE_KEYS);
  set(IDC_CHOOSE_SPACE_CHECK, IDS_CHOOSE_SPACE);
  set(IDC_CANDIDATES_PER_PAGE_LABEL, IDS_CANDIDATES_PER_PAGE);
  set(IDC_SELECTION_ACTION_LABEL, IDS_SELECTION_ACTION);
  set(IDC_SELECTION_CURSOR_LABEL, IDS_SELECTION_CURSOR);
  set(IDC_SELECT_BEFORE_RADIO, IDS_SELECT_BEFORE);
  set(IDC_SELECT_AFTER_RADIO, IDS_SELECT_AFTER);
  set(IDC_MOVE_CURSOR_CHECK, IDS_MOVE_CURSOR);
  set(IDC_CANDIDATE_PRESENTATION_LABEL, IDS_CANDIDATE_PRESENTATION);
  set(IDC_VERTICAL_RADIO, IDS_VERTICAL);
  set(IDC_HORIZONTAL_RADIO, IDS_HORIZONTAL);
  set(IDC_CANDIDATE_FONT_SIZE_LABEL, IDS_CANDIDATE_FONT_SIZE);
  set(IDC_SHIFT_TOGGLE_LABEL, IDS_SHIFT_TOGGLE);
  set(IDC_SHIFT_TOGGLE_CHECK, IDS_SHIFT_TOGGLE_OPEN_CLOSE);
  set(IDC_SHIFT_LETTER_LABEL, IDS_SHIFT_LETTER);
  set(IDC_SHIFT_LETTER_UPPER_RADIO, IDS_SHIFT_LETTER_UPPER);
  set(IDC_SHIFT_LETTER_LOWER_RADIO, IDS_SHIFT_LETTER_LOWER);
  set(IDC_SHIFT_ENTER_LABEL, IDS_SHIFT_ENTER);
  set(IDC_SHIFT_ENTER_CHECK, IDS_SHIFT_ENTER);
  set(IDC_ESC_LABEL, IDS_ESC_CLEAR);
  set(IDC_ESC_CLEAR_CHECK, IDS_ESC_CLEAR);
  set(IDC_CTRL_ENTER_LABEL, IDS_CTRL_ENTER);
  set(IDC_CANDIDATES_PUNCTUATION_LABEL, IDS_CANDIDATES_PUNCTUATION);
  set(IDC_REPEATED_PUNCTUATION_CHECK, IDS_REPEATED_PUNCTUATION);
  set(IDC_ERROR_BEEP_LABEL, IDS_ERROR_BEEP);
  set(IDC_ERROR_BEEP_CHECK, IDS_ERROR_BEEP);
  set(IDC_MANUAL_LINK, IDS_MANUAL_LINK);
  set(IDC_PROJECT_HOMEPAGE_LINK, IDS_PROJECT_HOMEPAGE);
  SetWindowTextW(hwnd, LoadLocalizedStringW(hInst, IDS_CONFIG_TITLE).c_str());
}

void InitializeComboContents() {
  for (const auto id : kInputModeLabels) {
    AddComboString(hModeCombo, LoadLocalizedStringW(GetModuleHandle(nullptr), id).c_str());
  }
  for (const auto& option : kLayoutOptions) {
    AddComboString(hLayoutCombo,
                   LoadLocalizedStringW(GetModuleHandle(nullptr), option.labelId).c_str());
  }
  for (const auto& option : kCandidateKeyOptions) {
    std::wstring ws(option.value, option.value + strlen(option.value));
    AddComboString(hCandidateKeysCombo, ws.c_str());
  }
  for (int count = 4; count <= 9; ++count) {
    wchar_t text[4] = {};
    _itow_s(count, text, 10);
    AddComboString(hCandidateKeysCountCombo, text);
  }
  for (const auto& option : kSelectionActionOptions) {
    AddComboString(hSelectionActionCombo,
                   LoadLocalizedStringW(GetModuleHandle(nullptr), option.labelId).c_str());
  }
  for (int fontSize : kCandidateFontSizes) {
    wchar_t text[4] = {};
    _itow_s(fontSize, text, 10);
    AddComboString(hCandidateFontSizeCombo, text);
  }
  for (const auto& option : kCtrlEnterOptions) {
    AddComboString(hCtrlEnterCombo,
                   LoadLocalizedStringW(GetModuleHandle(nullptr), option.labelId).c_str());
  }
}

void BindControls(HWND hwnd) {
  g_ThemedControls.clear();
  g_TextControls.clear();
  g_GroupBoxes.clear();
  g_LinkLabels.clear();
  g_ComboBoxes.clear();
  g_Separators.clear();

  hReloadBtn = BindControl(hwnd, IDC_RELOAD_BUTTON);
  TrackTextControl(BindControl(hwnd, IDC_INPUT_MODE_LABEL));
  hModeCombo = BindControl(hwnd, IDC_INPUT_MODE_COMBO);
  g_ComboBoxes.push_back(hModeCombo);
  TrackTextControl(BindControl(hwnd, IDC_KEYBOARD_LAYOUT_LABEL));
  hLayoutCombo = BindControl(hwnd, IDC_KEYBOARD_LAYOUT_COMBO);
  g_ComboBoxes.push_back(hLayoutCombo);
  TrackTextControl(BindControl(hwnd, IDC_SELECTION_KEYS_LABEL));
  hCandidateKeysCombo = BindControl(hwnd, IDC_SELECTION_KEYS_COMBO);
  g_ComboBoxes.push_back(hCandidateKeysCombo);
  hChooseSpaceCheck = BindControl(hwnd, IDC_CHOOSE_SPACE_CHECK);
  TrackTextControl(BindControl(hwnd, IDC_CANDIDATES_PER_PAGE_LABEL));
  hCandidateKeysCountCombo = BindControl(hwnd, IDC_CANDIDATES_PER_PAGE_COMBO);
  g_ComboBoxes.push_back(hCandidateKeysCountCombo);
  TrackTextControl(BindControl(hwnd, IDC_SELECTION_ACTION_LABEL));
  hSelectionActionCombo = BindControl(hwnd, IDC_SELECTION_ACTION_COMBO);
  g_ComboBoxes.push_back(hSelectionActionCombo);
  TrackTextControl(BindControl(hwnd, IDC_SELECTION_CURSOR_LABEL));
  hSelectBeforeRadio = BindControl(hwnd, IDC_SELECT_BEFORE_RADIO);
  hSelectAfterRadio = BindControl(hwnd, IDC_SELECT_AFTER_RADIO);
  hMoveCursorCheck = BindControl(hwnd, IDC_MOVE_CURSOR_CHECK);
  TrackTextControl(BindControl(hwnd, IDC_CANDIDATE_PRESENTATION_LABEL));
  hVerticalRadio = BindControl(hwnd, IDC_VERTICAL_RADIO);
  hHorizontalRadio = BindControl(hwnd, IDC_HORIZONTAL_RADIO);
  TrackTextControl(BindControl(hwnd, IDC_CANDIDATE_FONT_SIZE_LABEL));
  hCandidateFontSizeCombo = BindControl(hwnd, IDC_CANDIDATE_FONT_SIZE_COMBO);
  g_ComboBoxes.push_back(hCandidateFontSizeCombo);
  TrackTextControl(BindControl(hwnd, IDC_SHIFT_TOGGLE_LABEL));
  hShiftToggleCheck = BindControl(hwnd, IDC_SHIFT_TOGGLE_CHECK);
  TrackTextControl(BindControl(hwnd, IDC_SHIFT_LETTER_LABEL));
  hUppercaseRadio = BindControl(hwnd, IDC_SHIFT_LETTER_UPPER_RADIO);
  hLowercaseRadio = BindControl(hwnd, IDC_SHIFT_LETTER_LOWER_RADIO);
  TrackTextControl(BindControl(hwnd, IDC_SHIFT_ENTER_LABEL));
  hShiftEnterCheck = BindControl(hwnd, IDC_SHIFT_ENTER_CHECK);
  TrackTextControl(BindControl(hwnd, IDC_ESC_LABEL));
  hEscClearCheck = BindControl(hwnd, IDC_ESC_CLEAR_CHECK);
  TrackTextControl(BindControl(hwnd, IDC_CTRL_ENTER_LABEL));
  hCtrlEnterCombo = BindControl(hwnd, IDC_CTRL_ENTER_COMBO);
  g_ComboBoxes.push_back(hCtrlEnterCombo);
  TrackTextControl(BindControl(hwnd, IDC_CANDIDATES_PUNCTUATION_LABEL));
  hRepeatedPunctuationCheck = BindControl(hwnd, IDC_REPEATED_PUNCTUATION_CHECK);
  TrackTextControl(BindControl(hwnd, IDC_ERROR_BEEP_LABEL));
  hErrorBeepCheck = BindControl(hwnd, IDC_ERROR_BEEP_CHECK);
  hManualLink = TrackTextControl(BindControl(hwnd, IDC_MANUAL_LINK, hLinkFont));
  hProjectHomepageLink =
      TrackTextControl(BindControl(hwnd, IDC_PROJECT_HOMEPAGE_LINK, hLinkFont));
  g_LinkLabels.push_back(hManualLink);
  g_LinkLabels.push_back(hProjectHomepageLink);

  for (HWND control : {hSelectBeforeRadio, hSelectAfterRadio, hVerticalRadio,
                       hHorizontalRadio, hUppercaseRadio, hLowercaseRadio}) {
    if (control != nullptr) {
      LONG_PTR style = GetWindowLongPtrW(control, GWL_STYLE);
      SetWindowLongPtrW(control, GWL_STYLE, (style & ~BS_TYPEMASK) | BS_OWNERDRAW);
    }
  }

  for (HWND control : {hReloadBtn, hModeCombo, hLayoutCombo, hCandidateKeysCombo,
                       hChooseSpaceCheck, hCandidateKeysCountCombo,
                       hSelectionActionCombo, hSelectBeforeRadio,
                       hSelectAfterRadio, hMoveCursorCheck, hVerticalRadio,
                       hHorizontalRadio, hCandidateFontSizeCombo,
                       hShiftToggleCheck, hUppercaseRadio, hLowercaseRadio,
                       hShiftEnterCheck, hEscClearCheck, hCtrlEnterCombo,
                       hRepeatedPunctuationCheck, hErrorBeepCheck}) {
    if (control != nullptr) {
      SetWindowTheme(control, g_DarkMode ? L"DarkMode_Explorer" : L"Explorer",
                     nullptr);
    }
  }

  InitializeComboContents();
  LocalizeControls(hwnd);
  UpdateUI();
  ApplyThemeToControls();
  CacheChildBaseRects(hwnd);
  ReflowChildControls(g_ScrollPos);
}

}  // namespace

INT_PTR CALLBACK DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {
    case WM_INITDIALOG:
      ApplyThemeToWindow(hwnd);
      BindControls(hwnd);
      return TRUE;
    case WM_DRAWITEM:
      DrawOwnerDrawButton(reinterpret_cast<const DRAWITEMSTRUCT*>(lParam));
      return TRUE;
    case WM_CLOSE:
      DestroyWindow(hwnd);
      return TRUE;
    case WM_GETMINMAXINFO: {
      MINMAXINFO* pMinMaxInfo = reinterpret_cast<MINMAXINFO*>(lParam);
      // Fix window width - cannot be resized horizontally
      int fixedWidth = Scale(550);
      pMinMaxInfo->ptMinTrackSize.x = fixedWidth;
      pMinMaxInfo->ptMaxTrackSize.x = fixedWidth;
      // Allow height adjustment while keeping the layout usable on smaller
      // laptops.
      pMinMaxInfo->ptMinTrackSize.y = Scale(460);
      pMinMaxInfo->ptMaxTrackSize.y = Scale(760);
      break;
    }
    case WM_SIZE: {
      RECT rect;
      GetClientRect(hwnd, &rect);
      int visibleHeight = rect.bottom - rect.top;
      int totalHeight = std::max(g_ContentHeight, visibleHeight);

      SCROLLINFO si = {sizeof(SCROLLINFO), SIF_RANGE | SIF_PAGE | SIF_POS};
      si.nMin = 0;
      si.nMax = std::max(0, totalHeight - 1);
      si.nPage = visibleHeight;
      si.nPos = std::min(g_ScrollPos, std::max(0, totalHeight - visibleHeight));
      SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
      ShowScrollBar(hwnd, SB_HORZ, FALSE);
      g_ScrollPos = si.nPos;
      ReflowChildControls(g_ScrollPos);
      InvalidateRect(hwnd, nullptr, TRUE);
      break;
    }
    case WM_MOUSEWHEEL: {
      int wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
      int scrollLines = wheelDelta > 0 ? -3 : 3;  // Scroll up or down

      ApplyVerticalScroll(hwnd, g_ScrollPos + scrollLines * kScrollLineHeight);
      break;
    }
    case WM_VSCROLL: {
      SCROLLINFO si = {sizeof(SCROLLINFO), SIF_ALL};
      GetScrollInfo(hwnd, SB_VERT, &si);
      int newPos = si.nPos;

      switch (LOWORD(wParam)) {
        case SB_LINEUP:
          newPos -= kScrollLineHeight;
          break;
        case SB_LINEDOWN:
          newPos += kScrollLineHeight;
          break;
        case SB_PAGEUP:
          newPos -= static_cast<int>(si.nPage);
          break;
        case SB_PAGEDOWN:
          newPos += static_cast<int>(si.nPage);
          break;
        case SB_THUMBTRACK:
          newPos = si.nTrackPos;
          break;
        default:
          break;
      }

      ApplyVerticalScroll(hwnd, newPos);
      break;
    }
    case WM_SETTINGCHANGE:
      if (lParam && wcscmp(reinterpret_cast<LPCWSTR>(lParam),
                           L"ImmersiveColorSet") == 0) {
        UpdateThemeColors();
        ApplyThemeToWindow(hwnd);
        ApplyThemeToControls();
        InvalidateRect(hwnd, nullptr, TRUE);
      }
      break;
    case WM_CTLCOLORSTATIC: {
      HDC hdc = reinterpret_cast<HDC>(wParam);
      SetBkMode(hdc, TRANSPARENT);
      HWND control = reinterpret_cast<HWND>(lParam);
      if (ContainsControl(g_LinkLabels, control)) {
        SetTextColor(hdc, RGB(0, 102, 204));
      } else {
        SetTextColor(hdc, g_TextColor);
      }
      return reinterpret_cast<LRESULT>(g_WindowBrush);
    }
    case WM_CTLCOLORBTN: {
      HDC hdc = reinterpret_cast<HDC>(wParam);
      SetBkMode(hdc, TRANSPARENT);
      SetTextColor(hdc, g_TextColor);
      return reinterpret_cast<LRESULT>(g_WindowBrush);
    }
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX: {
      HDC hdc = reinterpret_cast<HDC>(wParam);
      SetBkMode(hdc, OPAQUE);
      SetBkColor(hdc, g_ControlColor);
      SetTextColor(hdc, g_TextColor);
      return reinterpret_cast<LRESULT>(g_ControlBrush);
    }
    case WM_ERASEBKGND: {
      RECT rect;
      GetClientRect(hwnd, &rect);
      FillRect(reinterpret_cast<HDC>(wParam), &rect, g_WindowBrush);
      return 1;
    }
    case WM_COMMAND:
      if (LOWORD(wParam) == kReloadCommand) {
        UpdateUI();
      } else if (HIWORD(wParam) == STN_CLICKED &&
                 LOWORD(wParam) == kManualLinkCommand) {
        ShellExecuteW(hwnd, L"open", kManualUrl, nullptr, nullptr,
                      SW_SHOWNORMAL);
      } else if (HIWORD(wParam) == STN_CLICKED &&
                 LOWORD(wParam) == kProjectHomepageCommand) {
        ShellExecuteW(hwnd, L"open", kProjectHomepageUrl, nullptr, nullptr,
                      SW_SHOWNORMAL);
      } else if (HIWORD(wParam) == BN_CLICKED ||
                 HIWORD(wParam) == CBN_SELCHANGE) {
        HWND clickedControl = reinterpret_cast<HWND>(lParam);
        if (clickedControl == hVerticalRadio) {
          SetChecked(hVerticalRadio, true);
          SetChecked(hHorizontalRadio, false);
        } else if (clickedControl == hHorizontalRadio) {
          SetChecked(hVerticalRadio, false);
          SetChecked(hHorizontalRadio, true);
        } else if (clickedControl == hSelectBeforeRadio) {
          SetChecked(hSelectBeforeRadio, true);
          SetChecked(hSelectAfterRadio, false);
        } else if (clickedControl == hSelectAfterRadio) {
          SetChecked(hSelectBeforeRadio, false);
          SetChecked(hSelectAfterRadio, true);
        } else if (clickedControl == hUppercaseRadio) {
          SetChecked(hUppercaseRadio, true);
          SetChecked(hLowercaseRadio, false);
        } else if (clickedControl == hLowercaseRadio) {
          SetChecked(hUppercaseRadio, false);
          SetChecked(hLowercaseRadio, true);
        }
        SaveAndNotify();
        if (IsRadioButton(clickedControl)) {
          InvalidateRect(hVerticalRadio, nullptr, TRUE);
          InvalidateRect(hHorizontalRadio, nullptr, TRUE);
          InvalidateRect(hSelectBeforeRadio, nullptr, TRUE);
          InvalidateRect(hSelectAfterRadio, nullptr, TRUE);
          InvalidateRect(hUppercaseRadio, nullptr, TRUE);
          InvalidateRect(hLowercaseRadio, nullptr, TRUE);
        }
      }
      return TRUE;
    case WM_DESTROY:
      DeleteObject(hUiFont);
      DeleteObject(hTitleFont);
      DeleteObject(hLinkFont);
      DeleteObject(g_WindowBrush);
      DeleteObject(g_ControlBrush);
      PostQuitMessage(0);
      return TRUE;
    default:
      return FALSE;
  }
  return FALSE;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
  HANDLE hSingleInstanceMutex =
      CreateMutexW(nullptr, TRUE, kSingleInstanceMutexName);
  if (hSingleInstanceMutex && GetLastError() == ERROR_ALREADY_EXISTS) {
    std::wstring windowTitle = LoadLocalizedStringW(hInstance, IDS_CONFIG_TITLE);
    HWND existingWindow = FindWindowW(L"#32770", windowTitle.c_str());
    if (existingWindow) {
      ShowWindow(existingWindow, SW_RESTORE);
      SetForegroundWindow(existingWindow);
    }
    CloseHandle(hSingleInstanceMutex);
    return 0;
  }

  INITCOMMONCONTROLSEX icc = {sizeof(INITCOMMONCONTROLSEX),
                              ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES};
  InitCommonControlsEx(&icc);

  UpdateThemeColors();

  hUiFont = CreateUIFont(11, FW_NORMAL);
  hTitleFont = CreateUIFont(11, FW_BOLD);
  hLinkFont = CreateUIFont(11, FW_NORMAL, true);
  std::wstring windowTitle = LoadLocalizedStringW(hInstance, IDS_CONFIG_TITLE);
  HWND hwnd = CreateDialogParamW(hInstance, MAKEINTRESOURCEW(IDD_CONFIG_DIALOG),
                                 nullptr, DlgProc, 0);
  if (hwnd == nullptr) {
    if (hSingleInstanceMutex) {
      ReleaseMutex(hSingleInstanceMutex);
      CloseHandle(hSingleInstanceMutex);
    }
    DeleteObject(hUiFont);
    DeleteObject(hTitleFont);
    DeleteObject(hLinkFont);
    DeleteObject(g_WindowBrush);
    DeleteObject(g_ControlBrush);
    return 0;
  }
  SetWindowTextW(hwnd, windowTitle.c_str());
  SendMessageW(hwnd, WM_SETICON, ICON_BIG,
               reinterpret_cast<LPARAM>(
                   LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_ICON_APP))));
  SendMessageW(hwnd, WM_SETICON, ICON_SMALL,
               reinterpret_cast<LPARAM>(
                   LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_ICON_APP))));
  ShowWindow(hwnd, nCmdShow);

  MSG msg;
  while (GetMessageW(&msg, nullptr, 0, 0)) {
    if (!IsDialogMessageW(hwnd, &msg)) {
      TranslateMessage(&msg);
      DispatchMessageW(&msg);
    }
  }

  if (hSingleInstanceMutex) {
    ReleaseMutex(hSingleInstanceMutex);
    CloseHandle(hSingleInstanceMutex);
  }
  return static_cast<int>(msg.wParam);
}
