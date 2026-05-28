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

#include "HalfWidthPunctuation.h"

#include <map>

namespace McBopomofo {

namespace HalfWidthPunctuationWrapper {
struct PunctInfo {
  std::string punctuation;
  std::string unicodeBraille;
  std::string asciiBraille;
};

static const std::map<HalfWidthPunctuation, PunctInfo> punctMap = {
    {HalfWidthPunctuation::period, {".", "⠲", "."}},
    {HalfWidthPunctuation::comma, {",", "⠂", ","}},
    {HalfWidthPunctuation::semicolon, {";", "⠒", ";"}},
    {HalfWidthPunctuation::dash, {"'", "⠄", "'"}},
    {HalfWidthPunctuation::questionMark, {"?", "⠦", "?"}},
    {HalfWidthPunctuation::exclamationMark, {"!", "⠖", "!"}},
    {HalfWidthPunctuation::colon, {":", "⠒", ":"}},
    {HalfWidthPunctuation::slash, {"-", "⠤", "-"}},
    {HalfWidthPunctuation::star, {"*", "⠔", "*"}},
    {HalfWidthPunctuation::dotDotDot, {"...", "⠄⠄⠄", "..."}},
    {HalfWidthPunctuation::singleQuotationMarkLeft, {"‘", "⠠⠦", "8"}},
    {HalfWidthPunctuation::singleQuotationMarkRight, {"’", "⠴⠄", "0"}},
    {HalfWidthPunctuation::doubleQuotationMarkLeft, {"“", "⠦", "8"}},
    {HalfWidthPunctuation::doubleQuotationMarkRight, {"”", "⠴", "0"}},
    {HalfWidthPunctuation::parenthesesLeft, {"(", "⠶", "("}},
    {HalfWidthPunctuation::parenthesesRight, {")", "⠶", ")"}},
    {HalfWidthPunctuation::bracketLeft, {"[", "⠠⠶", "["}},
    {HalfWidthPunctuation::bracketRight, {"]", "⠶⠄", "]"}},
};

const std::vector<std::string>& allPunctuation() {
  static const std::vector<std::string> punctuations = []() {
    std::vector<std::string> v;
    for (const auto& [k, info] : punctMap) {
      v.push_back(info.punctuation);
    }
    return v;
  }();
  return punctuations;
}

const std::vector<std::string>& allBraille(BrailleType type) {
  static const std::vector<std::string> unicodeBrailles = []() {
    std::vector<std::string> v;
    for (const auto& [k, info] : punctMap) {
      v.push_back(info.unicodeBraille);
    }
    return v;
  }();
  static const std::vector<std::string> asciiBrailles = []() {
    std::vector<std::string> v;
    for (const auto& [k, info] : punctMap) {
      v.push_back(info.asciiBraille);
    }
    return v;
  }();
  return (type == BrailleType::UNICODE) ? unicodeBrailles : asciiBrailles;
}

std::optional<HalfWidthPunctuation> fromPunctuation(const std::string& b) {
  for (const auto& [k, info] : punctMap) {
    if (info.punctuation == b) {
      return k;
    }
  }
  return std::nullopt;
}

std::optional<HalfWidthPunctuation> fromBraille(const std::string& b,
                                                BrailleType type) {
  for (const auto& [k, info] : punctMap) {
    if (type == BrailleType::UNICODE && info.unicodeBraille == b) {
      return k;
    }
    if (type == BrailleType::ASCII && info.asciiBraille == b) {
      return k;
    }
  }
  return std::nullopt;
}

std::string toBpmf(HalfWidthPunctuation c) {
  auto it = punctMap.find(c);
  if (it != punctMap.end()) {
    return it->second.punctuation;
  }
  return "";
}

std::string toBraille(HalfWidthPunctuation c, BrailleType type) {
  auto it = punctMap.find(c);
  if (it != punctMap.end()) {
    return (type == BrailleType::UNICODE) ? it->second.unicodeBraille
                                          : it->second.asciiBraille;
  }
  return "";
}
}  // namespace HalfWidthPunctuationWrapper

}  // namespace McBopomofo
