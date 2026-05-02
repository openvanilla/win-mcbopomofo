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
