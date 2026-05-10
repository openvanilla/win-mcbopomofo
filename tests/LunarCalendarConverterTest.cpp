#include <gtest/gtest.h>

#include "LunarCalendarConverter.h"

using namespace McBopomofo;

TEST(LunarCalendarConverterTest, SolarToLunarMatchesReferenceExample) {
    LunarDate lunar{};
    ASSERT_TRUE(tryConvertSolarToLunar(2015, 1, 15, &lunar));
    EXPECT_EQ(lunar.year, 2014);
    EXPECT_EQ(lunar.month, 11);
    EXPECT_EQ(lunar.day, 25);
    EXPECT_FALSE(lunar.isLeapMonth);
}

TEST(LunarCalendarConverterTest, LunarToSolarMatchesReferenceExample) {
    LunarDate lunar{
        .year = 2014,
        .month = 11,
        .day = 25,
        .isLeapMonth = false,
    };
    int year = 0;
    int month = 0;
    int day = 0;
    ASSERT_TRUE(tryConvertLunarToSolar(lunar, &year, &month, &day));
    EXPECT_EQ(year, 2015);
    EXPECT_EQ(month, 1);
    EXPECT_EQ(day, 15);
}

TEST(LunarCalendarConverterTest, FormatLunarDateUsesChineseStyle) {
    LunarDate lunar{
        .year = 2024,
        .month = 1,
        .day = 1,
        .isLeapMonth = false,
    };
    EXPECT_EQ(formatLunarDate(lunar), "2024年正月初一");
}

TEST(LunarCalendarConverterTest, FormatLunarDateIncludesLeapMonth) {
    LunarDate lunar{
        .year = 2023,
        .month = 2,
        .day = 3,
        .isLeapMonth = true,
    };
    EXPECT_EQ(formatLunarDate(lunar), "2023年閏二月初三");
}
