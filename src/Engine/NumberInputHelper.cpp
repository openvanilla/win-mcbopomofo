#include "NumberInputHelper.h"
#include "ChineseNumbers/ChineseNumbers.h"
#include "ChineseNumbers/SuzhouNumbers.h"
#include <vector>
#include <string>

namespace McBopomofo {
namespace NumberInputHelper {

std::vector<std::string> FillCandidatesWithNumber(
    std::string number,
    [[maybe_unused]] std::shared_ptr<Formosa::Gramambular2::LanguageModel> languageModel) {
    if (number.empty()) return {};

    std::vector<std::string> candidates;
    std::string intPart = number;
    std::string decPart = "";
    size_t dotPos = number.find('.');
    if (dotPos != std::string::npos) {
        intPart = number.substr(0, dotPos);
        decPart = number.substr(dotPos + 1);
    }

    candidates.push_back(ChineseNumbers::Generate(intPart, decPart, ChineseNumbers::ChineseNumberCase::LOWERCASE));
    candidates.push_back(ChineseNumbers::Generate(intPart, decPart, ChineseNumbers::ChineseNumberCase::UPPERCASE));
    candidates.push_back(SuzhouNumbers::Generate(intPart, decPart, "", false));

    return candidates;
}

} // namespace NumberInputHelper
} // namespace McBopomofo
