#pragma once

#include <string>

namespace McBopomofo {

struct LunarDate {
  int year;
  int month;
  int day;
  bool isLeapMonth;
};

[[nodiscard]] bool TryConvertSolarToLunar(int year, int month, int day,
                                          LunarDate* output);

[[nodiscard]] bool TryConvertLunarToSolar(const LunarDate& lunar, int* year,
                                          int* month, int* day);

[[nodiscard]] std::string FormatLunarDate(const LunarDate& lunar);

}  // namespace McBopomofo
