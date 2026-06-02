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

#ifndef SRC_PATH_COMPAT_H_
#define SRC_PATH_COMPAT_H_

#include <filesystem>
#include <string>

#define WIN32_LEAN_AND_MEAN
#include <shlobj.h>
#include <windows.h>

#include "UTFHelper.h"

namespace McBopomofo {
namespace fcitx5_compat {

// Minimal adaptation for Windows
inline std::string locate(const std::string& path) {
#ifdef _WIN32
  wchar_t szExePath[MAX_PATH];
  GetModuleFileNameW(NULL, szExePath, MAX_PATH);
  std::filesystem::path exeDir = std::filesystem::path(szExePath).parent_path();
  std::filesystem::path fullPath = exeDir / path;
  if (std::filesystem::exists(fullPath)) {
    return Utf16ToUtf8(fullPath.wstring());
  }
  // Try directly relative
  if (std::filesystem::exists(path)) {
    return Utf16ToUtf8(std::filesystem::absolute(path).wstring());
  }
  return Utf16ToUtf8(fullPath.wstring());
#else
  if (std::filesystem::exists(path)) return path;
  if (std::filesystem::exists("data/" + path)) return "data/" + path;
  return path;
#endif
}

inline std::string userDirectory() {
  wchar_t path[MAX_PATH];
  if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, path))) {
    std::filesystem::path p(path);
    p /= "WinMcBopomofo";
    std::filesystem::create_directories(p);
    return Utf16ToUtf8(p.wstring());
  }
  return "";
}

}  // namespace fcitx5_compat
}  // namespace McBopomofo

#endif  // SRC_PATH_COMPAT_H_
