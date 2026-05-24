// Copyright (c) 2023 and onwards The McBopomofo Authors.
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

#include "InputMacro.h"

#include <icu.h>

#include <cmath>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "UTFHelper.h"

namespace McBopomofo {

namespace {
std::string FormatDate(const std::string& calendarName, int dayOffset,
                       UDateFormatStyle dateStyle);
std::string FormatWithPattern(const std::string& calendarName, int yearOffset,
                              int dateOffset, const std::wstring& pattern);
std::string FormatTime(UDateFormatStyle timeStyle);
std::string FormatTimeZone(UCalendarDisplayNameType type);
int GetCurrentYear();
std::string GetGanzhi(int year);
std::string GetChineseZodiac(int year);
std::string ConvertWeekdayUnit(std::string original);
void AddMacro(std::unordered_map<std::string, std::unique_ptr<InputMacro>>& m,
              std::unique_ptr<InputMacro> p);
}  // namespace

class InputMacroDate : public InputMacro {
 public:
  InputMacroDate(std::string macroName, std::string calendar, int offset,
                 UDateFormatStyle style)
      : name_(std::move(macroName)),
        calendarName_(std::move(calendar)),
        dayOffset_(offset),
        dateStyle_(style) {}
  [[nodiscard]] std::string name() const override { return name_; }
  [[nodiscard]] std::string replacement() const override {
    return FormatDate(calendarName_, dayOffset_, dateStyle_);
  }

 private:
  std::string name_;
  std::string calendarName_;
  int dayOffset_;
  UDateFormatStyle dateStyle_;
};

class InputMacroYear : public InputMacro {
 public:
  InputMacroYear(std::string macroName, std::string calendar, int offset,
                 std::wstring pattern)
      : name_(std::move(macroName)),
        calendarName_(std::move(calendar)),
        yearOffset_(offset),
        pattern_(std::move(pattern)) {}
  [[nodiscard]] std::string name() const override { return name_; }
  [[nodiscard]] std::string replacement() const override {
    return FormatWithPattern(calendarName_, yearOffset_, /*dateOffset*/ 0,
                             pattern_) +
           "年";
  }

 private:
  std::string name_;
  std::string calendarName_;
  int yearOffset_;
  std::wstring pattern_;
};

class InputMacroDayOfTheWeek : public InputMacro {
 public:
  InputMacroDayOfTheWeek(std::string macroName, std::string calendar,
                         int offset, std::wstring pattern)
      : name_(std::move(macroName)),
        calendarName_(std::move(calendar)),
        dayOffset_(offset),
        pattern_(std::move(pattern)) {}
  [[nodiscard]] std::string name() const override { return name_; }
  [[nodiscard]] std::string replacement() const override {
    return FormatWithPattern(calendarName_, /*yearOffset*/ 0, dayOffset_,
                             pattern_);
  }

 private:
  std::string name_;
  std::string calendarName_;
  int dayOffset_;
  std::wstring pattern_;
};

class InputMacroDateTime : public InputMacro {
 public:
  InputMacroDateTime(std::string macroName, UDateFormatStyle style)
      : name_(std::move(macroName)), timeStyle_(style) {}
  [[nodiscard]] std::string name() const override { return name_; }
  [[nodiscard]] std::string replacement() const override {
    return FormatTime(timeStyle_);
  }

 private:
  std::string name_;
  UDateFormatStyle timeStyle_;
};

class InputMacroTimeZone : public InputMacro {
 public:
  InputMacroTimeZone(std::string macroName, UCalendarDisplayNameType type)
      : name_(std::move(macroName)), type_(type) {}
  [[nodiscard]] std::string name() const override { return name_; }
  [[nodiscard]] std::string replacement() const override {
    return FormatTimeZone(type_);
  }

 private:
  std::string name_;
  UCalendarDisplayNameType type_;
};

template <typename transform>
class InputMacroTransform : public InputMacro {
 public:
  InputMacroTransform(std::string macroName, int yearOffset, transform t)
      : name_(std::move(macroName)),
        yearOffset_(yearOffset),
        t_(std::move(t)) {}
  [[nodiscard]] std::string name() const override { return name_; }
  [[nodiscard]] std::string replacement() const override {
    int year = GetCurrentYear();
    return t_(year + yearOffset_);
  }

 private:
  std::string name_;
  int yearOffset_;
  transform t_;
};

class InputMacroGanZhi
    : public InputMacroTransform<std::function<decltype(GetGanzhi)>> {
 public:
  InputMacroGanZhi(std::string macroName, int yearOffset)
      : InputMacroTransform(std::move(macroName), yearOffset, GetGanzhi) {}
};

class InputMacroZodiac
    : public InputMacroTransform<std::function<decltype(GetChineseZodiac)>> {
 public:
  InputMacroZodiac(std::string macroName, int yearOffset)
      : InputMacroTransform(std::move(macroName), yearOffset,
                            GetChineseZodiac) {}
};

class InputMacroDateTodayShort : public InputMacroDate {
 public:
  InputMacroDateTodayShort()
      : InputMacroDate("MACRO@DATE_TODAY_SHORT", "", 0, UDAT_SHORT) {}
};

class InputMacroDateTodayMedium : public InputMacroDate {
 public:
  InputMacroDateTodayMedium()
      : InputMacroDate("MACRO@DATE_TODAY_MEDIUM", "", 0, UDAT_MEDIUM) {}
};

class InputMacroDateTodayMediumRoc : public InputMacroDate {
 public:
  InputMacroDateTodayMediumRoc()
      : InputMacroDate("MACRO@DATE_TODAY_MEDIUM_ROC", "roc", 0, UDAT_MEDIUM) {}
};

class InputMacroDateTodayMediumChinese : public InputMacroDate {
 public:
  InputMacroDateTodayMediumChinese()
      : InputMacroDate("MACRO@DATE_TODAY_MEDIUM_CHINESE", "chinese", 0,
                       UDAT_MEDIUM) {}
};

class InputMacroDateTodayMediumJapanese : public InputMacroDate {
 public:
  InputMacroDateTodayMediumJapanese()
      : InputMacroDate("MACRO@DATE_TODAY_MEDIUM_JAPANESE", "japanese", 0,
                       UDAT_MEDIUM) {}
};

class InputMacroDateYesterdayShort : public InputMacroDate {
 public:
  InputMacroDateYesterdayShort()
      : InputMacroDate("MACRO@DATE_YESTERDAY_SHORT", "", -1, UDAT_SHORT) {}
};

class InputMacroDateYesterdayMedium : public InputMacroDate {
 public:
  InputMacroDateYesterdayMedium()
      : InputMacroDate("MACRO@DATE_YESTERDAY_MEDIUM", "", -1, UDAT_MEDIUM) {}
};

class InputMacroDateYesterdayMediumRoc : public InputMacroDate {
 public:
  InputMacroDateYesterdayMediumRoc()
      : InputMacroDate("MACRO@DATE_YESTERDAY_MEDIUM_ROC", "roc", -1,
                       UDAT_MEDIUM) {}
};

class InputMacroDateYesterdayMediumChinese : public InputMacroDate {
 public:
  InputMacroDateYesterdayMediumChinese()
      : InputMacroDate("MACRO@DATE_YESTERDAY_MEDIUM_CHINESE", "chinese", -1,
                       UDAT_MEDIUM) {}
};

class InputMacroDateYesterdayMediumJapanese : public InputMacroDate {
 public:
  InputMacroDateYesterdayMediumJapanese()
      : InputMacroDate("MACRO@DATE_YESTERDAY_MEDIUM_JAPANESE", "japanese", -1,
                       UDAT_MEDIUM) {}
};

class InputMacroDateTomorrowShort : public InputMacroDate {
 public:
  InputMacroDateTomorrowShort()
      : InputMacroDate("MACRO@DATE_TOMORROW_SHORT", "", 1, UDAT_SHORT) {}
};

class InputMacroDateTomorrowMedium : public InputMacroDate {
 public:
  InputMacroDateTomorrowMedium()
      : InputMacroDate("MACRO@DATE_TOMORROW_MEDIUM", "", 1, UDAT_MEDIUM) {}
};

class InputMacroDateTomorrowMediumRoc : public InputMacroDate {
 public:
  InputMacroDateTomorrowMediumRoc()
      : InputMacroDate("MACRO@DATE_TOMORROW_MEDIUM_ROC", "roc", 1,
                       UDAT_MEDIUM) {}
};

class InputMacroDateTomorrowMediumChinese : public InputMacroDate {
 public:
  InputMacroDateTomorrowMediumChinese()
      : InputMacroDate("MACRO@DATE_TOMORROW_MEDIUM_CHINESE", "chinese", 1,
                       UDAT_MEDIUM) {}
};

class InputMacroDateTomorrowMediumJapanese : public InputMacroDate {
 public:
  InputMacroDateTomorrowMediumJapanese()
      : InputMacroDate("MACRO@DATE_TOMORROW_MEDIUM_JAPANESE", "japanese", 1,
                       UDAT_MEDIUM) {}
};

class InputMacroThisYearPlain : public InputMacroYear {
 public:
  InputMacroThisYearPlain()
      : InputMacroYear("MACRO@THIS_YEAR_PLAIN", "", 0, L"y") {}
};

class InputMacroThisYearPlainWithEra : public InputMacroYear {
 public:
  InputMacroThisYearPlainWithEra()
      : InputMacroYear("MACRO@THIS_YEAR_PLAIN_WITH_ERA", "", 0, L"Gy") {}
};

class InputMacroThisYearRoc : public InputMacroYear {
 public:
  InputMacroThisYearRoc()
      : InputMacroYear("MACRO@THIS_YEAR_ROC", "roc", 0, L"Gy") {}
};

class InputMacroThisYearJapanese : public InputMacroYear {
 public:
  InputMacroThisYearJapanese()
      : InputMacroYear("MACRO@THIS_YEAR_JAPANESE", "japanese", 0, L"Gy") {}
};

class InputMacroLastYearPlain : public InputMacroYear {
 public:
  InputMacroLastYearPlain()
      : InputMacroYear("MACRO@LAST_YEAR_PLAIN", "", -1, L"y") {}
};

class InputMacroLastYearPlainWithEra : public InputMacroYear {
 public:
  InputMacroLastYearPlainWithEra()
      : InputMacroYear("MACRO@LAST_YEAR_PLAIN_WITH_ERA", "", -1, L"Gy") {}
};

class InputMacroLastYearRoc : public InputMacroYear {
 public:
  InputMacroLastYearRoc()
      : InputMacroYear("MACRO@LAST_YEAR_ROC", "roc", -1, L"Gy") {}
};

class InputMacroLastYearJapanese : public InputMacroYear {
 public:
  InputMacroLastYearJapanese()
      : InputMacroYear("MACRO@LAST_YEAR_JAPANESE", "japanese", -1, L"Gy") {}
};

class InputMacroNextYearPlain : public InputMacroYear {
 public:
  InputMacroNextYearPlain()
      : InputMacroYear("MACRO@NEXT_YEAR_PLAIN", "", 1, L"y") {}
};

class InputMacroNextYearPlainWithEra : public InputMacroYear {
 public:
  InputMacroNextYearPlainWithEra()
      : InputMacroYear("MACRO@NEXT_YEAR_PLAIN_WITH_ERA", "", 1, L"Gy") {}
};

class InputMacroNextYearRoc : public InputMacroYear {
 public:
  InputMacroNextYearRoc()
      : InputMacroYear("MACRO@NEXT_YEAR_ROC", "roc", 1, L"Gy") {}
};

class InputMacroNextYearJapanese : public InputMacroYear {
 public:
  InputMacroNextYearJapanese()
      : InputMacroYear("MACRO@NEXT_YEAR_JAPANESE", "japanese", 1, L"Gy") {}
};

class InputMacroWeekdayTodayShort : public InputMacroDayOfTheWeek {
 public:
  InputMacroWeekdayTodayShort()
      : InputMacroDayOfTheWeek("MACRO@DATE_TODAY_WEEKDAY_SHORT", "", 0, L"E") {}
};

class InputMacroWeekdayToday : public InputMacroDayOfTheWeek {
 public:
  InputMacroWeekdayToday()
      : InputMacroDayOfTheWeek("MACRO@DATE_TODAY_WEEKDAY", "", 0, L"EEEE") {}
};

class InputMacroWeekdayToday2 : public InputMacroDayOfTheWeek {
 public:
  InputMacroWeekdayToday2()
      : InputMacroDayOfTheWeek("MACRO@DATE_TODAY2_WEEKDAY", "", 0, L"EEEE") {}
  [[nodiscard]] std::string replacement() const override {
    std::string original(InputMacroDayOfTheWeek::replacement());
    return ConvertWeekdayUnit(original);
  }
};

class InputMacroWeekdayTodayJapanese : public InputMacroDayOfTheWeek {
 public:
  InputMacroWeekdayTodayJapanese()
      : InputMacroDayOfTheWeek("MACRO@DATE_TODAY_WEEKDAY_JAPANESE", "japanese",
                               0, L"EEEE") {}
};

class InputMacroWeekdayYesterdayShort : public InputMacroDayOfTheWeek {
 public:
  InputMacroWeekdayYesterdayShort()
      : InputMacroDayOfTheWeek("MACRO@DATE_YESTERDAY_WEEKDAY_SHORT", "", -1,
                               L"E") {}
};

class InputMacroWeekdayYesterday : public InputMacroDayOfTheWeek {
 public:
  InputMacroWeekdayYesterday()
      : InputMacroDayOfTheWeek("MACRO@DATE_YESTERDAY_WEEKDAY", "", -1,
                               L"EEEE") {}
};

class InputMacroWeekdayYesterday2 : public InputMacroDayOfTheWeek {
 public:
  InputMacroWeekdayYesterday2()
      : InputMacroDayOfTheWeek("MACRO@DATE_YESTERDAY2_WEEKDAY", "", -1,
                               L"EEEE") {}
  [[nodiscard]] std::string replacement() const override {
    std::string original(InputMacroDayOfTheWeek::replacement());
    return ConvertWeekdayUnit(original);
  }
};

class InputMacroWeekdayYesterdayJapanese : public InputMacroDayOfTheWeek {
 public:
  InputMacroWeekdayYesterdayJapanese()
      : InputMacroDayOfTheWeek("MACRO@DATE_YESTERDAY_WEEKDAY_JAPANESE",
                               "japanese", -1, L"EEEE") {}
};

class InputMacroWeekdayTomorrowShort : public InputMacroDayOfTheWeek {
 public:
  InputMacroWeekdayTomorrowShort()
      : InputMacroDayOfTheWeek("MACRO@DATE_TOMORROW_WEEKDAY_SHORT", "", 1,
                               L"E") {}
};

class InputMacroWeekdayTomorrow : public InputMacroDayOfTheWeek {
 public:
  InputMacroWeekdayTomorrow()
      : InputMacroDayOfTheWeek("MACRO@DATE_TOMORROW_WEEKDAY", "", 1, L"EEEE") {}
};

class InputMacroWeekdayTomorrow2 : public InputMacroDayOfTheWeek {
 public:
  InputMacroWeekdayTomorrow2()
      : InputMacroDayOfTheWeek("MACRO@DATE_TOMORROW2_WEEKDAY", "", 1, L"EEEE") {
  }
  [[nodiscard]] std::string replacement() const override {
    std::string original(InputMacroDayOfTheWeek::replacement());
    return ConvertWeekdayUnit(original);
  }
};

class InputMacroWeekdayTomorrowJapanese : public InputMacroDayOfTheWeek {
 public:
  InputMacroWeekdayTomorrowJapanese()
      : InputMacroDayOfTheWeek("MACRO@DATE_TOMORROW_WEEKDAY_JAPANESE",
                               "japanese", 1, L"EEEE") {}
};

class InputMacroDateTimeNowShort : public InputMacroDateTime {
 public:
  InputMacroDateTimeNowShort()
      : InputMacroDateTime("MACRO@TIME_NOW_SHORT", UDAT_SHORT) {}
};

class InputMacroDateTimeNowMedium : public InputMacroDateTime {
 public:
  InputMacroDateTimeNowMedium()
      : InputMacroDateTime("MACRO@TIME_NOW_MEDIUM", UDAT_MEDIUM) {}
};

class InputMacroTimeZoneStandard : public InputMacroTimeZone {
 public:
  InputMacroTimeZoneStandard()
      : InputMacroTimeZone("MACRO@TIMEZONE_STANDARD", UCAL_STANDARD) {}
};

class InputMacroTimeZoneShortGeneric : public InputMacroTimeZone {
 public:
  InputMacroTimeZoneShortGeneric()
      : InputMacroTimeZone("MACRO@TIMEZONE_GENERIC_SHORT",
                           UCAL_SHORT_STANDARD) {}
};

class InputMacroThisYearGanZhi : public InputMacroGanZhi {
 public:
  InputMacroThisYearGanZhi() : InputMacroGanZhi("MACRO@THIS_YEAR_GANZHI", 0) {}
};

class InputMacroLastYearGanZhi : public InputMacroGanZhi {
 public:
  InputMacroLastYearGanZhi() : InputMacroGanZhi("MACRO@LAST_YEAR_GANZHI", -1) {}
};

class InputMacroNextYearGanZhi : public InputMacroGanZhi {
 public:
  InputMacroNextYearGanZhi() : InputMacroGanZhi("MACRO@NEXT_YEAR_GANZHI", 1) {}
};

class InputMacroThisYearChineseZodiac : public InputMacroZodiac {
 public:
  InputMacroThisYearChineseZodiac()
      : InputMacroZodiac("MACRO@THIS_YEAR_CHINESE_ZODIAC", 0) {}
};

class InputMacroLastYearChineseZodiac : public InputMacroZodiac {
 public:
  InputMacroLastYearChineseZodiac()
      : InputMacroZodiac("MACRO@LAST_YEAR_CHINESE_ZODIAC", -1) {}
};

class InputMacroNextYearChineseZodiac : public InputMacroZodiac {
 public:
  InputMacroNextYearChineseZodiac()
      : InputMacroZodiac("MACRO@NEXT_YEAR_CHINESE_ZODIAC", 1) {}
};

InputMacroController::InputMacroController() {
  AddMacro(macros_, std::make_unique<InputMacroDateTodayShort>());
  AddMacro(macros_, std::make_unique<InputMacroDateTodayMedium>());
  AddMacro(macros_, std::make_unique<InputMacroDateTodayMediumRoc>());
  AddMacro(macros_, std::make_unique<InputMacroDateTodayMediumChinese>());
  AddMacro(macros_, std::make_unique<InputMacroDateTodayMediumJapanese>());
  AddMacro(macros_, std::make_unique<InputMacroThisYearPlain>());
  AddMacro(macros_, std::make_unique<InputMacroThisYearPlainWithEra>());
  AddMacro(macros_, std::make_unique<InputMacroThisYearRoc>());
  AddMacro(macros_, std::make_unique<InputMacroThisYearJapanese>());
  AddMacro(macros_, std::make_unique<InputMacroLastYearPlain>());
  AddMacro(macros_, std::make_unique<InputMacroLastYearPlainWithEra>());
  AddMacro(macros_, std::make_unique<InputMacroLastYearRoc>());
  AddMacro(macros_, std::make_unique<InputMacroLastYearJapanese>());
  AddMacro(macros_, std::make_unique<InputMacroNextYearPlain>());
  AddMacro(macros_, std::make_unique<InputMacroNextYearPlainWithEra>());
  AddMacro(macros_, std::make_unique<InputMacroNextYearRoc>());
  AddMacro(macros_, std::make_unique<InputMacroNextYearJapanese>());
  AddMacro(macros_, std::make_unique<InputMacroWeekdayTodayShort>());
  AddMacro(macros_, std::make_unique<InputMacroWeekdayToday>());
  AddMacro(macros_, std::make_unique<InputMacroWeekdayToday2>());
  AddMacro(macros_, std::make_unique<InputMacroWeekdayTodayJapanese>());
  AddMacro(macros_, std::make_unique<InputMacroWeekdayYesterdayShort>());
  AddMacro(macros_, std::make_unique<InputMacroWeekdayYesterday>());
  AddMacro(macros_, std::make_unique<InputMacroWeekdayYesterday2>());
  AddMacro(macros_, std::make_unique<InputMacroWeekdayYesterdayJapanese>());
  AddMacro(macros_, std::make_unique<InputMacroWeekdayTomorrowShort>());
  AddMacro(macros_, std::make_unique<InputMacroWeekdayTomorrow>());
  AddMacro(macros_, std::make_unique<InputMacroWeekdayTomorrow2>());
  AddMacro(macros_, std::make_unique<InputMacroWeekdayTomorrowJapanese>());
  AddMacro(macros_, std::make_unique<InputMacroDateYesterdayShort>());
  AddMacro(macros_, std::make_unique<InputMacroDateYesterdayMedium>());
  AddMacro(macros_, std::make_unique<InputMacroDateYesterdayMediumRoc>());
  AddMacro(macros_, std::make_unique<InputMacroDateYesterdayMediumChinese>());
  AddMacro(macros_, std::make_unique<InputMacroDateYesterdayMediumJapanese>());
  AddMacro(macros_, std::make_unique<InputMacroDateTomorrowShort>());
  AddMacro(macros_, std::make_unique<InputMacroDateTomorrowMedium>());
  AddMacro(macros_, std::make_unique<InputMacroDateTomorrowMediumRoc>());
  AddMacro(macros_, std::make_unique<InputMacroDateTomorrowMediumChinese>());
  AddMacro(macros_, std::make_unique<InputMacroDateTomorrowMediumJapanese>());
  AddMacro(macros_, std::make_unique<InputMacroDateTimeNowShort>());
  AddMacro(macros_, std::make_unique<InputMacroDateTimeNowMedium>());
  AddMacro(macros_, std::make_unique<InputMacroTimeZoneStandard>());
  AddMacro(macros_, std::make_unique<InputMacroTimeZoneShortGeneric>());
  AddMacro(macros_, std::make_unique<InputMacroThisYearGanZhi>());
  AddMacro(macros_, std::make_unique<InputMacroLastYearGanZhi>());
  AddMacro(macros_, std::make_unique<InputMacroNextYearGanZhi>());
  AddMacro(macros_, std::make_unique<InputMacroThisYearChineseZodiac>());
  AddMacro(macros_, std::make_unique<InputMacroLastYearChineseZodiac>());
  AddMacro(macros_, std::make_unique<InputMacroNextYearChineseZodiac>());
}

std::string InputMacroController::handle(const std::string& input) const {
  const auto& it = macros_.find(input);
  if (it != macros_.cend()) {
    return it->second->replacement();
  }
  return input;
}

namespace {

std::string CreateLocaleName(const std::string& calendarName) {
  std::string localeName = calendarName == "japanese" ? "ja_JP" : "zh_Hant_TW";
  if (!calendarName.empty()) {
    localeName += "@calendar=" + calendarName;
  }
  return localeName;
}

std::string FormatWithStyle(const std::string& calendarName, int yearOffset,
                            int dayOffset, UDateFormatStyle dateStyle,
                            UDateFormatStyle timeStyle) {
  UErrorCode status = U_ZERO_ERROR;
  std::string locale = CreateLocaleName(calendarName);

  UCalendar* cal =
      ucal_open(nullptr, -1, locale.c_str(), UCAL_DEFAULT, &status);
  if (U_FAILURE(status)) return "";

  ucal_setMillis(cal, ucal_getNow(), &status);

  if (yearOffset != 0) {
    ucal_add(cal, UCAL_YEAR, yearOffset, &status);
  }
  if (dayOffset != 0) {
    ucal_add(cal, UCAL_DATE, dayOffset, &status);
  }

  UDateFormat* df = udat_open(timeStyle, dateStyle, locale.c_str(), nullptr, -1,
                              nullptr, 0, &status);
  if (U_FAILURE(status)) {
    ucal_close(cal);
    return "";
  }

  UChar result[256] = {0};
  int32_t len = udat_format(df, ucal_getMillis(cal, &status), result, 256,
                            nullptr, &status);

  udat_close(df);
  ucal_close(cal);

  if (U_FAILURE(status)) return "";

  return Utf16ToUtf8(std::wstring(reinterpret_cast<wchar_t*>(result), len));
}

std::string FormatWithPattern(const std::string& calendarName, int yearOffset,
                              int dateOffset, const std::wstring& pattern) {
  UErrorCode status = U_ZERO_ERROR;
  std::string locale = CreateLocaleName(calendarName);

  UCalendar* cal =
      ucal_open(nullptr, -1, locale.c_str(), UCAL_DEFAULT, &status);
  if (U_FAILURE(status)) return "";

  ucal_setMillis(cal, ucal_getNow(), &status);

  if (yearOffset != 0) {
    ucal_add(cal, UCAL_YEAR, yearOffset, &status);
  }
  if (dateOffset != 0) {
    ucal_add(cal, UCAL_DATE, dateOffset, &status);
  }

  UDateFormat* df =
      udat_open(UDAT_PATTERN, UDAT_PATTERN, locale.c_str(), nullptr, -1,
                reinterpret_cast<const UChar*>(pattern.c_str()),
                (int32_t)pattern.length(), &status);
  if (U_FAILURE(status)) {
    ucal_close(cal);
    return "";
  }

  UChar result[256] = {0};
  int32_t len = udat_format(df, ucal_getMillis(cal, &status), result, 256,
                            nullptr, &status);

  udat_close(df);
  ucal_close(cal);

  if (U_FAILURE(status)) return "";

  return Utf16ToUtf8(std::wstring(reinterpret_cast<wchar_t*>(result), len));
}

std::string FormatDate(const std::string& calendarName, int dayOffset,
                       UDateFormatStyle dateStyle) {
  return FormatWithStyle(calendarName, /*yearOffset*/ 0, dayOffset, dateStyle,
                         /*timeStyle*/ UDAT_NONE);
}

std::string FormatTime(UDateFormatStyle timeStyle) {
  return FormatWithStyle(/*calendarName*/ "", /*yearOffset*/ 0, /*dayOffset*/ 0,
                         /*dateStyle*/ UDAT_NONE, timeStyle);
}

std::string FormatTimeZone(UCalendarDisplayNameType type) {
  UErrorCode status = U_ZERO_ERROR;
  UCalendar* cal = ucal_open(nullptr, -1, "zh_Hant_TW", UCAL_DEFAULT, &status);
  if (U_FAILURE(status)) return "";

  UChar result[256] = {0};
  int32_t len = ucal_getTimeZoneDisplayName(cal, type, "zh_Hant_TW", result,
                                            256, &status);

  ucal_close(cal);

  if (U_FAILURE(status)) return "";

  return Utf16ToUtf8(std::wstring(reinterpret_cast<wchar_t*>(result), len));
}

int GetCurrentYear() {
  UErrorCode status = U_ZERO_ERROR;
  UCalendar* cal =
      ucal_open(nullptr, -1, "zh_Hant_TW", UCAL_GREGORIAN, &status);
  if (U_FAILURE(status)) return 0;

  ucal_setMillis(cal, ucal_getNow(), &status);
  int32_t year = ucal_get(cal, UCAL_YEAR, &status);

  ucal_close(cal);
  return year;
}

int getYearBase(int year) {
  if (year < 4) {
    year = year * -1;
    return 60 - ((year + 2) % 60);
  }
  return (year - 3) % 60;
}

std::string GetGanzhi(int year) {
  const std::vector<std::string> gan(
      {"癸", "甲", "乙", "丙", "丁", "戊", "己", "庚", "辛", "壬"});
  const std::vector<std::string> zhi(
      {"亥", "子", "丑", "寅", "卯", "辰", "巳", "午", "未", "申", "酉", "戌"});
  size_t base = static_cast<size_t>(getYearBase(year));
  size_t ganIndex = base % gan.size();
  size_t zhiIndex = base % zhi.size();
  return gan[ganIndex] + zhi[zhiIndex] + "年";
}

std::string GetChineseZodiac(int year) {
  const std::vector<std::string> gan(
      {"水", "木", "木", "火", "火", "土", "土", "金", "金", "水"});
  const std::vector<std::string> zhi(
      {"豬", "鼠", "牛", "虎", "兔", "龍", "蛇", "馬", "羊", "猴", "雞", "狗"});
  size_t base = static_cast<size_t>(getYearBase(year));
  size_t ganIndex = base % gan.size();
  size_t zhiIndex = base % zhi.size();
  return gan[ganIndex] + zhi[zhiIndex] + "年";
}

std::string ConvertWeekdayUnit(std::string original) {
  std::string src = "星期";
  std::string dst = "禮拜";
  size_t pos = original.find(src);
  if (pos != std::string::npos) {
    return original.replace(pos, src.length(), dst);
  }
  return original;
}

void AddMacro(std::unordered_map<std::string, std::unique_ptr<InputMacro>>& m,
              std::unique_ptr<InputMacro> p) {
  m.insert({p->name(), std::move(p)});
}

}  // namespace

}  // namespace McBopomofo
