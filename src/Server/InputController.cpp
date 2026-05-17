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

#include "InputController.h"

#include <windows.h>

#include <algorithm>
#include <array>
#include <cctype>

#include "Ipc.h"
#include "Log.h"
#include "McBopomofoLM.h"
#include "UTF8Helper.h"
#include "UTFHelper.h"

namespace McBopomofo {

namespace {

constexpr size_t kForceVerticalCandidateThreshold = 8;

int CandidateCount(InputState* state) {
  if (auto* choosing = dynamic_cast<InputStates::ChoosingCandidate*>(state)) {
    return static_cast<int>(choosing->candidates.size());
  }
  if (auto* selecting =
          dynamic_cast<InputStates::SelectingDictionary*>(state)) {
    return static_cast<int>(selecting->menu.size());
  }
  if (dynamic_cast<InputStates::ShowingCharInfo*>(state) != nullptr) {
    return 2;
  }
  if (auto* associated = dynamic_cast<InputStates::AssociatedPhrases*>(state)) {
    return associated->autoTriggered
               ? std::min(1, static_cast<int>(associated->candidates.size()))
               : static_cast<int>(associated->candidates.size());
  }
  if (auto* associatedPlain =
          dynamic_cast<InputStates::AssociatedPhrasesPlain*>(state)) {
    return static_cast<int>(associatedPlain->candidates.size());
  }
  if (auto* number = dynamic_cast<InputStates::NumberInput*>(state)) {
    return static_cast<int>(number->candidates.size());
  }
  if (auto* selectingFeature =
          dynamic_cast<InputStates::SelectingFeature*>(state)) {
    return static_cast<int>(selectingFeature->features.size());
  }
  if (auto* selectingDateMacro =
          dynamic_cast<InputStates::SelectingDateMacro*>(state)) {
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

int SelectionIndexFromKey(const Key& key, bool useShiftKey,
                          const std::string& candidateKeys,
                          int candidateKeysCount) {
  if (useShiftKey) {
    if (!key.shiftPressed) {
      return -1;
    }
    if (key.ascii >= '1' && key.ascii <= '9') {
      return key.ascii - '1';
    }
    switch (key.ascii) {
      case '!':
        return 0;
      case '@':
        return 1;
      case '#':
        return 2;
      case '$':
        return 3;
      case '%':
        return 4;
      case '^':
        return 5;
      case '&':
        return 6;
      case '*':
        return 7;
      case '(':
        return 8;
      default:
        return -1;
    }
  }

  char ascii = static_cast<char>(key.ascii);
  ascii = static_cast<char>(std::tolower(static_cast<unsigned char>(ascii)));
  auto found = candidateKeys.find(ascii);
  if (found != std::string::npos &&
      found < static_cast<size_t>(candidateKeysCount)) {
    return static_cast<int>(found);
  }
  return -1;
}

bool HasInvalidDictionaryPrefix(const std::string& reading) {
  const char* invalidPrefixes[] = {
      "_half_punctuation_", "_ctrl_punctuation_", "_letter_",
      "_number_",           "_punctuation_",
  };
  for (const char* prefix : invalidPrefixes) {
    if (reading.rfind(prefix, 0) == 0) {
      return true;
    }
  }
  return false;
}

}  // namespace

void InputController::notifyUI_() {
  if (ui_) {
    ui_->update(buildStateUpdatePayload_());
  }
}

IPC::StateUpdatePayload InputController::buildStateUpdatePayload_() const {
  IPC::StateUpdatePayload payload;
  payload.forceVertical = false;
  payload.selectionStyle = IPC::CandidateSelectionStyle::kStandard;
  payload.markStart = -1;
  payload.markEnd = -1;
  payload.candidateIndex = candidateIndex_;
  payload.candidateFontSize = candidateFontSize_;
  payload.candidateWindowVertical = candidateWindowVertical_;
  payload.candidateKeys = candidateKeys_;
  payload.candidateKeysCount = candidateKeysCount_;
  payload.candidateWindowColors = candidateWindowColors_;

  auto* state = currentState_.get();
  if (auto* notEmptyState = dynamic_cast<InputStates::NotEmpty*>(state)) {
    payload.tooltip = notEmptyState->tooltip;
  }

  if (dynamic_cast<InputStates::NumberInput*>(state) != nullptr ||
      dynamic_cast<InputStates::SelectingDictionary*>(state) != nullptr ||
      dynamic_cast<InputStates::ShowingCharInfo*>(state) != nullptr ||
      dynamic_cast<InputStates::SelectingFeature*>(state) != nullptr ||
      dynamic_cast<InputStates::SelectingDateMacro*>(state) != nullptr) {
    payload.forceVertical = true;
  }

  if (auto* inputting = dynamic_cast<InputStates::Inputting*>(state)) {
    payload.composingBuffer = inputting->composingBuffer;
    payload.cursorIndex = static_cast<int>(inputting->cursorIndex);
  } else if (auto* choosing =
                 dynamic_cast<InputStates::ChoosingCandidate*>(state)) {
    payload.composingBuffer = choosing->composingBuffer;
    payload.cursorIndex = static_cast<int>(choosing->cursorIndex);
    for (const auto& c : choosing->candidates) {
      payload.candidates.push_back(c.value);
      if (CodePointCount(c.value) > 8) {
        payload.forceVertical = true;
      }
    }
  } else if (auto* selDict =
                 dynamic_cast<InputStates::SelectingDictionary*>(state)) {
    payload.composingBuffer = selDict->composingBuffer;
    payload.cursorIndex = static_cast<int>(selDict->cursorIndex);
    for (const auto& m : selDict->menu) {
      payload.candidates.push_back(m);
    }
  } else if (auto* charInfo =
                 dynamic_cast<InputStates::ShowingCharInfo*>(state)) {
    payload.composingBuffer = charInfo->composingBuffer;
    payload.cursorIndex = static_cast<int>(charInfo->cursorIndex);
    payload.candidates = {
        "UTF8 String Length: " +
            std::to_string(charInfo->selectedPhrase.length()),
        "Code Point Count: " +
            std::to_string(CodePointCount(charInfo->selectedPhrase))};
  } else if (auto* marking = dynamic_cast<InputStates::Marking*>(state)) {
    payload.composingBuffer = marking->composingBuffer;
    payload.cursorIndex = static_cast<int>(marking->cursorIndex);
    payload.markStart = static_cast<int>(marking->head.length());
    payload.markEnd =
        static_cast<int>(marking->head.length() + marking->markedText.length());
  } else if (auto* assoc =
                 dynamic_cast<InputStates::AssociatedPhrases*>(state)) {
    payload.composingBuffer = assoc->composingBuffer;
    payload.cursorIndex = static_cast<int>(assoc->cursorIndex);
    payload.hint = assoc->prefixValue;
    payload.selectionStyle = assoc->autoTriggered
                                 ? IPC::CandidateSelectionStyle::kShiftReturn
                                 : IPC::CandidateSelectionStyle::kStandard;
    if (assoc->autoTriggered) {
      if (!assoc->candidates.empty()) {
        payload.candidates.push_back(assoc->candidates.front().value);
      }
    } else {
      for (const auto& c : assoc->candidates) {
        payload.candidates.push_back(c.value);
      }
    }
  } else if (auto* assocPlain =
                 dynamic_cast<InputStates::AssociatedPhrasesPlain*>(state)) {
    payload.selectionStyle = IPC::CandidateSelectionStyle::kShiftDigits;
    for (const auto& c : assocPlain->candidates) {
      payload.candidates.push_back(c.value);
    }
  } else if (auto* numInput = dynamic_cast<InputStates::NumberInput*>(state)) {
    payload.composingBuffer = numInput->composingBuffer;
    payload.cursorIndex = static_cast<int>(numInput->cursorIndex);
    for (const auto& c : numInput->candidates) {
      payload.candidates.push_back(c);
    }
  } else if (auto* big5 = dynamic_cast<InputStates::Big5*>(state)) {
    payload.composingBuffer = big5->composingBuffer();
    payload.cursorIndex = static_cast<int>(payload.composingBuffer.length());
  } else if (auto* irohaState = dynamic_cast<InputStates::Iroha*>(state)) {
    payload.composingBuffer = irohaState->composingBuffer();
    payload.cursorIndex = static_cast<int>(payload.composingBuffer.length());
  } else if (auto* selectingFeature =
                 dynamic_cast<InputStates::SelectingFeature*>(state)) {
    for (const auto& feature : selectingFeature->features) {
      payload.candidates.push_back(feature.name);
    }
  } else if (auto* selectingDateMacro =
                 dynamic_cast<InputStates::SelectingDateMacro*>(state)) {
    payload.candidates = selectingDateMacro->menu;
  } else if (auto* iroha = dynamic_cast<InputStates::IrohaCandidate*>(state)) {
    payload.composingBuffer = iroha->composingBuffer();
    payload.cursorIndex = static_cast<int>(payload.composingBuffer.length());
    payload.candidates = iroha->candidates;
  } else if (auto* customMenu = dynamic_cast<InputStates::CustomMenu*>(state)) {
    payload.composingBuffer = customMenu->composingBuffer;
    payload.cursorIndex = static_cast<int>(customMenu->cursorIndex);
    for (const auto& entry : customMenu->entries) {
      payload.candidates.push_back(entry.name);
    }
  }

  return payload;
}

InputController::InputController(
    std::shared_ptr<KeyHandler> keyHandler, UIInterface* ui,
    std::unique_ptr<LocalizedStrings> localizedStrings)
    : keyHandler_(std::move(keyHandler)),
      ui_(ui),
      currentState_(std::make_unique<InputStates::Empty>()),
      localizedStrings_(std::move(localizedStrings)) {}

void InputController::setDataDirectory(const std::filesystem::path& dataDir) {
  try {
    std::filesystem::path openccPath = dataDir / "opencc" / "tw2s.json";
    std::array<std::filesystem::path, 4> openccRequiredFiles = {
        dataDir / "opencc" / "TSPhrases.ocd2",
        dataDir / "opencc" / "TSCharacters.ocd2",
        dataDir / "opencc" / "TWVariantsRev.ocd2",
        dataDir / "opencc" / "TWVariantsRevPhrases.ocd2",
    };

    FCITX_MCBOPOMOFO_INFO()
        << "OpenCC config path: " << openccPath.string()
        << ", exists: " << std::filesystem::exists(openccPath);
    for (const auto& path : openccRequiredFiles) {
      FCITX_MCBOPOMOFO_INFO() << "OpenCC dictionary path: " << path.string()
                              << ", exists: " << std::filesystem::exists(path);
    }

    openccConverter_ =
        std::make_unique<opencc::SimpleConverter>(openccPath.string());
    FCITX_MCBOPOMOFO_INFO()
        << "OpenCC initialized successfully from " << openccPath.string();

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
      FCITX_MCBOPOMOFO_INFO()
          << "Chinese conversion is available; default state is disabled.";
    }
  } catch (const std::exception& e) {
    FCITX_MCBOPOMOFO_ERROR() << "Failed to initialize OpenCC: " << e.what();
    openccConverter_.reset();
  }
}

void InputController::toggleChineseConversion() {
  bool current = keyHandler_->chineseConversionEnabled();
  keyHandler_->setChineseConversionEnabled(!current);
  FCITX_MCBOPOMOFO_INFO() << "Chinese conversion toggled to: "
                          << keyHandler_->chineseConversionEnabled();
}

bool InputController::isChineseConversionEnabled() const {
  return keyHandler_->chineseConversionEnabled();
}

void InputController::setChineseConversionEnabled(bool enabled) {
  keyHandler_->setChineseConversionEnabled(enabled);
  FCITX_MCBOPOMOFO_INFO() << "Chinese conversion set to: "
                          << keyHandler_->chineseConversionEnabled();
}

void InputController::handleError_() const {
  if (beepOnError_) {
    MessageBeep(MB_ICONHAND);
  }
}

bool InputController::handleKey(const Key& key) {
  if (auto* numberInput =
          dynamic_cast<InputStates::NumberInput*>(currentState_.get())) {
    if (keyHandler_->handleNumberInput(
            key, numberInput,
            [this](std::unique_ptr<InputState> state) {
              enterNewState_(std::move(currentState_), std::move(state));
            },
            [this]() { handleError_(); })) {
      return true;
    }
  }

  if (IsCandidateState(currentState_.get())) {
    bool result = handleCandidateKey_(
        key,
        [this](std::unique_ptr<InputState> state) {
          this->enterNewState_(std::move(currentState_), std::move(state));
        },
        [this]() { handleError_(); });
    if (result) {
      return true;
    }
  }

  bool consumed = keyHandler_->handle(
      key, currentState_.get(),
      [this](std::unique_ptr<InputState> state) {
        this->enterNewState_(std::move(currentState_), std::move(state));
      },
      [this]() { handleError_(); });

  return consumed;
}

bool InputController::handleCandidateKey_(
    const Key& key, const McBopomofo::KeyHandler::StateCallback& stateCallback,
    const McBopomofo::KeyHandler::ErrorCallback& errorCallback) {
  auto* associated =
      dynamic_cast<InputStates::AssociatedPhrases*>(currentState_.get());
  auto* associatedPlain =
      dynamic_cast<InputStates::AssociatedPhrasesPlain*>(currentState_.get());
  auto* numberInput =
      dynamic_cast<InputStates::NumberInput*>(currentState_.get());
  auto* choosingPunctuation =
      dynamic_cast<InputStates::ChoosingPunctuationList*>(currentState_.get());
  auto* choosing =
      dynamic_cast<InputStates::ChoosingCandidate*>(currentState_.get());
  bool canHandleChoosingCandidate =
      choosing != nullptr && choosingPunctuation == nullptr;

  if (associated) {
    if (associated->autoTriggered) {
      if (key.ascii == Key::TAB) {
        auto expanded = std::make_unique<InputStates::AssociatedPhrases>(
            std::move(associated->previousState), associated->prefixCursorIndex,
            associated->prefixReading, associated->prefixValue,
            associated->selectedCandidateIndex, associated->candidates, false);
        stateCallback(std::move(expanded));
        return true;
      }

      if (key.ascii == Key::RETURN && key.shiftPressed) {
        selectCandidate_(candidateIndex_ >= 0 ? candidateIndex_ : 0,
                         stateCallback);
        return true;
      }
      stateCallback(keyHandler_->buildInputtingState());
      return false;
    }
  }

  int count = CandidateCount(currentState_.get());
  if (count == 0) {
    if (key.ascii == Key::ESC || key.ascii == Key::BACKSPACE) {
      stateCallback(std::make_unique<InputStates::EmptyIgnoringPrevious>());
      return true;
    }
    return true;
  }

  if (candidateIndex_ < 0 || candidateIndex_ >= count) {
    candidateIndex_ = 0;
  }

  bool useShiftKey = numberInput != nullptr || associatedPlain != nullptr;
  char ascii = static_cast<char>(std::tolower(
      static_cast<unsigned char>(static_cast<char>(key.ascii))));
  if (canHandleChoosingCandidate && !useShiftKey) {
    if (selectionAction_ == "JK") {
      if (ascii == 'j') {
        moveCandidateCursor_(false);
        notifyUI_();
        return true;
      }
      if (ascii == 'k') {
        moveCandidateCursor_(true);
        notifyUI_();
        return true;
      }
    } else if (selectionAction_ == "HL") {
      if (ascii == 'h') {
        moveCandidateCursor_(false);
        notifyUI_();
        return true;
      }
      if (ascii == 'l') {
        moveCandidateCursor_(true);
        notifyUI_();
        return true;
      }
    }
  }

  int selectionIndex = SelectionIndexFromKey(key, useShiftKey, candidateKeys_,
                                             candidateKeysCount_);
  if (selectionIndex != -1) {
    int actualIndex = useShiftKey ? selectionIndex
                                  : (candidateIndex_ / candidateKeysCount_) *
                                            candidateKeysCount_ +
                                        selectionIndex;
    if (actualIndex < count) {
      selectCandidate_(actualIndex, stateCallback);
    }
    return true;
  }

  bool shiftReturn = key.ascii == Key::RETURN && key.shiftPressed;
  if (keyHandler_->inputMode() == InputMode::McBopomofo && !useShiftKey &&
      shiftReturn && canHandleChoosingCandidate) {
    if (candidateIndex_ >= 0 &&
        candidateIndex_ < static_cast<int>(choosing->candidates.size())) {
      auto copy = std::make_unique<InputStates::ChoosingCandidate>(*choosing);
      const auto& candidate = choosing->candidates[candidateIndex_];
      auto associatedState =
          keyHandler_->buildAssociatedPhrasesStateFromCandidateChoosingState(
              std::move(copy), choosing->originalCursor, candidate.reading,
              candidate.value, static_cast<size_t>(candidateIndex_));
      if (associatedState != nullptr) {
        stateCallback(std::move(associatedState));
      }
    }
    return true;
  }

  bool returnPressed = key.ascii == Key::RETURN;
  if (returnPressed) {
    selectCandidate_(candidateIndex_, stateCallback);
    return true;
  }

  if (key.ascii == Key::ESC || key.ascii == Key::BACKSPACE) {
    if (auto* selecting = dynamic_cast<InputStates::SelectingDictionary*>(
            currentState_.get())) {
      stateCallback(std::move(selecting->previousState));
      return true;
    }

    if (auto* showingCharInfo =
            dynamic_cast<InputStates::ShowingCharInfo*>(currentState_.get())) {
      stateCallback(std::move(showingCharInfo->previousState));
      return true;
    }

    if (auto* customMenu =
            dynamic_cast<InputStates::CustomMenu*>(currentState_.get())) {
      if (auto* previousChoosing = dynamic_cast<InputStates::ChoosingCandidate*>(
              customMenu->previousState.get())) {
        stateCallback(
            std::make_unique<InputStates::ChoosingCandidate>(*previousChoosing));
      } else {
        stateCallback(std::make_unique<InputStates::EmptyIgnoringPrevious>());
      }
      return true;
    }

    if (associated != nullptr) {
      if (auto* previousChoosing = dynamic_cast<InputStates::ChoosingCandidate*>(
              associated->previousState.get())) {
        stateCallback(
            std::make_unique<InputStates::ChoosingCandidate>(*previousChoosing));
      } else if (auto* inputting = dynamic_cast<InputStates::Inputting*>(
                     associated->previousState.get())) {
        stateCallback(std::make_unique<InputStates::Inputting>(*inputting));
      } else {
        stateCallback(std::make_unique<InputStates::EmptyIgnoringPrevious>());
      }
      return true;
    }

    if (choosingPunctuation != nullptr) {
      keyHandler_->candidatePanelPunctuationListCancelled(
          choosingPunctuation->originalCursor, stateCallback);
      return true;
    }

    size_t originalCursor = 0;
    if (choosing != nullptr) {
      originalCursor = choosing->originalCursor;
    }
    keyHandler_->candidatePanelCancelled(originalCursor, stateCallback);
    return true;
  }

  if (choosingPunctuation != nullptr) {
    if (keyHandler_->candidatePanelPunctuationMaybeEntered(
            key, choosingPunctuation->originalCursor, stateCallback)) {
      return true;
    }
  }

  if (key.ascii == Key::SPACE) {
    moveCandidatePage_(true);
    notifyUI_();
    return true;
  }

  if (key.name == Key::KeyName::LEFT && key.shiftPressed &&
      canHandleChoosingCandidate) {
    size_t cursor = keyHandler_->candidateCursorIndex();
    if (cursor > 0) {
      --cursor;
    }
    keyHandler_->setCandidateCursorIndex(cursor);

    auto inputting = keyHandler_->buildInputtingState();
    auto newChoosing = keyHandler_->buildChoosingCandidateState(
        inputting.get(), keyHandler_->candidateCursorIndex());
    stateCallback(std::move(newChoosing));
    return true;
  }
  if (key.name == Key::KeyName::RIGHT && key.shiftPressed &&
      canHandleChoosingCandidate) {
    size_t cursor = keyHandler_->candidateCursorIndex();
    ++cursor;
    keyHandler_->setCandidateCursorIndex(cursor);

    auto inputting = keyHandler_->buildInputtingState();
    auto newChoosing = keyHandler_->buildChoosingCandidateState(
        inputting.get(), keyHandler_->candidateCursorIndex());
    stateCallback(std::move(newChoosing));
    return true;
  }

  if (handleCandidateNavigation_(key)) {
    return true;
  }

  if (keyHandler_->inputMode() == InputMode::McBopomofo && key.ascii == '?' &&
      canHandleChoosingCandidate) {
    if (candidateIndex_ >= 0 &&
        candidateIndex_ < static_cast<int>(choosing->candidates.size())) {
      auto* dictionaryServices = keyHandler_->getDictionaryServices();
      if (dictionaryServices != nullptr && dictionaryServices->hasServices()) {
        const auto& candidate = choosing->candidates[candidateIndex_];
        if (!HasInvalidDictionaryPrefix(candidate.reading)) {
          auto copy = std::make_unique<InputStates::ChoosingCandidate>(*choosing);
          auto newState = keyHandler_->buildSelectingDictionaryState(
              std::move(copy), candidate.value,
              static_cast<size_t>(candidateIndex_));
          stateCallback(std::move(newState));
        }
      }
    }
    return true;
  }

  if (keyHandler_->inputMode() == InputMode::McBopomofo &&
      canHandleChoosingCandidate &&
      (key.ascii == '+' || key.ascii == '=' || key.ascii == '-' ||
       key.ascii == '_')) {
    if (candidateIndex_ >= 0 &&
        candidateIndex_ < static_cast<int>(choosing->candidates.size())) {
      const auto candidate = choosing->candidates[candidateIndex_];
      if (!HasInvalidDictionaryPrefix(candidate.reading) &&
          candidate.reading.find('-') != std::string::npos &&
          candidate.value == candidate.rawValue) {
        std::vector<InputStates::CustomMenu::MenuEntry> entries;
        bool boost = key.ascii == '+' || key.ascii == '=';
        if (boost) {
          entries.emplace_back(
              localizedStrings_->boost(),
              [this, reading = candidate.reading, value = candidate.value,
               stateCallback]() {
                keyHandler_->boostPhrase(reading, value);
                stateCallback(keyHandler_->buildInputtingState());
              });
        } else {
          entries.emplace_back(
              localizedStrings_->exclude(),
              [this, reading = candidate.reading, value = candidate.value,
               stateCallback]() {
                keyHandler_->excludePhrase(reading, value);
                stateCallback(keyHandler_->buildInputtingState());
              });
        }
        entries.emplace_back(localizedStrings_->cancel(),
                             [this, stateCallback]() {
                               auto inputting = keyHandler_->buildInputtingState();
                               auto newChoosing =
                                   keyHandler_->buildChoosingCandidateState(
                                       inputting.get(),
                                       keyHandler_->candidateCursorIndex());
                               stateCallback(std::move(newChoosing));
                             });

        auto copy = std::make_unique<InputStates::ChoosingCandidate>(*choosing);
        auto menu = std::make_unique<InputStates::CustomMenu>(
            std::move(copy),
            boost ? localizedStrings_->boostPrompt()
                  : localizedStrings_->excludePrompt(),
            std::move(entries));
        stateCallback(std::move(menu));
      }
    }
    return true;
  }

  if (associated != nullptr && !key.shiftPressed) {
    return false;
  }

  if (associatedPlain != nullptr && !key.shiftPressed) {
    stateCallback(std::make_unique<InputStates::Empty>());
    return false;
  }

  if (canHandleChoosingCandidate) {
    bool handled =
        keyHandler_->handleCandidateKeyForTraditionalBopomofoIfRequired(
            key,
            [this, stateCallback]() {
              selectCandidate_(candidateIndex_, stateCallback);
            },
            stateCallback, errorCallback);
    if (handled) {
      return true;
    }
  }

  return true;
}

bool InputController::handleCandidateNavigation_(const Key& key) {
  bool isVertical = candidateWindowVertical_ ||
                    IsForcedVerticalCandidateState(currentState_.get());
  if (key.name == Key::KeyName::HOME) {
    candidateIndex_ = 0;
  } else if (key.name == Key::KeyName::END) {
    candidateIndex_ = std::max(0, CandidateCount(currentState_.get()) - 1);
  } else if (key.name == Key::KeyName::PAGE_UP) {
    moveCandidatePage_(false);
  } else if (key.name == Key::KeyName::PAGE_DOWN) {
    moveCandidatePage_(true);
  } else if (isVertical && key.name == Key::KeyName::UP) {
    moveCandidateCursor_(false);
  } else if (isVertical && key.name == Key::KeyName::DOWN) {
    moveCandidateCursor_(true);
  } else if (isVertical && key.name == Key::KeyName::LEFT) {
    moveCandidatePage_(false);
  } else if (isVertical && key.name == Key::KeyName::RIGHT) {
    moveCandidatePage_(true);
  } else if (!isVertical && key.name == Key::KeyName::LEFT) {
    moveCandidateCursor_(false);
  } else if (!isVertical && key.name == Key::KeyName::RIGHT) {
    moveCandidateCursor_(true);
  } else if (!isVertical && key.name == Key::KeyName::UP) {
    moveCandidatePage_(false);
  } else if (!isVertical && key.name == Key::KeyName::DOWN) {
    moveCandidatePage_(true);
  } else {
    return false;
  }

  notifyUI_();
  return true;
}

void InputController::moveCandidateCursor_(bool forward) {
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

void InputController::moveCandidatePage_(bool forward) {
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

void InputController::reset() {
  candidateIndex_ = -1;
  keyHandler_->reset();
  auto empty = std::make_unique<InputStates::Empty>();
  this->enterNewState_(std::move(currentState_), std::move(empty));
}

void InputController::selectCandidate(int index) {
  selectCandidate_(index, [this](std::unique_ptr<InputState> state) {
    enterNewState_(std::move(currentState_), std::move(state));
  });
}

void InputController::selectCandidate_(
    int index, const McBopomofo::KeyHandler::StateCallback& stateCallback) {
  if (auto* choosing = dynamic_cast<InputStates::ChoosingPunctuationList*>(
          currentState_.get())) {
    if (index >= 0 && index < static_cast<int>(choosing->candidates.size())) {
      candidateIndex_ = -1;
      keyHandler_->candidateSelected(choosing->candidates[index],
                                     choosing->originalCursor, stateCallback);
    }
    return;
  }

  if (auto* choosing =
          dynamic_cast<InputStates::ChoosingCandidate*>(currentState_.get())) {
    if (index >= 0 && index < static_cast<int>(choosing->candidates.size())) {
      candidateIndex_ = -1;
      keyHandler_->candidateSelected(choosing->candidates[index],
                                     choosing->originalCursor, stateCallback);
    }
    return;
  }

  if (auto* associated =
          dynamic_cast<InputStates::AssociatedPhrases*>(currentState_.get())) {
    if (index >= 0 && index < static_cast<int>(associated->candidates.size())) {
      candidateIndex_ = -1;
      keyHandler_->candidateAssociatedPhraseSelected(
          associated->prefixCursorIndex, associated->candidates[index],
          associated->prefixReading, associated->prefixValue, stateCallback);
    }
    return;
  }

  if (auto* associatedPlain =
          dynamic_cast<InputStates::AssociatedPhrasesPlain*>(
              currentState_.get())) {
    if (index >= 0 &&
        index < static_cast<int>(associatedPlain->candidates.size())) {
      candidateIndex_ = -1;
      keyHandler_->candidateSelected(associatedPlain->candidates[index], 0,
                                     stateCallback);
    }
    return;
  }

  if (auto* selecting = dynamic_cast<InputStates::SelectingDictionary*>(
          currentState_.get())) {
    if (index >= 0 && index < static_cast<int>(selecting->menu.size())) {
      auto* dictionaryServices = keyHandler_->getDictionaryServices();
      if (dictionaryServices != nullptr) {
        dictionaryServices->lookup(selecting->selectedPhrase, index, selecting,
                                   stateCallback);
      }
      stateCallback(std::move(selecting->previousState));
    }
    return;
  }

  if (auto* numberInput =
          dynamic_cast<InputStates::NumberInput*>(currentState_.get())) {
    if (index >= 0 &&
        index < static_cast<int>(numberInput->candidates.size())) {
      std::string text = numberInput->candidates[index];
      stateCallback(std::make_unique<InputStates::Committing>(text));
    }
    return;
  }

  if (auto* selectingFeature =
          dynamic_cast<InputStates::SelectingFeature*>(currentState_.get())) {
    if (index >= 0 &&
        index < static_cast<int>(selectingFeature->features.size())) {
      stateCallback(selectingFeature->nextState(static_cast<size_t>(index)));
    }
    return;
  }

  if (auto* selectingDateMacro =
          dynamic_cast<InputStates::SelectingDateMacro*>(currentState_.get())) {
    if (index >= 0 &&
        index < static_cast<int>(selectingDateMacro->menu.size())) {
      std::string text = selectingDateMacro->menu[index];
      stateCallback(std::make_unique<InputStates::Committing>(text));
    }
    return;
  }

  if (auto* iroha =
          dynamic_cast<InputStates::IrohaCandidate*>(currentState_.get())) {
    if (index >= 0 && index < static_cast<int>(iroha->candidates.size())) {
      std::string text = iroha->candidates[index];
      auto seq = std::make_unique<InputStates::StateSequence>();
      seq->push_back(std::make_unique<InputStates::Committing>(text));
      seq->push_back(std::make_unique<InputStates::Iroha>(""));
      stateCallback(std::move(seq));
    }
    return;
  }

  if (auto* customMenu =
          dynamic_cast<InputStates::CustomMenu*>(currentState_.get())) {
    if (index >= 0 && index < static_cast<int>(customMenu->entries.size()) &&
        customMenu->entries[index].callback) {
      customMenu->entries[index].callback();
    }
    return;
  }
}

void InputController::setInputMode(InputMode mode) {
  keyHandler_->setInputMode(mode);
}

void InputController::setKeyboardLayout(
    const Formosa::Mandarin::BopomofoKeyboardLayout* layout) {
  keyHandler_->setKeyboardLayout(layout);
}

void InputController::setSelectPhraseAfterCursorAsCandidate(bool flag) {
  keyHandler_->setSelectPhraseAfterCursorAsCandidate(flag);
}

void InputController::setMoveCursorAfterSelection(bool flag) {
  keyHandler_->setMoveCursorAfterSelection(flag);
}

void InputController::setPutLowercaseLettersToComposingBuffer(bool flag) {
  keyHandler_->setPutLowercaseLettersToComposingBuffer(flag);
}

void InputController::setEscKeyClearsEntireComposingBuffer(bool flag) {
  keyHandler_->setEscKeyClearsEntireComposingBuffer(flag);
}

void InputController::setShiftEnterEnabled(bool flag) {
  keyHandler_->setShiftEnterEnabled(flag);
}

void InputController::setCtrlEnterKeyBehavior(KeyHandlerCtrlEnter behavior) {
  keyHandler_->setCtrlEnterKeyBehavior(behavior);
}

void InputController::setAssociatedPhrasesEnabled(bool enabled) {
  keyHandler_->setAssociatedPhrasesEnabled(enabled);
}

void InputController::setHalfWidthPunctuationEnabled(bool enabled) {
  keyHandler_->setHalfWidthPunctuationEnabled(enabled);
}

void InputController::setBopomofoFontAnnotationSupportEnabled(bool enabled) {
  keyHandler_->setBopomofoFontAnnotationSupportEnabled(enabled);
}

void InputController::setRepeatedPunctuationToSelectCandidateEnabled(
    bool enabled) {
  keyHandler_->setRepeatedPunctuationToSelectCandidateEnabled(enabled);
}

void InputController::setChooseCandidateUsingSpace(bool enabled) {
  keyHandler_->setChooseCandidateUsingSpace(enabled);
}

void InputController::setCandidateKeys(const std::string& keys) {
  if (keys == "123456789" || keys == "asdfghjkl" || keys == "asdfzxcvb") {
    candidateKeys_ = keys;
  } else {
    candidateKeys_ = "123456789";
  }
}

void InputController::setCandidateKeysCount(int count) {
  if (count >= 4 && count <= 9) {
    candidateKeysCount_ = count;
  } else {
    candidateKeysCount_ = 9;
  }
}

void InputController::setCandidateWindowVertical(bool vertical) {
  candidateWindowVertical_ = vertical;
}

void InputController::setSelectionAction(const std::string& action) {
  if (action == "None" || action == "JK" || action == "HL") {
    selectionAction_ = action;
  } else {
    selectionAction_ = "None";
  }
}

void InputController::setCandidateWindowColors(
    const IPC::CandidateWindowColors& candidateWindowColors) {
  candidateWindowColors_ = candidateWindowColors;
}

void InputController::refreshUI() { notifyUI_(); }

void InputController::enterNewState_(std::unique_ptr<InputState> previousState,
                                     std::unique_ptr<InputState> newState) {
  if (auto* sequence =
          dynamic_cast<InputStates::StateSequence*>(newState.get())) {
    for (size_t i = 0; i < sequence->states.size(); ++i) {
      auto& s = sequence->states[i];
      enterNewState_(std::move(previousState), std::move(s));
      if (i + 1 < sequence->states.size()) {
        previousState = std::move(currentState_);
      }
    }
    return;
  }

  std::string commitText;
  if (auto* commit = dynamic_cast<InputStates::Committing*>(newState.get())) {
    commitText = commit->text;
    if (ui_) ui_->commitString(commitText);
    newState = std::make_unique<InputStates::Empty>();
    currentState_ = std::move(newState);
    notifyUI_();
    return;
  }

  if (dynamic_cast<InputStates::Empty*>(newState.get()) != nullptr) {
    if (ui_) ui_->reset();
    if (auto* inputting =
            dynamic_cast<InputStates::NotEmpty*>(previousState.get())) {
      std::string text = inputting->composingBuffer;
      if (!text.empty() && ui_) {
        ui_->commitString(text);
      }
    }
    currentState_ = std::move(newState);
    candidateIndex_ = -1;
    notifyUI_();
    return;
  }

  if (dynamic_cast<InputStates::EmptyIgnoringPrevious*>(newState.get()) !=
      nullptr) {
    if (ui_) ui_->reset();
    currentState_ = std::make_unique<InputStates::Empty>();
    candidateIndex_ = -1;
    notifyUI_();
    return;
  }

  if (dynamic_cast<InputStates::SelectingFeature*>(newState.get()) != nullptr &&
      dynamic_cast<InputStates::SelectingFeature*>(previousState.get()) ==
          nullptr) {
    candidateIndex_ = 0;
  }

  if (dynamic_cast<InputStates::SelectingDateMacro*>(newState.get()) !=
          nullptr &&
      dynamic_cast<InputStates::SelectingDateMacro*>(previousState.get()) ==
          nullptr) {
    candidateIndex_ = 0;
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
  notifyUI_();
}

}  // namespace McBopomofo
