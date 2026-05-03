#pragma once
#include <memory>
#include "KeyHandler.h"
#include "InputState.h"
#include "UIInterface.h"

namespace McBopomofo {

class InputController {
public:
    InputController(std::shared_ptr<KeyHandler> keyHandler, UIInterface* ui);
    ~InputController() = default;

    // Handles a key press and returns true if the key was consumed by the IME.
    bool HandleKey(const Key& key);

    // Forces the current composing string to be committed and resets the state.
    void Reset();

    // Selects a candidate by its index in the current candidate list.
    void SelectCandidate(int index);

    // Settings passthrough
    void SetInputMode(InputMode mode);
    void SetKeyboardLayout(const Formosa::Mandarin::BopomofoKeyboardLayout* layout);
    void SetSelectPhraseAfterCursorAsCandidate(bool flag);
    void SetMoveCursorAfterSelection(bool flag);
    void SetPutLowercaseLettersToComposingBuffer(bool flag);
    void SetEscKeyClearsEntireComposingBuffer(bool flag);
    void SetShiftEnterEnabled(bool flag);
    void SetCtrlEnterKeyBehavior(KeyHandlerCtrlEnter behavior);
    void SetAssociatedPhrasesEnabled(bool enabled);
    void SetHalfWidthPunctuationEnabled(bool enabled);
    void SetRepeatedPunctuationToSelectCandidateEnabled(bool enabled);
    void SetChooseCandidateUsingSpace(bool enabled);
    void SetCandidateWindowVertical(bool vertical);

    int GetCandidateIndex() const { return candidateIndex_; }

private:
    void ChangeState(std::unique_ptr<InputState> newState);

    std::shared_ptr<KeyHandler> keyHandler_;
    UIInterface* ui_;
    std::unique_ptr<InputState> currentState_;
    int candidateIndex_ = -1;
    bool candidateWindowVertical_ = false;
};

} // namespace McBopomofo
