#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

#include "McBopomofoLM.h"
#include "KeyHandler.h"
#include "InputController.h"
#include "InputMacro.h"
#include "WindowsKeyBridge.h"
#include "UIInterface.h"
#include "Settings.h"
#include "UTFHelper.h"
#include "NamedPipe.h"
#include "Log.h"

using namespace McBopomofo;

#define WM_USER_TRAY (WM_USER + 1)
#define IDM_RESTART 1001
#define IDM_EXIT 1002

class ServerUI : public UIInterface {
public:
    IPC::StateUpdatePayload currentState;

    void Reset() override {
        currentState = IPC::StateUpdatePayload();
    }

    void CommitString(const std::string& text) override {
        currentState.commitString = text;
    }

    void Update(InputState* state) override {
        currentState.commitString.clear();
        currentState.forceVertical = false;

        // Determine if we need to force vertical layout
        if (dynamic_cast<InputStates::NumberInput*>(state) != nullptr ||
            dynamic_cast<InputStates::SelectingDictionary*>(state) != nullptr ||
            dynamic_cast<InputStates::SelectingFeature*>(state) != nullptr ||
            dynamic_cast<InputStates::SelectingDateMacro*>(state) != nullptr) {
            currentState.forceVertical = true;
        }

        if (auto* inputting = dynamic_cast<InputStates::Inputting*>(state)) {
            currentState.composingBuffer = inputting->composingBuffer;
            currentState.cursorIndex = (int)inputting->cursorIndex;
            currentState.candidates.clear();
        } else if (auto* choosing = dynamic_cast<InputStates::ChoosingCandidate*>(state)) {
            currentState.composingBuffer = choosing->composingBuffer;
            currentState.cursorIndex = (int)choosing->cursorIndex;
            currentState.candidates.clear();
            for (const auto& c : choosing->candidates) {
                currentState.candidates.push_back(c.value);
                
                // If any candidate string length > 8, force vertical
                if (c.value.length() > 8 * 3) { // Approximate check for UTF-8 lengths
                    currentState.forceVertical = true;
                }
            }
        } else {
            currentState.composingBuffer.clear();
        }
    }
};

class DummyLocalizedStrings : public LocalizedStrings {
public:
    std::string cursorIsBetweenSyllables(const std::string&, const std::string&) override { return "Cursor between syllables"; }
    std::string bopomofoFontAnnotationModeTooltip(bool, bool) override { return "Font annotation mode"; }
    std::string syllablesRequired(size_t s) override { return "Requires " + std::to_string(s) + " syllables"; }
    std::string syllablesMaximum(size_t s) override { return "Maximum " + std::to_string(s) + " syllables"; }
    std::string phraseAlreadyExists() override { return "Phrase exists"; }
    std::string pressEnterToAddThePhrase() override { return "Press Enter to add"; }
    std::string markedWithSyllablesAndStatus(const std::string&, const std::string&, const std::string& s) override { return s; }
    std::string markingNotAvailableInFontAnnotationMode() override { return "Not available in font annotation mode"; }
};

class DummyUserPhraseAdder : public UserPhraseAdder {
public:
    void addUserPhrase(const std::string_view&, const std::string_view&) override {}
    void removeUserPhrase(const std::string_view&, const std::string_view&) override {}
};

LRESULT CALLBACK TrayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_USER_TRAY) {
        if (LOWORD(lParam) == WM_RBUTTONUP) {
            POINT pt;
            GetCursorPos(&pt);
            HMENU hMenu = CreatePopupMenu();
            InsertMenuW(hMenu, -1, MF_BYPOSITION | MF_STRING, IDM_RESTART, L"Restart Server");
            InsertMenuW(hMenu, -1, MF_BYPOSITION | MF_STRING, IDM_EXIT, L"Exit Server");
            SetForegroundWindow(hwnd);
            TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL);
            DestroyMenu(hMenu);
        }
        return 0;
    } else if (msg == WM_COMMAND) {
        if (LOWORD(wParam) == IDM_EXIT) {
            PostQuitMessage(0);
        } else if (LOWORD(wParam) == IDM_RESTART) {
            WCHAR path[MAX_PATH];
            GetModuleFileNameW(NULL, path, MAX_PATH);
            ShellExecuteW(NULL, L"open", path, L"data/data.txt", NULL, SW_SHOW);
            PostQuitMessage(0);
        }
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int main(int argc, char* argv[]) {
    FCITX_MCBOPOMOFO_INFO() << "Win-McBopomofo Server daemon starting...";
    
    std::string dataPath = "data/data.txt";
    if (argc >= 2) {
        dataPath = argv[1];
    }

    auto lm = std::make_shared<McBopomofoLM>();
    lm->loadLanguageModel(dataPath.c_str());

    if (!lm->isDataModelLoaded()) {
        FCITX_MCBOPOMOFO_ERROR() << "Failed to load language model from: " << dataPath;
        return 1;
    }

    // Set macro converter in LM
    InputMacroController macroController;
    lm->setMacroConverter([&macroController](const std::string& input) {
        return macroController.handle(input);
    });

    std::shared_ptr<KeyHandler> keyHandler(new KeyHandler(
        lm, 
        std::shared_ptr<VariantAnnotator>(nullptr), 
        std::static_pointer_cast<UserPhraseAdder>(std::make_shared<DummyUserPhraseAdder>()), 
        std::unique_ptr<LocalizedStrings>(new DummyLocalizedStrings())
    ));

    ServerUI ui;
    InputController controller(keyHandler, &ui);

    Settings settings;
    settings.ApplyTo(controller);

    FCITX_MCBOPOMOFO_INFO() << "Starting Named Pipe server at " << IPC::PIPE_NAME;

    IPC::NamedPipeServer server(IPC::PIPE_NAME, [&](const std::string& req) {
        IPC::KeyEventPayload keyReq;
        if (IPC::DeserializeKeyEvent(req, keyReq)) {
            FCITX_MCBOPOMOFO_INFO() << "IPC Recv: VK=" << keyReq.vk << ", ASCII=" << keyReq.ascii << ", SHIFT=" << keyReq.shift << ", CTRL=" << keyReq.ctrl;
            
            // Reset UI payload before processing
            ui.currentState.commitString.clear();
            
            // Map Windows VK/states to McBopomofo::Key
            bool consumed = controller.HandleKey(MapIPCKey(keyReq));
            
            ui.currentState.consumed = consumed;
            
            FCITX_MCBOPOMOFO_INFO() << "IPC Reply: Consumed=" << consumed << ", Commit=" << ui.currentState.commitString << ", Comp=" << ui.currentState.composingBuffer;
            return IPC::SerializeStateUpdate(ui.currentState);
        }
        FCITX_MCBOPOMOFO_WARN() << "IPC Failed to deserialize request.";
        return std::string();
    });

    server.Start();

    // Create a hidden window for the Tray Icon
    WNDCLASSEXW wcex = { sizeof(WNDCLASSEXW) };
    wcex.lpfnWndProc = TrayWndProc;
    wcex.hInstance = GetModuleHandle(NULL);
    wcex.lpszClassName = L"WinMcBopomofoServerTray";
    RegisterClassExW(&wcex);

    HWND hwndTray = CreateWindowExW(0, L"WinMcBopomofoServerTray", L"", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, wcex.hInstance, NULL);

    NOTIFYICONDATAW nid = { sizeof(NOTIFYICONDATAW) };
    nid.hWnd = hwndTray;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_USER_TRAY;
    nid.hIcon = LoadIcon(NULL, IDI_APPLICATION); // Standard exe icon
    wcscpy_s(nid.szTip, L"Win-McBopomofo Server");

    Shell_NotifyIconW(NIM_ADD, &nid);

    FCITX_MCBOPOMOFO_INFO() << "Server is running in background. Check System Tray to exit.";

    // Standard message loop to keep the process alive
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    server.Stop();
    Shell_NotifyIconW(NIM_DELETE, &nid);
    DestroyWindow(hwndTray);

    return 0;
}
