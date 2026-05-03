#include "InputMacro.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <vector>
#include <array>

#include <functional>
#include <memory>

namespace McBopomofo {

namespace {

std::string FormatDate(int dayOffset, const char* format) {
    auto now = std::chrono::system_clock::now();
    now += std::chrono::hours(24 * dayOffset);
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_s(&tm, &t);
    std::stringstream ss;
    ss << std::put_time(&tm, format);
    return ss.str();
}

std::string FormatRocDate(int dayOffset, bool includeYear, const char* format) {
    auto now = std::chrono::system_clock::now();
    now += std::chrono::hours(24 * dayOffset);
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_s(&tm, &t);
    std::stringstream ss;
    if (includeYear) {
        ss << "民國" << (tm.tm_year + 1900 - 1911) << "年";
    }
    ss << std::put_time(&tm, format);
    return ss.str();
}

std::string GetGanzhi(int year) {
    static const std::array<const char*, 10> gan = {"癸", "甲", "乙", "丙", "丁", "戊", "己", "庚", "辛", "壬"};
    static const std::array<const char*, 12> zhi = {"亥", "子", "丑", "寅", "卯", "辰", "巳", "午", "未", "申", "酉", "戌"};

    int base = year < 4 ? 60 - ((year * -1 + 2) % 60) : (year - 3) % 60;
    int ganBase = base % 10;
    int zhiBase = base % 12;

    return std::string(gan[ganBase]) + zhi[zhiBase] + "年";
}

std::string GetChineseZodiac(int year) {
    static const std::array<const char*, 10> gan = {"水", "木", "木", "火", "火", "土", "土", "金", "金", "水"};
    static const std::array<const char*, 12> zhi = {"豬", "鼠", "牛", "虎", "兔", "龍", "蛇", "馬", "羊", "猴", "雞", "狗"};

    int base = year < 4 ? 60 - ((year * -1 + 2) % 60) : (year - 3) % 60;
    int ganBase = base % 10;
    int zhiBase = base % 12;

    return std::string(gan[ganBase]) + zhi[zhiBase] + "年";
}

std::string GetJapaneseWeekday(int dayOffset) {
    auto now = std::chrono::system_clock::now();
    now += std::chrono::hours(24 * dayOffset);
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_s(&tm, &t);
    static const std::array<const char*, 7> weekdays = {"日曜日", "月曜日", "火曜日", "水曜日", "木曜日", "金曜日", "土曜日"};
    return weekdays[tm.tm_wday];
}

std::string GetChineseDate(int dayOffset) {
    auto now = std::chrono::system_clock::now();
    now += std::chrono::hours(24 * dayOffset);
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_s(&tm, &t);
    int year = tm.tm_year + 1900;
    int month = tm.tm_mon + 1;
    int day = tm.tm_mday;

    static const std::array<const char*, 10> digits = {"〇", "一", "二", "三", "四", "五", "六", "七", "八", "九"};
    
    std::string yearStr;
    std::string y = std::to_string(year);
    for (char c : y) {
        yearStr += digits[c - '0'];
    }

    auto toChinese = [&](int n) {
        if (n < 10) return std::string(digits[n]);
        if (n == 10) return std::string("十");
        if (n < 20) return "十" + std::string(digits[n % 10]);
        if (n % 10 == 0) return std::string(digits[n / 10]) + "十";
        return std::string(digits[n / 10]) + "十" + std::string(digits[n % 10]);
    };

    return yearStr + "年" + toChinese(month) + "月" + toChinese(day) + "日";
}

std::string GetJapaneseYear(int dayOffset) {
    auto now = std::chrono::system_clock::now();
    now += std::chrono::hours(24 * dayOffset);
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_s(&tm, &t);
    int year = tm.tm_year + 1900;
    int month = tm.tm_mon + 1;
    int day = tm.tm_mday;

    if (year >= 2019) {
        if (year > 2019 || (month > 5 || (month == 5 && day >= 1))) {
            int reiwaYear = year - 2018;
            std::string yearStr = (reiwaYear == 1) ? "元" : std::to_string(reiwaYear);
            return "令和" + yearStr + "年";
        }
    }
    return std::to_string(year) + "年";
}

std::string GetJapaneseDate(int dayOffset) {
    auto now = std::chrono::system_clock::now();
    now += std::chrono::hours(24 * dayOffset);
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_s(&tm, &t);
    int month = tm.tm_mon + 1;
    int day = tm.tm_mday;

    return GetJapaneseYear(dayOffset) + std::to_string(month) + "月" + std::to_string(day) + "日";
}

// Very simplified Lunar calendar helper
std::string GetLunarDate(int dayOffset) {
    auto now = std::chrono::system_clock::now();
    now += std::chrono::hours(24 * dayOffset);
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_s(&tm, &t);

    // This is a simplified version that just returns "農曆M月D日".
    // A full implementation would require a Lunar calendar algorithm or table.
    return "農曆" + std::to_string(tm.tm_mon + 1) + "月" + std::to_string(tm.tm_mday) + "日";
}

std::string GetChineseWeekday(int dayOffset, bool shortFormat) {
    auto now = std::chrono::system_clock::now();
    now += std::chrono::hours(24 * dayOffset);
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_s(&tm, &t);
    static const std::array<const char*, 7> weekdays = {"日", "一", "二", "三", "四", "五", "六"};
    return shortFormat ? (std::string("週") + weekdays[tm.tm_wday]) : (std::string("星期") + weekdays[tm.tm_wday]);
}

class StaticInputMacro : public InputMacro {
public:
    StaticInputMacro(std::string name, std::string replacement)
        : name_(std::move(name)), replacement_(std::move(replacement)) {}
    std::string name() const override { return name_; }
    std::string replacement() const override { return replacement_; }
private:
    std::string name_;
    std::string replacement_;
};

class FuncInputMacro : public InputMacro {
public:
    FuncInputMacro(std::string name, std::function<std::string()> func)
        : name_(std::move(name)), func_(std::move(func)) {}
    std::string name() const override { return name_; }
    std::string replacement() const override { return func_(); }
private:
    std::string name_;
    std::function<std::string()> func_;
};

} // namespace

InputMacroController::InputMacroController() {
    auto add = [this](std::string name, std::function<std::string()> func) {
        macros_[name] = std::make_unique<FuncInputMacro>(name, func);
    };

    // Year Plain
    add("MACRO@THIS_YEAR_PLAIN", []() { return FormatDate(0, "%Y年"); });
    add("MACRO@LAST_YEAR_PLAIN", []() { return FormatDate(-365, "%Y年"); });
    add("MACRO@NEXT_YEAR_PLAIN", []() { return FormatDate(365, "%Y年"); });

    add("MACRO@THIS_YEAR_PLAIN_WITH_ERA", []() { return FormatDate(0, "西元%Y年"); });
    add("MACRO@LAST_YEAR_PLAIN_WITH_ERA", []() { return FormatDate(-365, "西元%Y年"); });
    add("MACRO@NEXT_YEAR_PLAIN_WITH_ERA", []() { return FormatDate(365, "西元%Y年"); });

    // Year ROC
    add("MACRO@THIS_YEAR_ROC", []() { return FormatRocDate(0, true, ""); });
    add("MACRO@LAST_YEAR_ROC", []() { return FormatRocDate(-365, true, ""); });
    add("MACRO@NEXT_YEAR_ROC", []() { return FormatRocDate(365, true, ""); });

    // Date Short
    add("MACRO@DATE_TODAY_SHORT", []() { return FormatDate(0, "%Y/%m/%d"); });
    add("MACRO@DATE_YESTERDAY_SHORT", []() { return FormatDate(-1, "%Y/%m/%d"); });
    add("MACRO@DATE_TOMORROW_SHORT", []() { return FormatDate(1, "%Y/%m/%d"); });

    // Date Medium
    add("MACRO@DATE_TODAY_MEDIUM", []() { return FormatDate(0, "%Y年%m月%d日"); });
    add("MACRO@DATE_YESTERDAY_MEDIUM", []() { return FormatDate(-1, "%Y年%m月%d日"); });
    add("MACRO@DATE_TOMORROW_MEDIUM", []() { return FormatDate(1, "%Y年%m月%d日"); });

    // Date Medium ROC
    add("MACRO@DATE_TODAY_MEDIUM_ROC", []() { return FormatRocDate(0, true, "%m月%d日"); });
    add("MACRO@DATE_YESTERDAY_MEDIUM_ROC", []() { return FormatRocDate(-1, true, "%m月%d日"); });
    add("MACRO@DATE_TOMORROW_MEDIUM_ROC", []() { return FormatRocDate(1, true, "%m月%d日"); });

    add("MACRO@DATE_TODAY_MEDIUM_CHINESE", []() { return GetChineseDate(0); });
    add("MACRO@DATE_TODAY_MEDIUM_JAPANESE", []() { return GetJapaneseDate(0); });
    add("MACRO@THIS_YEAR_JAPANESE", []() { return GetJapaneseYear(0); });
    add("MACRO@DATE_TODAY2_WEEKDAY", []() { return FormatDate(0, "(%a)"); });

    // Time
    add("MACRO@TIME_NOW_SHORT", []() { return FormatDate(0, "%H:%M"); });
    add("MACRO@TIME_NOW_MEDIUM", []() { return FormatDate(0, "%H:%M:%S"); });

    // Ganzhi
    add("MACRO@THIS_YEAR_GANZHI", []() {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm; localtime_s(&tm, &t);
        return GetGanzhi(tm.tm_year + 1900);
    });
    add("MACRO@LAST_YEAR_GANZHI", []() {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm; localtime_s(&tm, &t);
        return GetGanzhi(tm.tm_year + 1900 - 1);
    });
    add("MACRO@NEXT_YEAR_GANZHI", []() {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm; localtime_s(&tm, &t);
        return GetGanzhi(tm.tm_year + 1900 + 1);
    });

    // Zodiac
    add("MACRO@THIS_YEAR_CHINESE_ZODIAC", []() {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm; localtime_s(&tm, &t);
        return GetChineseZodiac(tm.tm_year + 1900);
    });
    add("MACRO@LAST_YEAR_CHINESE_ZODIAC", []() {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm; localtime_s(&tm, &t);
        return GetChineseZodiac(tm.tm_year + 1900 - 1);
    });
    add("MACRO@NEXT_YEAR_CHINESE_ZODIAC", []() {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm; localtime_s(&tm, &t);
        return GetChineseZodiac(tm.tm_year + 1900 + 1);
    });

    // Weekdays
    add("MACRO@DATE_TODAY_WEEKDAY_SHORT", []() { return GetChineseWeekday(0, true); });
    add("MACRO@DATE_TODAY_WEEKDAY", []() { return GetChineseWeekday(0, false); });
    add("MACRO@DATE_TODAY_WEEKDAY_JAPANESE", []() { return GetJapaneseWeekday(0); });

    add("MACRO@DATE_YESTERDAY_WEEKDAY_SHORT", []() { return GetChineseWeekday(-1, true); });
    add("MACRO@DATE_YESTERDAY_WEEKDAY", []() { return GetChineseWeekday(-1, false); });
    add("MACRO@DATE_YESTERDAY_WEEKDAY_JAPANESE", []() { return GetJapaneseWeekday(-1); });

    add("MACRO@DATE_TOMORROW_WEEKDAY_SHORT", []() { return GetChineseWeekday(1, true); });
    add("MACRO@DATE_TOMORROW_WEEKDAY", []() { return GetChineseWeekday(1, false); });
    add("MACRO@DATE_TOMORROW_WEEKDAY_JAPANESE", []() { return GetJapaneseWeekday(1); });

    // Japanese Dates
    add("MACRO@DATE_TODAY_JAPANESE", []() { return GetJapaneseDate(0); });
    add("MACRO@DATE_YESTERDAY_JAPANESE", []() { return GetJapaneseDate(-1); });
    add("MACRO@DATE_TOMORROW_JAPANESE", []() { return GetJapaneseDate(1); });

    // Lunar Dates
    add("MACRO@DATE_TODAY_LUNAR", []() { return GetLunarDate(0); });
    add("MACRO@DATE_YESTERDAY_LUNAR", []() { return GetLunarDate(-1); });
    add("MACRO@DATE_TOMORROW_LUNAR", []() { return GetLunarDate(1); });
}

std::string InputMacroController::handle(const std::string& input) const {
    auto it = macros_.find(input);
    if (it != macros_.end()) {
        return it->second->replacement();
    }
    return input;
}

} // namespace McBopomofo
