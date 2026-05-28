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

#ifndef BOPOMOFO_BRAILLE_CONVERTER_H_
#define BOPOMOFO_BRAILLE_CONVERTER_H_

#include <string>
#include <variant>
#include <vector>

#include "BopomofoSyllable.h"
#include "BrailleType.h"

namespace McBopomofo {

class BopomofoBrailleConverter {
 public:
  /**
   * Converts Bopomofo syllables to Braille.
   * @param bopomofo Bopomofo syllables in Unicode.
   * @param type The type of Braille.
   * @returns Converted Braille.
   */
  static std::string convertBpmfToBraille(
      const std::string& bopomofo, BrailleType type = BrailleType::UNICODE);

  /**
   * Converts a Braille string to tokens. The tokens could be BopomofoSyllable
   * objects or strings.
   * @param braille Braille in Unicode/ASCII.
   * @param type The type of Braille.
   * @returns Tokens. A token could be a BopomofoSyllable object or a string.
   */
  using Token = std::variant<BopomofoSyllable, std::string>;
  static std::vector<Token> convertBrailleToTokens(
      const std::string& braille, BrailleType type = BrailleType::UNICODE);

  /**
   * Converts Braille to Bopomofo syllables.
   * @param braille Braille in Unicode/ASCII.
   * @param type The type of Braille.
   * @returns Converted Bopomofo syllables in Unicode.
   */
  static std::string convertBrailleToBpmf(
      const std::string& braille, BrailleType type = BrailleType::UNICODE);
};

}  // namespace McBopomofo

#endif  // BOPOMOFO_BRAILLE_CONVERTER_H_
