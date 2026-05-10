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
#include <shellapi.h>

#include <array>
#include <filesystem>
#include <fstream>
#include <functional>
#include <mutex>

#include "InputController.h"
#include "InputMacro.h"
#include "KeyHandler.h"
#include "Log.h"
#include "McBopomofoLM.h"
#include "NamedPipe.h"
#include "PathCompat.h"
#include "Settings.h"
#include "SettingsApp.h"
#include "UIInterface.h"
#include "UTF8Helper.h"
#include "UTFHelper.h"
#include "VariantAnnotator.h"
#include "WindowsKeyBridge.h"
#include "resource.h"

using namespace McBopomofo;

#define WM_USER_TRAY (WM_USER + 1)
#define IDM_RESTART 1001
#define IDM_EXIT 1002

constexpr const wchar_t* kServerSingleInstanceMutexName =
    L"Local\\WinMcBopomofoServerSingleInstance";
InputController* g_Controller = nullptr;
bool g_RestartRequested = false;

namespace {

void LogDataFileStatus(const char* label, const std::filesystem::path& path) {
  FCITX_MCBOPOMOFO_INFO() << label << ": " << path.string()
                          << ", exists: " << std::filesystem::exists(path);
}

std::shared_ptr<VariantAnnotator> LoadVariantAnnotator(
    const std::filesystem::path& exeDir) {
  auto annotator = std::make_shared<VariantAnnotator>();
  const std::filesystem::path puaPath = exeDir / "data" / "bpmfvs-pua.txt";
  const std::filesystem::path variantsPath =
      exeDir / "data" / "bpmfvs-variants.txt";

  LogDataFileStatus("Bopomofo annotation file", puaPath);
  LogDataFileStatus("Bopomofo annotation file", variantsPath);

  const bool puaLoaded = annotator->loadPUAFile(puaPath);
  const bool variantsLoaded = annotator->loadVariantsFile(variantsPath);

  FCITX_MCBOPOMOFO_INFO() << "Bopomofo annotation PUA db loaded: " << puaLoaded;
  FCITX_MCBOPOMOFO_INFO() << "Bopomofo annotation variants db loaded: "
                          << variantsLoaded;

  if (!puaLoaded && !variantsLoaded) {
    FCITX_MCBOPOMOFO_WARN()
        << "Bopomofo annotation data failed to load; annotation mode will"
        << " remain inactive.";
  }

  return annotator;
}

void OpenFileInExplorer(const std::wstring& path) {
  std::wstring args = L"/select,\"" + path + L"\"";
  ShellExecuteW(nullptr, L"open", L"explorer.exe", args.c_str(), nullptr,
                SW_SHOWNORMAL);
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

  std::wstring args = L"-NoExit -Command \"Get-Content -Path '" + logPath +
                      L"' -Wait -Tail 50\"";
  ShellExecuteW(nullptr, L"open", L"powershell.exe", args.c_str(), nullptr,
                SW_SHOWNORMAL);
}

static void RelaunchCurrentProcess() {
  std::wstring commandLine = GetCommandLineW();
  STARTUPINFOW si = {};
  si.cb = sizeof(si);
  PROCESS_INFORMATION pi = {};
  std::wstring mutableCommandLine = commandLine;
  if (CreateProcessW(nullptr, mutableCommandLine.data(), nullptr, nullptr,
                     FALSE, 0, nullptr, nullptr, &si, &pi)) {
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
  }
}

}  // namespace

class WinUserPhraseAdder : public UserPhraseAdder {
 public:
  WinUserPhraseAdder(std::shared_ptr<McBopomofoLM> lm) : lm_(lm) {
    userPhrasesPath_ = fcitx5_compat::userDirectory() + "/user.txt";
    excludedPhrasesPath_ = fcitx5_compat::userDirectory() + "/exclude.txt";
  }

  void addUserPhrase(const std::string_view& reading,
                     const std::string_view& phrase) override {
    std::ofstream ofs(userPhrasesPath_, std::ios::app);
    if (ofs) {
      ofs << phrase << " " << reading << "\n";
      ofs.close();
      lm_->loadUserPhrases(userPhrasesPath_.c_str(),
                           excludedPhrasesPath_.c_str());
    }
  }

  void removeUserPhrase(const std::string_view& reading,
                        const std::string_view& phrase) override {
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
    lm_->loadUserPhrases(userPhrasesPath_.c_str(),
                         excludedPhrasesPath_.c_str());
  }

 private:
  std::shared_ptr<McBopomofoLM> lm_;
  std::string userPhrasesPath_;
  std::string excludedPhrasesPath_;
};

class ServerUI : public UIInterface {
 public:
  IPC::StateUpdatePayload currentState;

  void reset() override {
    std::string savedCommit = currentState.commitString;
    currentState = IPC::StateUpdatePayload();
    currentState.commitString = savedCommit;
  }

  void commitString(const std::string& text) override {
    currentState.commitString += text;
  }

  void update(const IPC::StateUpdatePayload& state) override {
    std::string savedCommit = currentState.commitString;
    currentState = state;
    currentState.commitString = savedCommit;
  }
};

class WinLocalizedStrings : public LocalizedStrings,
                           public InputController::LocalizedStrings {
 public:
  std::string cursorIsBetweenSyllables(const std::string& prevReading,
                                       const std::string& nextReading) override {
    std::wstring fmt =
        LoadLocalizedStringW(GetModuleHandle(NULL), IDS_CURSOR_BETWEEN_SYLLABLES);
    WCHAR buffer[256] = {};
    swprintf_s(buffer, fmt.c_str(), Utf8ToUtf16(prevReading).c_str(),
               Utf8ToUtf16(nextReading).c_str());
    return Utf16ToUtf8(buffer);
  }

  std::string syllablesRequired(size_t syllables) override {
    std::wstring fmt =
        LoadLocalizedStringW(GetModuleHandle(NULL), IDS_SYLLABLES_REQUIRED);
    WCHAR buffer[128] = {};
    swprintf_s(buffer, fmt.c_str(), static_cast<int>(syllables));
    return Utf16ToUtf8(buffer);
  }

  std::string syllablesMaximum(size_t syllables) override {
    std::wstring fmt =
        LoadLocalizedStringW(GetModuleHandle(NULL), IDS_SYLLABLES_MAXIMUM);
    WCHAR buffer[128] = {};
    swprintf_s(buffer, fmt.c_str(), static_cast<int>(syllables));
    return Utf16ToUtf8(buffer);
  }

  std::string phraseAlreadyExists() override {
    return Utf16ToUtf8(
        LoadLocalizedStringW(GetModuleHandle(NULL), IDS_PHRASE_ALREADY_EXISTS));
  }

  std::string pressEnterToAddThePhrase() override {
    return Utf16ToUtf8(LoadLocalizedStringW(GetModuleHandle(NULL),
                                            IDS_PRESS_ENTER_TO_ADD_THE_PHRASE));
  }

  std::string markedWithSyllablesAndStatus(const std::string&,
                                           const std::string&,
                                           const std::string& status) override {
    return status;
  }

  std::string bopomofoFontAnnotationModeTooltip(bool, bool pua) override {
    UINT id = pua ? IDS_FONT_ANNOTATION_MODE_TOOLTIP_ADVANCED
                  : IDS_FONT_ANNOTATION_MODE_TOOLTIP;
    return Utf16ToUtf8(LoadLocalizedStringW(GetModuleHandle(NULL), id));
  }

  std::string markingNotAvailableInFontAnnotationMode() override {
    return Utf16ToUtf8(LoadLocalizedStringW(
        GetModuleHandle(NULL),
        IDS_MARKING_NOT_AVAILABLE_IN_FONT_ANNOTATION_MODE));
  }

  // InputController::LocalizedStrings
  std::string boost() override {
    return Utf16ToUtf8(LoadLocalizedStringW(GetModuleHandle(NULL), IDS_BOOST));
  }
  std::string exclude() override {
    return Utf16ToUtf8(LoadLocalizedStringW(GetModuleHandle(NULL), IDS_EXCLUDE));
  }
  std::string cancel() override {
    return Utf16ToUtf8(LoadLocalizedStringW(GetModuleHandle(NULL), IDS_CANCEL));
  }
  std::string boostPrompt() override {
    return Utf16ToUtf8(
        LoadLocalizedStringW(GetModuleHandle(NULL), IDS_BOOST_PROMPT));
  }
  std::string excludePrompt() override {
    return Utf16ToUtf8(
        LoadLocalizedStringW(GetModuleHandle(NULL), IDS_EXCLUDE_PROMPT));
  }
};

class DummyUserPhraseAdder : public UserPhraseAdder {
 public:
  void addUserPhrase(const std::string_view&,
                     const std::string_view&) override {}
  void removeUserPhrase(const std::string_view&,
                        const std::string_view&) override {}
};

class WatchedFile {
 public:
  explicit WatchedFile(std::filesystem::path path) : path_(std::move(path)) {
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
    FCITX_MCBOPOMOFO_INFO()
        << "Watching settings file: " << settingsFile_.Path().string();
    FCITX_MCBOPOMOFO_INFO()
        << "Watching user phrases file: " << userPhrasesFile_.Path().string();
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
      FCITX_MCBOPOMOFO_INFO()
          << "User phrase files changed; reloading user phrases.";
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

#define IDM_SETTINGS 1003
#define IDM_OPEN_USER_PHRASES 1004
#define IDM_OPEN_EXCLUDED_PHRASES 1005
#define IDM_OPEN_USER_DIR 1006
#define IDM_TOGGLE_CONVERSION 1007
#define IDM_OPEN_LOG_FOLDER 1008
#define IDM_TOGGLE_LOGGING 1009
#define IDM_TRACE_LOG 1010
#define IDM_EXIT 1002

static LRESULT CALLBACK TrayWndProc(HWND hwnd, UINT msg, WPARAM wParam,
                                    LPARAM lParam) {
  if (msg == WM_USER_TRAY) {
    if (LOWORD(lParam) == WM_RBUTTONUP) {
      POINT pt;
      GetCursorPos(&pt);
      HMENU hMenu = CreatePopupMenu();

      UINT loggingFlags = MF_BYPOSITION | MF_STRING;
      if (ServerLoggingEnabled()) {
        loggingFlags |= MF_CHECKED;
      }

      HINSTANCE hInst = GetModuleHandle(NULL);

      InsertMenuW(hMenu, 0xFFFFFFFFU, MF_BYPOSITION | MF_STRING, IDM_SETTINGS,
                  LoadLocalizedStringW(hInst, IDS_SETTINGS).c_str());
      InsertMenuW(hMenu, 0xFFFFFFFFU, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
      InsertMenuW(hMenu, 0xFFFFFFFFU, MF_BYPOSITION | MF_STRING,
                  IDM_OPEN_USER_PHRASES,
                  LoadLocalizedStringW(hInst, IDS_EDIT_USER_PHRASES).c_str());
      InsertMenuW(hMenu, 0xFFFFFFFFU, MF_BYPOSITION | MF_STRING,
                  IDM_OPEN_EXCLUDED_PHRASES,
                  LoadLocalizedStringW(hInst, IDS_EDIT_EXCLUDED_PHRASES).c_str());
      InsertMenuW(hMenu, 0xFFFFFFFFU, MF_BYPOSITION | MF_STRING,
                  IDM_OPEN_USER_DIR,
                  LoadLocalizedStringW(hInst, IDS_OPEN_USER_DATA_FOLDER).c_str());
      InsertMenuW(hMenu, 0xFFFFFFFFU, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
      InsertMenuW(hMenu, 0xFFFFFFFFU, MF_BYPOSITION | MF_STRING,
                  IDM_OPEN_LOG_FOLDER,
                  LoadLocalizedStringW(hInst, IDS_OPEN_LOG_FOLDER).c_str());
      InsertMenuW(hMenu, 0xFFFFFFFFU, loggingFlags, IDM_TOGGLE_LOGGING,
                  LoadLocalizedStringW(hInst, IDS_ENABLE_LOGGING).c_str());
      InsertMenuW(hMenu, 0xFFFFFFFFU, MF_BYPOSITION | MF_STRING, IDM_TRACE_LOG,
                  LoadLocalizedStringW(hInst, IDS_TRACE_LOG).c_str());
      InsertMenuW(hMenu, 0xFFFFFFFFU, MF_BYPOSITION | MF_STRING, IDM_RESTART,
                  LoadLocalizedStringW(hInst, IDS_RESTART_SERVER).c_str());
      SetForegroundWindow(hwnd);
      TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0,
                     hwnd, NULL);
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
        g_Controller->toggleChineseConversion();
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

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
  HANDLE hSingleInstanceMutex =
      CreateMutexW(nullptr, TRUE, kServerSingleInstanceMutexName);
  if (hSingleInstanceMutex && GetLastError() == ERROR_ALREADY_EXISTS) {
    return 0;
  }

  HINSTANCE hInst = GetModuleHandle(NULL);
  FCITX_MCBOPOMOFO_INFO() << Utf16ToUtf8(LoadLocalizedStringW(hInst, IDS_DAEMON_STARTING));

  WCHAR szExePath[MAX_PATH];
  GetModuleFileNameW(NULL, szExePath, MAX_PATH);
  std::filesystem::path exeDir = std::filesystem::path(szExePath).parent_path();

  std::string dataPath = (exeDir / "data" / "data.txt").string();
  int argc = 0;
  LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
  if (argv != nullptr && argc >= 2) {
    dataPath = Utf16ToUtf8(argv[1]);
  }
  LocalFree(argv);
  LogDataFileStatus("Language model data file", dataPath);

  auto lm = std::make_shared<McBopomofoLM>();
  lm->loadLanguageModel(dataPath.c_str());
  FCITX_MCBOPOMOFO_INFO() << "Language model loaded: "
                          << lm->isDataModelLoaded();

  std::string assocPath =
      (exeDir / "data" / "associated-phrases-v2.txt").string();
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
    FCITX_MCBOPOMOFO_INFO()
        << "Phrase replacement map loaded from: " << variantsPath;
  } else {
    FCITX_MCBOPOMOFO_WARN()
        << "Phrase replacement file missing: " << variantsPath;
  }

  auto variantAnnotator = LoadVariantAnnotator(exeDir);

  if (!lm->isDataModelLoaded()) {
    FCITX_MCBOPOMOFO_ERROR()
        << "Failed to load language model from: " << dataPath;
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
      lm, variantAnnotator, std::make_shared<WinUserPhraseAdder>(lm),
      std::unique_ptr<LocalizedStrings>(new WinLocalizedStrings())));

  std::string dictionaryServiceJsonPath =
      (exeDir / "data" / "dictionary_service.json").string();
  LogDataFileStatus("Dictionary service file", dictionaryServiceJsonPath);
  keyHandler->getDictionaryServices()->load(dictionaryServiceJsonPath);
  FCITX_MCBOPOMOFO_INFO() << "Dictionary service load attempted from: "
                          << dictionaryServiceJsonPath;

  ServerUI ui;
  InputController controller(
      keyHandler, &ui,
      std::unique_ptr<InputController::LocalizedStrings>(
          new WinLocalizedStrings()));
  g_Controller = &controller;

  controller.setDataDirectory(exeDir / "data");

  Settings settings;
  settings.applyTo(controller);
  std::mutex reloadMutex;

  auto reloadSettings = [&]() {
    FCITX_MCBOPOMOFO_INFO()
        << "Reloading settings from: "
        << (std::filesystem::path(userDir) / "mcbopomofo.ini").string();
    settings.load();
    settings.applyTo(controller);
  };

  auto reloadUserPhrases = [&]() {
    FCITX_MCBOPOMOFO_INFO()
        << "Reloading user phrase files from: " << userPhrasesPath << " and "
        << excludedPhrasesPath;
    lm->loadUserPhrases(userPhrasesPath.c_str(), excludedPhrasesPath.c_str());
  };

  ServerFileReloader fileReloader(
      std::filesystem::path(userDir) / "mcbopomofo.ini", userPhrasesPath,
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
      FCITX_MCBOPOMOFO_INFO()
          << "IPC Recv: VK=" << keyReq.vk << ", ASCII=" << keyReq.ascii
          << ", SHIFT=" << keyReq.shift << ", CTRL=" << keyReq.ctrl;
      bool consumed = true;
      consumed = controller.handleKey(mapIpcKey(keyReq));
      ui.currentState.consumed = consumed;
      return IPC::SerializeStateUpdate(ui.currentState);
    }

    IPC::SelectCandidatePayload selReq;
    if (IPC::DeserializeSelectCandidate(req, selReq)) {
      FCITX_MCBOPOMOFO_INFO()
          << "IPC Recv: SELECT_CANDIDATE Index=" << selReq.index;
      controller.selectCandidate(selReq.index);
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
      controller.reset();
      ui.currentState.consumed = true;
      return IPC::SerializeStateUpdate(ui.currentState);
    }

    if (IPC::IsOpenSettingsCommand(req)) {
      FCITX_MCBOPOMOFO_INFO() << "IPC Recv: OPEN_SETTINGS";
      OpenSettingsApp();
      ui.currentState.consumed = true;
      return IPC::SerializeStateUpdate(ui.currentState);
    }

    FCITX_MCBOPOMOFO_WARN() << "IPC Failed to deserialize request.";
    return std::string();
  });

  server.Start();

  // Create a hidden window for the Tray Icon
  WNDCLASSEXW wcex = {sizeof(WNDCLASSEXW)};
  wcex.lpfnWndProc = TrayWndProc;
  wcex.hInstance = GetModuleHandle(NULL);
  wcex.hIcon = LoadIconW(wcex.hInstance, MAKEINTRESOURCEW(IDI_ICON_APP));
  wcex.hIconSm = LoadIconW(wcex.hInstance, MAKEINTRESOURCEW(IDI_ICON_APP));
  wcex.lpszClassName = L"WinMcBopomofoServerTray";
  RegisterClassExW(&wcex);

  HWND hwndTray =
      CreateWindowExW(0, L"WinMcBopomofoServerTray", L"", 0, 0, 0, 0, 0,
                      HWND_MESSAGE, NULL, wcex.hInstance, NULL);

  NOTIFYICONDATAW nid = {sizeof(NOTIFYICONDATAW)};
  nid.hWnd = hwndTray;
  nid.uID = 1u;
  nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
  nid.uCallbackMessage = WM_USER_TRAY;
  nid.hIcon = LoadIconW(wcex.hInstance, MAKEINTRESOURCEW(IDI_ICON_APP));
  wcscpy_s(nid.szTip, LoadLocalizedStringW(hInst, IDS_TRAY_TIP).c_str());

  Shell_NotifyIconW(NIM_ADD, &nid);

  FCITX_MCBOPOMOFO_INFO()
      << Utf16ToUtf8(LoadLocalizedStringW(hInst, IDS_SERVER_RUNNING));

  // Standard message loop to keep the process alive and poll file changes.
  MSG msg;
  bool running = true;
  while (running) {
    DWORD waitResult =
        MsgWaitForMultipleObjects(0, nullptr, FALSE, 1000, QS_ALLINPUT);
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
