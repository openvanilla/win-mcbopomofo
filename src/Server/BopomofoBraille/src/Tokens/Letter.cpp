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

#include "Letter.h"

#include <map>

namespace McBopomofo {

namespace LetterWrapper {
struct LetterInfo {
  std::string unicodeBraille;
  std::string brailleCode;
};

static const std::map<Letter, LetterInfo> letterMap = {
    {Letter::a, {"⠁", "1"}},     {Letter::b, {"⠃", "12"}},
    {Letter::c, {"⠉", "14"}},    {Letter::d, {"⠙", "145"}},
    {Letter::e, {"⠑", "15"}},    {Letter::f, {"⠋", "124"}},
    {Letter::g, {"⠛", "1245"}},  {Letter::h, {"⠓", "125"}},
    {Letter::i, {"⠊", "24"}},    {Letter::j, {"⠚", "245"}},
    {Letter::k, {"⠅", "13"}},    {Letter::l, {"⠇", "123"}},
    {Letter::m, {"⠍", "134"}},   {Letter::n, {"⠝", "1345"}},
    {Letter::o, {"⠕", "135"}},   {Letter::p, {"⠏", "1234"}},
    {Letter::q, {"⠟", "12345"}}, {Letter::r, {"⠗", "1235"}},
    {Letter::s, {"⠎", "234"}},   {Letter::t, {"⠞", "2345"}},
    {Letter::u, {"⠥", "136"}},   {Letter::v, {"⠧", "1236"}},
    {Letter::w, {"⠺", "2456"}},  {Letter::x, {"⠭", "1346"}},
    {Letter::y, {"⠽", "13456"}}, {Letter::z, {"⠵", "1356"}},
};

std::optional<Letter> fromLetter(const std::string& b) {
  if (b.size() == 1) {
    char c = b[0];
    if (c >= 'a' && c <= 'z') {
      return static_cast<Letter>(c);
    }
  }
  return std::nullopt;
}

std::optional<Letter> fromBraille(const std::string& b, BrailleType type) {
  if (type == BrailleType::ASCII) {
    return fromLetter(b);
  }
  for (const auto& [k, v] : letterMap) {
    if (v.unicodeBraille == b) {
      return k;
    }
  }
  return std::nullopt;
}

std::optional<Letter> fromBrailleCode(const std::string& b) {
  for (const auto& [k, v] : letterMap) {
    if (v.brailleCode == b) {
      return k;
    }
  }
  return std::nullopt;
}

std::string toLetter(Letter c) { return std::string(1, static_cast<char>(c)); }

std::string toBraille(Letter c, BrailleType type) {
  if (type == BrailleType::ASCII) {
    return toLetter(c);
  }
  auto it = letterMap.find(c);
  if (it != letterMap.end()) {
    return it->second.unicodeBraille;
  }
  return "";
}

std::string toBrailleCode(Letter c) {
  auto it = letterMap.find(c);
  if (it != letterMap.end()) {
    return it->second.brailleCode;
  }
  return "";
}
}  // namespace LetterWrapper

}  // namespace McBopomofo
