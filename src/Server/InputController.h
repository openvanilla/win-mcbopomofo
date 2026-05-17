// Copyright (c) 2026 and onwards The McBopomofo Authors.
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

#pragma once
#include <SimpleConverter.hpp>
#include <memory>

#include "InputState.h"
#include "KeyHandler.h"
#include "UIInterface.h"

namespace McBopomofo {

class InputController {
 public:
  class LocalizedStrings {
   public:
    virtual ~LocalizedStrings() = default;
    virtual std::string boost() = 0;
    virtual std::string exclude() = 0;
    virtual std::string cancel() = 0;
    virtual std::string boostPrompt() = 0;
    virtual std::string excludePrompt() = 0;
  };

  InputController(std::shared_ptr<KeyHandler> keyHandler, UIInterface* ui,
                  std::unique_ptr<LocalizedStrings> localizedStrings);
  ~InputController() = default;

  // Handles a key press and returns true if the key was consumed by the IME.
  bool handleKey(const Key& key);

  // Forces the current composing string to be committed and resets the state.
  void reset();

  // Selects a candidate by its index in the current candidate list.
  void selectCandidate(int index);

  // Settings passthrough
  void setInputMode(InputMode mode);
  void setKeyboardLayout(
      const Formosa::Mandarin::BopomofoKeyboardLayout* layout);
  void setSelectPhraseAfterCursorAsCandidate(bool flag);
  void setMoveCursorAfterSelection(bool flag);
  void setPutLowercaseLettersToComposingBuffer(bool flag);
  void setEscKeyClearsEntireComposingBuffer(bool flag);
  void setShiftEnterEnabled(bool flag);
  void setCtrlEnterKeyBehavior(KeyHandlerCtrlEnter behavior);
  void setAssociatedPhrasesEnabled(bool enabled);
  void setHalfWidthPunctuationEnabled(bool enabled);
  void setBopomofoFontAnnotationSupportEnabled(bool enabled);
  void setRepeatedPunctuationToSelectCandidateEnabled(bool enabled);
  void setChooseCandidateUsingSpace(bool enabled);
  void setCandidateKeys(const std::string& keys);
  void setCandidateKeysCount(int count);
  void setCandidateWindowVertical(bool vertical);
  void setChineseConversionEnabled(bool enabled);
  void setCandidateFontSize(int size) { candidateFontSize_ = size; }
  void setCandidateWindowColors(
      const IPC::CandidateWindowColors& candidateWindowColors);
  void setBeepOnError(bool enabled) { beepOnError_ = enabled; }
  void refreshUI();

  void setDataDirectory(const std::filesystem::path& dataDir);
  void toggleChineseConversion();
  bool isChineseConversionEnabled() const;

  int candidateIndex() const { return candidateIndex_; }
  InputState* currentState() const { return currentState_.get(); }
  void setStateForTesting(std::unique_ptr<InputState> state,
                          int candidateIndex = -1) {
    currentState_ = std::move(state);
    candidateIndex_ = candidateIndex;
  }

 private:
  void enterNewState_(std::unique_ptr<InputState> previousState,
                    std::unique_ptr<InputState> newState);
  void handleError_() const;
  void notifyUI_();
  IPC::StateUpdatePayload buildStateUpdatePayload_() const;
  bool handleCandidateKey_(
      const Key& key,
      const McBopomofo::KeyHandler::StateCallback& stateCallback,
      const McBopomofo::KeyHandler::ErrorCallback& errorCallback);
  bool handleCandidateNavigation_(const Key& key);
  void moveCandidateCursor_(bool forward);
  void moveCandidatePage_(bool forward);
  void selectCandidate_(
      int index,
      const McBopomofo::KeyHandler::StateCallback& stateCallback);

  std::shared_ptr<KeyHandler> keyHandler_;
  UIInterface* ui_;
  std::unique_ptr<InputState> currentState_;
  int candidateIndex_ = -1;
  std::string candidateKeys_ = "123456789";
  int candidateKeysCount_ = 9;
  bool candidateWindowVertical_ = false;
  int candidateFontSize_ = 16;
  IPC::CandidateWindowColors candidateWindowColors_;
  bool beepOnError_ = true;

  std::unique_ptr<opencc::SimpleConverter> openccConverter_;
  std::unique_ptr<LocalizedStrings> localizedStrings_;
};

}  // namespace McBopomofo
