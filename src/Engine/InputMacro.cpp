#include "InputMacro.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <vector>
#include <array>

namespace McBopomofo {

namespace {

// Use a simple stub for Chinese calendar related things if needed, 
// but we can implement most with std::chrono or Windows API.
// For now, let's use a very basic std::chrono implementation.

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

std::string GetGanzhi(int year) {
    static const std::array<const char*, 10> gan = {"庚", "辛", "壬", "癸", "甲", "乙", "丙", "丁", "戊", "己"};
    static const std::array<const char*, 12> zhi = {"申", "酉", "戌", "亥", "子", "丑", "寅", "卯", "辰", "巳", "午", "未"};
    return std::string(gan[year % 10]) + zhi[year % 12];
}

std::string GetChineseZodiac(int year) {
    static const std::array<const char*, 12> zodiac = {"猴", "雞", "狗", "豬", "鼠", "牛", "虎", "兔", "龍", "蛇", "馬", "羊"};
    return zodiac[year % 12];
}

class DateInputMacro : public InputMacro {
public:
    DateInputMacro(std::string name, int offset, std::string format)
        : name_(std::move(name)), offset_(offset), format_(std::move(format)) {}
    std::string name() const override { return name_; }
    std::string replacement() const override { return FormatDate(offset_, format_.c_str()); }
private:
    std::string name_;
    int offset_;
    std::string format_;
};

class GanzhiInputMacro : public InputMacro {
public:
    std::string name() const override { return "MACRO@GANZHI_YEAR"; }
    std::string replacement() const override {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm;
        localtime_s(&tm, &t);
        return GetGanzhi(tm.tm_year + 1900);
    }
};

class ZodiacInputMacro : public InputMacro {
public:
    std::string name() const override { return "MACRO@ZODIAC_YEAR"; }
    std::string replacement() const override {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm;
        localtime_s(&tm, &t);
        return GetChineseZodiac(tm.tm_year + 1900);
    }
};

} // namespace

InputMacroController::InputMacroController() {
    auto add = [this](std::unique_ptr<InputMacro> m) {
        macros_[m->name()] = std::move(m);
    };

    add(std::make_unique<DateInputMacro>("MACRO@DATE_TODAY_SHORT", 0, "%Y/%m/%d"));
    add(std::make_unique<DateInputMacro>("MACRO@DATE_TODAY_MEDIUM", 0, "%Y年%m月%d日"));
    add(std::make_unique<DateInputMacro>("MACRO@DATE_YESTERDAY_SHORT", -1, "%Y/%m/%d"));
    add(std::make_unique<DateInputMacro>("MACRO@DATE_TOMORROW_SHORT", 1, "%Y/%m/%d"));
    add(std::make_unique<GanzhiInputMacro>());
    add(std::make_unique<ZodiacInputMacro>());
    // More can be added here
}

std::string InputMacroController::handle(const std::string& input) const {
    auto it = macros_.find(input);
    if (it != macros_.end()) {
        return it->second->replacement();
    }
    return input;
}

} // namespace McBopomofo
