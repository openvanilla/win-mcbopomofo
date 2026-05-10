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

#ifndef SRC_KEYHANDLER_H_
#define SRC_KEYHANDLER_H_

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "Mandarin/Mandarin.h"
#include "DictionaryService.h"
#include "UserOverrideModel.h"
#include "gramambular2/language_model.h"
#include "gramambular2/reading_grid.h"
#include "InputMode.h"
#include "InputState.h"
#include "Key.h"

namespace McBopomofo {

class VariantAnnotator;

class LocalizedStrings {
public:
    virtual ~LocalizedStrings() = default;
    virtual std::string cursorIsBetweenSyllables(const std::string& prevReading, const std::string& nextReading) = 0;
    virtual std::string syllablesRequired(size_t syllables) = 0;
    virtual std::string syllablesMaximum(size_t syllables) = 0;
    virtual std::string phraseAlreadyExists() = 0;
    virtual std::string pressEnterToAddThePhrase() = 0;
    virtual std::string markedWithSyllablesAndStatus(const std::string& marked, const std::string& readingUiText, const std::string& status) = 0;
    virtual std::string bopomofoFontAnnotationModeTooltip(bool hasUnicodeVariantSelectors, bool hasPUABlocks) = 0;
    virtual std::string markingNotAvailableInFontAnnotationMode() = 0;
};

class UserPhraseAdder {
public:
    virtual ~UserPhraseAdder() = default;
    virtual void addUserPhrase(const std::string_view& reading, const std::string_view& phrase) = 0;
    virtual void removeUserPhrase(const std::string_view& reading, const std::string_view& phrase) = 0;
};

enum class KeyHandlerCtrlEnter {
  Disabled,
  OutputBpmfReadings,
  OutputHTMLRubyText,
  OutputHanyuPinyin,
};

class KeyHandler {
 public:
  explicit KeyHandler(
      std::shared_ptr<Formosa::Gramambular2::LanguageModel> languageModel,
      std::shared_ptr<VariantAnnotator> variantAnnotator,
      std::shared_ptr<UserPhraseAdder> userPhraseAdder,
      std::unique_ptr<LocalizedStrings> localizedStrings);

  using StateCallback =
      std::function<void(std::unique_ptr<McBopomofo::InputState>)>;
  using ErrorCallback = std::function<void(void)>;
  using SelectCurrentCandidateCallback = std::function<void(void)>;

  bool handle(Key key, McBopomofo::InputState* state,
              StateCallback stateCallback, ErrorCallback errorCallback);

  bool handleAssociatedPhrases(InputStates::Inputting* state,
                               StateCallback stateCallback,
                               ErrorCallback errorCallback, bool useShiftKey);

  bool handleNumberInput(Key key, InputStates::NumberInput* state,
                         StateCallback stateCallback,
                         KeyHandler::ErrorCallback errorCallback);

  void candidateSelected(
      const InputStates::ChoosingCandidate::Candidate& candidate,
      size_t originalCursor, StateCallback stateCallback);

  void candidateAssociatedPhraseSelected(
      size_t index, const InputStates::ChoosingCandidate::Candidate& candidate,
      const std::string& selectedReading, const std::string& selectedValue,
      const StateCallback& stateCallback);

  void dictionaryServiceSelected(std::string phrase, size_t index,
                                 InputState* currentState,
                                 StateCallback stateCallback);

  bool candidatePanelPunctuationMaybeEntered(Key key, size_t originalCursor,
                                             StateCallback stateCallback);

  void candidatePanelPunctuationListCancelled(size_t originalCursor,
                                              StateCallback stateCallback);

  void candidatePanelCancelled(size_t originalCursor,
                               StateCallback stateCallback);

  bool handleCandidateKeyForTraditionalBopomofoIfRequired(
      Key key, SelectCurrentCandidateCallback SelectCurrentCandidateCallback,
      StateCallback stateCallback, ErrorCallback errorCallback);

  void boostPhrase(const std::string& reading, const std::string& value);

  void excludePhrase(const std::string& reading, const std::string& value);

  void reset();

  bool hasDictionaryServices();
  DictionaryServices* getDictionaryServices() { return &dictionaryServices_; }

  std::unique_ptr<InputStates::SelectingDictionary>
  buildSelectingDictionaryState(
      std::unique_ptr<InputStates::NotEmpty> nonEmptyState,
      const std::string& selectedPhrase, size_t selectedIndex);

  McBopomofo::InputMode inputMode();
  void setInputMode(McBopomofo::InputMode mode);
  void setKeyboardLayout(const Formosa::Mandarin::BopomofoKeyboardLayout* layout);
  void setSelectPhraseAfterCursorAsCandidate(bool flag);
  void setMoveCursorAfterSelection(bool flag);
  void setPutLowercaseLettersToComposingBuffer(bool flag);
  void setEscKeyClearsEntireComposingBuffer(bool flag);
  void setShiftEnterEnabled(bool flag);
  void setCtrlEnterKeyBehavior(KeyHandlerCtrlEnter behavior);
  void setAssociatedPhrasesEnabled(bool enabled);
  void setHalfWidthPunctuationEnabled(bool enabled);
  void setOnAddNewPhrase(std::function<void(const std::string&)> onAddNewPhrase);
  void setRepeatedPunctuationToSelectCandidateEnabled(bool enabled);
  void setChooseCandidateUsingSpace(bool enabled);
  void setBopomofoFontAnnotationSupportEnabled(bool enabled);
  bool bopomofoFontAnnotationSupportEnabled() const { return bopomofoFontAnnotationSupportEnabled_; }

  void setChineseConversionEnabled(bool enabled);
  bool chineseConversionEnabled() const { return chineseConversionEnabled_; }
  std::shared_ptr<Formosa::Gramambular2::LanguageModel> getLM() const { return lm_; }

  size_t actualCandidateCursorIndex();
  size_t computeActualCandidateCursorIndex(size_t index);
  size_t candidateCursorIndex();
  void setCandidateCursorIndex(size_t newCursor);

  std::unique_ptr<InputStates::Inputting> buildInputtingState();
  std::unique_ptr<InputStates::ChoosingCandidate> buildChoosingCandidateState(
      InputStates::NotEmpty* nonEmptyState, size_t originalCursor);

  static constexpr char kJoinSeparator[] = "-";

  std::unique_ptr<InputStates::AssociatedPhrases> buildAssociatedPhrasesState(
      std::unique_ptr<InputStates::NotEmpty> previousState,
      size_t prefixCursorIndex, std::string prefixCombinedReading,
      std::string prefixValue, size_t selectedCandidateIndex, bool useShiftKey);

  std::unique_ptr<InputStates::AssociatedPhrases>
  buildAssociatedPhrasesStateFromCandidateChoosingState(
      std::unique_ptr<InputStates::NotEmpty> previousState,
      size_t candidateCursorIndex, std::string prefixCombinedReading,
      std::string prefixValue, size_t selectedCandidateIndex);

  std::unique_ptr<InputStates::AssociatedPhrasesPlain>
  buildAssociatedPhrasesPlainState(const std::string& reading,
                                   const std::string& value);

 private:
  bool handleBig5(Key key, McBopomofo::InputStates::Big5* state,
                  StateCallback stateCallback,
                  KeyHandler::ErrorCallback errorCallback);
  bool handleIroha(Key key, McBopomofo::InputStates::Iroha* state,
                   StateCallback stateCallback,
                   KeyHandler::ErrorCallback errorCallback);

  bool handleTabKey(bool isShiftPressed, McBopomofo::InputState* state,
                    const StateCallback& stateCallback,
                    const ErrorCallback& errorCallback);
  bool handleCursorKeys(Key key, McBopomofo::InputState* state,
                        const StateCallback& stateCallback,
                        const ErrorCallback& errorCallback);
  bool handleDeleteKeys(Key key, McBopomofo::InputState* state,
                        const StateCallback& stateCallback,
                        const ErrorCallback& errorCallback);
  bool handlePunctuation(const std::string& punctuationUnigramKey,
                         McBopomofo::InputState* state,
                         const StateCallback& stateCallback,
                         const ErrorCallback& errorCallback);

  struct ComposedString {
    std::string head;
    std::string tail;
    std::string tooltip;
  };
  ComposedString getComposedString(size_t builderCursor);
  std::string getHTMLRubyText();
  std::string getHanyuPinyin();

  std::unique_ptr<InputStates::Marking> buildMarkingState(size_t beginCursorIndex);

  void pinNode(size_t originalCursor,
               const InputStates::ChoosingCandidate::Candidate& candidate,
               bool useMoveCursorAfterSelectionSetting = true);

  void pinNodeWithAssociatedPhrase(size_t prefixCursorIndex,
                                   const std::string& prefixReading,
                                   const std::string& prefixValue,
                                   const std::string& associatedPhraseReading,
                                   const std::string& associatedPhraseValue);

  void walk();

  std::shared_ptr<Formosa::Gramambular2::LanguageModel> lm_;
  std::shared_ptr<VariantAnnotator> variantAnnotator_;
  Formosa::Gramambular2::ReadingGrid grid_;
  std::shared_ptr<UserPhraseAdder> userPhraseAdder_;
  std::unique_ptr<LocalizedStrings> localizedStrings_;

  UserOverrideModel userOverrideModel_;
  Formosa::Mandarin::BopomofoReadingBuffer reading_;
  Formosa::Gramambular2::ReadingGrid::WalkResult latestWalk_;
  DictionaryServices dictionaryServices_;

  McBopomofo::InputMode inputMode_ = McBopomofo::InputMode::McBopomofo;
  bool selectPhraseAfterCursorAsCandidate_ = false;
  bool moveCursorAfterSelection_ = false;
  bool putLowercaseLettersToComposingBuffer_ = false;
  bool escKeyClearsEntireComposingBuffer_ = false;
  bool shiftEnterEnabled_ = true;
  bool associatedPhrasesEnabled_ = false;
  bool halfWidthPunctuationEnabled_ = false;
  bool repeatedPunctuationToSelectCandidateEnabled_ = false;
  bool chooseCandidateUsingSpace_ = true;
  bool bopomofoFontAnnotationSupportEnabled_ = false;
  bool chineseConversionEnabled_ = false;
  KeyHandlerCtrlEnter ctrlEnterKey_ = KeyHandlerCtrlEnter::Disabled;
  std::function<void(const std::string&)> onAddNewPhrase_ = [](const std::string&){};
};

}  // namespace McBopomofo

#endif  // SRC_KEYHANDLER_H_
