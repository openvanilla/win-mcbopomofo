// Copyright (c) 2022 and onwards The McBopomofo Authors.
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

#include "IcuTransformInputHelper.h"

#include <icu.h>

#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace McBopomofo {
namespace IcuTransformInputHelper {

struct TransliteratorDeleter {
  void operator()(UTransliterator* tr) const {
    if (tr) {
      utrans_close(tr);
    }
  }
};

using UniqueTransliterator =
    std::unique_ptr<UTransliterator, TransliteratorDeleter>;

static UTransliterator* getTransliterator(const std::string& id) {
  static std::unordered_map<std::string, UniqueTransliterator> cache;
  auto it = cache.find(id);
  if (it == cache.end()) {
    UErrorCode status = U_ZERO_ERROR;
    UChar uId[64];
    int32_t uIdLen = 0;
    u_strFromUTF8(uId, 64, &uIdLen, id.c_str(),
                  static_cast<int32_t>(id.length()), &status);
    UTransliterator* tr =
        utrans_openU(uId, uIdLen, UTRANS_FORWARD, nullptr, 0, nullptr, &status);
    it = cache.emplace(id, UniqueTransliterator(tr)).first;
  }
  return it->second.get();
}

static std::string transformString(const std::string& id,
                                   const std::string& input) {
  if (input.empty()) {
    return "";
  }
  UTransliterator* tr = getTransliterator(id);
  if (!tr) {
    return "";
  }
  UErrorCode status = U_ZERO_ERROR;
  UChar buffer[256];
  int32_t textLength = 0;
  u_strFromUTF8(buffer, 256, &textLength, input.c_str(),
                static_cast<int32_t>(input.length()), &status);
  if (U_FAILURE(status)) {
    return "";
  }

  int32_t limit = textLength;
  utrans_transUChars(tr, buffer, &textLength, 256, 0, &limit, &status);
  if (U_FAILURE(status)) {
    return "";
  }

  char destUtf8[512];
  int32_t destUtf8Len = 0;
  u_strToUTF8(destUtf8, 512, &destUtf8Len, buffer, textLength, &status);
  if (U_FAILURE(status)) {
    return "";
  }

  return std::string(destUtf8, destUtf8Len);
}

std::vector<std::string> FillCandidatesWithString(const std::string& string) {
  std::vector<std::string> candidates;
  static const std::vector<std::string> transforms = {
      "Latin-Hiragana",    // Hiragana script
      "Latin-Katakana",    // Katakana script
      "Latin-Hangul",      // Hangul script
      "Latin-Thai",        // Thai script
      "Latin-Greek",       // Greek script
      "Latin-Cyrillic",    // Cyrillic script
      "Latin-Arabic",      // Arabic script
      "Latin-Hebrew",      // Hebrew script
      "Latin-Devanagari",  // Devanagari script
  };
  for (const auto& transform : transforms) {
    std::string transformed = transformString(transform, string);
    if (!transformed.empty()) {
      candidates.emplace_back(transformed);
    }
  }
  return candidates;
}

}  // namespace IcuTransformInputHelper
}  // namespace McBopomofo
