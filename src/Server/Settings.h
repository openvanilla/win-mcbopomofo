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

#include <string>

#include "InputController.h"

namespace McBopomofo {

class Settings {
 public:
  Settings();
  ~Settings() = default;

  // Load settings from the INI file
  void load();

  // Save settings to the INI file
  void save();

  // Apply the currently loaded settings to the InputController
  void applyTo(InputController& controller);

  // Getters and Setters for Settings
  InputMode inputMode() const { return inputMode_; }
  void setInputMode(InputMode mode) { inputMode_ = mode; }

  std::string keyboardLayout() const { return keyboardLayout_; }
  void setKeyboardLayout(const std::string& layout) {
    keyboardLayout_ = layout;
  }

  bool selectPhraseAfterCursorAsCandidate() const {
    return selectPhraseAfterCursorAsCandidate_;
  }
  void setSelectPhraseAfterCursorAsCandidate(bool v) {
    selectPhraseAfterCursorAsCandidate_ = v;
  }

  bool moveCursorAfterSelection() const { return moveCursorAfterSelection_; }
  void setMoveCursorAfterSelection(bool v) { moveCursorAfterSelection_ = v; }

  bool putLowercaseLettersToComposingBuffer() const {
    return putLowercaseLettersToComposingBuffer_;
  }
  void setPutLowercaseLettersToComposingBuffer(bool v) {
    putLowercaseLettersToComposingBuffer_ = v;
  }

  bool escKeyClearsEntireComposingBuffer() const {
    return escKeyClearsEntireComposingBuffer_;
  }
  void setEscKeyClearsEntireComposingBuffer(bool v) {
    escKeyClearsEntireComposingBuffer_ = v;
  }

  bool shiftEnterEnabled() const { return shiftEnterEnabled_; }
  void setShiftEnterEnabled(bool v) { shiftEnterEnabled_ = v; }

  KeyHandlerCtrlEnter ctrlEnterKeyBehavior() const {
    return ctrlEnterKeyBehavior_;
  }
  void setCtrlEnterKeyBehavior(KeyHandlerCtrlEnter v) {
    ctrlEnterKeyBehavior_ = v;
  }

  bool associatedPhrasesEnabled() const { return associatedPhrasesEnabled_; }
  void setAssociatedPhrasesEnabled(bool v) { associatedPhrasesEnabled_ = v; }

  bool halfWidthPunctuationEnabled() const {
    return halfWidthPunctuationEnabled_;
  }
  void setHalfWidthPunctuationEnabled(bool v) {
    halfWidthPunctuationEnabled_ = v;
  }

  bool chineseConversionEnabled() const { return chineseConversionEnabled_; }
  void setChineseConversionEnabled(bool v) { chineseConversionEnabled_ = v; }

  bool bopomofoFontAnnotationSupportEnabled() const {
    return bopomofoFontAnnotationSupportEnabled_;
  }
  void setBopomofoFontAnnotationSupportEnabled(bool v) {
    bopomofoFontAnnotationSupportEnabled_ = v;
  }

  bool repeatedPunctuationToSelectCandidateEnabled() const {
    return repeatedPunctuationToSelectCandidateEnabled_;
  }
  void setRepeatedPunctuationToSelectCandidateEnabled(bool v) {
    repeatedPunctuationToSelectCandidateEnabled_ = v;
  }

  bool chooseCandidateUsingSpace() const { return chooseCandidateUsingSpace_; }
  void setChooseCandidateUsingSpace(bool v) { chooseCandidateUsingSpace_ = v; }

  std::string candidateKeys() const { return candidateKeys_; }
  void setCandidateKeys(const std::string& v) { candidateKeys_ = v; }

  int candidateKeysCount() const { return candidateKeysCount_; }
  void setCandidateKeysCount(int v) { candidateKeysCount_ = v; }

  bool candidateWindowVertical() const { return candidateWindowVertical_; }
  void setCandidateWindowVertical(bool v) { candidateWindowVertical_ = v; }

  std::string selectionAction() const { return selectionAction_; }
  void setSelectionAction(const std::string& v) { selectionAction_ = v; }

  int candidateFontSize() const { return candidateFontSize_; }
  void setCandidateFontSize(int v) { candidateFontSize_ = v; }

  bool shiftToggleOpenClose() const { return shiftToggleOpenClose_; }
  void setShiftToggleOpenClose(bool v) { shiftToggleOpenClose_ = v; }

  bool beepOnError() const { return beepOnError_; }
  void setBeepOnError(bool v) { beepOnError_ = v; }

 private:
  IPC::CandidateWindowColors readCandidateWindowColors_() const;
  std::wstring iniFilePath_() const;
  std::wstring readString_(const wchar_t* section, const wchar_t* key,
                           const wchar_t* defaultVal);
  void writeString_(const wchar_t* section, const wchar_t* key,
                    const std::wstring& val);

  int readInt_(const wchar_t* section, const wchar_t* key, int defaultVal);
  void writeInt_(const wchar_t* section, const wchar_t* key, int val);

  bool readBool_(const wchar_t* section, const wchar_t* key, bool defaultVal);
  void writeBool_(const wchar_t* section, const wchar_t* key, bool val);

  InputMode inputMode_ = InputMode::McBopomofo;
  std::string keyboardLayout_ = "Standard";
  bool selectPhraseAfterCursorAsCandidate_ = false;
  bool moveCursorAfterSelection_ = false;
  bool putLowercaseLettersToComposingBuffer_ = false;
  bool escKeyClearsEntireComposingBuffer_ = false;
  bool shiftEnterEnabled_ = true;
  KeyHandlerCtrlEnter ctrlEnterKeyBehavior_ = KeyHandlerCtrlEnter::Disabled;
  bool associatedPhrasesEnabled_ = false;
  bool halfWidthPunctuationEnabled_ = false;
  bool chineseConversionEnabled_ = false;
  bool bopomofoFontAnnotationSupportEnabled_ = false;
  bool repeatedPunctuationToSelectCandidateEnabled_ = false;
  bool chooseCandidateUsingSpace_ = true;
  std::string candidateKeys_ = "123456789";
  int candidateKeysCount_ = 9;
  bool candidateWindowVertical_ = false;
  std::string selectionAction_ = "None";
  int candidateFontSize_ = 16;
  IPC::CandidateWindowColors candidateWindowColors_;
  bool shiftToggleOpenClose_ = true;
  bool beepOnError_ = true;
};

}  // namespace McBopomofo
