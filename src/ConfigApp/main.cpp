#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include "Settings.h"
#include "Ipc.h"
#include "NamedPipe.h"
#include "UTFHelper.h"

#pragma comment(lib, "comctl32.lib")

using namespace McBopomofo;

const wchar_t* const CLASS_NAME = L"McBopomofoConfigClass";
HWND hLayoutCombo, hModeCombo, hVerticalCheck, hApplyBtn;
Settings settings;

void UpdateUI() {
    settings.Load();
    
    // Layout
    const char* layouts[] = { "Standard", "ETen", "Hsu", "ETen26", "HanyuPinyin", "IBM" };
    for (int i = 0; i < 6; ++i) {
        if (settings.GetKeyboardLayout() == layouts[i]) {
            SendMessage(hLayoutCombo, CB_SETCURSEL, i, 0);
            break;
        }
    }

    // Mode
    SendMessage(hModeCombo, CB_SETCURSEL, (int)settings.GetInputMode(), 0);

    // Vertical
    SendMessage(hVerticalCheck, BM_SETCHECK, settings.GetCandidateWindowVertical() ? BST_CHECKED : BST_UNCHECKED, 0);
}

void SaveAndNotify() {
    // Layout
    int layoutIdx = (int)SendMessage(hLayoutCombo, CB_GETCURSEL, 0, 0);
    const char* layouts[] = { "Standard", "ETen", "Hsu", "ETen26", "HanyuPinyin", "IBM" };
    settings.SetKeyboardLayout(layouts[layoutIdx]);

    // Mode
    settings.SetInputMode((InputMode)SendMessage(hModeCombo, CB_GETCURSEL, 0, 0));

    // Vertical
    settings.SetCandidateWindowVertical(SendMessage(hVerticalCheck, BM_GETCHECK, 0, 0) == BST_CHECKED);

    settings.Save();

    // Notify server via IPC
    IPC::NamedPipeClient client(IPC::PIPE_NAME);
    std::string response;
    client.Call(IPC::SerializeReloadSettings(), response);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        CreateWindowW(L"Static", L"鍵盤佈局 (Keyboard Layout):", WS_VISIBLE | WS_CHILD, 10, 10, 180, 20, hwnd, NULL, NULL, NULL);
        hLayoutCombo = CreateWindowW(L"ComboBox", L"", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST, 200, 10, 160, 200, hwnd, NULL, NULL, NULL);
        const wchar_t* layouts[] = { L"標準 (Standard)", L"倚天 (ETen)", L"許氏 (Hsu)", L"倚天26鍵 (ETen26)", L"漢語拼音 (Pinyin)", L"IBM" };
        for (auto l : layouts) SendMessage(hLayoutCombo, CB_ADDSTRING, 0, (LPARAM)l);

        CreateWindowW(L"Static", L"輸入模式 (Input Mode):", WS_VISIBLE | WS_CHILD, 10, 40, 180, 20, hwnd, NULL, NULL, NULL);
        hModeCombo = CreateWindowW(L"ComboBox", L"", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST, 200, 40, 160, 200, hwnd, NULL, NULL, NULL);
        const wchar_t* modes[] = { L"小麥注音 (McBopomofo)", L"普通注音 (Plain)" };
        for (auto m : modes) SendMessage(hModeCombo, CB_ADDSTRING, 0, (LPARAM)m);

        hVerticalCheck = CreateWindowW(L"Button", L"使用垂直候選字窗 (Vertical Candidate Window)", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 10, 70, 350, 20, hwnd, NULL, NULL, NULL);

        hApplyBtn = CreateWindowW(L"Button", L"套用設定 (Apply)", WS_VISIBLE | WS_CHILD, 10, 100, 150, 30, hwnd, (HMENU)1, NULL, NULL);
        
        UpdateUI();
        break;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == 1) {
            SaveAndNotify();
            MessageBoxW(hwnd, L"設定已儲存，並已通知輸入法引擎更新。", L"小麥注音設定", MB_OK);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    InitCommonControls();
    
    WNDCLASSEXW wcex = { sizeof(WNDCLASSEXW) };
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = CLASS_NAME;
    RegisterClassExW(&wcex);

    HWND hwnd = CreateWindowExW(0, CLASS_NAME, L"小麥注音設定 (Win-McBopomofo Settings)", WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, 400, 200, NULL, NULL, hInstance, NULL);
    ShowWindow(hwnd, nCmdShow);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}
