#include "Log.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

namespace McBopomofo {

LogMessageContext::LogMessageContext(const char* level) : level_(level) {}

LogMessageContext::~LogMessageContext() {
    char tempPath[MAX_PATH];
    if (GetTempPathA(MAX_PATH, tempPath)) {
        strcat_s(tempPath, "mcbopomofo_server.log");
        FILE* fp = nullptr;
        if (fopen_s(&fp, tempPath, "a") == 0) {
            fprintf(fp, "[%lu] [%s] %s\n", GetCurrentProcessId(), level_, stream_.str().c_str());
            fclose(fp);
        }
    }
}

} // namespace McBopomofo
