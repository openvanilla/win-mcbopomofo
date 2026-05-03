#include "InputController.h"
#include <windows.h>
#include <shellapi.h>

namespace McBopomofo {

InputController::InputController(std::shared_ptr<KeyHandler> keyHandler, UIInterface* ui)
    : keyHandler_(std::move(keyHandler)), ui_(ui) {
    currentState_ = std::make_unique<InputStates::Empty>();
}

bool InputController::HandleKey(const Key& key) {
    if (auto* selDict = dynamic_cast<InputStates::SelectingDictionary*>(currentState_.get())) {
        const int pageSize = 9;
        int count = (int)selDict->menu.size();

        if (key.name == Key::KeyName::UP || key.name == Key::KeyName::DOWN ||
            key.name == Key::KeyName::LEFT || key.name == Key::KeyName::RIGHT ||
            key.name == Key::KeyName::HOME || key.name == Key::KeyName::END ||
            key.name == Key::KeyName::PAGE_UP || key.name == Key::KeyName::PAGE_DOWN) {
            
            if (key.name == Key::KeyName::HOME) {
                candidateIndex_ = 0;
            } else if (key.name == Key::KeyName::END) {
                candidateIndex_ = count - 1;
            } else if (key.name == Key::KeyName::UP || key.name == Key::KeyName::LEFT) {
                if (candidateIndex_ > 0) {
                    candidateIndex_--;
                } else {
                    candidateIndex_ = count - 1;
                }
            } else if (key.name == Key::KeyName::DOWN || key.name == Key::KeyName::RIGHT) {
                if (candidateIndex_ < count - 1) {
                    candidateIndex_++;
                } else {
                    candidateIndex_ = 0;
                }
            } else if (key.name == Key::KeyName::PAGE_UP) {
                candidateIndex_ = std::max(0, candidateIndex_ - pageSize);
            } else if (key.name == Key::KeyName::PAGE_DOWN) {
                candidateIndex_ = std::min(count - 1, candidateIndex_ + pageSize);
            }
            if (ui_) ui_->Update(currentState_.get());
            return true;
        }

        if (key.ascii == Key::RETURN) {
            auto* dictionaryServices = keyHandler_->getDictionaryServices();
            if (dictionaryServices && candidateIndex_ >= 0 && candidateIndex_ < count) {
                std::string url = dictionaryServices->getUrlForPhrase(selDict->selectedPhrase, candidateIndex_);
                if (!url.empty()) {
                    // Send to client to open URL via ui_->CommitString or another mechanism, 
                    // or open directly here.
                    // Wait, earlier I added ShellExecuteW in CLangBarButton. Here we can use ShellExecuteA in the server.
#ifdef _WIN32
                    ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOW);
#endif
                }
                
                // Return to previous state
                ChangeState(std::move(selDict->previousState));
            }
            return true;
        }

        if (key.ascii >= '1' && key.ascii <= '9') {
            int pageIndex = candidateIndex_ / pageSize;
            int indexOnPage = key.ascii - '1';
            int actualIndex = pageIndex * pageSize + indexOnPage;
            if (actualIndex < count) {
                auto* dictionaryServices = keyHandler_->getDictionaryServices();
                if (dictionaryServices) {
                    std::string url = dictionaryServices->getUrlForPhrase(selDict->selectedPhrase, actualIndex);
                    if (!url.empty()) {
#ifdef _WIN32
                        ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOW);
#endif
                    }
                    ChangeState(std::move(selDict->previousState));
                }
            }
            return true;
        }

        if (key.ascii == Key::ESC) {
            ChangeState(std::move(selDict->previousState));
            return true;
        }

        return true; // Block all other keys
    }

    if (auto* choosing = dynamic_cast<InputStates::ChoosingCandidate*>(currentState_.get())) {
        const int pageSize = 9;
        int count = (int)choosing->candidates.size();
        
        if (key.ascii == Key::SPACE) {
            int pageIndex = candidateIndex_ / pageSize;
            int nextPageIndex = pageIndex + 1;
            int totalPages = (count + pageSize - 1) / pageSize;
            if (nextPageIndex < totalPages) {
                candidateIndex_ = nextPageIndex * pageSize;
            } else {
                candidateIndex_ = 0;
            }
            if (ui_) ui_->Update(currentState_.get());
            return true;
        }

        if (key.name == Key::KeyName::UP || key.name == Key::KeyName::DOWN ||
            key.name == Key::KeyName::LEFT || key.name == Key::KeyName::RIGHT ||
            key.name == Key::KeyName::HOME || key.name == Key::KeyName::END ||
            key.name == Key::KeyName::PAGE_UP || key.name == Key::KeyName::PAGE_DOWN) {
            
            bool isVertical = candidateWindowVertical_;
            int pageIndex = candidateIndex_ / pageSize;
            int startIndex = pageIndex * pageSize;
            int endIndex = std::min(count, startIndex + pageSize);

            auto goToNextItem = [&]() {
                if (candidateIndex_ < count - 1) {
                    candidateIndex_++;
                } else {
                    candidateIndex_ = 0; // Wrap around
                }
            };

            auto goToPreviousItem = [&]() {
                if (candidateIndex_ > 0) {
                    candidateIndex_--;
                } else {
                    candidateIndex_ = count - 1; // Wrap around
                }
            };

            auto goToNextPage = [&]() {
                int nextPageIndex = pageIndex + 1;
                int totalPages = (count + pageSize - 1) / pageSize;
                if (nextPageIndex < totalPages) {
                    candidateIndex_ = nextPageIndex * pageSize;
                } else {
                    // Wrap to first page but keep the first item
                    candidateIndex_ = 0;
                }
            };

            auto goToPreviousPage = [&]() {
                int prevPageIndex = pageIndex - 1;
                if (prevPageIndex >= 0) {
                    candidateIndex_ = prevPageIndex * pageSize;
                } else {
                    // Wrap to last page
                    int totalPages = (count + pageSize - 1) / pageSize;
                    candidateIndex_ = (totalPages - 1) * pageSize;
                }
            };

            if (key.name == Key::KeyName::HOME) {
                candidateIndex_ = 0;
            } else if (key.name == Key::KeyName::END) {
                candidateIndex_ = count - 1;
            } else if (key.name == Key::KeyName::PAGE_UP) {
                goToPreviousPage();
            } else if (key.name == Key::KeyName::PAGE_DOWN) {
                goToNextPage();
            } else if (key.name == Key::KeyName::LEFT) {
                if (isVertical) goToPreviousPage();
                else goToPreviousItem();
            } else if (key.name == Key::KeyName::RIGHT) {
                if (isVertical) goToNextPage();
                else goToNextItem();
            } else if (key.name == Key::KeyName::UP) {
                if (isVertical) goToPreviousItem();
                else goToPreviousPage();
            } else if (key.name == Key::KeyName::DOWN) {
                if (isVertical) goToNextItem();
                else goToNextPage();
            }

            if (ui_) ui_->Update(currentState_.get());
            return true;
        }

        if (key.ascii == Key::RETURN) {
            SelectCandidate(candidateIndex_);
            return true;
        }

        if (key.ascii >= '1' && key.ascii <= '9') {
            int pageIndex = candidateIndex_ / pageSize;
            int indexOnPage = key.ascii - '1';
            int actualIndex = pageIndex * pageSize + indexOnPage;
            if (actualIndex < count) {
                SelectCandidate(actualIndex);
                return true;
            }
            return true; // Consume but do nothing
        }

        if (key.ascii == '?') {
            if (candidateIndex_ >= 0 && candidateIndex_ < count) {
                auto* dictionaryServices = keyHandler_->getDictionaryServices();
                if (dictionaryServices && dictionaryServices->hasServices()) {
                    std::string phrase = choosing->candidates[candidateIndex_].value;
                    std::vector<std::string> menu = dictionaryServices->menuForPhrase(phrase);
                    auto copy = std::make_unique<InputStates::ChoosingCandidate>(*choosing);
                    auto newState = std::make_unique<InputStates::SelectingDictionary>(
                        std::move(copy), phrase, candidateIndex_, std::move(menu)
                    );
                    ChangeState(std::move(newState));
                    return true;
                }
            }
        }

        // When the candidate window appears, block other keys to control the candidate window.
        // Allow ESC to dismiss the window.
        if (key.ascii != Key::ESC) {
            return true;
        }
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

void InputController::Reset() {
    candidateIndex_ = -1;
    keyHandler_->handleForceCommitAndReset(
        [this](std::unique_ptr<InputState> state) {
            this->ChangeState(std::move(state));
        });
}

void InputController::SelectCandidate(int index) {
    if (auto* choosing = dynamic_cast<InputStates::ChoosingCandidate*>(currentState_.get())) {
        if (index >= 0 && index < choosing->candidates.size()) {
            candidateIndex_ = -1;
            keyHandler_->candidateSelected(
                choosing->candidates[index],
                choosing->originalCursor,
                [this](std::unique_ptr<InputState> state) {
                    this->ChangeState(std::move(state));
                });
        }
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

void InputController::SetRepeatedPunctuationToSelectCandidateEnabled(bool enabled) {
    keyHandler_->setRepeatedPunctuationToSelectCandidateEnabled(enabled);
}

void InputController::SetChooseCandidateUsingSpace(bool enabled) {
    keyHandler_->setChooseCandidateUsingSpace(enabled);
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
    }

    if (auto* commit = dynamic_cast<InputStates::Committing*>(newState.get())) {
        if (ui_) ui_->CommitString(commit->text);
        currentState_ = std::make_unique<InputStates::Empty>();
        candidateIndex_ = -1;
        if (ui_) ui_->Reset();
        return;
    }

    if (auto* empty = dynamic_cast<InputStates::Empty*>(newState.get())) {
        currentState_ = std::move(newState);
        candidateIndex_ = -1;
        if (ui_) ui_->Reset();
        return;
    }

    if (auto* emptyIgnore = dynamic_cast<InputStates::EmptyIgnoringPrevious*>(newState.get())) {
        currentState_ = std::make_unique<InputStates::Empty>();
        candidateIndex_ = -1;
        if (ui_) ui_->Reset();
        return;
    }

    // Normal state update (Inputting, ChoosingCandidate, Marking, etc.)
    if (dynamic_cast<InputStates::ChoosingCandidate*>(newState.get()) != nullptr) {
        if (candidateIndex_ == -1) {
            candidateIndex_ = 0; // Initialize highlight to the first candidate
        }
    } else {
        candidateIndex_ = -1;
    }

    currentState_ = std::move(newState);
    if (ui_) ui_->Update(currentState_.get());
}

} // namespace McBopomofo
