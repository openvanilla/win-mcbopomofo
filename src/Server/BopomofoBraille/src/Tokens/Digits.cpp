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

#include "Digits.h"

#include <algorithm>
#include <map>

namespace McBopomofo {

namespace DigitWrapper {
static const std::map<Digit, std::string> digitToBrailleMap = {
    {Digit::zero, "⠴"},  {Digit::one, "⠂"},   {Digit::two, "⠆"},
    {Digit::three, "⠒"}, {Digit::four, "⠲"},  {Digit::five, "⠢"},
    {Digit::six, "⠖"},   {Digit::seven, "⠶"}, {Digit::eight, "⠦"},
    {Digit::nine, "⠔"},
};

const std::vector<std::string>& allDigits() {
  static const std::vector<std::string> digits = {"0", "1", "2", "3", "4",
                                                  "5", "6", "7", "8", "9"};
  return digits;
}

const std::vector<std::string>& allBraille() {
  static const std::vector<std::string> brailles = {"⠴", "⠂", "⠆", "⠒", "⠲",
                                                    "⠢", "⠖", "⠶", "⠦", "⠔"};
  return brailles;
}

std::optional<Digit> fromDigit(const std::string& b) {
  if (b.size() == 1) {
    char c = b[0];
    if (c >= '0' && c <= '9') {
      return static_cast<Digit>(c);
    }
  }
  return std::nullopt;
}

std::optional<Digit> fromBraille(const std::string& b, BrailleType type) {
  if (type == BrailleType::ASCII) {
    return fromDigit(b);
  }
  for (const auto& [k, v] : digitToBrailleMap) {
    if (v == b) {
      return k;
    }
  }
  return std::nullopt;
}

std::string toDigit(Digit c) { return std::string(1, static_cast<char>(c)); }

std::string toBraille(Digit c, BrailleType type) {
  if (type == BrailleType::ASCII) {
    return toDigit(c);
  }
  auto it = digitToBrailleMap.find(c);
  if (it != digitToBrailleMap.end()) {
    return it->second;
  }
  return "";
}
}  // namespace DigitWrapper

namespace DigitRelatedWrapper {
struct DigitRelatedInfo {
  std::string punctuation;
  std::string unicodeBraille;
  std::string asciiBraille;
};

static const std::map<DigitRelated, DigitRelatedInfo> infoMap = {
    {DigitRelated::point, {".", "⠨", "."}},
    {DigitRelated::percent, {"%", "⠈⠴", "`%"}},
    {DigitRelated::celsius, {"°C", "⠘⠨⠡ ⠰⠠⠉", "~.* ;,c"}},
};

std::optional<DigitRelated> fromPunctuation(const std::string& b) {
  for (const auto& [k, v] : infoMap) {
    if (v.punctuation == b) {
      return k;
    }
  }
  return std::nullopt;
}

std::optional<DigitRelated> fromBraille(const std::string& b,
                                        BrailleType type) {
  for (const auto& [k, v] : infoMap) {
    if (type == BrailleType::UNICODE && v.unicodeBraille == b) {
      return k;
    }
    if (type == BrailleType::ASCII && v.asciiBraille == b) {
      return k;
    }
  }
  return std::nullopt;
}

std::string toPunctuation(DigitRelated c) {
  auto it = infoMap.find(c);
  if (it != infoMap.end()) {
    return it->second.punctuation;
  }
  return "";
}

std::string toBraille(DigitRelated c, BrailleType type) {
  auto it = infoMap.find(c);
  if (it != infoMap.end()) {
    return (type == BrailleType::UNICODE) ? it->second.unicodeBraille
                                          : it->second.asciiBraille;
  }
  return "";
}
}  // namespace DigitRelatedWrapper

}  // namespace McBopomofo
