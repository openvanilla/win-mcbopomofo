#include "DictionaryService.h"

namespace McBopomofo {

DictionaryServices::DictionaryServices() {}
DictionaryServices::~DictionaryServices() {}

void DictionaryServices::load() {
    // Stubbed for now to avoid json-c/fmt dependencies on Windows
}

void DictionaryServices::lookup(std::string phrase, size_t serviceIndex, InputState* state, const StateCallback& stateCallback) {
    // Stubbed
}

std::vector<std::string> DictionaryServices::menuForPhrase(const std::string& phrase) {
    return {};
}

bool DictionaryServices::hasServices() {
    return false;
}

} // namespace McBopomofo
