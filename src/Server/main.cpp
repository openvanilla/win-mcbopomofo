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
#include "UTF8Helper.h"
#include "NamedPipe.h"
#include "Log.h"

#include "PathCompat.h"
#include <array>
#include <fstream>
#include <filesystem>
#include <functional>
#include <mutex>

using namespace McBopomofo;

#define WM_USER_TRAY (WM_USER + 1)
#define IDM_RESTART 1001
#define IDM_EXIT 1002

constexpr const wchar_t* kServerSingleInstanceMutexName = L"Local\\WinMcBopomofoServerSingleInstance";

namespace {

void LogDataFileStatus(const char* label, const std::filesystem::path& path) {
    FCITX_MCBOPOMOFO_INFO() << label << ": " << path.string()
                            << ", exists: " << std::filesystem::exists(path);
}

}

class WinUserPhraseAdder : public UserPhraseAdder {
public:
    WinUserPhraseAdder(std::shared_ptr<McBopomofoLM> lm) : lm_(lm) {
        userPhrasesPath_ = fcitx5_compat::userDirectory() + "/user.txt";
        excludedPhrasesPath_ = fcitx5_compat::userDirectory() + "/exclude.txt";
    }

    void addUserPhrase(const std::string_view& reading, const std::string_view& phrase) override {
        std::ofstream ofs(userPhrasesPath_, std::ios::app);
        if (ofs) {
            ofs << phrase << " " << reading << "\n";
            ofs.close();
            lm_->loadUserPhrases(userPhrasesPath_.c_str(), excludedPhrasesPath_.c_str());
        }
    }

    void removeUserPhrase(const std::string_view& reading, const std::string_view& phrase) override {
        std::ifstream ifs(userPhrasesPath_);
        if (!ifs) return;

        std::vector<std::string> lines;
        std::string line;
        std::string target = std::string(phrase) + " " + std::string(reading);
        while (std::getline(ifs, line)) {
            if (!line.empty() && line != target) {
                lines.push_back(line);
            }
        }
        ifs.close();

        std::ofstream ofs(userPhrasesPath_);
        for (const auto& l : lines) {
            ofs << l << "\n";
        }
        ofs.close();
        lm_->loadUserPhrases(userPhrasesPath_.c_str(), excludedPhrasesPath_.c_str());
    }

private:
    std::shared_ptr<McBopomofoLM> lm_;
    std::string userPhrasesPath_;
    std::string excludedPhrasesPath_;
};

class ServerUI : public UIInterface {
public:
    IPC::StateUpdatePayload currentState;
    InputController* controller = nullptr;

    void Reset() override {
        std::string savedCommit = currentState.commitString;
        currentState = IPC::StateUpdatePayload();
        currentState.commitString = savedCommit;
    }

    void CommitString(const std::string& text) override {
        currentState.commitString += text;
    }

    void Update(InputState* state) override {
        currentState.forceVertical = false;
        currentState.markStart = -1;
        currentState.markEnd = -1;
        currentState.candidateIndex = controller ? controller->GetCandidateIndex() : -1;
        currentState.tooltip = "";
        if (auto* notEmptyState = dynamic_cast<InputStates::NotEmpty*>(state)) {
            currentState.tooltip = notEmptyState->tooltip;
        }

        // Determine if we need to force vertical layout
        if (dynamic_cast<InputStates::NumberInput*>(state) != nullptr ||
            dynamic_cast<InputStates::SelectingDictionary*>(state) != nullptr ||
            dynamic_cast<InputStates::ShowingCharInfo*>(state) != nullptr ||
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
                if (CodePointCount(c.value) > 8) {
                    currentState.forceVertical = true;
                }
            }
        } else if (auto* selDict = dynamic_cast<InputStates::SelectingDictionary*>(state)) {
            currentState.composingBuffer = selDict->composingBuffer;
            currentState.cursorIndex = (int)selDict->cursorIndex;
            currentState.candidates.clear();
            for (const auto& m : selDict->menu) {
                currentState.candidates.push_back(m);
            }
        } else if (auto* charInfo = dynamic_cast<InputStates::ShowingCharInfo*>(state)) {
            currentState.composingBuffer = charInfo->composingBuffer;
            currentState.cursorIndex = (int)charInfo->cursorIndex;
            currentState.candidates = {
                "UTF8 String Length: " + std::to_string(charInfo->selectedPhrase.length()),
                "Code Point Count: " + std::to_string(CodePointCount(charInfo->selectedPhrase))
            };
        } else if (auto* marking = dynamic_cast<InputStates::Marking*>(state)) {            currentState.composingBuffer = marking->composingBuffer;
            currentState.cursorIndex = (int)marking->cursorIndex;
            currentState.candidates.clear();
            currentState.markStart = (int)marking->head.length();
            currentState.markEnd = (int)(marking->head.length() + marking->markedText.length());
        } else if (auto* assoc = dynamic_cast<InputStates::AssociatedPhrases*>(state)) {
            currentState.composingBuffer = assoc->composingBuffer;
            currentState.cursorIndex = (int)assoc->cursorIndex;
            currentState.candidates.clear();
            for (const auto& c : assoc->candidates) {
                currentState.candidates.push_back(c.value);
            }
        } else if (auto* assocPlain = dynamic_cast<InputStates::AssociatedPhrasesPlain*>(state)) {
            currentState.composingBuffer.clear();
            currentState.cursorIndex = 0;
            currentState.candidates.clear();
            for (const auto& c : assocPlain->candidates) {
                currentState.candidates.push_back(c.value);
            }
        } else if (auto* numInput = dynamic_cast<InputStates::NumberInput*>(state)) {
            currentState.composingBuffer = numInput->composingBuffer;
            currentState.cursorIndex = (int)numInput->cursorIndex;
            currentState.candidates.clear();
            for (const auto& c : numInput->candidates) {
                currentState.candidates.push_back(c);
            }
        } else if (auto* big5 = dynamic_cast<InputStates::Big5*>(state)) {
            currentState.composingBuffer = big5->composingBuffer();
            currentState.cursorIndex = (int)currentState.composingBuffer.length();
            currentState.candidates.clear();
        } else if (auto* iroha = dynamic_cast<InputStates::Iroha*>(state)) {
            currentState.composingBuffer = iroha->composingBuffer();
            currentState.cursorIndex = (int)currentState.composingBuffer.length();
            currentState.candidates.clear();
        } else if (auto* selectingFeature = dynamic_cast<InputStates::SelectingFeature*>(state)) {
            currentState.composingBuffer.clear();
            currentState.cursorIndex = 0;
            currentState.candidates.clear();
            for (const auto& feature : selectingFeature->features) {
                currentState.candidates.push_back(feature.name);
            }
        } else if (auto* selectingDateMacro = dynamic_cast<InputStates::SelectingDateMacro*>(state)) {
            currentState.composingBuffer.clear();
            currentState.cursorIndex = 0;
            currentState.candidates = selectingDateMacro->menu;
        } else if (auto* iroha = dynamic_cast<InputStates::IrohaCandidate*>(state)) {
            currentState.composingBuffer = iroha->composingBuffer();
            currentState.cursorIndex = (int)currentState.composingBuffer.length();
            currentState.candidates = iroha->candidates;
        } else if (auto* customMenu = dynamic_cast<InputStates::CustomMenu*>(state)) {
            currentState.composingBuffer = customMenu->composingBuffer;
            currentState.cursorIndex = (int)customMenu->cursorIndex;
            currentState.candidates.clear();
            for (const auto& entry : customMenu->entries) {
                currentState.candidates.push_back(entry.name);
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

class WatchedFile {
public:
    explicit WatchedFile(std::filesystem::path path)
        : path_(std::move(path)) {
        Refresh();
    }

    const std::filesystem::path& Path() const { return path_; }

    bool HasChanged() {
        bool currentExists = std::filesystem::exists(path_);
        std::filesystem::file_time_type currentWriteTime{};
        if (currentExists) {
            std::error_code ec;
            currentWriteTime = std::filesystem::last_write_time(path_, ec);
            if (ec) {
                currentExists = false;
                currentWriteTime = {};
            }
        }

        bool changed = currentExists != exists_ ||
                       (currentExists && currentWriteTime != lastWriteTime_);
        exists_ = currentExists;
        lastWriteTime_ = currentWriteTime;
        return changed;
    }

private:
    void Refresh() {
        exists_ = std::filesystem::exists(path_);
        if (exists_) {
            std::error_code ec;
            lastWriteTime_ = std::filesystem::last_write_time(path_, ec);
            if (ec) {
                exists_ = false;
                lastWriteTime_ = {};
            }
        }
    }

    std::filesystem::path path_;
    bool exists_ = false;
    std::filesystem::file_time_type lastWriteTime_{};
};

class ServerFileReloader {
public:
    ServerFileReloader(std::filesystem::path settingsPath,
                       std::filesystem::path userPhrasesPath,
                       std::filesystem::path excludedPhrasesPath,
                       std::function<void()> reloadSettings,
                       std::function<void()> reloadUserPhrases)
        : settingsFile_(std::move(settingsPath)),
          userPhrasesFile_(std::move(userPhrasesPath)),
          excludedPhrasesFile_(std::move(excludedPhrasesPath)),
          reloadSettings_(std::move(reloadSettings)),
          reloadUserPhrases_(std::move(reloadUserPhrases)) {}

    void LogWatchedFiles() const {
        FCITX_MCBOPOMOFO_INFO() << "Watching settings file: "
                                << settingsFile_.Path().string();
        FCITX_MCBOPOMOFO_INFO() << "Watching user phrases file: "
                                << userPhrasesFile_.Path().string();
        FCITX_MCBOPOMOFO_INFO() << "Watching excluded phrases file: "
                                << excludedPhrasesFile_.Path().string();
    }

    void Check() {
        if (settingsFile_.HasChanged()) {
            FCITX_MCBOPOMOFO_INFO() << "Settings file changed; reloading settings.";
            reloadSettings_();
        }

        bool userPhrasesChanged = userPhrasesFile_.HasChanged();
        bool excludedPhrasesChanged = excludedPhrasesFile_.HasChanged();
        if (userPhrasesChanged || excludedPhrasesChanged) {
            FCITX_MCBOPOMOFO_INFO() << "User phrase files changed; reloading user phrases.";
            reloadUserPhrases_();
        }
    }

private:
    WatchedFile settingsFile_;
    WatchedFile userPhrasesFile_;
    WatchedFile excludedPhrasesFile_;
    std::function<void()> reloadSettings_;
    std::function<void()> reloadUserPhrases_;
};

bool IsCtrlSpace(const IPC::KeyEventPayload& key) {
    return key.vk == VK_SPACE && key.ctrl;
}

#define IDM_SETTINGS 1003
#define IDM_OPEN_USER_PHRASES 1004
#define IDM_OPEN_EXCLUDED_PHRASES 1005
#define IDM_OPEN_USER_DIR 1006
#define IDM_TOGGLE_CONVERSION 1007
#define IDM_OPEN_LOG_FOLDER 1008
#define IDM_TOGGLE_LOGGING 1009
#define IDM_TRACE_LOG 1010
#define IDM_EXIT 1002

InputController* g_Controller = nullptr;
bool g_RestartRequested = false;

void OpenFileInExplorer(const std::wstring& path) {
    std::wstring args = L"/select,\"" + path + L"\"";
    ShellExecuteW(nullptr, L"open", L"explorer.exe", args.c_str(), nullptr, SW_SHOWNORMAL);
}

void OpenLogFolder() {
    std::wstring logPath = GetLogFilePath();
    if (logPath.empty()) {
        return;
    }

    OpenFileInExplorer(logPath);
}

void ToggleLogging() {
    bool enabled = !ServerLoggingEnabled();
    SetServerLoggingEnabled(enabled);
    if (enabled) {
        FCITX_MCBOPOMOFO_INFO() << "Logging enabled.";
    }
}

void RestartServer() {
    g_RestartRequested = true;
    PostQuitMessage(0);
}

void TraceLog() {
    std::wstring logPath = GetLogFilePath();
    if (logPath.empty()) {
        return;
    }

    std::wstring args =
        L"-NoExit -Command \"Get-Content -Path '" + logPath +
        L"' -Wait -Tail 50\"";
    ShellExecuteW(nullptr, L"open", L"powershell.exe", args.c_str(), nullptr, SW_SHOWNORMAL);
}

void RelaunchCurrentProcess() {
    std::wstring commandLine = GetCommandLineW();
    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {};
    std::wstring mutableCommandLine = commandLine;
    if (CreateProcessW(nullptr, mutableCommandLine.data(), nullptr, nullptr, FALSE,
                       0, nullptr, nullptr, &si, &pi)) {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
}

void OpenSettingsApp() {
    WCHAR path[MAX_PATH] = {};
    if (GetModuleFileNameW(nullptr, path, MAX_PATH) == 0) {
        return;
    }

    std::filesystem::path configPath(path);
    configPath.replace_filename(L"McBopomofoConfig.exe");
    ShellExecuteW(nullptr, L"open", configPath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

LRESULT CALLBACK TrayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_USER_TRAY) {
        if (LOWORD(lParam) == WM_RBUTTONUP) {
            POINT pt;
            GetCursorPos(&pt);
            HMENU hMenu = CreatePopupMenu();
            
            bool isConversionEnabled = g_Controller && g_Controller->IsChineseConversionEnabled();
            LPCWSTR conversionText = isConversionEnabled ? L"輸出：簡體中文" : L"輸出：繁體中文";
            UINT loggingFlags = MF_BYPOSITION | MF_STRING;
            if (ServerLoggingEnabled()) {
                loggingFlags |= MF_CHECKED;
            }

            InsertMenuW(hMenu, 0xFFFFFFFFU, MF_BYPOSITION | MF_STRING, IDM_SETTINGS, L"設定");
            InsertMenuW(hMenu, 0xFFFFFFFFU, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
            InsertMenuW(hMenu, 0xFFFFFFFFU, MF_BYPOSITION | MF_STRING, IDM_TOGGLE_CONVERSION, conversionText);
            InsertMenuW(hMenu, 0xFFFFFFFFU, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
            InsertMenuW(hMenu, 0xFFFFFFFFU, MF_BYPOSITION | MF_STRING, IDM_OPEN_USER_PHRASES, L"編輯使用者詞庫");
            InsertMenuW(hMenu, 0xFFFFFFFFU, MF_BYPOSITION | MF_STRING, IDM_OPEN_EXCLUDED_PHRASES, L"編輯排除詞庫");
            InsertMenuW(hMenu, 0xFFFFFFFFU, MF_BYPOSITION | MF_STRING, IDM_OPEN_USER_DIR, L"開啟使用者資料夾");
            InsertMenuW(hMenu, 0xFFFFFFFFU, MF_BYPOSITION | MF_STRING, IDM_OPEN_LOG_FOLDER, L"開啟 Log 資料夾");
            InsertMenuW(hMenu, 0xFFFFFFFFU, loggingFlags, IDM_TOGGLE_LOGGING, L"啟用 Logging");
            InsertMenuW(hMenu, 0xFFFFFFFFU, MF_BYPOSITION | MF_STRING, IDM_TRACE_LOG, L"Trace Log");
            InsertMenuW(hMenu, 0xFFFFFFFFU, MF_BYPOSITION | MF_STRING, IDM_RESTART, L"重新啟動 Server");
            //InsertMenuW(hMenu, 0xFFFFFFFFU, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
            //InsertMenuW(hMenu, 0xFFFFFFFFU, MF_BYPOSITION | MF_STRING, IDM_EXIT, L"結束 (Exit)");
            SetForegroundWindow(hwnd);
            TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL);
            DestroyMenu(hMenu);
        }
        return 0;
    } else if (msg == WM_COMMAND) {
        if (LOWORD(wParam) == IDM_EXIT) {
            PostQuitMessage(0);
        } else if (LOWORD(wParam) == IDM_SETTINGS) {
            OpenSettingsApp();
        } else if (LOWORD(wParam) == IDM_TOGGLE_CONVERSION) {
            if (g_Controller) {
                g_Controller->ToggleChineseConversion();
            }
        } else if (LOWORD(wParam) == IDM_OPEN_USER_PHRASES) {
            std::string path = fcitx5_compat::userDirectory() + "/user.txt";
            ShellExecuteA(NULL, "open", path.c_str(), NULL, NULL, SW_SHOW);
        } else if (LOWORD(wParam) == IDM_OPEN_EXCLUDED_PHRASES) {
            std::string path = fcitx5_compat::userDirectory() + "/exclude.txt";
            ShellExecuteA(NULL, "open", path.c_str(), NULL, NULL, SW_SHOW);
        } else if (LOWORD(wParam) == IDM_OPEN_USER_DIR) {
            std::string path = fcitx5_compat::userDirectory();
            ShellExecuteA(NULL, "open", path.c_str(), NULL, NULL, SW_SHOW);
        } else if (LOWORD(wParam) == IDM_OPEN_LOG_FOLDER) {
            OpenLogFolder();
        } else if (LOWORD(wParam) == IDM_TOGGLE_LOGGING) {
            ToggleLogging();
        } else if (LOWORD(wParam) == IDM_TRACE_LOG) {
            TraceLog();
        } else if (LOWORD(wParam) == IDM_RESTART) {
            RestartServer();
        }
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int main(int argc, char* argv[]) {
    HANDLE hSingleInstanceMutex = CreateMutexW(nullptr, TRUE, kServerSingleInstanceMutexName);
    if (hSingleInstanceMutex && GetLastError() == ERROR_ALREADY_EXISTS) {
        return 0;
    }

    FCITX_MCBOPOMOFO_INFO() << "Win-McBopomofo Server daemon starting...";
    
    WCHAR szExePath[MAX_PATH];
    GetModuleFileNameW(NULL, szExePath, MAX_PATH);
    std::filesystem::path exeDir = std::filesystem::path(szExePath).parent_path();

    std::string dataPath = (exeDir / "data" / "data.txt").string();
    if (argc >= 2) {
        dataPath = argv[1];
    }
    LogDataFileStatus("Language model data file", dataPath);

    auto lm = std::make_shared<McBopomofoLM>();
    lm->loadLanguageModel(dataPath.c_str());
    FCITX_MCBOPOMOFO_INFO() << "Language model loaded: " << lm->isDataModelLoaded();

    std::string assocPath = (exeDir / "data" / "associated-phrases-v2.txt").string();
    LogDataFileStatus("Associated phrases file", assocPath);
    if (std::filesystem::exists(assocPath)) {
        lm->loadAssociatedPhrasesV2(assocPath.c_str());
        FCITX_MCBOPOMOFO_INFO() << "Associated phrases loaded from: " << assocPath;
    } else {
        FCITX_MCBOPOMOFO_WARN() << "Associated phrases file missing: " << assocPath;
    }

    std::string variantsPath = (exeDir / "data" / "bpmfvs-variants.txt").string();
    LogDataFileStatus("Phrase replacement file", variantsPath);
    if (std::filesystem::exists(variantsPath)) {
        lm->loadPhraseReplacementMap(variantsPath.c_str());
        FCITX_MCBOPOMOFO_INFO() << "Phrase replacement map loaded from: " << variantsPath;
    } else {
        FCITX_MCBOPOMOFO_WARN() << "Phrase replacement file missing: " << variantsPath;
    }

    std::array<std::filesystem::path, 2> annotationFiles = {
        exeDir / "data" / "bpmfvs-pua.txt",
        exeDir / "data" / "bpmfvs-variants.txt",
    };
    for (const auto& path : annotationFiles) {
        LogDataFileStatus("Bopomofo annotation file", path);
    }
    FCITX_MCBOPOMOFO_WARN()
        << "Server is currently constructed without VariantAnnotator; "
        << "bopomofo annotation files are not loaded by the server.";

    if (!lm->isDataModelLoaded()) {
        FCITX_MCBOPOMOFO_ERROR() << "Failed to load language model from: " << dataPath;
        if (hSingleInstanceMutex) {
            ReleaseMutex(hSingleInstanceMutex);
            CloseHandle(hSingleInstanceMutex);
        }
        return 1;
    }

    std::string userDir = fcitx5_compat::userDirectory();
    std::string userPhrasesPath = userDir + "/user.txt";
    std::string excludedPhrasesPath = userDir + "/exclude.txt";
    
    // Create empty files if they don't exist
    if (!std::filesystem::exists(userPhrasesPath)) {
        std::ofstream(userPhrasesPath).close();
    }
    if (!std::filesystem::exists(excludedPhrasesPath)) {
        std::ofstream(excludedPhrasesPath).close();
    }

    lm->loadUserPhrases(userPhrasesPath.c_str(), excludedPhrasesPath.c_str());

    // Set macro converter in LM
    InputMacroController macroController;
    lm->setMacroConverter([&macroController](const std::string& input) {
        return macroController.handle(input);
    });

    std::shared_ptr<KeyHandler> keyHandler(new KeyHandler(
        lm, 
        std::shared_ptr<VariantAnnotator>(nullptr), 
        std::make_shared<WinUserPhraseAdder>(lm), 
        std::unique_ptr<LocalizedStrings>(new DummyLocalizedStrings())
    ));

    std::string dictionaryServiceJsonPath = (exeDir / "data" / "dictionary_service.json").string();
    LogDataFileStatus("Dictionary service file", dictionaryServiceJsonPath);
    keyHandler->getDictionaryServices()->load(dictionaryServiceJsonPath);
    FCITX_MCBOPOMOFO_INFO() << "Dictionary service load attempted from: "
                            << dictionaryServiceJsonPath;

    ServerUI ui;
    InputController controller(keyHandler, &ui);
    ui.controller = &controller;
    g_Controller = &controller;

    controller.SetDataDirectory(exeDir / "data");

    Settings settings;
    settings.ApplyTo(controller);
    std::mutex reloadMutex;

    auto reloadSettings = [&]() {
        FCITX_MCBOPOMOFO_INFO() << "Reloading settings from: "
                                << (std::filesystem::path(userDir) / "mcbopomofo.ini").string();
        settings.Load();
        settings.ApplyTo(controller);
    };

    auto reloadUserPhrases = [&]() {
        FCITX_MCBOPOMOFO_INFO() << "Reloading user phrase files from: "
                                << userPhrasesPath << " and " << excludedPhrasesPath;
        lm->loadUserPhrases(userPhrasesPath.c_str(), excludedPhrasesPath.c_str());
    };

    ServerFileReloader fileReloader(
        std::filesystem::path(userDir) / "mcbopomofo.ini",
        userPhrasesPath,
        excludedPhrasesPath,
        [&]() {
            std::lock_guard<std::mutex> lock(reloadMutex);
            reloadSettings();
        },
        [&]() {
            std::lock_guard<std::mutex> lock(reloadMutex);
            reloadUserPhrases();
        });
    fileReloader.LogWatchedFiles();

    FCITX_MCBOPOMOFO_INFO() << "Starting Named Pipe server at " << IPC::PIPE_NAME;

    IPC::NamedPipeServer server(IPC::PIPE_NAME, [&](const std::string& req) {
        std::lock_guard<std::mutex> lock(reloadMutex);

        // Reset UI payload before processing
        ui.currentState.commitString.clear();

        IPC::KeyEventPayload keyReq;
        if (IPC::DeserializeKeyEvent(req, keyReq)) {
            FCITX_MCBOPOMOFO_INFO() << "IPC Recv: VK=" << keyReq.vk << ", ASCII=" << keyReq.ascii << ", SHIFT=" << keyReq.shift << ", CTRL=" << keyReq.ctrl;
            bool consumed = true;
            if (IsCtrlSpace(keyReq)) {
                FCITX_MCBOPOMOFO_INFO() << "IPC Ctrl+Space: Chinese/English mode toggle handled before input controller.";
            } else {
                consumed = controller.HandleKey(MapIPCKey(keyReq));
            }
            ui.currentState.consumed = consumed;
            return IPC::SerializeStateUpdate(ui.currentState);
        }
        
        IPC::SelectCandidatePayload selReq;
        if (IPC::DeserializeSelectCandidate(req, selReq)) {
            FCITX_MCBOPOMOFO_INFO() << "IPC Recv: SELECT_CANDIDATE Index=" << selReq.index;
            controller.SelectCandidate(selReq.index);
            ui.currentState.consumed = true;
            return IPC::SerializeStateUpdate(ui.currentState);
        }

        if (IPC::IsReloadSettingsCommand(req)) {
            FCITX_MCBOPOMOFO_INFO() << "IPC Recv: RELOAD_SETTINGS";
            reloadSettings();
            ui.currentState.consumed = true;
            return IPC::SerializeStateUpdate(ui.currentState);
        }

        if (IPC::IsResetCommand(req)) {
            FCITX_MCBOPOMOFO_INFO() << "IPC Recv: RESET";
            controller.Reset();
            ui.currentState.consumed = true;
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
    nid.uID = 1u;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_USER_TRAY;
    nid.hIcon = LoadIcon(NULL, IDI_APPLICATION); // Standard exe icon
    wcscpy_s(nid.szTip, L"小麥注音伺服器 (Win-McBopomofo)");

    Shell_NotifyIconW(NIM_ADD, &nid);

    FCITX_MCBOPOMOFO_INFO() << "Server is running in background. Check System Tray to exit.";

    // Standard message loop to keep the process alive and poll file changes.
    MSG msg;
    bool running = true;
    while (running) {
        DWORD waitResult = MsgWaitForMultipleObjects(0, nullptr, FALSE, 1000, QS_ALLINPUT);
        if (waitResult == WAIT_TIMEOUT) {
            fileReloader.Check();
            continue;
        }

        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                running = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    
    server.Stop();
    Shell_NotifyIconW(NIM_DELETE, &nid);
    DestroyWindow(hwndTray);

    if (hSingleInstanceMutex) {
        ReleaseMutex(hSingleInstanceMutex);
        CloseHandle(hSingleInstanceMutex);
    }

    if (g_RestartRequested) {
        RelaunchCurrentProcess();
    }

    return 0;
}
