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

#include "LunarCalendarConverter.h"

#include <array>

extern "C" {
#include "LunarSolarConverter.h"
}

namespace McBopomofo {

namespace {

constexpr int kMinSupportedYear = 1900;
constexpr int kMaxSupportedYear = 2100;

std::string formatLunarMonth_(int month) {
  static const std::array<const char*, 12> kMonths = {
      "正", "二", "三", "四", "五", "六",
      "七", "八", "九", "十", "冬", "臘"};
  if (month < 1 || month > 12) {
    return std::to_string(month);
  }
  return kMonths[static_cast<size_t>(month - 1)];
}

std::string formatLunarDay_(int day) {
  static const std::array<const char*, 10> kDigits = {
      "", "一", "二", "三", "四", "五", "六", "七", "八", "九"};
  if (day <= 0 || day > 30) {
    return std::to_string(day);
  }
  if (day == 10) {
    return "初十";
  }
  if (day == 20) {
    return "二十";
  }
  if (day == 30) {
    return "三十";
  }

  const int tens = day / 10;
  const int ones = day % 10;
  const char* prefix = "";
  switch (tens) {
    case 0:
      prefix = "初";
      break;
    case 1:
      prefix = "十";
      break;
    case 2:
      prefix = "廿";
      break;
    default:
      prefix = "";
      break;
  }
  return std::string(prefix) + kDigits[static_cast<size_t>(ones)];
}

bool isSupportedSolarYear_(int year) {
  return year >= kMinSupportedYear && year <= kMaxSupportedYear;
}

bool isSupportedLunarYear_(int year) {
  return year >= kMinSupportedYear && year <= kMaxSupportedYear;
}

}  // namespace

bool tryConvertSolarToLunar(int year, int month, int day, LunarDate* output) {
  if (output == nullptr || !isSupportedSolarYear_(year)) {
    return false;
  }

  Solar solar = {
      day,
      month,
      year,
  };
  Lunar lunar = SolarToLunar(solar);
  output->year = lunar.lunarYear;
  output->month = lunar.lunarMonth;
  output->day = lunar.lunarDay;
  output->isLeapMonth = lunar.isleap;
  return true;
}

bool tryConvertLunarToSolar(const LunarDate& lunar, int* year, int* month,
                            int* day) {
  if (year == nullptr || month == nullptr || day == nullptr ||
      !isSupportedLunarYear_(lunar.year)) {
    return false;
  }

  Lunar rawLunar = {
      lunar.isLeapMonth,
      lunar.day,
      lunar.month,
      lunar.year,
  };
  Solar solar = LunarToSolar(rawLunar);
  *year = solar.solarYear;
  *month = solar.solarMonth;
  *day = solar.solarDay;
  return true;
}

std::string formatLunarDate(const LunarDate& lunar) {
  return std::to_string(lunar.year) + "年" +
         (lunar.isLeapMonth ? "閏" : "") +
         formatLunarMonth_(lunar.month) +
         "月" +
         formatLunarDay_(lunar.day);
}

}  // namespace McBopomofo
