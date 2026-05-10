#pragma once

#include <string>

namespace McBopomofo {

struct LunarDate {
  int year;
  int month;
  int day;
  bool isLeapMonth;
};

[[nodiscard]] bool tryConvertSolarToLunar(int year, int month, int day,
                                          LunarDate* output);

[[nodiscard]] bool tryConvertLunarToSolar(const LunarDate& lunar, int* year,
                                          int* month, int* day);

[[nodiscard]] std::string formatLunarDate(const LunarDate& lunar);

}  // namespace McBopomofo
