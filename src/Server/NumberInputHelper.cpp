#include "NumberInputHelper.h"

#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

#include "ChineseNumbers/ChineseNumbers.h"
#include "ChineseNumbers/SuzhouNumbers.h"
#include "RomanNumbers/RomanNumbers.h"

namespace McBopomofo {
namespace NumberInputHelper {

std::vector<std::string> FillCandidatesWithNumber(
    std::string number,
    std::shared_ptr<Formosa::Gramambular2::LanguageModel> languageModel) {
    std::vector<std::string> candidates;
    if (number.empty()) {
        return candidates;
    }

    std::stringstream intStream;
    std::stringstream decStream;
    bool dotFound = false;

    for (char c : number) {
        if (c == '.') {
            dotFound = true;
            continue;
        }
        if (dotFound) {
            decStream << c;
        } else {
            intStream << c;
        }
    }
    std::string intPart = intStream.str();
    std::string decPart = decStream.str();

    candidates.push_back(ChineseNumbers::Generate(intPart, decPart, ChineseNumbers::ChineseNumberCase::LOWERCASE));
    candidates.push_back(ChineseNumbers::Generate(intPart, decPart, ChineseNumbers::ChineseNumberCase::UPPERCASE));

    if (decPart.empty()) {
        int value = std::atoi(intPart.c_str());
        if (value > 0 && value <= 3999) {
            candidates.push_back(RomanNumbers::ConvertFromInt(
                value, RomanNumbers::RomanNumbersStyle::ALPHABETS));
            candidates.push_back(RomanNumbers::ConvertFromInt(
                value, RomanNumbers::RomanNumbersStyle::FULL_WIDTH_UPPER));
            candidates.push_back(RomanNumbers::ConvertFromInt(
                value, RomanNumbers::RomanNumbersStyle::FULL_WIDTH_LOWER));
        }
    }

    std::string key = "_number_" + number;
    if (languageModel && languageModel->hasUnigrams(key)) {
        auto unigrams = languageModel->getUnigrams(key);
        for (const auto& unigram : unigrams) {
            if (std::find(candidates.begin(), candidates.end(), unigram.value()) ==
                candidates.end()) {
                candidates.push_back(unigram.value());
            }
        }
    }

    candidates.push_back(SuzhouNumbers::Generate(intPart, decPart, "單位", true));

    return candidates;
}

} // namespace NumberInputHelper
} // namespace McBopomofo
