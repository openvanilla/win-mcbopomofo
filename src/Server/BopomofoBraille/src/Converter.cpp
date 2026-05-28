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

#include "BopomofoBraille/Converter.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>

#include "Tokens/Digits.h"
#include "Tokens/FullWidthPunctuation.h"
#include "Tokens/HalfWidthPunctuation.h"
#include "Tokens/Letter.h"
#include "Engine/UTF8Helper.h"

namespace McBopomofo {

namespace {

enum class ConverterState {
  initial = 0,
  bpmf = 1,
  digits = 2,
  letters = 3,
};

class StringCursor {
 private:
  std::vector<std::string> characters;
  size_t idx = 0;

 public:
  StringCursor(const std::string& text) { characters = Split(text); }

  bool isAtEnd() const { return idx >= characters.size(); }

  std::string current() const {
    if (idx < characters.size()) {
      return characters[idx];
    }
    return "";
  }

  std::string peek(size_t offset = 0) const {
    if (idx + offset < characters.size()) {
      return characters[idx + offset];
    }
    return "";
  }

  std::string substring(size_t length) const {
    std::string out = "";
    for (size_t i = 0; i < length && (idx + i) < characters.size(); ++i) {
      out += characters[idx + i];
    }
    return out;
  }

  void advance(size_t n = 1) { idx += n; }

  size_t remaining() const { return characters.size() - idx; }
};

// Helper to check if a vector of strings contains a string
bool vectorContains(const std::vector<std::string>& vec,
                    const std::string& val) {
  return std::find(vec.begin(), vec.end(), val) != vec.end();
}

std::optional<std::string> bpmf2br_HandleDigitsState(StringCursor& cursor,
                                                     BrailleType type) {
  const std::string substring = cursor.current();
  if (vectorContains(DigitWrapper::allDigits(), substring)) {
    cursor.advance();
    auto digit = DigitWrapper::fromDigit(substring);
    if (digit) {
      return DigitWrapper::toBraille(*digit, type);
    }
  }
  size_t target = std::min(static_cast<size_t>(2), cursor.remaining());
  for (size_t i = target; i >= 1; --i) {
    std::string sub = cursor.substring(i);
    auto punctuation = DigitRelatedWrapper::fromPunctuation(sub);
    if (punctuation) {
      cursor.advance(i);
      return DigitRelatedWrapper::toBraille(*punctuation, type);
    }
  }
  return std::nullopt;
}

std::optional<std::string> bpmf2br_HandleLettersState(StringCursor& cursor,
                                                      BrailleType type) {
  const std::string substring = cursor.current();
  if (substring.size() == 1) {
    char c = substring[0];
    if (std::isalpha(static_cast<unsigned char>(c))) {
      char lowered = std::tolower(static_cast<unsigned char>(c));
      std::string output = "";
      if (std::isupper(static_cast<unsigned char>(c))) {
        std::string prefix = (type == BrailleType::ASCII) ? "," : "⠠";
        output += prefix;
      }
      auto letter = LetterWrapper::fromLetter(std::string(1, lowered));
      if (letter) {
        output += LetterWrapper::toBraille(*letter, type);
      }
      cursor.advance();
      return output;
    }
  }
  if (vectorContains(HalfWidthPunctuationWrapper::allPunctuation(),
                     substring)) {
    cursor.advance();
    auto punctuation = HalfWidthPunctuationWrapper::fromPunctuation(substring);
    if (punctuation) {
      return HalfWidthPunctuationWrapper::toBraille(*punctuation, type);
    }
  }
  return std::nullopt;
}

std::optional<std::string> bpmf2br_ProcessBopomofo(StringCursor& cursor,
                                                   BrailleType type) {
  size_t target = std::min(static_cast<size_t>(4), cursor.remaining());
  for (size_t i = target; i >= 1; --i) {
    std::string sub = cursor.substring(i);
    try {
      auto b = BopomofoSyllable::fromBpmf(sub, type);
      cursor.advance(i);
      return b.braille;
    } catch (...) {
    }
  }
  return std::nullopt;
}

std::optional<std::string> bpmf2br_ProcessFullWidthPunctuation(
    StringCursor& cursor, BrailleType type) {
  const std::string substring = cursor.current();
  auto punctuation = FullWidthPunctuationWrapper::fromBpmf(substring);
  if (punctuation) {
    cursor.advance();
    return FullWidthPunctuationWrapper::toBraille(*punctuation, type);
  }
  return std::nullopt;
}

std::optional<std::string> bpmf2br_ProcessDigits(StringCursor& cursor,
                                                 BrailleType type) {
  const std::string substring = cursor.current();
  if (substring.size() == 1 &&
      std::isdigit(static_cast<unsigned char>(substring[0]))) {
    cursor.advance();
    auto digit = DigitWrapper::fromDigit(substring);
    if (digit) {
      std::string body = DigitWrapper::toBraille(*digit, type);
      std::string prefix = (type == BrailleType::ASCII) ? "#" : "⠼";
      return prefix + body;
    }
  }
  return std::nullopt;
}

std::optional<std::string> bpmf2br_ProcessLetters(StringCursor& cursor,
                                                  BrailleType type) {
  const std::string substring = cursor.current();
  if (substring.size() == 1 &&
      std::isalpha(static_cast<unsigned char>(substring[0]))) {
    char c = substring[0];
    char lowered = std::tolower(static_cast<unsigned char>(c));
    std::string output = "";
    if (std::isupper(static_cast<unsigned char>(c))) {
      std::string prefix = (type == BrailleType::ASCII) ? "," : "⠠";
      output += prefix;
    }
    auto letter = LetterWrapper::fromLetter(std::string(1, lowered));
    if (letter) {
      output += LetterWrapper::toBraille(*letter, type);
    }
    cursor.advance();
    return output;
  }
  return std::nullopt;
}

std::optional<std::string> bpmf2br_ProcessHalfWidthPunctuation(
    StringCursor& cursor, BrailleType type) {
  const std::string substring = cursor.current();
  if (vectorContains(HalfWidthPunctuationWrapper::allPunctuation(),
                     substring)) {
    cursor.advance();
    auto punctuation = HalfWidthPunctuationWrapper::fromPunctuation(substring);
    if (punctuation) {
      return HalfWidthPunctuationWrapper::toBraille(*punctuation, type);
    }
  }
  return std::nullopt;
}

std::optional<std::string> br2t_HandleDigitsState(StringCursor& cursor,
                                                  BrailleType type) {
  const std::string substring = cursor.current();
  auto digit = DigitWrapper::fromBraille(substring, type);
  if (digit) {
    cursor.advance();
    return DigitWrapper::toDigit(*digit);
  }

  size_t target = std::min(static_cast<size_t>(7), cursor.remaining());
  for (size_t i = target; i >= 1; --i) {
    std::string sub = cursor.substring(i);
    auto punctuation = DigitRelatedWrapper::fromBraille(sub, type);
    if (punctuation) {
      cursor.advance(i);
      return DigitRelatedWrapper::toPunctuation(*punctuation);
    }
  }
  return std::nullopt;
}

std::optional<std::string> br2t_HandleLettersState(StringCursor& cursor,
                                                   BrailleType type) {
  std::string substring = cursor.current();
  bool isUppercase = false;
  size_t consumed = 1;
  std::string prefix = (type == BrailleType::ASCII) ? "," : "⠠";
  if (substring == prefix) {
    isUppercase = true;
    substring = cursor.peek(1);
    consumed = 2;
  }
  auto letter = LetterWrapper::fromBraille(substring, type);
  if (letter) {
    cursor.advance(consumed);
    std::string letStr = LetterWrapper::toLetter(*letter);
    if (isUppercase && !letStr.empty()) {
      letStr[0] = std::toupper(static_cast<unsigned char>(letStr[0]));
    }
    return letStr;
  }

  size_t target = std::min(static_cast<size_t>(3), cursor.remaining());
  for (size_t i = target; i >= 1; --i) {
    std::string sub = cursor.substring(i);
    auto punctuation = HalfWidthPunctuationWrapper::fromBraille(sub, type);
    if (punctuation) {
      cursor.advance(i);
      return HalfWidthPunctuationWrapper::toBpmf(*punctuation);
    }
  }
  return std::nullopt;
}

std::optional<BopomofoSyllable> br2t_ProcessBopomofo(StringCursor& cursor,
                                                     BrailleType type) {
  size_t target = std::min(static_cast<size_t>(3), cursor.remaining());
  if (target > 0) {
    for (size_t i = target; i >= 1; --i) {
      std::string sub = cursor.substring(i);
      if (!sub.empty() && sub.back() == ' ') {
        continue;
      }
      try {
        auto b = BopomofoSyllable::fromBraille(sub, type);
        cursor.advance(i);
        return b;
      } catch (...) {
      }
    }
  }
  return std::nullopt;
}

std::optional<std::string> br2t_ProcessFullWidthPunctuation(
    StringCursor& cursor, ConverterState state, BrailleType type) {
  size_t target = std::min(static_cast<size_t>(4), cursor.remaining());
  for (size_t i = target; i >= 1; --i) {
    std::string sub = cursor.substring(i);
    if (!sub.empty() && sub.back() == ' ') {
      continue;
    }
    auto punctuation = FullWidthPunctuationWrapper::fromBraille(sub, type);
    if (punctuation) {
      if (state == ConverterState::initial) {
        if (!FullWidthPunctuationWrapper::supposedToBeAtStart(*punctuation)) {
          continue;
        }
      }
      cursor.advance(i);
      return FullWidthPunctuationWrapper::toBpmf(*punctuation);
    }
  }
  return std::nullopt;
}

std::optional<std::string> br2t_ProcessDigits(StringCursor& cursor,
                                              BrailleType type) {
  const std::string substring = cursor.current();
  std::string prefix = (type == BrailleType::ASCII) ? "#" : "⠼";
  if (substring == prefix) {
    auto digit = DigitWrapper::fromBraille(cursor.peek(1), type);
    if (digit) {
      cursor.advance(2);
      return DigitWrapper::toDigit(*digit);
    }
  }
  return std::nullopt;
}

std::optional<std::string> br2t_ProcessLetters(StringCursor& cursor,
                                               BrailleType type) {
  const std::string substring = cursor.current();
  std::string prefix = (type == BrailleType::ASCII) ? "," : "⠠";
  if (substring == prefix && cursor.remaining() > 1) {
    auto letter = LetterWrapper::fromBraille(cursor.peek(1), type);
    if (letter) {
      cursor.advance(2);
      std::string letStr = LetterWrapper::toLetter(*letter);
      if (!letStr.empty()) {
        letStr[0] = std::toupper(static_cast<unsigned char>(letStr[0]));
      }
      return letStr;
    }
  }
  auto letter = LetterWrapper::fromBraille(substring, type);
  if (letter) {
    cursor.advance();
    return LetterWrapper::toLetter(*letter);
  }
  return std::nullopt;
}

std::optional<std::string> br2t_ProcessHalfWidthPunctuation(
    StringCursor& cursor, BrailleType type) {
  size_t target = std::min(static_cast<size_t>(3), cursor.remaining());
  for (size_t i = target; i >= 1; --i) {
    std::string sub = cursor.substring(i);
    auto punctuation = HalfWidthPunctuationWrapper::fromBraille(sub, type);
    if (punctuation) {
      cursor.advance(i);
      return HalfWidthPunctuationWrapper::toBpmf(*punctuation);
    }
  }
  return std::nullopt;
}

}  // namespace

std::string BopomofoBrailleConverter::convertBpmfToBraille(
    const std::string& bopomofo, BrailleType type) {
  ConverterState state = ConverterState::initial;
  std::string output = "";
  StringCursor cursor(bopomofo);

  while (!cursor.isAtEnd()) {
    // Prevent duplicate spaces
    if (cursor.current() == " ") {
      if (!output.empty() && output.back() != ' ') {
        output += " ";
      }
      cursor.advance();
      state = ConverterState::initial;
      continue;
    }

    // If the state is digits, try to convert digits to Braille.
    if (state == ConverterState::digits) {
      auto result = bpmf2br_HandleDigitsState(cursor, type);
      if (result) {
        output += *result;
        continue;
      }
      state = ConverterState::initial;
      output += " ";
    }

    // If the state is letters, try to convert letters to Braille.
    if (state == ConverterState::letters) {
      auto result = bpmf2br_HandleLettersState(cursor, type);
      if (result) {
        output += *result;
        continue;
      }
      state = ConverterState::initial;
      output += " ";
    }

    // Try to convert Bopomofo syllables to Braille.
    auto bpmfResult = bpmf2br_ProcessBopomofo(cursor, type);
    if (bpmfResult) {
      output += *bpmfResult;
      state = ConverterState::bpmf;
      continue;
    }

    // Try to convert FullWidthPunctuation to Braille.
    auto fwPunctResult = bpmf2br_ProcessFullWidthPunctuation(cursor, type);
    if (fwPunctResult) {
      output += *fwPunctResult;
      state = ConverterState::bpmf;
      continue;
    }

    // Try to convert Digits to Braille.
    auto digitResult = bpmf2br_ProcessDigits(cursor, type);
    if (digitResult) {
      if (state != ConverterState::initial) {
        output += " ";
      }
      output += *digitResult;
      state = ConverterState::digits;
      continue;
    }

    // Try to convert Letters to Braille.
    auto letterResult = bpmf2br_ProcessLetters(cursor, type);
    if (letterResult) {
      if (state != ConverterState::initial) {
        output += " ";
      }
      output += *letterResult;
      state = ConverterState::letters;
      continue;
    }

    // Try to convert HalfWidthPunctuation to Braille.
    auto hwPunctResult = bpmf2br_ProcessHalfWidthPunctuation(cursor, type);
    if (hwPunctResult) {
      if (state == ConverterState::initial) {
        output += " ";
      }
      output += *hwPunctResult;
      state = ConverterState::letters;
      continue;
    }

    if (cursor.isAtEnd()) {
      break;
    }

    // If the state is not initial, add a space.
    if (state != ConverterState::initial) {
      output += " ";
    }
    state = ConverterState::initial;
    output += cursor.current();
    cursor.advance();
  }
  return output;
}

std::vector<BopomofoBrailleConverter::Token>
BopomofoBrailleConverter::convertBrailleToTokens(const std::string& braille,
                                                 BrailleType type) {
  ConverterState state = ConverterState::initial;
  std::vector<Token> output;
  std::string nonBpmfText = "";
  StringCursor cursor(braille);

  while (!cursor.isAtEnd()) {
    if (cursor.current() == " ") {
      if (nonBpmfText.empty()) {
        nonBpmfText += " ";
      } else if (nonBpmfText.back() != ' ') {
        nonBpmfText += " ";
      }
      cursor.advance();
      state = ConverterState::initial;
      continue;
    }

    // If the state is digits, try to convert digits to Bopomofo.
    if (state == ConverterState::digits) {
      auto result = br2t_HandleDigitsState(cursor, type);
      if (result) {
        nonBpmfText += *result;
        continue;
      }
      state = ConverterState::initial;
    }

    // If the state is letters, try to convert letters to Bopomofo.
    if (state == ConverterState::letters) {
      auto result = br2t_HandleLettersState(cursor, type);
      if (result) {
        nonBpmfText += *result;
        continue;
      }
      state = ConverterState::initial;
    }

    // Try to convert Braille to Bopomofo.
    auto bpmfResult = br2t_ProcessBopomofo(cursor, type);
    if (bpmfResult) {
      if (!nonBpmfText.empty()) {
        output.push_back(nonBpmfText);
        nonBpmfText = "";
      }
      output.push_back(*bpmfResult);
      state = ConverterState::bpmf;
      continue;
    }

    // Try to convert FullWidthPunctuation to Bopomofo.
    auto fwPunctResult = br2t_ProcessFullWidthPunctuation(cursor, state, type);
    if (fwPunctResult) {
      nonBpmfText += *fwPunctResult;
      state = ConverterState::bpmf;
      continue;
    }

    // Try to convert Digits to Bopomofo.
    auto digitResult = br2t_ProcessDigits(cursor, type);
    if (digitResult) {
      nonBpmfText += *digitResult;
      state = ConverterState::digits;
      continue;
    }

    // Try to convert Letters to Bopomofo.
    auto letterResult = br2t_ProcessLetters(cursor, type);
    if (letterResult) {
      nonBpmfText += *letterResult;
      state = ConverterState::letters;
      continue;
    }

    // Try to convert HalfWidthPunctuation to Bopomofo.
    auto hwPunctResult = br2t_ProcessHalfWidthPunctuation(cursor, type);
    if (hwPunctResult) {
      nonBpmfText += *hwPunctResult;
      state = ConverterState::letters;
      continue;
    }

    if (cursor.isAtEnd()) {
      break;
    }

    nonBpmfText += cursor.current();
    cursor.advance();
  }

  if (!nonBpmfText.empty()) {
    output.push_back(nonBpmfText);
  }

  return output;
}

std::string BopomofoBrailleConverter::convertBrailleToBpmf(
    const std::string& braille, BrailleType type) {
  auto tokens = convertBrailleToTokens(braille, type);
  std::string output = "";
  for (const auto& token : tokens) {
    if (std::holds_alternative<BopomofoSyllable>(token)) {
      output += std::get<BopomofoSyllable>(token).bpmf;
    } else {
      output += std::get<std::string>(token);
    }
  }
  return output;
}

}  // namespace McBopomofo
