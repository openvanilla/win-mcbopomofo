#include "Globals.h"
#include <stdio.h>
#include <stdarg.h>

void LogMessage(const char* format, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    // Also write to a file in TEMP
    char tempPath[MAX_PATH];
    if (GetTempPathA(MAX_PATH, tempPath)) {
        strcat_s(tempPath, "mcbopomofo_tip.log");
        FILE* fp = nullptr;
        if (fopen_s(&fp, tempPath, "a") == 0) {
            fprintf(fp, "[%lu] %s\n", GetCurrentProcessId(), buffer);
            fclose(fp);
        }
    }
}