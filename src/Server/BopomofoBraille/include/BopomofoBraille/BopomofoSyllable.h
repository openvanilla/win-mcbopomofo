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

#ifndef BOPOMOFO_BRAILLE_BOPOMOFO_SYLLABLE_H_
#define BOPOMOFO_BRAILLE_BOPOMOFO_SYLLABLE_H_

#include <string>

#include "BrailleType.h"

namespace McBopomofo {

class BopomofoSyllable {
 public:
  /** The Bopomofo syllables in string representation. */
  std::string bpmf;
  /** The Braille in string representation. */
  std::string braille;
  /** The type of Braille representation. */
  BrailleType type;

  BopomofoSyllable(std::string bpmf, std::string braille, BrailleType type)
      : bpmf(std::move(bpmf)), braille(std::move(braille)), type(type) {}

  /**
   * Creates a new instance from a Bopomofo string.
   * @param bpmf The Bopomofo string.
   * @param type The type of Braille.
   * @returns A new instance of BopomofoSyllable.
   */
  static BopomofoSyllable fromBpmf(const std::string& bpmf,
                                   BrailleType type = BrailleType::UNICODE);

  /**
   * Creates a new instance from a Braille string.
   * @param braille The Braille string.
   * @param type The type of Braille.
   * @returns A new instance of BopomofoSyllable.
   */
  static BopomofoSyllable fromBraille(const std::string& braille,
                                      BrailleType type = BrailleType::UNICODE);

 private:
  // Internal helper methods for building and validating
  // These will be fully declared and implemented in BopomofoSyllable.cpp.
};

}  // namespace McBopomofo

#endif  // BOPOMOFO_BRAILLE_BOPOMOFO_SYLLABLE_H_
