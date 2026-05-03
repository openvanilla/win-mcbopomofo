#pragma once
#include <string>

namespace McBopomofo {

std::wstring Utf8ToUtf16(const std::string& utf8);
std::string Utf16ToUtf8(const std::wstring& utf16);
size_t Utf8OffsetToUtf16Offset(const std::string& utf8, size_t utf8Offset);

}  // namespace McBopomofo
