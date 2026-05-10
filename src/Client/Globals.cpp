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