#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <uxtheme.h>

#include <algorithm>
#include <array>
#include <string>

#include "Settings.h"
#include "Ipc.h"
#include "NamedPipe.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

using namespace McBopomofo;

namespace {

constexpr const wchar_t* kClassName = L"McBopomofoConfigClass";
constexpr const wchar_t* kSingleInstanceMutexName = L"Local\\WinMcBopomofoConfigSingleInstance";
constexpr int kReloadCommand = 1;

constexpr COLORREF kWindowColor = RGB(246, 247, 249);
constexpr COLORREF kTextColor = RGB(32, 33, 36);

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
HBRUSH hWindowBrush = nullptr;
Settings settings;

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
    return CreateFontW(
        -MulDiv(pointSize, dpi, 72),
        0,
        0,
        0,
        weight,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        L"Microsoft JhengHei UI");
}

void ApplyFont(HWND hwnd, HFONT font = nullptr) {
    SendMessageW(hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(font ? font : hUiFont), TRUE);
}

HWND TrackControl(HWND hwnd, HFONT font = nullptr) {
    ApplyFont(hwnd, font);
    return hwnd;
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
    return TrackControl(CreateWindowW(L"Static", text, WS_VISIBLE | WS_CHILD, Scale(x), Scale(y), Scale(width), Scale(26), parent, nullptr, nullptr, nullptr));
}

HWND CreateSectionTitle(HWND parent, const wchar_t* text, int x, int y, int width) {
    return TrackControl(CreateWindowW(L"Static", text, WS_VISIBLE | WS_CHILD, Scale(x), Scale(y), Scale(width), Scale(34), parent, nullptr, nullptr, nullptr), hTitleFont);
}

HWND CreateGroup(HWND parent, int x, int y, int width, int height) {
    HWND group = CreateWindowW(
        L"Button",
        L"",
        WS_VISIBLE | WS_CHILD | BS_GROUPBOX,
        Scale(x),
        Scale(y),
        Scale(width),
        Scale(height),
        parent,
        nullptr,
        nullptr,
        nullptr);
    return TrackControl(group);
}

HWND CreateCombo(HWND parent, int x, int y, int width) {
    HWND combo = CreateWindowW(
        L"ComboBox",
        L"",
        WS_VISIBLE | WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST,
        Scale(x),
        Scale(y),
        Scale(width),
        Scale(180),
        parent,
        nullptr,
        nullptr,
        nullptr);
    SetWindowTheme(combo, L"Explorer", nullptr);
    return TrackControl(combo);
}

HWND CreateCheck(HWND parent, const wchar_t* text, int x, int y, int width) {
    HWND check = CreateWindowW(
        L"Button",
        text,
        WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX,
        Scale(x),
        Scale(y),
        Scale(width),
        Scale(28),
        parent,
        nullptr,
        nullptr,
        nullptr);
    SetWindowTheme(check, L"Explorer", nullptr);
    return TrackControl(check);
}

HWND CreateRadio(HWND parent, const wchar_t* text, int x, int y, int width, bool startsGroup) {
    DWORD style = WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_AUTORADIOBUTTON;
    if (startsGroup) {
        style |= WS_GROUP;
    }
    HWND radio = CreateWindowW(L"Button", text, style, Scale(x), Scale(y), Scale(width), Scale(28), parent, nullptr, nullptr, nullptr);
    SetWindowTheme(radio, L"Explorer", nullptr);
    return TrackControl(radio);
}

void SetLayoutSelection() {
    auto layout = settings.GetKeyboardLayout();
    auto it = std::find_if(kLayoutOptions.begin(), kLayoutOptions.end(), [&](const ComboOption& option) {
        return layout == option.value;
    });
    int index = it == kLayoutOptions.end() ? 0 : static_cast<int>(std::distance(kLayoutOptions.begin(), it));
    SendMessageW(hLayoutCombo, CB_SETCURSEL, index, 0);
}

void SetCtrlEnterSelection() {
    auto behavior = settings.GetCtrlEnterKeyBehavior();
    auto it = std::find_if(kCtrlEnterOptions.begin(), kCtrlEnterOptions.end(), [&](const CtrlEnterOption& option) {
        return behavior == option.value;
    });
    int index = it == kCtrlEnterOptions.end() ? 0 : static_cast<int>(std::distance(kCtrlEnterOptions.begin(), it));
    SendMessageW(hCtrlEnterCombo, CB_SETCURSEL, index, 0);
}

void SetCandidateKeysSelection() {
    auto keys = settings.GetCandidateKeys();
    auto it = std::find_if(kCandidateKeyOptions.begin(), kCandidateKeyOptions.end(), [&](const ComboOption& option) {
        return keys == option.value;
    });
    int index = it == kCandidateKeyOptions.end() ? 0 : static_cast<int>(std::distance(kCandidateKeyOptions.begin(), it));
    SendMessageW(hCandidateKeysCombo, CB_SETCURSEL, index, 0);
}

void SetCandidateKeysCountSelection() {
    int count = settings.GetCandidateKeysCount();
    SendMessageW(hCandidateKeysCountCombo, CB_SETCURSEL, count >= 4 && count <= 9 ? count - 4 : 5, 0);
}

void UpdateUI() {
    settings.Load();

    SetLayoutSelection();
    int inputMode = static_cast<int>(settings.GetInputMode());
    SendMessageW(hModeCombo, CB_SETCURSEL, inputMode == 1 ? 1 : 0, 0);

    bool candidateWindowVertical = settings.GetCandidateWindowVertical();
    SetChecked(hVerticalRadio, candidateWindowVertical);
    SetChecked(hHorizontalRadio, !candidateWindowVertical);
    SetCandidateKeysSelection();
    SetCandidateKeysCountSelection();

    bool selectAfterCursor = settings.GetSelectPhraseAfterCursorAsCandidate();
    SetChecked(hSelectBeforeRadio, !selectAfterCursor);
    SetChecked(hSelectAfterRadio, selectAfterCursor);
    SetChecked(hMoveCursorCheck, settings.GetMoveCursorAfterSelection());

    bool putLowercase = settings.GetPutLowercaseLettersToComposingBuffer();
    SetChecked(hUppercaseRadio, !putLowercase);
    SetChecked(hLowercaseRadio, putLowercase);

    SetChecked(hEscClearCheck, settings.GetEscKeyClearsEntireComposingBuffer());
    SetChecked(hShiftEnterCheck, settings.GetShiftEnterEnabled());
    SetCtrlEnterSelection();
    SetChecked(hRepeatedPunctuationCheck, settings.GetRepeatedPunctuationToSelectCandidateEnabled());
    SetChecked(hChooseSpaceCheck, settings.GetChooseCandidateUsingSpace());
}

void NotifyServer() {
    IPC::NamedPipeClient client(IPC::PIPE_NAME);
    std::string response;
    client.Call(IPC::SerializeReloadSettings(), response);
    SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, 0, SMTO_ABORTIFHUNG, 100, nullptr);
}

void SaveAndNotify() {
    int layoutIdx = ComboSelection(hLayoutCombo, 0);
    settings.SetKeyboardLayout(kLayoutOptions[layoutIdx].value);

    int modeIdx = ComboSelection(hModeCombo, 0);
    settings.SetInputMode(modeIdx == 1 ? InputMode::PlainBopomofo : InputMode::McBopomofo);

    settings.SetCandidateWindowVertical(IsChecked(hVerticalRadio));
    int candidateKeysIdx = ComboSelection(hCandidateKeysCombo, 0);
    settings.SetCandidateKeys(kCandidateKeyOptions[candidateKeysIdx].value);
    settings.SetCandidateKeysCount(ComboSelection(hCandidateKeysCountCombo, 5) + 4);

    settings.SetSelectPhraseAfterCursorAsCandidate(IsChecked(hSelectAfterRadio));
    settings.SetMoveCursorAfterSelection(IsChecked(hMoveCursorCheck));
    settings.SetPutLowercaseLettersToComposingBuffer(IsChecked(hLowercaseRadio));
    settings.SetEscKeyClearsEntireComposingBuffer(IsChecked(hEscClearCheck));
    settings.SetShiftEnterEnabled(IsChecked(hShiftEnterCheck));

    int ctrlEnterIdx = ComboSelection(hCtrlEnterCombo, 0);
    settings.SetCtrlEnterKeyBehavior(kCtrlEnterOptions[ctrlEnterIdx].value);

    settings.SetRepeatedPunctuationToSelectCandidateEnabled(IsChecked(hRepeatedPunctuationCheck));
    settings.SetChooseCandidateUsingSpace(IsChecked(hChooseSpaceCheck));

    settings.Save();
    NotifyServer();
}

void CreateControls(HWND hwnd) {
    CreateSectionTitle(hwnd, L"小麥注音偏好設定", 28, 22, 520);

    hReloadBtn = CreateWindowW(
        L"Button",
        L"重新載入",
        WS_VISIBLE | WS_CHILD | WS_TABSTOP,
        Scale(468),
        Scale(20),
        Scale(104),
        Scale(34),
        hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kReloadCommand)),
        nullptr,
        nullptr);
    TrackControl(hReloadBtn);

    CreateGroup(hwnd, 24, 72, 548, 116);
    CreateLabel(hwnd, L"鍵盤佈局", 56, 104, 140);
    hLayoutCombo = CreateCombo(hwnd, 210, 100, 260);
    for (const auto& option : kLayoutOptions) {
        AddComboString(hLayoutCombo, option.label);
    }

    CreateLabel(hwnd, L"輸入模式", 56, 144, 140);
    hModeCombo = CreateCombo(hwnd, 210, 140, 260);
    for (const auto* label : kInputModeLabels) {
        AddComboString(hModeCombo, label);
    }

    CreateGroup(hwnd, 24, 206, 548, 284);
    CreateLabel(hwnd, L"候選字窗", 56, 240, 140);
    hVerticalRadio = CreateRadio(hwnd, L"垂直", 210, 236, 120, true);
    hHorizontalRadio = CreateRadio(hwnd, L"水平", 210, 268, 120, false);

    CreateLabel(hwnd, L"選字按鍵", 56, 310, 140);
    hCandidateKeysCombo = CreateCombo(hwnd, 210, 306, 260);
    for (const auto& option : kCandidateKeyOptions) {
        AddComboString(hCandidateKeysCombo, option.label);
    }

    CreateLabel(hwnd, L"每頁候選字", 56, 350, 140);
    hCandidateKeysCountCombo = CreateCombo(hwnd, 210, 346, 120);
    for (int count = 4; count <= 9; ++count) {
        wchar_t text[4] = {};
        _itow_s(count, text, 10);
        AddComboString(hCandidateKeysCountCombo, text);
    }

    CreateLabel(hwnd, L"選字游標", 56, 390, 140);
    hSelectBeforeRadio = CreateRadio(hwnd, L"以游標前方字詞為候選字", 210, 386, 310, true);
    hSelectAfterRadio = CreateRadio(hwnd, L"以游標後方字詞為候選字", 210, 418, 310, false);
    hMoveCursorCheck = CreateCheck(hwnd, L"選字後移動游標", 210, 450, 280);

    CreateGroup(hwnd, 24, 508, 548, 184);
    CreateLabel(hwnd, L"輸入行為", 56, 542, 140);
    hUppercaseRadio = CreateRadio(hwnd, L"Shift + 字母輸出大寫字母", 210, 538, 320, true);
    hLowercaseRadio = CreateRadio(hwnd, L"Shift + 字母輸出小寫字母", 210, 570, 320, false);
    hEscClearCheck = CreateCheck(hwnd, L"Esc 清除整個輸入緩衝區", 210, 602, 320);
    hShiftEnterCheck = CreateCheck(hwnd, L"Shift + Enter 顯示聯想詞", 210, 634, 320);

    CreateLabel(hwnd, L"Ctrl + Enter", 56, 666, 140);
    hCtrlEnterCombo = CreateCombo(hwnd, 210, 662, 260);
    for (const auto& option : kCtrlEnterOptions) {
        AddComboString(hCtrlEnterCombo, option.label);
    }

    CreateGroup(hwnd, 24, 710, 548, 108);
    CreateLabel(hwnd, L"候選字與標點", 56, 744, 140);
    hRepeatedPunctuationCheck = CreateCheck(hwnd, L"重複標點時選擇候選標點", 210, 740, 330);
    hChooseSpaceCheck = CreateCheck(hwnd, L"使用空白鍵選取候選字", 210, 772, 320);

    UpdateUI();
}

}  // namespace

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        CreateControls(hwnd);
        break;
    case WM_CTLCOLORSTATIC: {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, kTextColor);
        return reinterpret_cast<LRESULT>(hWindowBrush);
    }
    case WM_CTLCOLORBTN: {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, kTextColor);
        return reinterpret_cast<LRESULT>(hWindowBrush);
    }
    case WM_ERASEBKGND: {
        RECT rect;
        GetClientRect(hwnd, &rect);
        FillRect(reinterpret_cast<HDC>(wParam), &rect, hWindowBrush);
        return 1;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == kReloadCommand) {
            UpdateUI();
        } else if (HIWORD(wParam) == BN_CLICKED || HIWORD(wParam) == CBN_SELCHANGE) {
            SaveAndNotify();
        }
        break;
    case WM_DESTROY:
        DeleteObject(hUiFont);
        DeleteObject(hTitleFont);
        DeleteObject(hWindowBrush);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    HANDLE hSingleInstanceMutex = CreateMutexW(nullptr, TRUE, kSingleInstanceMutexName);
    if (hSingleInstanceMutex && GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND existingWindow = FindWindowW(kClassName, nullptr);
        if (existingWindow) {
            ShowWindow(existingWindow, SW_RESTORE);
            SetForegroundWindow(existingWindow);
        }
        CloseHandle(hSingleInstanceMutex);
        return 0;
    }

    INITCOMMONCONTROLSEX icc = {sizeof(INITCOMMONCONTROLSEX), ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES};
    InitCommonControlsEx(&icc);

    hUiFont = CreateUIFont(12, FW_NORMAL);
    hTitleFont = CreateUIFont(17, FW_SEMIBOLD);
    hWindowBrush = CreateSolidBrush(kWindowColor);

    WNDCLASSEXW wcex = {sizeof(WNDCLASSEXW)};
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
    wcex.hbrBackground = hWindowBrush;
    wcex.lpszClassName = kClassName;
    RegisterClassExW(&wcex);

    HWND hwnd = CreateWindowExW(
        WS_EX_CONTROLPARENT,
        kClassName,
        L"小麥注音偏好設定",
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        Scale(620),
        Scale(930),
        nullptr,
        nullptr,
        hInstance,
        nullptr);
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
