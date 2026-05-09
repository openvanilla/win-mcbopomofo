#include "InputMacro.h"

#include <gtest/gtest.h>

namespace McBopomofo {
namespace {

TEST(InputMacroTest, ChineseDateMacroUsesLunarDateFormat) {
  InputMacroController controller;

  const std::string value =
      controller.handle("MACRO@DATE_TODAY_MEDIUM_CHINESE");

  EXPECT_EQ(value.rfind("農曆", 0), 0U);
}

TEST(InputMacroTest, LunarDateMacroMatchesChineseDateMacro) {
  InputMacroController controller;

  EXPECT_EQ(controller.handle("MACRO@DATE_TODAY_MEDIUM_CHINESE"),
            controller.handle("MACRO@DATE_TODAY_LUNAR"));
}

}  // namespace
}  // namespace McBopomofo
