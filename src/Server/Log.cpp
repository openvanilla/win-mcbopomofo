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

#include "Log.h"

#define WIN32_LEAN_AND_MEAN
#include <stdio.h>
#include <windows.h>

#include <atomic>
#include <mutex>
#include <string>

namespace McBopomofo {

namespace {

constexpr ULONGLONG kMaxLogFileBytes = 10ULL * 1024ULL * 1024ULL;
constexpr int kMaxRotatedLogFiles = 3;

std::atomic_bool g_loggingEnabled{false};
std::mutex g_logFileMutex;

ULONGLONG ElapsedMsSinceProcessStart() {
  static const ULONGLONG kStartTick = GetTickCount64();
  return GetTickCount64() - kStartTick;
}

std::wstring RotatedLogFilePath(const std::wstring& logPath, int index) {
  return logPath + L"." + std::to_wstring(index);
}

bool LogFileExceedsLimit(const std::wstring& logPath) {
  WIN32_FILE_ATTRIBUTE_DATA data = {};
  if (!GetFileAttributesExW(logPath.c_str(), GetFileExInfoStandard, &data)) {
    return false;
  }
  if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
    return false;
  }

  ULARGE_INTEGER size = {};
  size.HighPart = data.nFileSizeHigh;
  size.LowPart = data.nFileSizeLow;
  return size.QuadPart >= kMaxLogFileBytes;
}

void RotateLogFileIfNeeded(const std::wstring& logPath) {
  if (!LogFileExceedsLimit(logPath)) {
    return;
  }

  DeleteFileW(RotatedLogFilePath(logPath, kMaxRotatedLogFiles).c_str());
  for (int i = kMaxRotatedLogFiles - 1; i >= 1; --i) {
    MoveFileExW(RotatedLogFilePath(logPath, i).c_str(),
                RotatedLogFilePath(logPath, i + 1).c_str(),
                MOVEFILE_REPLACE_EXISTING);
  }
  MoveFileExW(logPath.c_str(), RotatedLogFilePath(logPath, 1).c_str(),
              MOVEFILE_REPLACE_EXISTING);
}

}  // namespace

std::wstring GetLogFilePath() {
  wchar_t tempPath[MAX_PATH];
  if (GetTempPathW(MAX_PATH, tempPath) == 0) {
    return L"";
  }

  std::wstring logPath(tempPath);
  logPath += L"mcbopomofo_server.log";
  return logPath;
}

bool ServerLoggingEnabled() { return g_loggingEnabled.load(); }

void SetServerLoggingEnabled(bool enabled) { g_loggingEnabled.store(enabled); }

LogMessageContext::LogMessageContext(const char* level) : level_(level) {}

LogMessageContext::~LogMessageContext() {
  if (!ServerLoggingEnabled()) {
    return;
  }

  std::wstring logPath = GetLogFilePath();
  if (!logPath.empty()) {
    std::lock_guard<std::mutex> lock(g_logFileMutex);
    RotateLogFileIfNeeded(logPath);

    FILE* fp = nullptr;
    if (_wfopen_s(&fp, logPath.c_str(), L"a") == 0) {
      fprintf(fp, "[%lu][+%llums] [%s] %s\n", GetCurrentProcessId(),
              ElapsedMsSinceProcessStart(), level_, stream_.str().c_str());
      fclose(fp);
    }
  }
}

}  // namespace McBopomofo
