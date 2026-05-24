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

#include "CandidateWindowColors.h"

#include <windows.h>
#include <winrt/Windows.UI.ViewManagement.h>
#include <winrt/base.h>

#include <string>

#include "Log.h"
#include "UTFHelper.h"

namespace McBopomofo {
namespace {

constexpr uint32_t kFallbackHighlightBlue = 0x0078D7;

uint32_t ToRgb(const winrt::Windows::UI::Color& color) {
  return (static_cast<uint32_t>(color.R) << 16) |
         (static_cast<uint32_t>(color.G) << 8) | static_cast<uint32_t>(color.B);
}

bool EnsureWinrtApartmentInitialized() {
  static thread_local bool initialized = false;
  if (initialized) {
    return true;
  }

  try {
    winrt::init_apartment(winrt::apartment_type::multi_threaded);
    initialized = true;
    return true;
  } catch (const winrt::hresult_error& e) {
    if (e.code() == RPC_E_CHANGED_MODE) {
      initialized = true;
      return true;
    }
    return false;
  }
}

bool AppsUseLightTheme() {
  DWORD useLightTheme = 1;
  DWORD size = sizeof(useLightTheme);
  HKEY hKey = nullptr;
  if (RegOpenKeyExW(
          HKEY_CURRENT_USER,
          L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
          0, KEY_READ, &hKey) == ERROR_SUCCESS) {
    RegQueryValueExW(hKey, L"AppsUseLightTheme", nullptr, nullptr,
                     reinterpret_cast<LPBYTE>(&useLightTheme), &size);
    RegCloseKey(hKey);
  }
  return useLightTheme != 0;
}

}  // namespace

IPC::CandidateWindowColors ReadCandidateWindowColors() {
  IPC::CandidateWindowColors colors;
  colors.highlightBackground = kFallbackHighlightBlue;
  if (!AppsUseLightTheme()) {
    colors.text = 0xF0F0F0;
    colors.background = 0x202020;
    colors.border = 0x404040;
  }

  if (EnsureWinrtApartmentInitialized()) {
    try {
      using namespace winrt::Windows::UI::ViewManagement;
      UISettings uiSettings;
      colors.highlightBackground =
          ToRgb(uiSettings.GetColorValue(UIColorType::Accent));
    } catch (const winrt::hresult_error& e) {
      FCITX_MCBOPOMOFO_WARN()
          << "Failed to read system accent color from UISettings: "
          << Utf16ToUtf8(std::wstring(e.message().c_str()));
    }
  }

  return colors;
}

}  // namespace McBopomofo
