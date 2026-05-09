#include "Log.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <atomic>

namespace McBopomofo {

namespace {

std::atomic_bool g_loggingEnabled{true};

}

std::wstring GetLogFilePath() {
    wchar_t tempPath[MAX_PATH];
    if (GetTempPathW(MAX_PATH, tempPath) == 0) {
        return L"";
    }

    std::wstring logPath(tempPath);
    logPath += L"mcbopomofo_server.log";
    return logPath;
}

bool ServerLoggingEnabled() {
    return g_loggingEnabled.load();
}

void SetServerLoggingEnabled(bool enabled) {
    g_loggingEnabled.store(enabled);
}

LogMessageContext::LogMessageContext(const char* level) : level_(level) {}

LogMessageContext::~LogMessageContext() {
    if (!ServerLoggingEnabled()) {
        return;
    }

    std::wstring logPath = GetLogFilePath();
    if (!logPath.empty()) {
        FILE* fp = nullptr;
        if (_wfopen_s(&fp, logPath.c_str(), L"a") == 0) {
            fprintf(fp, "[%lu] [%s] %s\n", GetCurrentProcessId(), level_, stream_.str().c_str());
            fclose(fp);
        }
    }
}

} // namespace McBopomofo
