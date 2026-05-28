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

#ifndef SRC_TOKENS_DIGITS_H_
#define SRC_TOKENS_DIGITS_H_

#include <optional>
#include <string>
#include <vector>

#include "BopomofoBraille/BrailleType.h"

namespace McBopomofo {

enum class Digit {
  zero = '0',
  one = '1',
  two = '2',
  three = '3',
  four = '4',
  five = '5',
  six = '6',
  seven = '7',
  eight = '8',
  nine = '9',
};

namespace DigitWrapper {
const std::vector<std::string>& allDigits();
const std::vector<std::string>& allBraille();

std::optional<Digit> fromDigit(const std::string& b);
std::optional<Digit> fromBraille(const std::string& b,
                                 BrailleType type = BrailleType::UNICODE);
std::string toDigit(Digit c);
std::string toBraille(Digit c, BrailleType type = BrailleType::UNICODE);
}  // namespace DigitWrapper

enum class DigitRelated {
  point = 0,
  percent = 1,
  celsius = 2,
};

namespace DigitRelatedWrapper {
std::optional<DigitRelated> fromPunctuation(const std::string& b);
std::optional<DigitRelated> fromBraille(
    const std::string& b, BrailleType type = BrailleType::UNICODE);
std::string toPunctuation(DigitRelated c);
std::string toBraille(DigitRelated c, BrailleType type = BrailleType::UNICODE);
}  // namespace DigitRelatedWrapper

}  // namespace McBopomofo

#endif  // SRC_TOKENS_DIGITS_H_
