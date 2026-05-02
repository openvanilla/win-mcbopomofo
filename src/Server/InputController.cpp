#include "InputController.h"

namespace McBopomofo {

InputController::InputController(std::shared_ptr<KeyHandler> keyHandler, UIInterface* ui)
    : keyHandler_(std::move(keyHandler)), ui_(ui) {
    currentState_ = std::make_unique<InputStates::Empty>();
}

bool InputController::HandleKey(const Key& key) {
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

void InputController::Reset() {
    keyHandler_->handleForceCommitAndReset(
        [this](std::unique_ptr<InputState> state) {
            this->ChangeState(std::move(state));
        });
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

void InputController::SetRepeatedPunctuationToSelectCandidateEnabled(bool enabled) {
    keyHandler_->setRepeatedPunctuationToSelectCandidateEnabled(enabled);
}

void InputController::SetChooseCandidateUsingSpace(bool enabled) {
    keyHandler_->setChooseCandidateUsingSpace(enabled);
}

void InputController::ChangeState(std::unique_ptr<InputState> newState) {
    if (auto* sequence = dynamic_cast<InputStates::StateSequence*>(newState.get())) {
        for (auto& s : sequence->states) {
            ChangeState(std::move(s));
        }
        return;
    }

    if (auto* commit = dynamic_cast<InputStates::Committing*>(newState.get())) {
        if (ui_) ui_->CommitString(commit->text);
        currentState_ = std::make_unique<InputStates::Empty>();
        if (ui_) ui_->Reset();
        return;
    }

    if (auto* empty = dynamic_cast<InputStates::Empty*>(newState.get())) {
        currentState_ = std::move(newState);
        if (ui_) ui_->Reset();
        return;
    }

    if (auto* emptyIgnore = dynamic_cast<InputStates::EmptyIgnoringPrevious*>(newState.get())) {
        currentState_ = std::make_unique<InputStates::Empty>();
        if (ui_) ui_->Reset();
        return;
    }

    // Normal state update (Inputting, ChoosingCandidate, Marking, etc.)
    currentState_ = std::move(newState);
    if (ui_) ui_->Update(currentState_.get());
}

} // namespace McBopomofo
