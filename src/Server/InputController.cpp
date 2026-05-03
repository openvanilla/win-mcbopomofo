#include "InputController.h"

#include <algorithm>
#include <cctype>
#include <shellapi.h>
#include <windows.h>

#include "UTF8Helper.h"
#include "McBopomofoLM.h"

namespace McBopomofo {

namespace {

constexpr size_t kForceVerticalCandidateThreshold = 8;

int CandidateCount(InputState* state) {
    if (auto* choosing = dynamic_cast<InputStates::ChoosingCandidate*>(state)) {
        return static_cast<int>(choosing->candidates.size());
    }
    if (auto* selecting = dynamic_cast<InputStates::SelectingDictionary*>(state)) {
        return static_cast<int>(selecting->menu.size());
    }
    if (dynamic_cast<InputStates::ShowingCharInfo*>(state) != nullptr) {
        return 2;
    }
    if (auto* associated = dynamic_cast<InputStates::AssociatedPhrases*>(state)) {
        return static_cast<int>(associated->candidates.size());
    }
    if (auto* associatedPlain = dynamic_cast<InputStates::AssociatedPhrasesPlain*>(state)) {
        return static_cast<int>(associatedPlain->candidates.size());
    }
    if (auto* number = dynamic_cast<InputStates::NumberInput*>(state)) {
        return static_cast<int>(number->candidates.size());
    }
    if (auto* selectingFeature = dynamic_cast<InputStates::SelectingFeature*>(state)) {
        return static_cast<int>(selectingFeature->features.size());
    }
    if (auto* selectingDateMacro = dynamic_cast<InputStates::SelectingDateMacro*>(state)) {
        return static_cast<int>(selectingDateMacro->menu.size());
    }
    if (auto* iroha = dynamic_cast<InputStates::IrohaCandidate*>(state)) {
        return static_cast<int>(iroha->candidates.size());
    }
    if (auto* customMenu = dynamic_cast<InputStates::CustomMenu*>(state)) {
        return static_cast<int>(customMenu->entries.size());
    }
    return 0;
}

bool IsCandidateState(InputState* state) {
    return CandidateCount(state) > 0 ||
           dynamic_cast<InputStates::ChoosingCandidate*>(state) != nullptr ||
           dynamic_cast<InputStates::SelectingDictionary*>(state) != nullptr ||
           dynamic_cast<InputStates::ShowingCharInfo*>(state) != nullptr ||
           dynamic_cast<InputStates::AssociatedPhrases*>(state) != nullptr ||
           dynamic_cast<InputStates::AssociatedPhrasesPlain*>(state) != nullptr ||
           dynamic_cast<InputStates::NumberInput*>(state) != nullptr ||
           dynamic_cast<InputStates::SelectingFeature*>(state) != nullptr ||
           dynamic_cast<InputStates::SelectingDateMacro*>(state) != nullptr ||
           dynamic_cast<InputStates::IrohaCandidate*>(state) != nullptr ||
           dynamic_cast<InputStates::CustomMenu*>(state) != nullptr;
}

bool IsForcedVerticalCandidateState(InputState* state) {
    if (dynamic_cast<InputStates::NumberInput*>(state) != nullptr ||
        dynamic_cast<InputStates::SelectingDictionary*>(state) != nullptr ||
        dynamic_cast<InputStates::ShowingCharInfo*>(state) != nullptr ||
        dynamic_cast<InputStates::SelectingFeature*>(state) != nullptr ||
        dynamic_cast<InputStates::SelectingDateMacro*>(state) != nullptr) {
        return true;
    }

    auto* choosing = dynamic_cast<InputStates::ChoosingCandidate*>(state);
    if (choosing == nullptr) {
        return false;
    }
    for (const auto& candidate : choosing->candidates) {
        if (CodePointCount(candidate.value) > kForceVerticalCandidateThreshold) {
            return true;
        }
    }
    return false;
}

int SelectionIndexFromKey(const Key& key, bool useShiftKey, const std::string& candidateKeys, int candidateKeysCount) {
    if (useShiftKey) {
        if (!key.shiftPressed) {
            return -1;
        }
        if (key.ascii >= '1' && key.ascii <= '9') {
            return key.ascii - '1';
        }
        switch (key.ascii) {
        case '!': return 0;
        case '@': return 1;
        case '#': return 2;
        case '$': return 3;
        case '%': return 4;
        case '^': return 5;
        case '&': return 6;
        case '*': return 7;
        case '(': return 8;
        default: return -1;
        }
    }

    char ascii = static_cast<char>(key.ascii);
    ascii = static_cast<char>(std::tolower(static_cast<unsigned char>(ascii)));
    auto found = candidateKeys.find(ascii);
    if (found != std::string::npos && found < static_cast<size_t>(candidateKeysCount)) {
        return static_cast<int>(found);
    }
    return -1;
}

bool HasInvalidDictionaryPrefix(const std::string& reading) {
    const char* invalidPrefixes[] = {
        "_half_punctuation_",
        "_ctrl_punctuation_",
        "_letter_",
        "_number_",
        "_punctuation_",
    };
    for (const char* prefix : invalidPrefixes) {
        if (reading.rfind(prefix, 0) == 0) {
            return true;
        }
    }
    return false;
}

}  // namespace

InputController::InputController(std::shared_ptr<KeyHandler> keyHandler, UIInterface* ui)
    : keyHandler_(std::move(keyHandler)), ui_(ui) {
    currentState_ = std::make_unique<InputStates::Empty>();
}

void InputController::SetDataDirectory(const std::filesystem::path& dataDir) {
    try {
        std::filesystem::path openccPath = dataDir / "opencc" / "tw2s.json";
        openccConverter_ = std::make_unique<opencc::SimpleConverter>(openccPath.string());
        
        auto lm = std::dynamic_pointer_cast<McBopomofoLM>(keyHandler_->getLM());
        if (lm) {
            lm->setExternalConverter([this](const std::string& input) {
                if (openccConverter_) {
                    return openccConverter_->Convert(input);
                }
                return input;
            });
            // We disable it by default. The user will toggle it via the tray menu.
            keyHandler_->setChineseConversionEnabled(false);
        }
    } catch (const std::exception&) {
        // Fallback or log if missing
        openccConverter_.reset();
    }
}

void InputController::ToggleChineseConversion() {
    bool current = keyHandler_->chineseConversionEnabled();
    keyHandler_->setChineseConversionEnabled(!current);
}

bool InputController::IsChineseConversionEnabled() const {
    return keyHandler_->chineseConversionEnabled();
}

bool InputController::HandleKey(const Key& key) {
    if (auto* numberInput = dynamic_cast<InputStates::NumberInput*>(currentState_.get())) {
        if (keyHandler_->handleNumberInput(
                key, numberInput,
                [this](std::unique_ptr<InputState> state) { ChangeState(std::move(state)); },
                []() {})) {
            return true;
        }
    }

    if (IsCandidateState(currentState_.get())) {
        return HandleCandidateKey(key);
    }

    bool consumed = keyHandler_->handle(
        key,
        currentState_.get(),
        [this](std::unique_ptr<InputState> state) {
            this->ChangeState(std::move(state));
        },
        []() {
            // Error callback (e.g. beep)
        });

    return consumed;
}

bool InputController::HandleCandidateKey(const Key& key) {
    int count = CandidateCount(currentState_.get());
    if (count == 0) {
        if (key.ascii == Key::ESC || key.ascii == Key::BACKSPACE) {
            ChangeState(std::make_unique<InputStates::EmptyIgnoringPrevious>());
            return true;
        }
        return true;
    }

    if (candidateIndex_ < 0 || candidateIndex_ >= count) {
        candidateIndex_ = 0;
    }

    auto* choosingPunctuation =
        dynamic_cast<InputStates::ChoosingPunctuationList*>(currentState_.get());
    auto* choosing =
        dynamic_cast<InputStates::ChoosingCandidate*>(currentState_.get());
    auto* associated =
        dynamic_cast<InputStates::AssociatedPhrases*>(currentState_.get());
    auto* associatedPlain =
        dynamic_cast<InputStates::AssociatedPhrasesPlain*>(currentState_.get());
    auto* numberInput = dynamic_cast<InputStates::NumberInput*>(currentState_.get());
    bool useShiftKey = numberInput != nullptr || associatedPlain != nullptr ||
                       (associated != nullptr && associated->useShiftKey);

    int selectionIndex = SelectionIndexFromKey(key, useShiftKey, candidateKeys_, candidateKeysCount_);
    if (selectionIndex != -1) {
        int actualIndex = useShiftKey ? selectionIndex
                                      : (candidateIndex_ / candidateKeysCount_) * candidateKeysCount_ + selectionIndex;
        if (actualIndex < count) {
            SelectCandidate(actualIndex);
        }
        return true;
    }

    bool shiftReturn = key.ascii == Key::RETURN && key.shiftPressed;
    if (keyHandler_->inputMode() == InputMode::McBopomofo && !useShiftKey &&
        shiftReturn && choosing != nullptr && choosingPunctuation == nullptr) {
        BuildAssociatedPhrasesForCurrentCandidate(*choosing);
        return true;
    }

    bool returnPressed = key.ascii == Key::RETURN && (!useShiftKey || key.shiftPressed);
    if (returnPressed) {
        SelectCandidate(candidateIndex_);
        return true;
    }

    if (key.ascii == Key::ESC || key.ascii == Key::BACKSPACE) {
        if (associated != nullptr && associated->useShiftKey) {
            return false;
        }
        CancelCandidatePanel();
        return true;
    }

    if (choosingPunctuation != nullptr) {
        if (keyHandler_->candidatePanelPunctuationMaybeEntered(
                key, choosingPunctuation->originalCursor,
                [this](std::unique_ptr<InputState> state) { ChangeState(std::move(state)); })) {
            return true;
        }
    }

    if (key.ascii == Key::SPACE) {
        if (associated != nullptr && associated->useShiftKey) {
            return false;
        }
        MoveCandidatePage(true);
        return true;
    }

    if (key.name == Key::KeyName::LEFT && key.shiftPressed && choosing != nullptr &&
        choosingPunctuation == nullptr) {
        MoveReadingCursorInCandidatePanel(false);
        return true;
    }
    if (key.name == Key::KeyName::RIGHT && key.shiftPressed && choosing != nullptr &&
        choosingPunctuation == nullptr) {
        MoveReadingCursorInCandidatePanel(true);
        return true;
    }

    if (HandleCandidateNavigation(key)) {
        return true;
    }

    if (keyHandler_->inputMode() == InputMode::McBopomofo && key.ascii == '?' &&
        choosing != nullptr && choosingPunctuation == nullptr) {
        EnterDictionaryState(*choosing);
        return true;
    }

    if (keyHandler_->inputMode() == InputMode::McBopomofo &&
        choosing != nullptr && choosingPunctuation == nullptr &&
        (key.ascii == '+' || key.ascii == '=' || key.ascii == '-' || key.ascii == '_')) {
        EnterPhraseActionMenu(*choosing, key.ascii == '+' || key.ascii == '=');
        return true;
    }

    if (associated != nullptr && !key.shiftPressed) {
        return false;
    }

    if (associatedPlain != nullptr && !key.shiftPressed) {
        ChangeState(std::make_unique<InputStates::Empty>());
        return false;
    }

    if (choosing != nullptr && choosingPunctuation == nullptr) {
        bool handled = keyHandler_->handleCandidateKeyForTraditionalBopomofoIfRequired(
            key,
            [this]() { SelectCandidate(candidateIndex_); },
            [this](std::unique_ptr<InputState> state) { ChangeState(std::move(state)); },
            []() {});
        if (handled) {
            return true;
        }
    }

    return true;
}

bool InputController::HandleCandidateNavigation(const Key& key) {
    bool isVertical = candidateWindowVertical_ ||
                      IsForcedVerticalCandidateState(currentState_.get());
    if (key.name == Key::KeyName::HOME) {
        candidateIndex_ = 0;
    } else if (key.name == Key::KeyName::END) {
        candidateIndex_ = std::max(0, CandidateCount(currentState_.get()) - 1);
    } else if (key.name == Key::KeyName::PAGE_UP) {
        MoveCandidatePage(false);
    } else if (key.name == Key::KeyName::PAGE_DOWN) {
        MoveCandidatePage(true);
    } else if (isVertical && key.name == Key::KeyName::UP) {
        MoveCandidateCursor(false);
    } else if (isVertical && key.name == Key::KeyName::DOWN) {
        MoveCandidateCursor(true);
    } else if (isVertical && key.name == Key::KeyName::LEFT) {
        MoveCandidatePage(false);
    } else if (isVertical && key.name == Key::KeyName::RIGHT) {
        MoveCandidatePage(true);
    } else if (!isVertical && key.name == Key::KeyName::LEFT) {
        MoveCandidateCursor(false);
    } else if (!isVertical && key.name == Key::KeyName::RIGHT) {
        MoveCandidateCursor(true);
    } else if (!isVertical && key.name == Key::KeyName::UP) {
        MoveCandidatePage(false);
    } else if (!isVertical && key.name == Key::KeyName::DOWN) {
        MoveCandidatePage(true);
    } else {
        return false;
    }

    if (ui_) {
        ui_->Update(currentState_.get());
    }
    return true;
}

void InputController::MoveCandidateCursor(bool forward) {
    int count = CandidateCount(currentState_.get());
    if (count <= 0) {
        candidateIndex_ = -1;
        return;
    }
    if (forward) {
        candidateIndex_ = (candidateIndex_ + 1) % count;
    } else {
        candidateIndex_ = candidateIndex_ > 0 ? candidateIndex_ - 1 : count - 1;
    }
}

void InputController::MoveCandidatePage(bool forward) {
    int count = CandidateCount(currentState_.get());
    if (count <= 0) {
        candidateIndex_ = -1;
        return;
    }
    int totalPages = (count + candidateKeysCount_ - 1) / candidateKeysCount_;
    int page = candidateIndex_ / candidateKeysCount_;
    if (forward) {
        page = page + 1 < totalPages ? page + 1 : 0;
    } else {
        page = page > 0 ? page - 1 : totalPages - 1;
    }
    candidateIndex_ = page * candidateKeysCount_;
}

void InputController::MoveReadingCursorInCandidatePanel(bool forward) {
    size_t cursor = keyHandler_->candidateCursorIndex();
    if (forward) {
        ++cursor;
    } else if (cursor > 0) {
        --cursor;
    }
    keyHandler_->setCandidateCursorIndex(cursor);

    auto inputting = keyHandler_->buildInputtingState();
    auto choosing = keyHandler_->buildChoosingCandidateState(
        inputting.get(), keyHandler_->candidateCursorIndex());
    ChangeState(std::move(choosing));
}

void InputController::CancelCandidatePanel() {
    if (auto* selecting = dynamic_cast<InputStates::SelectingDictionary*>(currentState_.get())) {
        ChangeState(std::move(selecting->previousState));
        return;
    }

    if (auto* showingCharInfo = dynamic_cast<InputStates::ShowingCharInfo*>(currentState_.get())) {
        ChangeState(std::move(showingCharInfo->previousState));
        return;
    }

    if (auto* customMenu = dynamic_cast<InputStates::CustomMenu*>(currentState_.get())) {
        if (auto* choosing = dynamic_cast<InputStates::ChoosingCandidate*>(customMenu->previousState.get())) {
            ChangeState(std::make_unique<InputStates::ChoosingCandidate>(*choosing));
        } else {
            ChangeState(std::make_unique<InputStates::EmptyIgnoringPrevious>());
        }
        return;
    }

    if (auto* associated = dynamic_cast<InputStates::AssociatedPhrases*>(currentState_.get())) {
        if (associated->useShiftKey) {
            return;
        }
        if (auto* choosing = dynamic_cast<InputStates::ChoosingCandidate*>(associated->previousState.get())) {
            ChangeState(std::make_unique<InputStates::ChoosingCandidate>(*choosing));
        } else if (auto* inputting = dynamic_cast<InputStates::Inputting*>(associated->previousState.get())) {
            ChangeState(std::make_unique<InputStates::Inputting>(*inputting));
        } else {
            ChangeState(std::make_unique<InputStates::EmptyIgnoringPrevious>());
        }
        return;
    }

    if (auto* punctuation = dynamic_cast<InputStates::ChoosingPunctuationList*>(currentState_.get())) {
        keyHandler_->candidatePanelPunctuationListCancelled(
            punctuation->originalCursor,
            [this](std::unique_ptr<InputState> state) { ChangeState(std::move(state)); });
        return;
    }

    size_t originalCursor = 0;
    if (auto* choosing = dynamic_cast<InputStates::ChoosingCandidate*>(currentState_.get())) {
        originalCursor = choosing->originalCursor;
    }
    keyHandler_->candidatePanelCancelled(
        originalCursor,
        [this](std::unique_ptr<InputState> state) { ChangeState(std::move(state)); });
}

void InputController::BuildAssociatedPhrasesForCurrentCandidate(
    InputStates::ChoosingCandidate& choosing) {
    if (candidateIndex_ < 0 || candidateIndex_ >= static_cast<int>(choosing.candidates.size())) {
        return;
    }

    auto copy = std::make_unique<InputStates::ChoosingCandidate>(choosing);
    const auto& candidate = choosing.candidates[candidateIndex_];
    auto associated = keyHandler_->buildAssociatedPhrasesStateFromCandidateChoosingState(
        std::move(copy), choosing.originalCursor, candidate.reading, candidate.value,
        static_cast<size_t>(candidateIndex_));
    if (associated != nullptr) {
        ChangeState(std::move(associated));
    }
}

void InputController::EnterDictionaryState(InputStates::ChoosingCandidate& choosing) {
    if (candidateIndex_ < 0 || candidateIndex_ >= static_cast<int>(choosing.candidates.size())) {
        return;
    }
    auto* dictionaryServices = keyHandler_->getDictionaryServices();
    if (dictionaryServices == nullptr || !dictionaryServices->hasServices()) {
        return;
    }

    const auto& candidate = choosing.candidates[candidateIndex_];
    if (HasInvalidDictionaryPrefix(candidate.reading)) {
        return;
    }

    auto copy = std::make_unique<InputStates::ChoosingCandidate>(choosing);
    auto newState = keyHandler_->buildSelectingDictionaryState(
        std::move(copy), candidate.value, static_cast<size_t>(candidateIndex_));
    ChangeState(std::move(newState));
}

void InputController::EnterPhraseActionMenu(InputStates::ChoosingCandidate& choosing,
                                            bool boost) {
    if (candidateIndex_ < 0 || candidateIndex_ >= static_cast<int>(choosing.candidates.size())) {
        return;
    }

    const auto candidate = choosing.candidates[candidateIndex_];
    if (HasInvalidDictionaryPrefix(candidate.reading) ||
        candidate.reading.find('-') == std::string::npos ||
        candidate.value != candidate.rawValue) {
        return;
    }

    std::vector<InputStates::CustomMenu::MenuEntry> entries;
    if (boost) {
        entries.emplace_back("Boost", [this, reading = candidate.reading, value = candidate.value]() {
            keyHandler_->boostPhrase(reading, value);
            ChangeState(keyHandler_->buildInputtingState());
        });
    } else {
        entries.emplace_back("Exclude", [this, reading = candidate.reading, value = candidate.value]() {
            keyHandler_->excludePhrase(reading, value);
            ChangeState(keyHandler_->buildInputtingState());
        });
    }
    entries.emplace_back("Cancel", [this]() {
        auto inputting = keyHandler_->buildInputtingState();
        auto choosing = keyHandler_->buildChoosingCandidateState(
            inputting.get(), keyHandler_->candidateCursorIndex());
        ChangeState(std::move(inputting));
        ChangeState(std::move(choosing));
    });

    auto copy = std::make_unique<InputStates::ChoosingCandidate>(choosing);
    auto menu = std::make_unique<InputStates::CustomMenu>(
        std::move(copy),
        boost ? "Do you want to boost the score of the phrase?" :
                "Do you want to exclude the phrase?",
        std::move(entries));
    ChangeState(std::move(menu));
}

void InputController::Reset() {
    candidateIndex_ = -1;
    keyHandler_->handleForceCommitAndReset(
        [this](std::unique_ptr<InputState> state) {
            this->ChangeState(std::move(state));
        });
}

void InputController::SelectCandidate(int index) {
    if (auto* choosing = dynamic_cast<InputStates::ChoosingPunctuationList*>(currentState_.get())) {
        if (index >= 0 && index < static_cast<int>(choosing->candidates.size())) {
            candidateIndex_ = -1;
            keyHandler_->candidateSelected(
                choosing->candidates[index],
                choosing->originalCursor,
                [this](std::unique_ptr<InputState> state) { ChangeState(std::move(state)); });
        }
        return;
    }

    if (auto* choosing = dynamic_cast<InputStates::ChoosingCandidate*>(currentState_.get())) {
        if (index >= 0 && index < static_cast<int>(choosing->candidates.size())) {
            candidateIndex_ = -1;
            keyHandler_->candidateSelected(
                choosing->candidates[index],
                choosing->originalCursor,
                [this](std::unique_ptr<InputState> state) { ChangeState(std::move(state)); });
        }
        return;
    }

    if (auto* associated = dynamic_cast<InputStates::AssociatedPhrases*>(currentState_.get())) {
        if (index >= 0 && index < static_cast<int>(associated->candidates.size())) {
            candidateIndex_ = -1;
            keyHandler_->candidateAssociatedPhraseSelected(
                associated->prefixCursorIndex,
                associated->candidates[index],
                associated->prefixReading,
                associated->prefixValue,
                [this](std::unique_ptr<InputState> state) { ChangeState(std::move(state)); });
        }
        return;
    }

    if (auto* associatedPlain = dynamic_cast<InputStates::AssociatedPhrasesPlain*>(currentState_.get())) {
        if (index >= 0 && index < static_cast<int>(associatedPlain->candidates.size())) {
            candidateIndex_ = -1;
            keyHandler_->candidateSelected(
                associatedPlain->candidates[index],
                0,
                [this](std::unique_ptr<InputState> state) { ChangeState(std::move(state)); });
        }
        return;
    }

    if (auto* selecting = dynamic_cast<InputStates::SelectingDictionary*>(currentState_.get())) {
        if (index >= 0 && index < static_cast<int>(selecting->menu.size())) {
            auto* dictionaryServices = keyHandler_->getDictionaryServices();
            if (dictionaryServices != nullptr) {
                std::string url = dictionaryServices->getUrlForPhrase(selecting->selectedPhrase, index);
                if (!url.empty()) {
                    ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOW);
                }
            }
            ChangeState(std::move(selecting->previousState));
        }
        return;
    }

    if (auto* numberInput = dynamic_cast<InputStates::NumberInput*>(currentState_.get())) {
        if (index >= 0 && index < static_cast<int>(numberInput->candidates.size())) {
            std::string text = numberInput->candidates[index];
            ChangeState(std::make_unique<InputStates::Committing>(text));
        }
        return;
    }

    if (auto* selectingFeature = dynamic_cast<InputStates::SelectingFeature*>(currentState_.get())) {
        if (index >= 0 && index < static_cast<int>(selectingFeature->features.size())) {
            ChangeState(selectingFeature->nextState(static_cast<size_t>(index)));
        }
        return;
    }

    if (auto* selectingDateMacro = dynamic_cast<InputStates::SelectingDateMacro*>(currentState_.get())) {
        if (index >= 0 && index < static_cast<int>(selectingDateMacro->menu.size())) {
            std::string text = selectingDateMacro->menu[index];
            ChangeState(std::make_unique<InputStates::Committing>(text));
        }
        return;
    }

    if (auto* iroha = dynamic_cast<InputStates::IrohaCandidate*>(currentState_.get())) {
        if (index >= 0 && index < static_cast<int>(iroha->candidates.size())) {
            std::string text = iroha->candidates[index];
            auto seq = std::make_unique<InputStates::StateSequence>();
            seq->push_back(std::make_unique<InputStates::Committing>(text));
            seq->push_back(std::make_unique<InputStates::Iroha>(""));
            ChangeState(std::move(seq));
        }
        return;
    }

    if (auto* customMenu = dynamic_cast<InputStates::CustomMenu*>(currentState_.get())) {
        if (index >= 0 && index < static_cast<int>(customMenu->entries.size()) &&
            customMenu->entries[index].callback) {
            customMenu->entries[index].callback();
        }
        return;
    }
}

void InputController::SetInputMode(InputMode mode) {
    keyHandler_->setInputMode(mode);
}

void InputController::SetKeyboardLayout(const Formosa::Mandarin::BopomofoKeyboardLayout* layout) {
    keyHandler_->setKeyboardLayout(layout);
}

void InputController::SetSelectPhraseAfterCursorAsCandidate(bool flag) {
    keyHandler_->setSelectPhraseAfterCursorAsCandidate(flag);
}

void InputController::SetMoveCursorAfterSelection(bool flag) {
    keyHandler_->setMoveCursorAfterSelection(flag);
}

void InputController::SetPutLowercaseLettersToComposingBuffer(bool flag) {
    keyHandler_->setPutLowercaseLettersToComposingBuffer(flag);
}

void InputController::SetEscKeyClearsEntireComposingBuffer(bool flag) {
    keyHandler_->setEscKeyClearsEntireComposingBuffer(flag);
}

void InputController::SetShiftEnterEnabled(bool flag) {
    keyHandler_->setShiftEnterEnabled(flag);
}

void InputController::SetCtrlEnterKeyBehavior(KeyHandlerCtrlEnter behavior) {
    keyHandler_->setCtrlEnterKeyBehavior(behavior);
}

void InputController::SetAssociatedPhrasesEnabled(bool enabled) {
    keyHandler_->setAssociatedPhrasesEnabled(enabled);
}

void InputController::SetHalfWidthPunctuationEnabled(bool enabled) {
    keyHandler_->setHalfWidthPunctuationEnabled(enabled);
}

void InputController::SetBopomofoFontAnnotationSupportEnabled(bool enabled) {
    keyHandler_->setBopomofoFontAnnotationSupportEnabled(enabled);
}

void InputController::SetRepeatedPunctuationToSelectCandidateEnabled(bool enabled) {
    keyHandler_->setRepeatedPunctuationToSelectCandidateEnabled(enabled);
}

void InputController::SetChooseCandidateUsingSpace(bool enabled) {
    keyHandler_->setChooseCandidateUsingSpace(enabled);
}

void InputController::SetCandidateKeys(const std::string& keys) {
    if (keys == "123456789" || keys == "asdfghjkl" || keys == "asdfzxcvb") {
        candidateKeys_ = keys;
    } else {
        candidateKeys_ = "123456789";
    }
}

void InputController::SetCandidateKeysCount(int count) {
    if (count >= 4 && count <= 9) {
        candidateKeysCount_ = count;
    } else {
        candidateKeysCount_ = 9;
    }
}

void InputController::SetCandidateWindowVertical(bool vertical) {
    candidateWindowVertical_ = vertical;
}

void InputController::ChangeState(std::unique_ptr<InputState> newState) {
    if (auto* sequence = dynamic_cast<InputStates::StateSequence*>(newState.get())) {
        for (auto& s : sequence->states) {
            ChangeState(std::move(s));
        }
        return;
    } else if (auto* commit = dynamic_cast<InputStates::Committing*>(newState.get())) {
        if (ui_) {
            std::string text = commit->text;
            if (keyHandler_->chineseConversionEnabled() && openccConverter_) {
                text = openccConverter_->Convert(text);
            }
            ui_->CommitString(text);
        }
        newState = std::make_unique<InputStates::Empty>();
    }

    if (dynamic_cast<InputStates::Empty*>(newState.get()) != nullptr) {        currentState_ = std::move(newState);
        candidateIndex_ = -1;
        if (ui_) ui_->Reset();
        return;
    }

    if (dynamic_cast<InputStates::EmptyIgnoringPrevious*>(newState.get()) != nullptr) {
        currentState_ = std::make_unique<InputStates::Empty>();
        candidateIndex_ = -1;
        if (ui_) ui_->Reset();
        return;
    }

    int newCandidateCount = CandidateCount(newState.get());
    if (IsCandidateState(newState.get())) {
        if (candidateIndex_ < 0 || candidateIndex_ >= newCandidateCount) {
            candidateIndex_ = newCandidateCount > 0 ? 0 : -1;
        }
    } else {
        candidateIndex_ = -1;
    }

    currentState_ = std::move(newState);
    if (ui_) ui_->Update(currentState_.get());
}

} // namespace McBopomofo
