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
#include "NamedPipe.h"

using namespace McBopomofo;

class ServerUI : public UIInterface {
public:
    IPC::StateUpdatePayload currentState;

    void Reset() override {
        currentState = IPC::StateUpdatePayload();
    }

    void CommitString(const std::string& text) override {
        currentState.commitString = text;
    }

    void Update(InputState* state) override {
        currentState.commitString.clear();
        currentState.forceVertical = false;

        // Determine if we need to force vertical layout
        if (dynamic_cast<InputStates::NumberInput*>(state) != nullptr ||
            dynamic_cast<InputStates::SelectingDictionary*>(state) != nullptr ||
            dynamic_cast<InputStates::SelectingFeature*>(state) != nullptr ||
            dynamic_cast<InputStates::SelectingDateMacro*>(state) != nullptr) {
            currentState.forceVertical = true;
        }

        if (auto* inputting = dynamic_cast<InputStates::Inputting*>(state)) {
            currentState.composingBuffer = inputting->composingBuffer;
            currentState.cursorIndex = (int)inputting->cursorIndex;
            currentState.candidates.clear();
        } else if (auto* choosing = dynamic_cast<InputStates::ChoosingCandidate*>(state)) {
            currentState.composingBuffer = choosing->composingBuffer;
            currentState.cursorIndex = (int)choosing->cursorIndex;
            currentState.candidates.clear();
            for (const auto& c : choosing->candidates) {
                currentState.candidates.push_back(c.value);
                
                // If any candidate string length > 8, force vertical
                if (c.value.length() > 8 * 3) { // Approximate check for UTF-8 lengths
                    currentState.forceVertical = true;
                }
            }
        } else {
            currentState.composingBuffer.clear();
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

    std::cout << "Win-McBopomofo Server daemon starting..." << std::endl;
    
    std::string dataPath = "data/data.txt";
    if (argc >= 2) {
        dataPath = argv[1];
    }

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

    ServerUI ui;
    InputController controller(keyHandler, &ui);

    Settings settings;
    settings.ApplyTo(controller);

    std::cout << "Starting Named Pipe server at " << IPC::PIPE_NAME << std::endl;

    IPC::NamedPipeServer server(IPC::PIPE_NAME, [&](const std::string& req) {
        IPC::KeyEventPayload keyReq;
        if (IPC::DeserializeKeyEvent(req, keyReq)) {
            // Reset UI payload before processing
            ui.currentState.commitString.clear();
            
            // Map Windows VK/states to McBopomofo::Key
            bool consumed = controller.HandleKey(MapIPCKey(keyReq));
            
            ui.currentState.consumed = consumed;
            return IPC::SerializeStateUpdate(ui.currentState);
        }
        return std::string();
    });

    server.Start();

    std::cout << "Server is running in background. Waiting for messages." << std::endl;

    // Standard message loop to keep the process alive
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return 0;
}
