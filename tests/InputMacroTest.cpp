#include "InputMacro.h"

#include <array>
#include <chrono>
#include <ctime>
#include <sstream>
#include <string>

#include <gtest/gtest.h>

namespace McBopomofo {
namespace {

std::string formatChineseDigitsForTest(int number) {
  static const std::array<const char*, 10> digits = {
      "〇", "一", "二", "三", "四", "五", "六", "七", "八", "九"};

  std::stringstream ss;
  for (char c : std::to_string(number)) {
    ss << digits[c - '0'];
  }
  return ss.str();
}

std::string formatChineseNumberForTest(int number) {
  static const std::array<const char*, 10> digits = {
      "", "一", "二", "三", "四", "五", "六", "七", "八", "九"};

  if (number <= 10) {
    if (number == 10) {
      return "十";
    }
    return digits[number];
  }

  if (number < 20) {
    return std::string("十") + digits[number % 10];
  }

  int tens = number / 10;
  int ones = number % 10;

  std::stringstream ss;
  ss << digits[tens] << "十";
  if (ones != 0) {
    ss << digits[ones];
  }
  return ss.str();
}

std::string expectedChineseDateForToday() {
  auto now = std::chrono::system_clock::now();
  std::time_t t = std::chrono::system_clock::to_time_t(now);
  std::tm tm;
  localtime_s(&tm, &t);

  return formatChineseDigitsForTest(tm.tm_year + 1900) + "年" +
         formatChineseNumberForTest(tm.tm_mon + 1) + "月" +
         formatChineseNumberForTest(tm.tm_mday) + "日";
}

TEST(InputMacroTest, ChineseDateMacroUsesGregorianChineseDateFormat) {
  InputMacroController controller;

  EXPECT_EQ(controller.handle("MACRO@DATE_TODAY_MEDIUM_CHINESE"),
            expectedChineseDateForToday());
}

TEST(InputMacroTest, ChineseDateMacroDiffersFromLunarDateMacro) {
  InputMacroController controller;

  EXPECT_NE(controller.handle("MACRO@DATE_TODAY_MEDIUM_CHINESE"),
            controller.handle("MACRO@DATE_TODAY_LUNAR"));
}

}  // namespace
}  // namespace McBopomofo
