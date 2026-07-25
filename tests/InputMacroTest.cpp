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

#include <gtest/gtest.h>

#include <string>

#include "InputMacro.h"

namespace McBopomofo {
namespace {

TEST(InputMacroTest, BasicMacroTest) {
  InputMacroController controller;

  // Verify that the controller can handle registered macros
  std::string thisYear = controller.handle("MACRO@THIS_YEAR_PLAIN");
  EXPECT_FALSE(thisYear.empty());
  EXPECT_NE(thisYear.find("年"), std::string::npos);

  std::string todayShort = controller.handle("MACRO@DATE_TODAY_SHORT");
  EXPECT_FALSE(todayShort.empty());

  std::string rocYear = controller.handle("MACRO@THIS_YEAR_ROC");
  EXPECT_FALSE(rocYear.empty());
  EXPECT_NE(rocYear.find("民國"), std::string::npos);

  std::string ganzhi = controller.handle("MACRO@THIS_YEAR_GANZHI");
  EXPECT_FALSE(ganzhi.empty());
  EXPECT_NE(ganzhi.find("年"), std::string::npos);

  std::string zodiac = controller.handle("MACRO@THIS_YEAR_CHINESE_ZODIAC");
  EXPECT_FALSE(zodiac.empty());
  EXPECT_NE(zodiac.find("年"), std::string::npos);
}

}  // namespace
}  // namespace McBopomofo
