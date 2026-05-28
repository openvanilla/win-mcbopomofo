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

#ifndef SRC_TOKENS_HALFWIDTHPUNCTUATION_H_
#define SRC_TOKENS_HALFWIDTHPUNCTUATION_H_

#include <optional>
#include <string>
#include <vector>

#include "BopomofoBraille/BrailleType.h"

namespace McBopomofo {

enum class HalfWidthPunctuation {
  period,
  comma,
  semicolon,
  dash,
  questionMark,
  exclamationMark,
  colon,
  slash,
  star,
  dotDotDot,
  singleQuotationMarkLeft,
  singleQuotationMarkRight,
  doubleQuotationMarkLeft,
  doubleQuotationMarkRight,
  parenthesesLeft,
  parenthesesRight,
  bracketLeft,
  bracketRight,
};

namespace HalfWidthPunctuationWrapper {
const std::vector<std::string>& allPunctuation();
const std::vector<std::string>& allBraille(
    BrailleType type = BrailleType::UNICODE);

std::optional<HalfWidthPunctuation> fromPunctuation(const std::string& b);
std::optional<HalfWidthPunctuation> fromBraille(
    const std::string& b, BrailleType type = BrailleType::UNICODE);
std::string toBpmf(HalfWidthPunctuation c);
std::string toBraille(HalfWidthPunctuation c,
                      BrailleType type = BrailleType::UNICODE);
}  // namespace HalfWidthPunctuationWrapper

}  // namespace McBopomofo

#endif  // SRC_TOKENS_HALFWIDTHPUNCTUATION_H_
