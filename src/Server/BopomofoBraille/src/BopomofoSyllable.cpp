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

#include "BopomofoBraille/BopomofoSyllable.h"

#include <algorithm>
#include <cctype>
#include <map>
#include <optional>
#include <stdexcept>
#include <vector>

#include "Engine/UTF8Helper.h"

namespace McBopomofo {

namespace {

constexpr size_t kMinimalBopomofoLength = 1;
constexpr size_t kMinimalBrailleLength = 2;

enum class Consonant {
  ㄅ,
  ㄆ,
  ㄇ,
  ㄈ,
  ㄉ,
  ㄊ,
  ㄋ,
  ㄌ,
  ㄍ,
  ㄎ,
  ㄏ,
  ㄐ,
  ㄑ,
  ㄒ,
  ㄓ,
  ㄔ,
  ㄕ,
  ㄖ,
  ㄗ,
  ㄘ,
  ㄙ
};

struct ConsonantInfo {
  std::string bpmf;
  std::string unicodeBraille;
  std::string asciiBraille;
  std::string brailleCode;
};

static const std::map<Consonant, ConsonantInfo> consonantMap = {
    {Consonant::ㄅ, {"ㄅ", "⠕", "o", "135"}},
    {Consonant::ㄆ, {"ㄆ", "⠏", "p", "1234"}},
    {Consonant::ㄇ, {"ㄇ", "⠍", "m", "134"}},
    {Consonant::ㄈ, {"ㄈ", "⠟", "q", "12345"}},
    {Consonant::ㄉ, {"ㄉ", "⠙", "d", "145"}},
    {Consonant::ㄊ, {"ㄊ", "⠋", "f", "124"}},
    {Consonant::ㄋ, {"ㄋ", "⠝", "n", "1345"}},
    {Consonant::ㄌ, {"ㄌ", "⠉", "c", "14"}},
    {Consonant::ㄍ, {"ㄍ", "⠅", "k", "13"}},
    {Consonant::ㄎ, {"ㄎ", "⠇", "l", "123"}},
    {Consonant::ㄏ, {"ㄏ", "⠗", "r", "1235"}},
    {Consonant::ㄐ, {"ㄐ", "⠅", "k", "13"}},
    {Consonant::ㄑ, {"ㄑ", "⠚", "j", "245"}},
    {Consonant::ㄒ, {"ㄒ", "⠑", "e", "15"}},
    {Consonant::ㄓ, {"ㄓ", "⠁", "a", "1"}},
    {Consonant::ㄔ, {"ㄔ", "⠃", "b", "12"}},
    {Consonant::ㄕ, {"ㄕ", "⠊", "i", "24"}},
    {Consonant::ㄖ, {"ㄖ", "⠛", "g", "1245"}},
    {Consonant::ㄗ, {"ㄗ", "⠓", "h", "125"}},
    {Consonant::ㄘ, {"ㄘ", "⠚", "j", "245"}},
    {Consonant::ㄙ, {"ㄙ", "⠑", "e", "15"}},
};

bool consonantIsSingle(Consonant c) {
  return c == Consonant::ㄓ || c == Consonant::ㄔ || c == Consonant::ㄕ ||
         c == Consonant::ㄖ || c == Consonant::ㄗ || c == Consonant::ㄘ ||
         c == Consonant::ㄙ;
}

enum class MiddleVowel { ㄧ, ㄨ, ㄩ };

struct MiddleVowelInfo {
  std::string bpmf;
  std::string unicodeBraille;
  std::string asciiBraille;
  std::string brailleCode;
};

static const std::map<MiddleVowel, MiddleVowelInfo> middleVowelMap = {
    {MiddleVowel::ㄧ, {"ㄧ", "⠡", "*", "16"}},
    {MiddleVowel::ㄨ, {"ㄨ", "⠌", "/", "34"}},
    {MiddleVowel::ㄩ, {"ㄩ", "⠳", "|", "1256"}},
};

enum class Vowel { ㄚ, ㄛ, ㄜ, ㄝ, ㄞ, ㄟ, ㄠ, ㄡ, ㄢ, ㄣ, ㄤ, ㄥ, ㄦ };

struct VowelInfo {
  std::string bpmf;
  std::string unicodeBraille;
  std::string asciiBraille;
  std::string brailleCode;
};

static const std::map<Vowel, VowelInfo> vowelMap = {
    {Vowel::ㄚ, {"ㄚ", "⠜", ">", "345"}},
    {Vowel::ㄛ, {"ㄛ", "⠣", "<", "126"}},
    {Vowel::ㄜ, {"ㄜ", "⠮", "!", "2346"}},
    {Vowel::ㄝ, {"ㄝ", "⠢", "5", "26"}},
    {Vowel::ㄞ, {"ㄞ", "⠺", "w", "2456"}},
    {Vowel::ㄟ, {"ㄟ", "⠴", "0", "356"}},
    {Vowel::ㄠ, {"ㄠ", "⠩", "%", "146"}},
    {Vowel::ㄡ, {"ㄡ", "⠷", "(", "12356"}},
    {Vowel::ㄢ, {"ㄢ", "⠧", "v", "1236"}},
    {Vowel::ㄣ, {"ㄣ", "⠥", "u", "136"}},
    {Vowel::ㄤ, {"ㄤ", "⠭", "x", "1346"}},
    {Vowel::ㄥ, {"ㄥ", "⠵", "z", "1356"}},
    {Vowel::ㄦ, {"ㄦ", "⠱", ":", "156"}},
};

enum class YiCombination {
  ㄧㄚ,
  ㄧㄛ,
  ㄧㄝ,
  ㄧㄞ,
  ㄧㄠ,
  ㄧㄡ,
  ㄧㄢ,
  ㄧㄣ,
  ㄧㄤ,
  ㄧㄥ
};

struct YiCombinationInfo {
  std::string bpmf;
  std::string unicodeBraille;
  std::string asciiBraille;
  std::string brailleCode;
};

static const std::map<YiCombination, YiCombinationInfo> yiCombinationMap = {
    {YiCombination::ㄧㄚ, {"ㄧㄚ", "⠾", ")", "23456"}},
    {YiCombination::ㄧㄛ, {"ㄧㄛ", "⠴", "0", "356"}},
    {YiCombination::ㄧㄝ, {"ㄧㄝ", "⠬", "+", "346"}},
    {YiCombination::ㄧㄞ, {"ㄧㄞ", "⠢", "5", "26"}},
    {YiCombination::ㄧㄠ, {"ㄧㄠ", "⠪", "{", "246"}},
    {YiCombination::ㄧㄡ, {"ㄧㄡ", "⠎", "s", "234"}},
    {YiCombination::ㄧㄢ, {"ㄧㄢ", "⠞", "t", "2345"}},
    {YiCombination::ㄧㄣ, {"ㄧㄣ", "⠹", "?", "1456"}},
    {YiCombination::ㄧㄤ, {"ㄧㄤ", "⠨", ".", "46"}},
    {YiCombination::ㄧㄥ, {"ㄧㄥ", "⠽", "y", "13456"}},
};

enum class WuCombination { ㄨㄚ, ㄨㄛ, ㄨㄞ, ㄨㄟ, ㄨㄢ, ㄨㄣ, ㄨㄤ, ㄨㄥ };

struct WuCombinationInfo {
  std::string bpmf;
  std::string unicodeBraille;
  std::string asciiBraille;
  std::string brailleCode;
};

static const std::map<WuCombination, WuCombinationInfo> wuCombinationMap = {
    {WuCombination::ㄨㄚ, {"ㄨㄚ", "⠔", "9", "35"}},
    {WuCombination::ㄨㄛ, {"ㄨㄛ", "⠒", "3", "25"}},
    {WuCombination::ㄨㄞ, {"ㄨㄞ", "⠶", "7", "2356"}},
    {WuCombination::ㄨㄟ, {"ㄨㄟ", "⠫", "$", "1246"}},
    {WuCombination::ㄨㄢ, {"ㄨㄢ", "⠻", "}", "12456"}},
    {WuCombination::ㄨㄣ, {"ㄨㄣ", "⠿", "=", "12345"}},
    {WuCombination::ㄨㄤ, {"ㄨㄤ", "⠸", "_", "456"}},
    {WuCombination::ㄨㄥ, {"ㄨㄥ", "⠯", "&", "12346"}},
};

enum class YuCombination { ㄩㄝ, ㄩㄢ, ㄩㄣ, ㄩㄥ };

struct YuCombinationInfo {
  std::string bpmf;
  std::string unicodeBraille;
  std::string asciiBraille;
  std::string brailleCode;
};

static const std::map<YuCombination, YuCombinationInfo> yuCombinationMap = {
    {YuCombination::ㄩㄝ, {"ㄩㄝ", "⠦", "8", "236"}},
    {YuCombination::ㄩㄢ, {"ㄩㄢ", "⠘", "~", "45"}},
    {YuCombination::ㄩㄣ, {"ㄩㄣ", "⠲", "4", "256"}},
    {YuCombination::ㄩㄥ, {"ㄩㄥ", "⠖", "6", "235"}},
};

enum class Tone { tone1, tone2, tone3, tone4, tone5 };

struct ToneInfo {
  std::string bpmf;
  std::string unicodeBraille;
  std::string asciiBraille;
  std::string brailleCode;
};

static const std::map<Tone, ToneInfo> toneMap = {
    {Tone::tone1, {"", "⠄", "'", "3"}},  {Tone::tone2, {"ˊ", "⠂", "1", "2"}},
    {Tone::tone3, {"ˇ", "⠈", "`", "4"}}, {Tone::tone4, {"ˋ", "⠐", "\"", "5"}},
    {Tone::tone5, {"˙", "⠁", "a", "1"}},
};

std::vector<std::string> consonantBrailles(BrailleType type) {
  static const std::vector<std::string> uni = []() {
    std::vector<std::string> v;
    for (const auto& [k, info] : consonantMap) v.push_back(info.unicodeBraille);
    return v;
  }();
  static const std::vector<std::string> asc = []() {
    std::vector<std::string> v;
    for (const auto& [k, info] : consonantMap) v.push_back(info.asciiBraille);
    return v;
  }();
  return (type == BrailleType::UNICODE) ? uni : asc;
}

std::vector<std::string> middleVowelBrailles(BrailleType type) {
  static const std::vector<std::string> uni = []() {
    std::vector<std::string> v;
    for (const auto& [k, info] : middleVowelMap)
      v.push_back(info.unicodeBraille);
    return v;
  }();
  static const std::vector<std::string> asc = []() {
    std::vector<std::string> v;
    for (const auto& [k, info] : middleVowelMap) v.push_back(info.asciiBraille);
    return v;
  }();
  return (type == BrailleType::UNICODE) ? uni : asc;
}

std::vector<std::string> vowelBrailles(BrailleType type) {
  static const std::vector<std::string> uni = []() {
    std::vector<std::string> v;
    for (const auto& [k, info] : vowelMap) v.push_back(info.unicodeBraille);
    return v;
  }();
  static const std::vector<std::string> asc = []() {
    std::vector<std::string> v;
    for (const auto& [k, info] : vowelMap) v.push_back(info.asciiBraille);
    return v;
  }();
  return (type == BrailleType::UNICODE) ? uni : asc;
}

std::vector<std::string> yiCombinationBrailles(BrailleType type) {
  static const std::vector<std::string> uni = []() {
    std::vector<std::string> v;
    for (const auto& [k, info] : yiCombinationMap)
      v.push_back(info.unicodeBraille);
    return v;
  }();
  static const std::vector<std::string> asc = []() {
    std::vector<std::string> v;
    for (const auto& [k, info] : yiCombinationMap)
      v.push_back(info.asciiBraille);
    return v;
  }();
  return (type == BrailleType::UNICODE) ? uni : asc;
}

std::vector<std::string> wuCombinationBrailles(BrailleType type) {
  static const std::vector<std::string> uni = []() {
    std::vector<std::string> v;
    for (const auto& [k, info] : wuCombinationMap)
      v.push_back(info.unicodeBraille);
    return v;
  }();
  static const std::vector<std::string> asc = []() {
    std::vector<std::string> v;
    for (const auto& [k, info] : wuCombinationMap)
      v.push_back(info.asciiBraille);
    return v;
  }();
  return (type == BrailleType::UNICODE) ? uni : asc;
}

std::vector<std::string> yuCombinationBrailles(BrailleType type) {
  static const std::vector<std::string> uni = []() {
    std::vector<std::string> v;
    for (const auto& [k, info] : yuCombinationMap)
      v.push_back(info.unicodeBraille);
    return v;
  }();
  static const std::vector<std::string> asc = []() {
    std::vector<std::string> v;
    for (const auto& [k, info] : yuCombinationMap)
      v.push_back(info.asciiBraille);
    return v;
  }();
  return (type == BrailleType::UNICODE) ? uni : asc;
}

std::vector<std::string> toneBrailles(BrailleType type) {
  static const std::vector<std::string> uni = []() {
    std::vector<std::string> v;
    for (const auto& [k, info] : toneMap) v.push_back(info.unicodeBraille);
    return v;
  }();
  static const std::vector<std::string> asc = []() {
    std::vector<std::string> v;
    for (const auto& [k, info] : toneMap) v.push_back(info.asciiBraille);
    return v;
  }();
  return (type == BrailleType::UNICODE) ? uni : asc;
}

// Map check helper
template <typename T>
bool vectorContains(const std::vector<T>& vec, const T& val) {
  return std::find(vec.begin(), vec.end(), val) != vec.end();
}

std::string trim(const std::string& str) {
  size_t first = str.find_first_not_of(" \t\r\n");
  if (std::string::npos == first) {
    return "";
  }
  size_t last = str.find_last_not_of(" \t\r\n");
  return str.substr(first, (last - first + 1));
}

std::string makeBpmf(const std::optional<Consonant>& consonant,
                     const std::optional<MiddleVowel>& middleVowel,
                     const std::optional<Vowel>& vowel, Tone tone) {
  std::string output = "";
  if (consonant) {
    output += consonantMap.at(*consonant).bpmf;
  }
  if (middleVowel) {
    output += middleVowelMap.at(*middleVowel).bpmf;
  }
  if (vowel) {
    output += vowelMap.at(*vowel).bpmf;
  }
  output += toneMap.at(tone).bpmf;
  return output;
}

std::string makeBraille(const std::optional<Consonant>& consonant,
                        const std::optional<MiddleVowel>& middleVowel,
                        const std::optional<Vowel>& vowel, Tone tone,
                        BrailleType type) {
  std::string output = "";
  if (consonant) {
    const auto& info = consonantMap.at(*consonant);
    output += (type == BrailleType::UNICODE) ? info.unicodeBraille
                                             : info.asciiBraille;
  }
  if (vowel) {
    if (middleVowel) {
      std::string combStr =
          middleVowelMap.at(*middleVowel).bpmf + vowelMap.at(*vowel).bpmf;
      bool combined = false;
      if (*middleVowel == MiddleVowel::ㄧ) {
        for (const auto& [k, info] : yiCombinationMap) {
          if (info.bpmf == combStr) {
            output += (type == BrailleType::UNICODE) ? info.unicodeBraille
                                                     : info.asciiBraille;
            combined = true;
            break;
          }
        }
      } else if (*middleVowel == MiddleVowel::ㄨ) {
        for (const auto& [k, info] : wuCombinationMap) {
          if (info.bpmf == combStr) {
            output += (type == BrailleType::UNICODE) ? info.unicodeBraille
                                                     : info.asciiBraille;
            combined = true;
            break;
          }
        }
      } else if (*middleVowel == MiddleVowel::ㄩ) {
        for (const auto& [k, info] : yuCombinationMap) {
          if (info.bpmf == combStr) {
            output += (type == BrailleType::UNICODE) ? info.unicodeBraille
                                                     : info.asciiBraille;
            combined = true;
            break;
          }
        }
      }
      if (!combined) {
        // Fallback (though buildCombination validation ensures valid
        // combination)
        const auto& mInfo = middleVowelMap.at(*middleVowel);
        output += (type == BrailleType::UNICODE) ? mInfo.unicodeBraille
                                                 : mInfo.asciiBraille;
        const auto& vInfo = vowelMap.at(*vowel);
        output += (type == BrailleType::UNICODE) ? vInfo.unicodeBraille
                                                 : vInfo.asciiBraille;
      }
    } else {
      const auto& info = vowelMap.at(*vowel);
      output += (type == BrailleType::UNICODE) ? info.unicodeBraille
                                               : info.asciiBraille;
    }
  } else if (middleVowel) {
    const auto& info = middleVowelMap.at(*middleVowel);
    output += (type == BrailleType::UNICODE) ? info.unicodeBraille
                                             : info.asciiBraille;
  } else if (consonant) {
    if (consonantIsSingle(*consonant)) {
      // ㄭ represented as ㄦ in Braille.
      const auto& info = vowelMap.at(Vowel::ㄦ);
      output += (type == BrailleType::UNICODE) ? info.unicodeBraille
                                               : info.asciiBraille;
    }
  }
  const auto& tInfo = toneMap.at(tone);
  output += (type == BrailleType::UNICODE) ? tInfo.unicodeBraille
                                           : tInfo.asciiBraille;
  return output;
}

bool shouldConnectWithYiOrYv(const std::string& next, BrailleType type) {
  std::string yiBraille =
      (type == BrailleType::UNICODE)
          ? middleVowelMap.at(MiddleVowel::ㄧ).unicodeBraille
          : middleVowelMap.at(MiddleVowel::ㄧ).asciiBraille;
  std::string yuBraille =
      (type == BrailleType::UNICODE)
          ? middleVowelMap.at(MiddleVowel::ㄩ).unicodeBraille
          : middleVowelMap.at(MiddleVowel::ㄩ).asciiBraille;
  return next == yiBraille || next == yuBraille ||
         vectorContains(yiCombinationBrailles(type), next) ||
         vectorContains(yuCombinationBrailles(type), next);
}

}  // namespace

BopomofoSyllable BopomofoSyllable::fromBpmf(const std::string& rawBpmf,
                                            BrailleType type) {
  std::string bpmf = trim(rawBpmf);
  if (CodePointCount(bpmf) < kMinimalBopomofoLength) {
    throw std::runtime_error("Invalid Bopomofo length");
  }

  std::optional<Consonant> consonant;
  std::optional<MiddleVowel> middleVowel;
  std::optional<Vowel> vowel;
  Tone tone = Tone::tone1;

  std::vector<std::string> chars = Split(bpmf);
  for (const auto& c : chars) {
    // Check Consonant
    bool isConsonant = false;
    for (const auto& [k, info] : consonantMap) {
      if (info.bpmf == c) {
        if (consonant) {
          throw std::runtime_error("Invalid Bopomofo: multiple consonants");
        }
        if (middleVowel || vowel) {
          throw std::runtime_error("Invalid Bopomofo: consonant after vowel");
        }
        consonant = k;
        isConsonant = true;
        break;
      }
    }
    if (isConsonant) continue;

    // Check MiddleVowel
    bool isMiddleVowel = false;
    for (const auto& [k, info] : middleVowelMap) {
      if (info.bpmf == c) {
        if (middleVowel) {
          throw std::runtime_error("Invalid Bopomofo: multiple middle vowels");
        }
        if (vowel) {
          throw std::runtime_error(
              "Invalid Bopomofo: middle vowel after vowel");
        }
        middleVowel = k;
        isMiddleVowel = true;
        break;
      }
    }
    if (isMiddleVowel) continue;

    // Check Vowel
    bool isVowel = false;
    for (const auto& [k, info] : vowelMap) {
      if (info.bpmf == c) {
        if (vowel) {
          throw std::runtime_error("Invalid Bopomofo: multiple vowels");
        }
        if (middleVowel) {
          // Check if combination is valid
          std::string combStr = middleVowelMap.at(*middleVowel).bpmf + c;
          bool validComb = false;
          if (*middleVowel == MiddleVowel::ㄧ) {
            for (const auto& [yk, yInfo] : yiCombinationMap) {
              if (yInfo.bpmf == combStr) {
                validComb = true;
                break;
              }
            }
          } else if (*middleVowel == MiddleVowel::ㄨ) {
            for (const auto& [wk, wInfo] : wuCombinationMap) {
              if (wInfo.bpmf == combStr) {
                validComb = true;
                break;
              }
            }
          } else if (*middleVowel == MiddleVowel::ㄩ) {
            for (const auto& [yk, yInfo] : yuCombinationMap) {
              if (yInfo.bpmf == combStr) {
                validComb = true;
                break;
              }
            }
          }
          if (!validComb) {
            throw std::runtime_error("Invalid Bopomofo: invalid combination");
          }
        }
        vowel = k;
        isVowel = true;
        break;
      }
    }
    if (isVowel) continue;

    // Check Tone
    bool isTone = false;
    for (const auto& [k, info] : toneMap) {
      if (k == Tone::tone1)
        continue;  // Skip tone1 since its bpmf representation is empty
      if (info.bpmf == c) {
        if (!consonant && !middleVowel && !vowel) {
          throw std::runtime_error(
              "Invalid Bopomofo: tone without consonant, middle vowel, or "
              "vowel");
        }
        if (tone != Tone::tone1) {
          throw std::runtime_error("Invalid Bopomofo: multiple tones");
        }
        tone = k;
        isTone = true;
        break;
      }
    }
    if (isTone) continue;

    throw std::runtime_error("Invalid Bopomofo: invalid character");
  }

  if (!consonant && !middleVowel && !vowel) {
    throw std::runtime_error("Invalid Bopomofo: invalid character");
  }

  std::string braille = makeBraille(consonant, middleVowel, vowel, tone, type);
  return BopomofoSyllable(bpmf, braille, type);
}

BopomofoSyllable BopomofoSyllable::fromBraille(const std::string& rawBraille,
                                               BrailleType type) {
  std::string braille = trim(rawBraille);
  std::vector<std::string> chars = Split(braille);
  if (chars.size() < kMinimalBrailleLength) {
    throw std::runtime_error("Invalid Braille length");
  }

  std::optional<Consonant> consonant;
  std::optional<MiddleVowel> middleVowel;
  std::optional<Vowel> vowel;
  std::optional<Tone> tone;

  for (size_t i = 0; i < chars.size(); ++i) {
    const auto& c = chars[i];

    // Special case: ㄦ (⠱ / :)
    std::string erBraille = (type == BrailleType::UNICODE)
                                ? vowelMap.at(Vowel::ㄦ).unicodeBraille
                                : vowelMap.at(Vowel::ㄦ).asciiBraille;
    if (c == erBraille) {
      if (i == 0) {
        vowel = Vowel::ㄦ;
      } else if (consonant && !consonantIsSingle(*consonant)) {
        throw std::runtime_error("Invalid Braille: other");
      }
      continue;
    }

    // Special case: ㄓ / tone5 (⠁ / a)
    std::string zhiBraille = (type == BrailleType::UNICODE)
                                 ? consonantMap.at(Consonant::ㄓ).unicodeBraille
                                 : consonantMap.at(Consonant::ㄓ).asciiBraille;
    if (c == zhiBraille) {
      if (i == 0) {
        consonant = Consonant::ㄓ;
      } else {
        if (!consonant && !middleVowel && !vowel) {
          throw std::runtime_error(
              "Invalid Braille: tone without consonant, middle vowel, or "
              "vowel");
        }
        if (tone) {
          throw std::runtime_error("Invalid Braille: multiple tones");
        }
        tone = Tone::tone5;
      }
      continue;
    }

    // Special case: ㄒㄙ, ㄑㄘ, ㄐㄍ
    std::string xiBraille = (type == BrailleType::UNICODE)
                                ? consonantMap.at(Consonant::ㄒ).unicodeBraille
                                : consonantMap.at(Consonant::ㄒ).asciiBraille;
    std::string qiBraille = (type == BrailleType::UNICODE)
                                ? consonantMap.at(Consonant::ㄑ).unicodeBraille
                                : consonantMap.at(Consonant::ㄑ).asciiBraille;
    std::string jiBraille = (type == BrailleType::UNICODE)
                                ? consonantMap.at(Consonant::ㄐ).unicodeBraille
                                : consonantMap.at(Consonant::ㄐ).asciiBraille;

    if (c == xiBraille || c == qiBraille || c == jiBraille) {
      if (consonant) {
        throw std::runtime_error("Invalid Braille: duplicated consonant");
      }
      if (i + 1 >= chars.size()) {
        throw std::runtime_error("Invalid Braille: other");
      }
      const auto& next = chars[i + 1];
      bool isConnected = shouldConnectWithYiOrYv(next, type);

      if (c == xiBraille) {
        consonant = isConnected ? Consonant::ㄒ : Consonant::ㄙ;
      } else if (c == qiBraille) {
        consonant = isConnected ? Consonant::ㄑ : Consonant::ㄘ;
      } else if (c == jiBraille) {
        consonant = isConnected ? Consonant::ㄐ : Consonant::ㄍ;
      }
      continue;
    }

    // Check general Consonant
    if (vectorContains(consonantBrailles(type), c)) {
      if (consonant) {
        throw std::runtime_error("Invalid Braille: multiple consonants");
      }
      if (middleVowel || vowel) {
        throw std::runtime_error("Invalid Braille: consonant after vowel");
      }
      for (const auto& [k, info] : consonantMap) {
        if (((type == BrailleType::UNICODE) ? info.unicodeBraille
                                            : info.asciiBraille) == c) {
          consonant = k;
          break;
        }
      }
      continue;
    }

    // Check general MiddleVowel
    if (vectorContains(middleVowelBrailles(type), c)) {
      if (middleVowel) {
        throw std::runtime_error("Invalid Braille: multiple middle vowels");
      }
      if (vowel) {
        throw std::runtime_error("Invalid Braille:  vowel already set");
      }
      for (const auto& [k, info] : middleVowelMap) {
        if (((type == BrailleType::UNICODE) ? info.unicodeBraille
                                            : info.asciiBraille) == c) {
          middleVowel = k;
          break;
        }
      }
      continue;
    }

    // Check general Vowel
    if (vectorContains(vowelBrailles(type), c)) {
      if (middleVowel || vowel) {
        throw std::runtime_error("Invalid Braille: multiple middle vowels");
      }
      for (const auto& [k, info] : vowelMap) {
        if (((type == BrailleType::UNICODE) ? info.unicodeBraille
                                            : info.asciiBraille) == c) {
          vowel = k;
          break;
        }
      }
      continue;
    }

    // Check ㄧ combinations
    if (vectorContains(yiCombinationBrailles(type), c)) {
      if (middleVowel || vowel) {
        throw std::runtime_error("Invalid Braille: multiple middle vowels");
      }
      middleVowel = MiddleVowel::ㄧ;
      for (const auto& [k, info] : yiCombinationMap) {
        if (((type == BrailleType::UNICODE) ? info.unicodeBraille
                                            : info.asciiBraille) == c) {
          std::string vStr = Split(info.bpmf)[1];
          for (const auto& [vk, vInfo] : vowelMap) {
            if (vInfo.bpmf == vStr) {
              vowel = vk;
              break;
            }
          }
          break;
        }
      }
      continue;
    }

    // Check ㄨ combinations
    if (vectorContains(wuCombinationBrailles(type), c)) {
      if (middleVowel || vowel) {
        throw std::runtime_error("Invalid Braille: multiple middle vowels");
      }
      middleVowel = MiddleVowel::ㄨ;
      for (const auto& [k, info] : wuCombinationMap) {
        if (((type == BrailleType::UNICODE) ? info.unicodeBraille
                                            : info.asciiBraille) == c) {
          std::string vStr = Split(info.bpmf)[1];
          for (const auto& [vk, vInfo] : vowelMap) {
            if (vInfo.bpmf == vStr) {
              vowel = vk;
              break;
            }
          }
          break;
        }
      }
      continue;
    }

    // Check ㄩ combinations
    if (vectorContains(yuCombinationBrailles(type), c)) {
      if (middleVowel || vowel) {
        throw std::runtime_error("Invalid Braille: multiple middle vowels");
      }
      middleVowel = MiddleVowel::ㄩ;
      for (const auto& [k, info] : yuCombinationMap) {
        if (((type == BrailleType::UNICODE) ? info.unicodeBraille
                                            : info.asciiBraille) == c) {
          std::string vStr = Split(info.bpmf)[1];
          for (const auto& [vk, vInfo] : vowelMap) {
            if (vInfo.bpmf == vStr) {
              vowel = vk;
              break;
            }
          }
          break;
        }
      }
      continue;
    }

    // Check Tone
    if (vectorContains(toneBrailles(type), c)) {
      if (tone) {
        throw std::runtime_error("Invalid Braille: multiple tones");
      }
      for (const auto& [k, info] : toneMap) {
        if (((type == BrailleType::UNICODE) ? info.unicodeBraille
                                            : info.asciiBraille) == c) {
          tone = k;
          break;
        }
      }
      continue;
    }

    throw std::runtime_error("Invalid character in Braille");
  }

  if (!tone) {
    throw std::runtime_error("Invalid Braille: no tone");
  }

  if (!middleVowel && !vowel &&
      (!consonant || !consonantIsSingle(*consonant))) {
    throw std::runtime_error("Invalid Braille: invalid character");
  }

  std::string bpmf = makeBpmf(consonant, middleVowel, vowel, *tone);
  return BopomofoSyllable(bpmf, braille, type);
}

}  // namespace McBopomofo
