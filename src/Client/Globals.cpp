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
#include <dwmapi.h>

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