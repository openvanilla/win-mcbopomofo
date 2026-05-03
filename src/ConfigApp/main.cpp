#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>

#include <algorithm>
#include <array>
#include <string>

#include "Settings.h"
#include "Ipc.h"
#include "NamedPipe.h"

#pragma comment(lib, "comctl32.lib")

using namespace McBopomofo;

namespace {

constexpr const wchar_t* kClassName = L"McBopomofoConfigClass";
constexpr int kApplyCommand = 1;
constexpr int kReloadCommand = 2;

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

const std::array<CtrlEnterOption, 4> kCtrlEnterOptions = {{
    {L"不作用", KeyHandlerCtrlEnter::Disabled},
    {L"輸出注音讀音", KeyHandlerCtrlEnter::OutputBpmfReadings},
    {L"輸出 HTML Ruby 文字", KeyHandlerCtrlEnter::OutputHTMLRubyText},
    {L"輸出漢語拼音", KeyHandlerCtrlEnter::OutputHanyuPinyin},
}};

HWND hLayoutCombo = nullptr;
HWND hModeCombo = nullptr;
HWND hSelectBeforeRadio = nullptr;
HWND hSelectAfterRadio = nullptr;
HWND hMoveCursorCheck = nullptr;
HWND hLowercaseRadio = nullptr;
HWND hUppercaseRadio = nullptr;
HWND hEscClearCheck = nullptr;
HWND hShiftEnterCheck = nullptr;
HWND hCtrlEnterCombo = nullptr;
HWND hAssociatedPhrasesCheck = nullptr;
HWND hHalfWidthPunctuationCheck = nullptr;
HWND hRepeatedPunctuationCheck = nullptr;
HWND hChooseSpaceCheck = nullptr;
HWND hVerticalCheck = nullptr;
HWND hApplyBtn = nullptr;
HWND hReloadBtn = nullptr;
Settings settings;

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
    return CreateWindowW(L"Static", text, WS_VISIBLE | WS_CHILD, x, y, width, 20, parent, nullptr, nullptr, nullptr);
}

HWND CreateCombo(HWND parent, int x, int y, int width) {
    return CreateWindowW(
        L"ComboBox",
        L"",
        WS_VISIBLE | WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST,
        x,
        y,
        width,
        180,
        parent,
        nullptr,
        nullptr,
        nullptr);
}

HWND CreateCheck(HWND parent, const wchar_t* text, int x, int y, int width) {
    return CreateWindowW(
        L"Button",
        text,
        WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX,
        x,
        y,
        width,
        22,
        parent,
        nullptr,
        nullptr,
        nullptr);
}

HWND CreateRadio(HWND parent, const wchar_t* text, int x, int y, int width, bool startsGroup) {
    DWORD style = WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_AUTORADIOBUTTON;
    if (startsGroup) {
        style |= WS_GROUP;
    }
    return CreateWindowW(L"Button", text, style, x, y, width, 22, parent, nullptr, nullptr, nullptr);
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

void UpdateUI() {
    settings.Load();

    SetLayoutSelection();
    int inputMode = static_cast<int>(settings.GetInputMode());
    SendMessageW(hModeCombo, CB_SETCURSEL, inputMode == 1 ? 1 : 0, 0);

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
    SetChecked(hAssociatedPhrasesCheck, settings.GetAssociatedPhrasesEnabled());
    SetChecked(hHalfWidthPunctuationCheck, settings.GetHalfWidthPunctuationEnabled());
    SetChecked(hRepeatedPunctuationCheck, settings.GetRepeatedPunctuationToSelectCandidateEnabled());
    SetChecked(hChooseSpaceCheck, settings.GetChooseCandidateUsingSpace());
    SetChecked(hVerticalCheck, settings.GetCandidateWindowVertical());
}

void NotifyServer() {
    IPC::NamedPipeClient client(IPC::PIPE_NAME);
    std::string response;
    client.Call(IPC::SerializeReloadSettings(), response);
}

void SaveAndNotify() {
    int layoutIdx = ComboSelection(hLayoutCombo, 0);
    settings.SetKeyboardLayout(kLayoutOptions[layoutIdx].value);

    int modeIdx = ComboSelection(hModeCombo, 0);
    settings.SetInputMode(modeIdx == 1 ? InputMode::PlainBopomofo : InputMode::McBopomofo);

    settings.SetSelectPhraseAfterCursorAsCandidate(IsChecked(hSelectAfterRadio));
    settings.SetMoveCursorAfterSelection(IsChecked(hMoveCursorCheck));
    settings.SetPutLowercaseLettersToComposingBuffer(IsChecked(hLowercaseRadio));
    settings.SetEscKeyClearsEntireComposingBuffer(IsChecked(hEscClearCheck));
    settings.SetShiftEnterEnabled(IsChecked(hShiftEnterCheck));

    int ctrlEnterIdx = ComboSelection(hCtrlEnterCombo, 0);
    settings.SetCtrlEnterKeyBehavior(kCtrlEnterOptions[ctrlEnterIdx].value);

    settings.SetAssociatedPhrasesEnabled(IsChecked(hAssociatedPhrasesCheck));
    settings.SetHalfWidthPunctuationEnabled(IsChecked(hHalfWidthPunctuationCheck));
    settings.SetRepeatedPunctuationToSelectCandidateEnabled(IsChecked(hRepeatedPunctuationCheck));
    settings.SetChooseCandidateUsingSpace(IsChecked(hChooseSpaceCheck));
    settings.SetCandidateWindowVertical(IsChecked(hVerticalCheck));

    settings.Save();
    NotifyServer();
}

void CreateControls(HWND hwnd) {
    CreateLabel(hwnd, L"鍵盤佈局", 20, 18, 150);
    hLayoutCombo = CreateCombo(hwnd, 190, 14, 220);
    for (const auto& option : kLayoutOptions) {
        AddComboString(hLayoutCombo, option.label);
    }

    CreateLabel(hwnd, L"輸入模式", 20, 52, 150);
    hModeCombo = CreateCombo(hwnd, 190, 48, 220);
    for (const auto* label : kInputModeLabels) {
        AddComboString(hModeCombo, label);
    }

    CreateLabel(hwnd, L"選字游標", 20, 90, 150);
    hSelectBeforeRadio = CreateRadio(hwnd, L"以游標前方字詞為候選字", 190, 88, 280, true);
    hSelectAfterRadio = CreateRadio(hwnd, L"以游標後方字詞為候選字", 190, 114, 280, false);
    hMoveCursorCheck = CreateCheck(hwnd, L"選字後移動游標", 190, 142, 260);

    CreateLabel(hwnd, L"輸入行為", 20, 182, 150);
    hUppercaseRadio = CreateRadio(hwnd, L"Shift + 字母輸出大寫字母", 190, 180, 290, true);
    hLowercaseRadio = CreateRadio(hwnd, L"Shift + 字母輸出小寫字母", 190, 206, 290, false);
    hEscClearCheck = CreateCheck(hwnd, L"Esc 清除整個輸入緩衝區", 190, 234, 290);
    hShiftEnterCheck = CreateCheck(hwnd, L"Shift + Enter 顯示聯想詞", 190, 262, 290);

    CreateLabel(hwnd, L"Ctrl + Enter", 20, 302, 150);
    hCtrlEnterCombo = CreateCombo(hwnd, 190, 298, 220);
    for (const auto& option : kCtrlEnterOptions) {
        AddComboString(hCtrlEnterCombo, option.label);
    }

    CreateLabel(hwnd, L"候選字與標點", 20, 340, 150);
    hAssociatedPhrasesCheck = CreateCheck(hwnd, L"啟用聯想詞", 190, 338, 260);
    hHalfWidthPunctuationCheck = CreateCheck(hwnd, L"使用半形標點", 190, 366, 260);
    hRepeatedPunctuationCheck = CreateCheck(hwnd, L"重複標點時選擇候選標點", 190, 394, 300);
    hChooseSpaceCheck = CreateCheck(hwnd, L"使用空白鍵選取候選字", 190, 422, 290);
    hVerticalCheck = CreateCheck(hwnd, L"使用垂直候選字窗", 190, 450, 260);

    hApplyBtn = CreateWindowW(
        L"Button",
        L"套用",
        WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_DEFPUSHBUTTON,
        190,
        500,
        100,
        32,
        hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kApplyCommand)),
        nullptr,
        nullptr);
    hReloadBtn = CreateWindowW(
        L"Button",
        L"重新載入",
        WS_VISIBLE | WS_CHILD | WS_TABSTOP,
        306,
        500,
        100,
        32,
        hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kReloadCommand)),
        nullptr,
        nullptr);

    UpdateUI();
}

}  // namespace

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        CreateControls(hwnd);
        break;
    case WM_COMMAND:
        if (LOWORD(wParam) == kApplyCommand) {
            SaveAndNotify();
            MessageBoxW(hwnd, L"設定已儲存，並已通知輸入法引擎更新。", L"小麥注音偏好設定", MB_OK);
        } else if (LOWORD(wParam) == kReloadCommand) {
            UpdateUI();
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    InitCommonControls();

    WNDCLASSEXW wcex = {sizeof(WNDCLASSEXW)};
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
    wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wcex.lpszClassName = kClassName;
    RegisterClassExW(&wcex);

    HWND hwnd = CreateWindowExW(
        WS_EX_CONTROLPARENT,
        kClassName,
        L"小麥注音偏好設定",
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        540,
        590,
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
    return static_cast<int>(msg.wParam);
}
