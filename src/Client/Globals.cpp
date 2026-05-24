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

#include "Globals.h"

#include <dwmapi.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <string>

#include "../Common/Ipc.h"
#include "../Common/NamedPipe.h"

namespace {

thread_local bool g_isRelayingClientLog = false;

ULONGLONG ElapsedMsSinceProcessStart() {
  static const ULONGLONG kStartTick = GetTickCount64();
  return GetTickCount64() - kStartTick;
}

void AppendLogLine(const char* path, DWORD processId, ULONGLONG elapsedMs,
                   const char* message) {
  FILE* fp = nullptr;
  if (fopen_s(&fp, path, "a") == 0) {
    fprintf(fp, "[%lu][+%llums] %s\n", processId, elapsedMs, message);
    fclose(fp);
  }
}

void AppendLogLineToTemp(DWORD processId, ULONGLONG elapsedMs,
                         const char* message) {
  char tempPath[MAX_PATH] = {0};
  DWORD len = GetTempPathA(MAX_PATH, tempPath);
  if (len == 0 || len >= MAX_PATH) {
    return;
  }

  std::string tempLogPath(tempPath);
  tempLogPath += "mcbopomofo_tip.log";
  AppendLogLine(tempLogPath.c_str(), processId, elapsedMs, message);
}

bool ShouldRelayToServer(const char* message) {
  static const char* const kPrefixes[] = {
      "Sending IPC request:",
      "Received IPC response:",
      "State deserialized.",
      "Failed to deserialize state update",
      "IPC Call failed",
      "CandidateUI ",
      "CandidateWindow ",
      "TooltipWindow ",
      "CCandidateListUIElement::",
      "CReadingInformationUIElement::",
      "MoveWindowsToRange ",
      "MoveWindowsToSelection ",
      "MoveWindowsToCaretFallback ",
  };

  for (const char* prefix : kPrefixes) {
    size_t prefixLength = strlen(prefix);
    if (strncmp(message, prefix, prefixLength) == 0) {
      return true;
    }
  }
  return false;
}

void RelayClientLogToServer(DWORD processId, ULONGLONG elapsedMs,
                            const char* message) {
  if (g_isRelayingClientLog || !ShouldRelayToServer(message)) {
    return;
  }

  g_isRelayingClientLog = true;

  McBopomofo::IPC::ClientLogPayload payload;
  payload.processId = processId;
  payload.elapsedMs = elapsedMs;
  payload.message = message;

  McBopomofo::IPC::NamedPipeClient pipe(McBopomofo::IPC::PIPE_NAME);
  std::string response;
  pipe.Call(McBopomofo::IPC::SerializeClientLog(payload), response);

  g_isRelayingClientLog = false;
}

void LogMessageImpl(bool relayToServer, const char* format, va_list args) {
  char buffer[1024];
  vsnprintf(buffer, sizeof(buffer), format, args);

  DWORD processId = GetCurrentProcessId();
  ULONGLONG elapsedMs = ElapsedMsSinceProcessStart();

  char dbgBuffer[1100];
  sprintf_s(dbgBuffer, "[WinMcBopomofo] [%lu][+%llums] %s\n", processId,
            elapsedMs, buffer);
  OutputDebugStringA(dbgBuffer);

  AppendLogLine("C:\\Users\\Public\\mcbopomofo_tip.log", processId, elapsedMs,
                buffer);
  AppendLogLineToTemp(processId, elapsedMs, buffer);

  if (relayToServer) {
    RelayClientLogToServer(processId, elapsedMs, buffer);
  }
}

}  // namespace

void LogMessage(const char* format, ...) {
  va_list args;
  va_start(args, format);
  LogMessageImpl(true, format, args);
  va_end(args);
}

void LogMessageFileOnly(const char* format, ...) {
  va_list args;
  va_start(args, format);
  LogMessageImpl(false, format, args);
  va_end(args);
}

float GetDpiScaleForWindow(HWND hwnd) {
  if (!hwnd) return 1.0f;
  UINT dpi = 96;
  HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
  auto pGetDpiForWindow =
      (UINT(WINAPI*)(HWND))GetProcAddress(hUser32, "GetDpiForWindow");
  if (pGetDpiForWindow) {
    dpi = pGetDpiForWindow(hwnd);
  } else {
    HDC hdc = GetDC(hwnd);
    if (hdc) {
      dpi = GetDeviceCaps(hdc, LOGPIXELSX);
      ReleaseDC(hwnd, hdc);
    }
  }
  return (float)dpi / 96.0f;
}

void EnableWindowDropShadow(HWND hwnd) {
  if (!hwnd) {
    return;
  }

  // Ask the window manager to keep a tiny frame so borderless popup windows
  // can still receive the standard DWM shadow.
  const MARGINS margins = {1, 1, 1, 1};
  DwmExtendFrameIntoClientArea(hwnd, &margins);

  BOOL enabled = FALSE;
  if (FAILED(DwmIsCompositionEnabled(&enabled)) || !enabled) {
    return;
  }

  const DWMNCRENDERINGPOLICY policy = DWMNCRP_ENABLED;
  DwmSetWindowAttribute(hwnd, DWMWA_NCRENDERING_POLICY, &policy,
                        sizeof(policy));

  // Keep the DWM shadow but suppress the compositor-drawn border/frame color
  // that otherwise shows up as a gray rectangle around popup windows.
  constexpr COLORREF kDwmColorNone = static_cast<COLORREF>(0xFFFFFFFE);
  DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, &kDwmColorNone,
                        sizeof(kDwmColorNone));
}
