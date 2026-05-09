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
    int len = WideCharToMultiByte(CP_UTF8, 0, utf16.c_str(), -1, NULL, 0, NULL, NULL);
    if (len <= 0) {
        return std::string();
    }
    std::string utf8(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, utf16.c_str(), -1, utf8.data(), len, NULL, NULL);
    utf8.resize(len - 1);
    return utf8;
}

size_t Utf8OffsetToUtf16Offset(const std::string& utf8, size_t utf8Offset) {
    if (utf8Offset == 0) return 0;
    if (utf8Offset >= utf8.length()) return Utf8ToUtf16(utf8).length();
    std::string sub = utf8.substr(0, utf8Offset);
    return Utf8ToUtf16(sub).length();
}

}  // namespace McBopomofo
