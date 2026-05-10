#include "Globals.h"

#include <stdarg.h>
#include <stdio.h>

void LogMessage(const char* format, ...) {
#ifdef _DEBUG
  char buffer[1024];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  // Send to debugger
  char dbgBuffer[1050];
  sprintf_s(dbgBuffer, "[WinMcBopomofo] [%lu] %s\n", GetCurrentProcessId(),
            buffer);
  OutputDebugStringA(dbgBuffer);

  // Also write to a file in Public Documents
  FILE* fp = nullptr;
  if (fopen_s(&fp, "C:\\Users\\Public\\mcbopomofo_tip.log", "a") == 0) {
    fprintf(fp, "[%lu] %s\n", GetCurrentProcessId(), buffer);
    fclose(fp);
  }
#endif
}