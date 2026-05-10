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
#include <uxtheme.h>

#include <algorithm>
#include <array>
#include <string>
#include <vector>

#include "Ipc.h"
#include "NamedPipe.h"
#include "Settings.h"
#include "resource.h"
#include "../Common/UTFHelper.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment( \
    linker,      \
    "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

using namespace McBopomofo;

namespace {

constexpr const wchar_t* kClassName = L"McBopomofoConfigClass";
constexpr const wchar_t* kSingleInstanceMutexName =
    L"Local\\WinMcBopomofoConfigSingleInstance";
constexpr int kReloadCommand = 1;
constexpr int kScrollLineHeight = 20;  // pixels per scroll line

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
}

struct ComboOption {
  const wchar_t* label;
  const char* value;
};

struct CtrlEnterOption {
  const wchar_t* label;
  KeyHandlerCtrlEnter value;
};

const std::array<ComboOption, 6> kLayoutOptions = {{
    {L"標準", "Standard"},
    {L"倚天", "ETen"},
    {L"許氏", "Hsu"},
    {L"倚天 26 鍵", "ETen26"},
    {L"漢語拼音", "HanyuPinyin"},
    {L"IBM", "IBM"},
}};

const std::array<const wchar_t*, 2> kInputModeLabels = {{
    L"小麥注音",
    L"普通注音",
}};

const std::array<ComboOption, 3> kCandidateKeyOptions = {{
    {L"123456789", "123456789"},
    {L"asdfghjkl", "asdfghjkl"},
    {L"asdfzxcvb", "asdfzxcvb"},
}};

const std::array<CtrlEnterOption, 4> kCtrlEnterOptions = {{
    {L"不作用", KeyHandlerCtrlEnter::Disabled},
    {L"輸出注音讀音", KeyHandlerCtrlEnter::OutputBpmfReadings},
    {L"輸出 HTML Ruby 文字", KeyHandlerCtrlEnter::OutputHTMLRubyText},
    {L"輸出漢語拼音", KeyHandlerCtrlEnter::OutputHanyuPinyin},
}};

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
HWND hReloadBtn = nullptr;
HWND hCandidateKeysCountCombo = nullptr;
HFONT hUiFont = nullptr;
HFONT hTitleFont = nullptr;
Settings settings;
std::vector<HWND> g_ThemedControls;
std::vector<HWND> g_GroupBoxes;
std::vector<HWND> g_CheckBoxes;
std::vector<HWND> g_RadioButtons;

int Scale(int value);

int MaxScrollPos(const SCROLLINFO& si) {
  return std::max(0, si.nMax - static_cast<int>(si.nPage));
}

void ApplyVerticalScroll(HWND hwnd, int requestedPos) {
  SCROLLINFO si = {sizeof(SCROLLINFO), SIF_ALL};
  GetScrollInfo(hwnd, SB_VERT, &si);

  int oldPos = si.nPos;
  int newPos = std::max(0, std::min(requestedPos, MaxScrollPos(si)));
  if (newPos == oldPos) {
    return;
  }

  si.nPos = newPos;
  SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
  g_ScrollPos = newPos;

  ScrollWindow(hwnd, 0, oldPos - newPos, nullptr, nullptr);
  UpdateWindow(hwnd);
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

HFONT CreateUIFont(int pointSize, int weight) {
  HDC hdc = GetDC(nullptr);
  int dpi = hdc ? GetDeviceCaps(hdc, LOGPIXELSY) : 96;
  if (hdc) {
    ReleaseDC(nullptr, hdc);
  }
  return CreateFontW(-MulDiv(pointSize, dpi, 72), 0, 0, 0, weight, FALSE, FALSE,
                     FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                     CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                     DEFAULT_PITCH | FF_DONTCARE, L"Microsoft JhengHei UI");
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

void ApplyThemeToControls() {
  const wchar_t* theme = g_DarkMode ? L"DarkMode_Explorer" : L"Explorer";
  for (HWND control : g_ThemedControls) {
    if (control && IsWindow(control)) {
      SetWindowTheme(control, theme, nullptr);
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
  return SendMessageW(control, BM_GETCHECK, 0, 0) == BST_CHECKED;
}

void SetChecked(HWND control, bool checked) {
  SendMessageW(control, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
}

HWND CreateLabel(HWND parent, const wchar_t* text, int x, int y, int width) {
  TrackContentBottom(y, 26);
  return TrackControl(CreateWindowW(L"Static", text, WS_VISIBLE | WS_CHILD,
                                    Scale(x), Scale(y), Scale(width), Scale(26),
                                    parent, nullptr, nullptr, nullptr));
}

HWND CreateSectionTitle(HWND parent, const wchar_t* text, int x, int y,
                        int width) {
  TrackContentBottom(y, 34);
  return TrackControl(
      CreateWindowW(L"Static", text, WS_VISIBLE | WS_CHILD, Scale(x), Scale(y),
                    Scale(width), Scale(34), parent, nullptr, nullptr, nullptr),
      hTitleFont);
}

HWND CreateGroup(HWND parent, int x, int y, int width, int height) {
  TrackContentBottom(y, height);
  HWND group = CreateWindowW(
      L"Button", L"", WS_VISIBLE | WS_CHILD | BS_GROUPBOX, Scale(x), Scale(y),
      Scale(width), Scale(height), parent, nullptr, nullptr, nullptr);
  g_GroupBoxes.push_back(group);
  return TrackControl(group);
}

HWND CreateCombo(HWND parent, int x, int y, int width) {
  TrackContentBottom(y, 180);
  HWND combo = CreateWindowW(
      L"ComboBox", L"", WS_VISIBLE | WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST,
      Scale(x), Scale(y), Scale(width), Scale(180), parent, nullptr, nullptr,
      nullptr);
  SetWindowTheme(combo, L"Explorer", nullptr);
  return TrackControl(combo);
}

HWND CreateCheck(HWND parent, const wchar_t* text, int x, int y, int width) {
  TrackContentBottom(y, 28);
  HWND check = CreateWindowW(
      L"Button", text, WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX,
      Scale(x), Scale(y), Scale(width), Scale(28), parent, nullptr, nullptr,
      nullptr);
  g_CheckBoxes.push_back(check);
  return TrackControl(check);
}

HWND CreateRadio(HWND parent, const wchar_t* text, int x, int y, int width,
                 bool startsGroup) {
  TrackContentBottom(y, 28);
  DWORD style = WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_AUTORADIOBUTTON;
  if (startsGroup) {
    style |= WS_GROUP;
  }
  HWND radio =
      CreateWindowW(L"Button", text, style, Scale(x), Scale(y), Scale(width),
                    Scale(28), parent, nullptr, nullptr, nullptr);
  g_RadioButtons.push_back(radio);
  return TrackControl(radio);
}

bool ContainsControl(const std::vector<HWND>& controls, HWND hwnd) {
  return std::find(controls.begin(), controls.end(), hwnd) != controls.end();
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

void DrawOwnerDrawButton(const DRAWITEMSTRUCT* item) {
  HDC hdc = item->hDC;
  RECT rect = item->rcItem;
  FillRect(hdc, &rect, g_WindowBrush);

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

  bool pushed = (item->itemState & ODS_SELECTED) != 0;
  bool focused = (item->itemState & ODS_FOCUS) != 0;
  bool checked = SendMessageW(item->hwndItem, BM_GETCHECK, 0, 0) == BST_CHECKED;
  bool isRadio = ContainsControl(g_RadioButtons, item->hwndItem);
  bool isCheck = ContainsControl(g_CheckBoxes, item->hwndItem);

  if (isRadio || isCheck) {
    int glyph = Scale(16);
    RECT glyphRect = {rect.left + Scale(2),
                      rect.top + (rect.bottom - rect.top - glyph) / 2,
                      rect.left + Scale(2) + glyph,
                      rect.top + (rect.bottom - rect.top + glyph) / 2};
    UINT state = 0;
    if (isRadio) {
      state = checked ? DFCS_BUTTONRADIO | DFCS_CHECKED : DFCS_BUTTONRADIO;
    } else {
      state = checked ? DFCS_BUTTONCHECK | DFCS_CHECKED : DFCS_BUTTONCHECK;
    }
    DrawFrameControl(hdc, &glyphRect, DFC_BUTTON, state);

    RECT textRect = rect;
    textRect.left += Scale(26);
    DrawControlText(hdc, item->hwndItem, textRect,
                    DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_END_ELLIPSIS);
    if (focused) {
      DrawFocusRect(hdc, &textRect);
    }
    return;
  }

  COLORREF buttonColor =
      g_DarkMode ? (pushed ? RGB(66, 68, 73) : RGB(55, 57, 62))
                 : (pushed ? RGB(229, 232, 236) : RGB(255, 255, 255));
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

  if (pushed) {
    OffsetRect(&rect, Scale(1), Scale(1));
  }
  DrawControlText(hdc, item->hwndItem, rect,
                  DT_SINGLELINE | DT_VCENTER | DT_CENTER | DT_END_ELLIPSIS);
  if (focused) {
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
  SetChecked(hRepeatedPunctuationCheck,
             settings.repeatedPunctuationToSelectCandidateEnabled());
  SetChecked(hChooseSpaceCheck, settings.chooseCandidateUsingSpace());
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

  settings.setSelectPhraseAfterCursorAsCandidate(IsChecked(hSelectAfterRadio));
  settings.setMoveCursorAfterSelection(IsChecked(hMoveCursorCheck));
  settings.setPutLowercaseLettersToComposingBuffer(IsChecked(hLowercaseRadio));
  settings.setEscKeyClearsEntireComposingBuffer(IsChecked(hEscClearCheck));
  settings.setShiftEnterEnabled(IsChecked(hShiftEnterCheck));

  int ctrlEnterIdx = ComboSelection(hCtrlEnterCombo, 0);
  settings.setCtrlEnterKeyBehavior(kCtrlEnterOptions[ctrlEnterIdx].value);

  settings.setRepeatedPunctuationToSelectCandidateEnabled(
      IsChecked(hRepeatedPunctuationCheck));
  settings.setChooseCandidateUsingSpace(IsChecked(hChooseSpaceCheck));

  settings.save();
  NotifyServer();
}

void CreateControls(HWND hwnd) {
  g_ContentHeight = 0;
  HINSTANCE hInst = GetModuleHandle(nullptr);

  CreateSectionTitle(hwnd, LoadLocalizedStringW(hInst, IDS_CONFIG_TITLE).c_str(), 28, 22, 520);

  hReloadBtn = CreateWindowW(
      L"Button", LoadLocalizedStringW(hInst, IDS_RELOAD).c_str(),
      WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_OWNERDRAW, Scale(468), Scale(20),
      Scale(104), Scale(34), hwnd,
      reinterpret_cast<HMENU>(static_cast<INT_PTR>(kReloadCommand)), nullptr,
      nullptr);
  TrackControl(hReloadBtn);
  TrackContentBottom(20, 34);

  CreateGroup(hwnd, 24, 72, 548, 116);
  CreateLabel(hwnd, LoadLocalizedStringW(hInst, IDS_KEYBOARD_LAYOUT).c_str(), 56,
              104, 140);
  hLayoutCombo = CreateCombo(hwnd, 210, 100, 260);
  for (const auto& option : kLayoutOptions) {
    AddComboString(hLayoutCombo, option.label);
  }

  CreateLabel(hwnd, LoadLocalizedStringW(hInst, IDS_INPUT_MODE).c_str(), 56, 144,
              140);
  hModeCombo = CreateCombo(hwnd, 210, 140, 260);
  for (const auto* label : kInputModeLabels) {
    AddComboString(hModeCombo, label);
  }

  CreateGroup(hwnd, 24, 206, 548, 284);
  CreateLabel(hwnd, LoadLocalizedStringW(hInst, IDS_CANDIDATE_WINDOW).c_str(),
              56, 240, 140);
  hVerticalRadio =
      CreateRadio(hwnd, LoadLocalizedStringW(hInst, IDS_VERTICAL).c_str(), 210,
                  236, 120, true);
  hHorizontalRadio =
      CreateRadio(hwnd, LoadLocalizedStringW(hInst, IDS_HORIZONTAL).c_str(), 210,
                  268, 120, false);

  CreateLabel(hwnd, LoadLocalizedStringW(hInst, IDS_CANDIDATE_KEYS).c_str(), 56,
              310, 140);
  hCandidateKeysCombo = CreateCombo(hwnd, 210, 306, 260);
  for (const auto& option : kCandidateKeyOptions) {
    AddComboString(hCandidateKeysCombo, option.label);
  }

  CreateLabel(hwnd, LoadLocalizedStringW(hInst, IDS_CANDIDATES_PER_PAGE).c_str(),
              56, 350, 140);
  hCandidateKeysCountCombo = CreateCombo(hwnd, 210, 346, 120);
  for (int count = 4; count <= 9; ++count) {
    wchar_t text[4] = {};
    _itow_s(count, text, 10);
    AddComboString(hCandidateKeysCountCombo, text);
  }

  CreateLabel(hwnd, LoadLocalizedStringW(hInst, IDS_SELECTION_CURSOR).c_str(),
              56, 390, 140);
  hSelectBeforeRadio = CreateRadio(
      hwnd, LoadLocalizedStringW(hInst, IDS_SELECT_BEFORE).c_str(), 210, 386,
      310, true);
  hSelectAfterRadio = CreateRadio(
      hwnd, LoadLocalizedStringW(hInst, IDS_SELECT_AFTER).c_str(), 210, 418, 310,
      false);
  hMoveCursorCheck =
      CreateCheck(hwnd, LoadLocalizedStringW(hInst, IDS_MOVE_CURSOR).c_str(),
                  210, 450, 280);

  CreateGroup(hwnd, 24, 508, 548, 184);
  CreateLabel(hwnd, LoadLocalizedStringW(hInst, IDS_INPUT_BEHAVIOR).c_str(), 56,
              542, 140);
  hUppercaseRadio = CreateRadio(
      hwnd, LoadLocalizedStringW(hInst, IDS_SHIFT_LETTER_UPPER).c_str(), 210,
      538, 320, true);
  hLowercaseRadio = CreateRadio(
      hwnd, LoadLocalizedStringW(hInst, IDS_SHIFT_LETTER_LOWER).c_str(), 210,
      570, 320, false);
  hEscClearCheck =
      CreateCheck(hwnd, LoadLocalizedStringW(hInst, IDS_ESC_CLEAR).c_str(), 210,
                  602, 320);
  hShiftEnterCheck =
      CreateCheck(hwnd, LoadLocalizedStringW(hInst, IDS_SHIFT_ENTER).c_str(),
                  210, 634, 320);

  CreateLabel(hwnd, LoadLocalizedStringW(hInst, IDS_CTRL_ENTER).c_str(), 56, 666,
              140);
  hCtrlEnterCombo = CreateCombo(hwnd, 210, 662, 260);
  for (const auto& option : kCtrlEnterOptions) {
    AddComboString(hCtrlEnterCombo, option.label);
  }

  CreateGroup(hwnd, 24, 710, 548, 108);
  CreateLabel(hwnd,
              LoadLocalizedStringW(hInst, IDS_CANDIDATES_PUNCTUATION).c_str(),
              56, 744, 140);
  hRepeatedPunctuationCheck = CreateCheck(
      hwnd, LoadLocalizedStringW(hInst, IDS_REPEATED_PUNCTUATION).c_str(), 210,
      740, 330);
  hChooseSpaceCheck =
      CreateCheck(hwnd, LoadLocalizedStringW(hInst, IDS_CHOOSE_SPACE).c_str(),
                  210, 772, 320);

  UpdateUI();
  ApplyThemeToControls();
  g_ContentHeight += Scale(24);
}

bool HandleOwnerDrawClick(HWND control) {
  UNREFERENCED_PARAMETER(control);
  return false;
}

}  // namespace

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {
    case WM_CREATE:
      ApplyThemeToWindow(hwnd);
      CreateControls(hwnd);
      break;
    case WM_GETMINMAXINFO: {
      MINMAXINFO* pMinMaxInfo = reinterpret_cast<MINMAXINFO*>(lParam);
      // Fix window width - cannot be resized horizontally
      int fixedWidth = Scale(620);
      pMinMaxInfo->ptMinTrackSize.x = fixedWidth;
      pMinMaxInfo->ptMaxTrackSize.x = fixedWidth;
      // Allow height adjustment, with minimum of 300px and maximum of 800px
      pMinMaxInfo->ptMinTrackSize.y = Scale(300);
      pMinMaxInfo->ptMaxTrackSize.y = Scale(800);
      break;
    }
    case WM_SIZE: {
      // Update scrollbar when window is resized
      RECT rect;
      GetClientRect(hwnd, &rect);
      int visibleHeight = rect.bottom - rect.top;
      int totalHeight = std::max(g_ContentHeight, visibleHeight);

      SCROLLINFO si = {sizeof(SCROLLINFO), SIF_RANGE | SIF_PAGE | SIF_POS};
      si.nMin = 0;
      si.nMax = totalHeight;
      si.nPage = visibleHeight;
      si.nPos = g_ScrollPos;
      SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
      ApplyVerticalScroll(hwnd, g_ScrollPos);
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
      SetTextColor(hdc, g_TextColor);
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
    case WM_DRAWITEM:
      DrawOwnerDrawButton(reinterpret_cast<DRAWITEMSTRUCT*>(lParam));
      return TRUE;
    case WM_ERASEBKGND: {
      RECT rect;
      GetClientRect(hwnd, &rect);
      FillRect(reinterpret_cast<HDC>(wParam), &rect, g_WindowBrush);
      return 1;
    }
    case WM_COMMAND:
      if (LOWORD(wParam) == kReloadCommand) {
        UpdateUI();
      } else if (HIWORD(wParam) == BN_CLICKED ||
                 HIWORD(wParam) == CBN_SELCHANGE) {
        if (HIWORD(wParam) == BN_CLICKED) {
          HandleOwnerDrawClick(reinterpret_cast<HWND>(lParam));
        }
        SaveAndNotify();
      }
      break;
    case WM_DESTROY:
      DeleteObject(hUiFont);
      DeleteObject(hTitleFont);
      DeleteObject(g_WindowBrush);
      DeleteObject(g_ControlBrush);
      PostQuitMessage(0);
      break;
    default:
      return DefWindowProcW(hwnd, msg, wParam, lParam);
  }
  return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
  HANDLE hSingleInstanceMutex =
      CreateMutexW(nullptr, TRUE, kSingleInstanceMutexName);
  if (hSingleInstanceMutex && GetLastError() == ERROR_ALREADY_EXISTS) {
    HWND existingWindow = FindWindowW(kClassName, nullptr);
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

  hUiFont = CreateUIFont(12, FW_NORMAL);
  hTitleFont = CreateUIFont(17, FW_SEMIBOLD);

  WNDCLASSEXW wcex = {sizeof(WNDCLASSEXW)};
  wcex.lpfnWndProc = WndProc;
  wcex.hInstance = hInstance;
  wcex.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
  wcex.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_ICON_APP));
  wcex.hIconSm = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_ICON_APP));
  wcex.hbrBackground = g_WindowBrush;
  wcex.lpszClassName = kClassName;
  RegisterClassExW(&wcex);

  std::wstring windowTitle = LoadLocalizedStringW(hInstance, IDS_CONFIG_TITLE);
  HWND hwnd =
      CreateWindowExW(WS_EX_CONTROLPARENT, kClassName, windowTitle.c_str(),
                      WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX | WS_VSCROLL,
                      CW_USEDEFAULT, CW_USEDEFAULT, Scale(620), Scale(640),
                      nullptr, nullptr, hInstance, nullptr);
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
