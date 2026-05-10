#include "Big5Utils.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <charconv>
#include <cstdint>

namespace Big5Utils {

std::string ConvertBig5fromUint16(uint16_t codePoint) {
  char big5[3] = {0};
  int big5Len = 0;
  if (codePoint <= 0xFF) {
    big5[0] = (char)codePoint;
    big5Len = 1;
  } else {
    big5[0] = (char)(codePoint >> 8);
    big5[1] = (char)(codePoint & 0xFF);
    big5Len = 2;
  }

  WCHAR wch[2] = {0};
  int wlen = MultiByteToWideChar(950, 0, big5, big5Len, wch, 2);
  if (wlen <= 0) return "";

  char utf8[5] = {0};
  int ulen = WideCharToMultiByte(CP_UTF8, 0, wch, wlen, utf8, 5, NULL, NULL);
  if (ulen <= 0) return "";

  std::string result(utf8, ulen);
  return result;
}

std::string ConvertBig5fromHexString(std::string hexString) {
  uint16_t codePoint = 0;
  auto [ptr, ec] = std::from_chars(
      hexString.data(), hexString.data() + hexString.size(), codePoint, 16);
  if (ec != std::errc()) return "";
  return ConvertBig5fromUint16(codePoint);
}

}  // namespace Big5Utils
