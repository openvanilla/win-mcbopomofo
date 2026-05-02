#pragma once
#include <string>

namespace McBopomofo {

std::wstring Utf8ToUtf16(const std::string& utf8);
std::string Utf16ToUtf8(const std::wstring& utf16);

}  // namespace McBopomofo
