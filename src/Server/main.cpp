#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "McBopomofoLM.h"
#include "UTFHelper.h"

int main(int argc, char* argv[]) {
    std::cout << "Win-McBopomofo Server starting..." << std::endl;
    
    if (argc < 2) {
        std::cout << "Usage: McBopomofoServer <path_to_data.txt>" << std::endl;
        return 1;
    }

    std::string dataPath = argv[1];
    std::cout << "Loading language model from: " << dataPath << std::endl;

    McBopomofo::McBopomofoLM lm;
    lm.loadLanguageModel(dataPath.c_str());

    if (!lm.isDataModelLoaded()) {
        std::cerr << "Failed to load language model." << std::endl;
        return 1;
    }

    std::cout << "Language model loaded successfully." << std::endl;

    // Simple test loop
    std::string line;
    std::cout << "Enter a Bopomofo reading (e.g. ã„•ã„¨) or 'q' to quit:" << std::endl;
    while (std::getline(std::cin, line)) {
        if (line == "q") break;
        if (line.empty()) continue;

        auto unigrams = lm.getUnigrams(line);
        if (unigrams.empty()) {
            std::cout << "No results found for: " << line << std::endl;
        } else {
            std::cout << "Results for " << line << ":" << std::endl;
            for (const auto& u : unigrams) {
                std::cout << "  " << u.value() << " (score: " << u.score() << ")" << std::endl;
            }
        }
        std::cout << "Enter a Bopomofo reading or 'q' to quit:" << std::endl;
    }
    
    return 0;
}
