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

#include "SettingsApp.h"

#include <filesystem>

namespace McBopomofo {
namespace {

bool ShellOpenPath(const std::filesystem::path& path) {
  HINSTANCE result = ShellExecuteW(nullptr, L"open", path.c_str(), nullptr,
                                   nullptr, SW_SHOWNORMAL);
  return reinterpret_cast<INT_PTR>(result) > 32;
}

bool OpenSettingsFromBasePath(const std::filesystem::path& basePath) {
  static constexpr const wchar_t* kGeneric = L"McBopomofoConfig.exe";
#if defined(_M_ARM64)
  static constexpr const wchar_t* kArchSpecific = L"McBopomofoConfig_arm64.exe";
#elif defined(_M_X64) || defined(_M_AMD64)
  static constexpr const wchar_t* kArchSpecific = L"McBopomofoConfig_x64.exe";
#else
  static constexpr const wchar_t* kArchSpecific = L"McBopomofoConfig_x86.exe";
#endif

  const wchar_t* candidates[] = {kGeneric, kArchSpecific};

  for (const wchar_t* candidate : candidates) {
    const auto configPath = basePath / candidate;
    if (std::filesystem::exists(configPath) && ShellOpenPath(configPath)) {
      return true;
    }
  }
  return false;
}

}  // namespace

bool OpenSettingsAppFromModule(HMODULE module) {
  WCHAR path[MAX_PATH] = {};
  if (!module || GetModuleFileNameW(module, path, MAX_PATH) == 0) {
    return false;
  }

  return OpenSettingsFromBasePath(std::filesystem::path(path).parent_path());
}

bool OpenSettingsApp() {
  if (OpenSettingsAppFromModule(GetModuleHandleW(nullptr))) {
    return true;
  }
  return OpenSettingsAppFromModule(GetModuleHandleW(L"McBopomofoTIP_v2.dll"));
}

}  // namespace McBopomofo
