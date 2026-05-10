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

#include "UTFHelper.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace McBopomofo {

std::wstring Utf8ToUtf16(const std::string& utf8) {
  if (utf8.empty()) {
    return std::wstring();
  }
  int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, NULL, 0);
  if (wlen <= 0) {
    return std::wstring();
  }
  std::wstring utf16(wlen, 0);
  MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, utf16.data(), wlen);
  utf16.resize(wlen - 1);
  return utf16;
}

std::string Utf16ToUtf8(const std::wstring& utf16) {
  if (utf16.empty()) {
    return std::string();
  }
  int len =
      WideCharToMultiByte(CP_UTF8, 0, utf16.c_str(), -1, NULL, 0, NULL, NULL);
  if (len <= 0) {
    return std::string();
  }
  std::string utf8(len, 0);
  WideCharToMultiByte(CP_UTF8, 0, utf16.c_str(), -1, utf8.data(), len, NULL,
                      NULL);
  utf8.resize(len - 1);
  return utf8;
}

size_t Utf8OffsetToUtf16Offset(const std::string& utf8, size_t utf8Offset) {
  if (utf8Offset == 0) return 0;
  if (utf8Offset >= utf8.length()) return Utf8ToUtf16(utf8).length();
  std::string sub = utf8.substr(0, utf8Offset);
  return Utf8ToUtf16(sub).length();
}

std::wstring LoadLocalizedStringW(HINSTANCE hInstance, UINT uID) {
  LPWSTR lpBuffer = nullptr;
  int length = LoadStringW(hInstance, uID, reinterpret_cast<LPWSTR>(&lpBuffer), 0);
  if (length > 0) {
    return std::wstring(lpBuffer, length);
  }
  return L"";
}

}  // namespace McBopomofo
