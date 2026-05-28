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

#ifndef SRC_TOKENS_FULLWIDTHPUNCTUATION_H_
#define SRC_TOKENS_FULLWIDTHPUNCTUATION_H_

#include <optional>
#include <string>
#include <vector>

#include "BopomofoBraille/BrailleType.h"

namespace McBopomofo {

enum class FullWidthPunctuation {
  period,
  dot,
  comma,
  semicolon,
  ideographicComma,
  questionMark,
  exclamationMark,
  colon,
  personNameMark,
  slash,
  bookNameMark,
  ellipsis,
  referenceMark,
  doubleRing,
  singleQuotationMarkLeft,
  singleQuotationMarkRight,
  doubleQuotationMarkLeft,
  doubleQuotationMarkRight,
  parenthesesLeft,
  parenthesesRight,
  bracketLeft,
  bracketRight,
  braceLeft,
  braceRight,
};

namespace FullWidthPunctuationWrapper {
const std::vector<std::string>& allPunctuation();
const std::vector<std::string>& allBraille(
    BrailleType type = BrailleType::UNICODE);

std::optional<FullWidthPunctuation> fromBpmf(const std::string& b);
std::optional<FullWidthPunctuation> fromBraille(
    const std::string& b, BrailleType type = BrailleType::UNICODE);
std::string toBpmf(FullWidthPunctuation c);
std::string toBraille(FullWidthPunctuation c,
                      BrailleType type = BrailleType::UNICODE);
bool supposedToBeAtStart(FullWidthPunctuation c);
}  // namespace FullWidthPunctuationWrapper

}  // namespace McBopomofo

#endif  // SRC_TOKENS_FULLWIDTHPUNCTUATION_H_
