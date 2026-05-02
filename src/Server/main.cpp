#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <iomanip>

#include "McBopomofoLM.h"
#include "KeyHandler.h"
#include "InputController.h"
#include "InputMacro.h"
#include "WindowsKeyBridge.h"
#include "UIInterface.h"
#include "Settings.h"
#include "UTFHelper.h"

using namespace McBopomofo;

class CLITestUI : public UIInterface {
public:
    void Reset() override {
        // std::cout << "\n[UI] Reset" << std::endl;
    }

    void CommitString(const std::string& text) override {
        std::cout << "\n[COMMIT] " << text << std::endl;
    }

    void Update(InputState* state) override {
        if (auto* inputting = dynamic_cast<InputStates::Inputting*>(state)) {
            std::cout << "\r[COMPOSING] " << inputting->composingBuffer 
                      << " (Cursor: " << inputting->cursorIndex << ")" << std::flush;
        } else if (auto* choosing = dynamic_cast<InputStates::ChoosingCandidate*>(state)) {
            std::cout << "\n[CANDIDATES] " << choosing->composingBuffer << std::endl;
            for (size_t i = 0; i < choosing->candidates.size(); ++i) {
                std::cout << i + 1 << ". " << choosing->candidates[i].value << "  ";
                if ((i + 1) % 5 == 0) std::cout << "\n";
            }
            std::cout << "\nSelection: " << std::flush;
        }
    }
};

class DummyLocalizedStrings : public LocalizedStrings {
public:
    std::string cursorIsBetweenSyllables(const std::string&, const std::string&) override { return "Cursor between syllables"; }
    std::string bopomofoFontAnnotationModeTooltip(bool, bool) override { return "Font annotation mode"; }
    std::string syllablesRequired(size_t s) override { return "Requires " + std::to_string(s) + " syllables"; }
    std::string syllablesMaximum(size_t s) override { return "Maximum " + std::to_string(s) + " syllables"; }
    std::string phraseAlreadyExists() override { return "Phrase exists"; }
    std::string pressEnterToAddThePhrase() override { return "Press Enter to add"; }
    std::string markedWithSyllablesAndStatus(const std::string&, const std::string&, const std::string& s) override { return s; }
    std::string markingNotAvailableInFontAnnotationMode() override { return "Not available in font annotation mode"; }
};

class DummyUserPhraseAdder : public UserPhraseAdder {
public:
    void addUserPhrase(const std::string_view&, const std::string_view&) override {}
    void removeUserPhrase(const std::string_view&, const std::string_view&) override {}
};

int main(int argc, char* argv[]) {
    // Set console output to UTF-8
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    std::cout << "Win-McBopomofo CLI Test Tool" << std::endl;
    
    if (argc < 2) {
        std::cout << "Usage: McBopomofoServer <path_to_data.txt>" << std::endl;
        return 1;
    }

    std::string dataPath = argv[1];
    auto lm = std::make_shared<McBopomofoLM>();
    lm->loadLanguageModel(dataPath.c_str());

    if (!lm->isDataModelLoaded()) {
        std::cerr << "Failed to load language model from: " << dataPath << std::endl;
        return 1;
    }

    // Set macro converter in LM
    InputMacroController macroController;
    lm->setMacroConverter([&macroController](const std::string& input) {
        return macroController.handle(input);
    });

    std::shared_ptr<KeyHandler> keyHandler(new KeyHandler(
        lm, 
        std::shared_ptr<VariantAnnotator>(nullptr), 
        std::static_pointer_cast<UserPhraseAdder>(std::make_shared<DummyUserPhraseAdder>()), 
        std::unique_ptr<LocalizedStrings>(new DummyLocalizedStrings())
    ));

    // Default to Standard Layout
    keyHandler->setKeyboardLayout(Formosa::Mandarin::BopomofoKeyboardLayout::StandardLayout());

    CLITestUI ui;
    InputController controller(keyHandler, &ui);

    Settings settings;
    settings.ApplyTo(controller);
    // If you want to force save defaults: settings.Save();

    std::cout << "Language model loaded. Typing '5j/ jp6' for 中文..." << std::endl;
    std::cout << "Available macros: MACRO@DATE_TODAY_SHORT, MACRO@GANZHI_YEAR, etc." << std::endl;
    std::cout << "Type 'exit' to quit." << std::endl;

    std::string input;
    while (true) {
        std::cout << "\n> " << std::flush;
        if (!std::getline(std::cin, input) || input == "exit") break;

        for (char c : input) {
            char ascii = c;
            Key::KeyName name = Key::KeyName::ASCII;
            if (c == ' ') {
                ascii = Key::SPACE;
            }
            controller.HandleKey(Key(ascii, name, false, false, false));
        }
        // Force an ENTER to commit if anything is left
        controller.HandleKey(Key(Key::RETURN, Key::KeyName::ASCII, false, false, false));
    }
    
    return 0;
}
