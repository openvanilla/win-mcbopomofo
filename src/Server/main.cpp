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
#include <dwmapi.h>
#include <shellapi.h>
#include <uxtheme.h>
#include <windows.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cwchar>
#include <filesystem>
#include <fstream>
#include <functional>
#include <mutex>
#include <string>

#include "CandidateWindow.h"
#include "CandidateWindowColors.h"
#include "InputController.h"
#include "InputMacro.h"
#include "KeyHandler.h"
#include "LanguageModelLoader.h"
#include "Log.h"
#include "McBopomofoLM.h"
#include "NamedPipe.h"
#include "PathCompat.h"
#include "Settings.h"
#include "SettingsApp.h"
#include "TooltipWindow.h"
#include "UIInterface.h"
#include "UTF8Helper.h"
#include "UTFHelper.h"
#include "VariantAnnotator.h"
#include "WindowsKeyBridge.h"
#include "resource.h"

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")

using namespace McBopomofo;

#define WM_USER_TRAY (WM_USER + 1)
#define WM_SERVER_UI_CHANGED (WM_USER + 2)
#define IDM_RESTART 1001
#define IDM_EXIT 1002
#define IDM_STOP_SERVER 1011

constexpr const wchar_t* kServerSingleInstanceMutexName =
    L"Local\\WinMcBopomofoServerSingleInstance";
InputController* g_Controller = nullptr;
bool g_RestartRequested = false;
std::function<void()> g_ReloadSettingsCallback;
std::function<void(bool)> g_SaveServerLoggingEnabledCallback;

class ServerPopupController {
 public:
  struct PopupLayout {
    bool showCandidateWindow = false;
    bool showTooltipWindow = false;
    uint64_t ownerHwnd = 0;
    int anchorLeft = 0;
    int anchorTop = 0;
    int anchorRight = 0;
    int anchorBottom = 0;
  };

  void Create(HINSTANCE hInstance) {
    candidateWindow_.Create(hInstance);
    tooltipWindow_.Create(hInstance);
  }

  void Destroy() {
    candidateWindow_.Destroy();
    tooltipWindow_.Destroy();
  }

  void SetState(const IPC::StateUpdatePayload& state) {
    std::lock_guard<std::mutex> lock(mutex_);
    state_ = state;
  }

  void SetLayout(const PopupLayout& layout) {
    std::lock_guard<std::mutex> lock(mutex_);
    layout_ = layout;
    hasLayout_ = true;
  }

  void ApplyPending() {
    IPC::StateUpdatePayload state;
    PopupLayout layout;
    bool hasLayout = false;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      state = state_;
      layout = layout_;
      hasLayout = hasLayout_;
    }

    RECT anchor = {static_cast<LONG>(layout.anchorLeft),
                   static_cast<LONG>(layout.anchorTop),
                   static_cast<LONG>(layout.anchorRight),
                   static_cast<LONG>(layout.anchorBottom)};

    if (!hasLayout ||
        (!layout.showCandidateWindow && !layout.showTooltipWindow)) {
      candidateWindow_.Hide();
      tooltipWindow_.Hide();
      return;
    }

    HWND owner =
        reinterpret_cast<HWND>(static_cast<uintptr_t>(layout.ownerHwnd));

    const bool showTooltip = layout.showTooltipWindow && !state.tooltip.empty();
    const bool showCandidate =
        layout.showCandidateWindow && !state.candidates.empty();

    if (showTooltip) {
      tooltipWindow_.SetOwnerWindow(owner);
      tooltipWindow_.UpdateUI(state.tooltip);
    } else {
      tooltipWindow_.Hide();
    }

    if (showCandidate) {
      candidateWindow_.SetOwnerWindow(owner);
      CandidateWindow::UpdateUIRequest request;
      request.candidates = state.candidates;
      request.cursorIndex = state.candidateIndex;
      request.forceVertical = state.forceVertical;
      request.selectionStyle = state.selectionStyle;
      request.candidateFontSize = state.candidateFontSize;
      request.hint = state.hint;
      request.candidateWindowVertical = state.candidateWindowVertical;
      request.candidateKeys = state.candidateKeys;
      request.candidateKeysCount = state.candidateKeysCount;
      request.colors = state.candidateWindowColors;
      candidateWindow_.UpdateUI(request);
    } else {
      candidateWindow_.Hide();
    }

    MoveWindows(anchor, showCandidate, showTooltip);
  }

 private:
  void MoveWindows(const RECT& anchor, bool showCandidate, bool showTooltip) {
    if (!showCandidate && !showTooltip) {
      return;
    }

    POINT ptTopLeft = {anchor.left, anchor.top};
    HMONITOR hMonitor = MonitorFromPoint(ptTopLeft, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = {0};
    mi.cbSize = sizeof(MONITORINFO);
    GetMonitorInfoW(hMonitor, &mi);

    const int screenBottom = mi.rcWork.bottom;
    const int screenRight = mi.rcWork.right;
    const int screenLeft = mi.rcWork.left;

    const int candidateHeight =
        showCandidate ? candidateWindow_.GetHeight() : 0;
    const int candidateWidth = showCandidate ? candidateWindow_.GetWidth() : 0;
    const int tooltipHeight = showTooltip ? tooltipWindow_.GetHeight() : 0;
    const int tooltipWidth = showTooltip ? tooltipWindow_.GetWidth() : 0;
    const int gap = showCandidate && showTooltip ? 4 : 0;
    const int totalRequiredHeight = tooltipHeight + gap + candidateHeight;
    const int yBelow = anchor.bottom + 10;

    int candidateY = 0;
    int tooltipY = 0;
    if (yBelow + totalRequiredHeight > screenBottom) {
      candidateY = anchor.top - candidateHeight - 10;
      tooltipY = candidateY - tooltipHeight - gap;
    } else {
      tooltipY = yBelow;
      candidateY = yBelow + (showTooltip ? tooltipHeight + gap : 0);
    }

    int x = anchor.left;
    const int maxWidth = std::max(candidateWidth, tooltipWidth);
    if (x + maxWidth > screenRight) {
      x = screenRight - maxWidth;
    }
    if (x < screenLeft) {
      x = screenLeft;
    }

    if (showTooltip) {
      tooltipWindow_.Move(x, tooltipY);
    }
    if (showCandidate) {
      candidateWindow_.Move(x, candidateY);
    }
  }

  std::mutex mutex_;
  IPC::StateUpdatePayload state_;
  PopupLayout layout_;
  bool hasLayout_ = false;
  CandidateWindow candidateWindow_;
  TooltipWindow tooltipWindow_;
};

ServerPopupController* g_ServerPopupController = nullptr;

namespace {

constexpr const char* kDisabledAppsFilename = "disabled-apps.txt";
constexpr const char* kDefaultDisabledApps[] = {
    "SheepShaver.exe",
};

std::string NormalizeProcessName(std::string value) {
  size_t start = 0;
  while (start < value.size() &&
         std::isspace(static_cast<unsigned char>(value[start]))) {
    ++start;
  }

  size_t end = value.size();
  while (end > start &&
         std::isspace(static_cast<unsigned char>(value[end - 1]))) {
    --end;
  }

  value = value.substr(start, end - start);
  size_t slash = value.find_last_of("\\/");
  if (slash != std::string::npos) {
    value.erase(0, slash + 1);
  }

  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char c) {
                   return static_cast<char>(std::tolower(c));
                 });
  return value;
}

std::filesystem::path DisabledAppsPath(const std::string& userDir) {
  return std::filesystem::path(userDir) / kDisabledAppsFilename;
}

void EnsureDisabledAppsFile(const std::filesystem::path& path) {
  if (std::filesystem::exists(path)) {
    return;
  }

  std::filesystem::create_directories(path.parent_path());

  std::ofstream file(path);
  if (!file) {
    FCITX_MCBOPOMOFO_INFO()
        << "Failed to create disabled apps file: " << path.string();
    return;
  }

  file << "# One process name per line. Matching is case-insensitive.\n";
  for (const char* app : kDefaultDisabledApps) {
    file << app << "\n";
  }
  FCITX_MCBOPOMOFO_INFO() << "Created disabled apps file: " << path.string();
}

bool IsProcessDisabledByList(const std::filesystem::path& path,
                             const std::string& processName) {
  const std::string normalizedProcess = NormalizeProcessName(processName);
  if (normalizedProcess.empty()) {
    return false;
  }

  std::ifstream file(path);
  if (!file) {
    return false;
  }

  std::string line;
  while (std::getline(file, line)) {
    const std::string normalizedLine = NormalizeProcessName(line);
    if (normalizedLine.empty() || normalizedLine[0] == '#' ||
        normalizedLine[0] == ';') {
      continue;
    }
    if (normalizedLine == normalizedProcess) {
      return true;
    }
  }
  return false;
}

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

void ApplyDarkThemeToWindow(HWND hwnd) {
  BOOL dark = IsDarkModeEnabled();
  DwmSetWindowAttribute(hwnd, 20 /* DWMWA_USE_IMMERSIVE_DARK_MODE */, &dark,
                        sizeof(dark));
  SetWindowTheme(hwnd, dark ? L"DarkMode_Explorer" : L"Explorer", nullptr);
}

void LogDataFileStatus(const char* label, const std::filesystem::path& path) {
  FCITX_MCBOPOMOFO_INFO() << label << ": " << path.string()
                          << ", exists: " << std::filesystem::exists(path);
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
  if (g_SaveServerLoggingEnabledCallback) {
    g_SaveServerLoggingEnabledCallback(enabled);
  }
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

static void EnablePerMonitorDpiAwareness() {
  HMODULE user32 = GetModuleHandleW(L"user32.dll");
  if (user32) {
    using SetProcessDpiAwarenessContextFn =
        BOOL(WINAPI*)(DPI_AWARENESS_CONTEXT);
    auto setProcessDpiAwarenessContext =
        reinterpret_cast<SetProcessDpiAwarenessContextFn>(
            GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
    if (setProcessDpiAwarenessContext &&
        setProcessDpiAwarenessContext(
            DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
      return;
    }
  }

  SetProcessDPIAware();
}

}  // namespace

static bool IsSystemColorSettingsChange(UINT msg, LPARAM lParam) {
  if (msg == WM_DWMCOLORIZATIONCOLORCHANGED || msg == WM_THEMECHANGED ||
      msg == WM_SYSCOLORCHANGE) {
    return true;
  }
  if (msg != WM_SETTINGCHANGE) {
    return false;
  }

  if (lParam == 0) {
    return true;
  }

  const auto area = reinterpret_cast<LPCWSTR>(lParam);
  return wcscmp(area, L"ImmersiveColorSet") == 0 ||
         wcscmp(area, L"WindowsThemeElement") == 0 ||
         wcscmp(area, L"UserPreferences") == 0 ||
         wcscmp(area, L"Policy") == 0;
}

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

class WinLocalizedStrings
    : public McBopomofo::LocalizedStrings,
      public McBopomofo::InputController::LocalizedStrings,
      public McBopomofo::LanguageModelLoader::LocalizedStrings {
 public:
  // LanguageModelLoader::LocalizedStrings
  std::string userPhraseFileHeader() override {
    return "# McBopomofo User Phrases\n";
  }
  std::string excludedPhraseFileHeader() override {
    return "# McBopomofo Excluded Phrases\n";
  }

  // KeyHandler::LocalizedStrings
  std::string cursorIsBetweenSyllables(
      const std::string& prevReading, const std::string& nextReading) override {
    std::wstring fmt = LoadLocalizedStringW(GetModuleHandle(NULL),
                                             IDS_CURSOR_BETWEEN_SYLLABLES);
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
    return Utf16ToUtf8(
        LoadLocalizedStringW(GetModuleHandle(NULL), IDS_EXCLUDE));
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
                     std::function<void()> reloadSettings,
                     std::function<void()> reloadUserPhrases)
      : settingsFile_(std::move(settingsPath)),
        reloadSettings_(std::move(reloadSettings)),
        reloadUserPhrases_(std::move(reloadUserPhrases)) {}

  void LogWatchedFiles() const {
    FCITX_MCBOPOMOFO_INFO()
        << "Watching settings file: " << settingsFile_.Path().string();
  }

  void Check() {
    if (settingsFile_.HasChanged()) {
      FCITX_MCBOPOMOFO_INFO() << "Settings file changed; reloading settings.";
      reloadSettings_();
    }
    reloadUserPhrases_();
  }

 private:
  WatchedFile settingsFile_;
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
      InsertMenuW(
          hMenu, 0xFFFFFFFFU, MF_BYPOSITION | MF_STRING,
          IDM_OPEN_EXCLUDED_PHRASES,
          LoadLocalizedStringW(hInst, IDS_EDIT_EXCLUDED_PHRASES).c_str());
      InsertMenuW(
          hMenu, 0xFFFFFFFFU, MF_BYPOSITION | MF_STRING, IDM_OPEN_USER_DIR,
          LoadLocalizedStringW(hInst, IDS_OPEN_USER_DATA_FOLDER).c_str());
      InsertMenuW(hMenu, 0xFFFFFFFFU, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
      InsertMenuW(hMenu, 0xFFFFFFFFU, MF_BYPOSITION | MF_STRING,
                  IDM_OPEN_LOG_FOLDER,
                  LoadLocalizedStringW(hInst, IDS_OPEN_LOG_FOLDER).c_str());
      InsertMenuW(hMenu, 0xFFFFFFFFU, loggingFlags, IDM_TOGGLE_LOGGING,
                  LoadLocalizedStringW(hInst, IDS_ENABLE_LOGGING).c_str());
      InsertMenuW(hMenu, 0xFFFFFFFFU, MF_BYPOSITION | MF_STRING, IDM_TRACE_LOG,
                  LoadLocalizedStringW(hInst, IDS_TRACE_LOG).c_str());
      InsertMenuW(hMenu, 0xFFFFFFFFU, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
      InsertMenuW(hMenu, 0xFFFFFFFFU, MF_BYPOSITION | MF_STRING, IDM_RESTART,
                  LoadLocalizedStringW(hInst, IDS_RESTART_SERVER).c_str());
      InsertMenuW(hMenu, 0xFFFFFFFFU, MF_BYPOSITION | MF_STRING,
                  IDM_STOP_SERVER,
                  LoadLocalizedStringW(hInst, IDS_STOP_SERVER).c_str());
      ApplyDarkThemeToWindow(hwnd);
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
      ShellExecuteW(NULL, L"open", Utf8ToUtf16(path).c_str(), NULL, NULL,
                    SW_SHOW);
    } else if (LOWORD(wParam) == IDM_OPEN_EXCLUDED_PHRASES) {
      std::string path = fcitx5_compat::userDirectory() + "/exclude.txt";
      ShellExecuteW(NULL, L"open", Utf8ToUtf16(path).c_str(), NULL, NULL,
                    SW_SHOW);
    } else if (LOWORD(wParam) == IDM_OPEN_USER_DIR) {
      std::string path = fcitx5_compat::userDirectory();
      ShellExecuteW(NULL, L"open", Utf8ToUtf16(path).c_str(), NULL, NULL,
                    SW_SHOW);
    } else if (LOWORD(wParam) == IDM_OPEN_LOG_FOLDER) {
      OpenLogFolder();
    } else if (LOWORD(wParam) == IDM_TOGGLE_LOGGING) {
      ToggleLogging();
    } else if (LOWORD(wParam) == IDM_TRACE_LOG) {
      TraceLog();
    } else if (LOWORD(wParam) == IDM_RESTART) {
      RestartServer();
    } else if (LOWORD(wParam) == IDM_STOP_SERVER) {
      PostQuitMessage(0);
    }
    return 0;
  } else if (IsSystemColorSettingsChange(msg, lParam)) {
    if (g_ReloadSettingsCallback) {
      FCITX_MCBOPOMOFO_INFO()
          << "System color settings changed; reloading settings.";
      g_ReloadSettingsCallback();
    }
    return 0;
  } else if (msg == WM_SERVER_UI_CHANGED) {
    if (g_ServerPopupController) {
      g_ServerPopupController->ApplyPending();
    }
    return 0;
  }
  return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
  EnablePerMonitorDpiAwareness();

  HANDLE hSingleInstanceMutex =
      CreateMutexW(nullptr, TRUE, kServerSingleInstanceMutexName);
  if (hSingleInstanceMutex && GetLastError() == ERROR_ALREADY_EXISTS) {
    return 0;
  }

  HINSTANCE hInst = GetModuleHandle(NULL);
  FCITX_MCBOPOMOFO_INFO() << Utf16ToUtf8(
      LoadLocalizedStringW(hInst, IDS_DAEMON_STARTING));

  WCHAR szExePath[MAX_PATH];
  GetModuleFileNameW(NULL, szExePath, MAX_PATH);
  std::filesystem::path exeDir = std::filesystem::path(szExePath).parent_path();

  auto loader = std::make_shared<LanguageModelLoader>(
      std::make_unique<WinLocalizedStrings>());

  auto lm = loader->getLM();
  auto variantAnnotator = loader->getVariantAnnotator();

  int argc = 0;
  LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
  if (argv != nullptr && argc >= 2) {
    std::string dataPath = Utf16ToUtf8(argv[1]);
    LogDataFileStatus("Language model data file (custom override)", dataPath);
    lm->loadLanguageModel(dataPath.c_str());
  }
  LocalFree(argv);

  if (!lm->isDataModelLoaded()) {
    FCITX_MCBOPOMOFO_ERROR()
        << "Failed to load language model.";
    if (hSingleInstanceMutex) {
      ReleaseMutex(hSingleInstanceMutex);
      CloseHandle(hSingleInstanceMutex);
    }
    return 1;
  }

  std::shared_ptr<KeyHandler> keyHandler(new KeyHandler(
      lm, variantAnnotator, loader,
      std::unique_ptr<LocalizedStrings>(new WinLocalizedStrings())));

  std::string dictionaryServiceJsonPath =
      (exeDir / "data" / "dictionary_service.json").string();
  LogDataFileStatus("Dictionary service file", dictionaryServiceJsonPath);
  keyHandler->getDictionaryServices()->load(dictionaryServiceJsonPath);
  FCITX_MCBOPOMOFO_INFO() << "Dictionary service load attempted from: "
                          << dictionaryServiceJsonPath;

  ServerUI ui;
  ServerPopupController popupController;
  g_ServerPopupController = &popupController;
  InputController controller(keyHandler, &ui,
                             std::unique_ptr<InputController::LocalizedStrings>(
                                 new WinLocalizedStrings()));
  g_Controller = &controller;

  controller.setDataDirectory(exeDir / "data");

  Settings settings;
  settings.applyTo(controller);
  controller.setCandidateWindowColors(ReadCandidateWindowColors());
  std::mutex reloadMutex;

  std::string userDir = fcitx5_compat::userDirectory();
  std::filesystem::path disabledAppsPath = DisabledAppsPath(userDir);
  EnsureDisabledAppsFile(disabledAppsPath);
  HWND hwndTray = nullptr;

  auto reloadSettings = [&]() {
    FCITX_MCBOPOMOFO_INFO()
        << "Reloading settings from: "
        << (std::filesystem::path(userDir) / "mcbopomofo.ini").string();
    settings.load();
    settings.applyTo(controller);
    controller.setCandidateWindowColors(ReadCandidateWindowColors());
    controller.refreshUI();
    popupController.SetState(ui.currentState);
    if (hwndTray) {
      PostMessageW(hwndTray, WM_SERVER_UI_CHANGED, 0, 0);
    }
  };
  g_ReloadSettingsCallback = [&]() {
    std::lock_guard<std::mutex> lock(reloadMutex);
    reloadSettings();
  };
  g_SaveServerLoggingEnabledCallback = [&](bool enabled) {
    std::lock_guard<std::mutex> lock(reloadMutex);
    settings.setServerLoggingEnabled(enabled);
    settings.save();
  };

  ServerFileReloader fileReloader(
      std::filesystem::path(userDir) / "mcbopomofo.ini",
      [&]() {
        std::lock_guard<std::mutex> lock(reloadMutex);
        reloadSettings();
      },
      [&]() {
        std::lock_guard<std::mutex> lock(reloadMutex);
        loader->reloadUserModelsIfNeeded();
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
      if (keyReq.hasCoords) {
        ServerPopupController::PopupLayout layout;
        layout.showCandidateWindow = !ui.currentState.candidates.empty();
        layout.showTooltipWindow = !ui.currentState.tooltip.empty();
        layout.ownerHwnd = keyReq.ownerHwnd;
        layout.anchorLeft = keyReq.anchorLeft;
        layout.anchorTop = keyReq.anchorTop;
        layout.anchorRight = keyReq.anchorRight;
        layout.anchorBottom = keyReq.anchorBottom;
        popupController.SetLayout(layout);
      }
      popupController.SetState(ui.currentState);
      if (hwndTray) {
        PostMessageW(hwndTray, WM_SERVER_UI_CHANGED, 0, 0);
      }
      return IPC::SerializeStateUpdate(ui.currentState);
    }

    IPC::SelectCandidatePayload selReq;
    if (IPC::DeserializeSelectCandidate(req, selReq)) {
      FCITX_MCBOPOMOFO_INFO()
          << "IPC Recv: SELECT_CANDIDATE Index=" << selReq.index;
      controller.selectCandidate(selReq.index);
      ui.currentState.consumed = true;
      popupController.SetState(ui.currentState);
      if (hwndTray) {
        PostMessageW(hwndTray, WM_SERVER_UI_CHANGED, 0, 0);
      }
      return IPC::SerializeStateUpdate(ui.currentState);
    }

    if (IPC::IsReloadSettingsCommand(req)) {
      FCITX_MCBOPOMOFO_INFO() << "IPC Recv: RELOAD_SETTINGS";
      reloadSettings();
      ui.currentState.consumed = true;
      popupController.SetState(ui.currentState);
      if (hwndTray) {
        PostMessageW(hwndTray, WM_SERVER_UI_CHANGED, 0, 0);
      }
      return IPC::SerializeStateUpdate(ui.currentState);
    }

    if (IPC::IsResetCommand(req)) {
      FCITX_MCBOPOMOFO_INFO() << "IPC Recv: RESET";
      controller.reset();
      ui.currentState.consumed = true;
      popupController.SetState(ui.currentState);
      if (hwndTray) {
        PostMessageW(hwndTray, WM_SERVER_UI_CHANGED, 0, 0);
      }
      return IPC::SerializeStateUpdate(ui.currentState);
    }

    if (IPC::IsOpenSettingsCommand(req)) {
      FCITX_MCBOPOMOFO_INFO() << "IPC Recv: OPEN_SETTINGS";
      OpenSettingsApp();
      ui.currentState.consumed = true;
      popupController.SetState(ui.currentState);
      if (hwndTray) {
        PostMessageW(hwndTray, WM_SERVER_UI_CHANGED, 0, 0);
      }
      return IPC::SerializeStateUpdate(ui.currentState);
    }

    if (IPC::IsGetSettingsCommand(req)) {
      FCITX_MCBOPOMOFO_INFO() << "IPC Recv: GET_SETTINGS";
      IPC::ClientSettingsPayload payload;
      payload.shiftToggleOpenClose = settings.shiftToggleOpenClose();
      return IPC::SerializeClientSettings(payload);
    }

    IPC::ProcessDisabledQueryPayload processDisabledQuery;
    if (IPC::DeserializeProcessDisabledQuery(req, processDisabledQuery)) {
      IPC::ProcessDisabledResponsePayload payload;
      payload.disabled = IsProcessDisabledByList(
          disabledAppsPath, processDisabledQuery.processName);
      FCITX_MCBOPOMOFO_INFO()
          << "IPC Recv: IS_PROCESS_DISABLED process="
          << processDisabledQuery.processName
          << " disabled=" << payload.disabled;
      return IPC::SerializeProcessDisabledResponse(payload);
    }

    IPC::ClientLogPayload clientLogReq;
    if (IPC::DeserializeClientLog(req, clientLogReq)) {
      FCITX_MCBOPOMOFO_INFO()
          << "TIP[" << clientLogReq.processId << "][+" << clientLogReq.elapsedMs
          << "ms] " << clientLogReq.message;
      return std::string("1");
    }



    FCITX_MCBOPOMOFO_WARN() << "IPC Failed to deserialize request.";
    return std::string();
  });

  // Create a hidden window for the Tray Icon
  WNDCLASSEXW wcex = {sizeof(WNDCLASSEXW)};
  wcex.lpfnWndProc = TrayWndProc;
  wcex.hInstance = GetModuleHandle(NULL);
  wcex.hIcon = LoadIconW(wcex.hInstance, MAKEINTRESOURCEW(IDI_ICON_APP));
  wcex.hIconSm = LoadIconW(wcex.hInstance, MAKEINTRESOURCEW(IDI_ICON_APP));
  wcex.lpszClassName = L"WinMcBopomofoServerTray";
  RegisterClassExW(&wcex);

  hwndTray = CreateWindowExW(0, L"WinMcBopomofoServerTray", L"",
                             WS_OVERLAPPED, 0, 0, 0, 0, nullptr, NULL,
                             wcex.hInstance, NULL);
  popupController.Create(hInst);

  NOTIFYICONDATAW nid = {sizeof(NOTIFYICONDATAW)};
  nid.hWnd = hwndTray;
  nid.uID = 1u;
  nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
  nid.uCallbackMessage = WM_USER_TRAY;
  nid.hIcon = LoadIconW(wcex.hInstance, MAKEINTRESOURCEW(IDI_ICON_APP));
  wcscpy_s(nid.szTip, LoadLocalizedStringW(hInst, IDS_TRAY_TIP).c_str());

  Shell_NotifyIconW(NIM_ADD, &nid);

  server.Start();

  FCITX_MCBOPOMOFO_INFO() << Utf16ToUtf8(
      LoadLocalizedStringW(hInst, IDS_SERVER_RUNNING));

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
  popupController.Destroy();
  Shell_NotifyIconW(NIM_DELETE, &nid);
  DestroyWindow(hwndTray);
  g_ServerPopupController = nullptr;
  g_SaveServerLoggingEnabledCallback = nullptr;

  if (hSingleInstanceMutex) {
    ReleaseMutex(hSingleInstanceMutex);
    CloseHandle(hSingleInstanceMutex);
  }

  g_ReloadSettingsCallback = nullptr;

  if (g_RestartRequested) {
    RelaunchCurrentProcess();
  }

  return 0;
}
