#pragma once

#include <string>
#include "InputController.h"

namespace McBopomofo {

class Settings {
public:
    Settings();
    ~Settings() = default;

    // Load settings from the INI file
    void Load();

    // Save settings to the INI file
    void Save();

    // Apply the currently loaded settings to the InputController
    void ApplyTo(InputController& controller);

    // Getters and Setters for Settings
    InputMode GetInputMode() const { return inputMode_; }
    void SetInputMode(InputMode mode) { inputMode_ = mode; }

    std::string GetKeyboardLayout() const { return keyboardLayout_; }
    void SetKeyboardLayout(const std::string& layout) { keyboardLayout_ = layout; }

    bool GetSelectPhraseAfterCursorAsCandidate() const { return selectPhraseAfterCursorAsCandidate_; }
    void SetSelectPhraseAfterCursorAsCandidate(bool v) { selectPhraseAfterCursorAsCandidate_ = v; }

    bool GetMoveCursorAfterSelection() const { return moveCursorAfterSelection_; }
    void SetMoveCursorAfterSelection(bool v) { moveCursorAfterSelection_ = v; }

    bool GetPutLowercaseLettersToComposingBuffer() const { return putLowercaseLettersToComposingBuffer_; }
    void SetPutLowercaseLettersToComposingBuffer(bool v) { putLowercaseLettersToComposingBuffer_ = v; }

    bool GetEscKeyClearsEntireComposingBuffer() const { return escKeyClearsEntireComposingBuffer_; }
    void SetEscKeyClearsEntireComposingBuffer(bool v) { escKeyClearsEntireComposingBuffer_ = v; }

    bool GetShiftEnterEnabled() const { return shiftEnterEnabled_; }
    void SetShiftEnterEnabled(bool v) { shiftEnterEnabled_ = v; }

    KeyHandlerCtrlEnter GetCtrlEnterKeyBehavior() const { return ctrlEnterKeyBehavior_; }
    void SetCtrlEnterKeyBehavior(KeyHandlerCtrlEnter v) { ctrlEnterKeyBehavior_ = v; }

    bool GetAssociatedPhrasesEnabled() const { return associatedPhrasesEnabled_; }
    void SetAssociatedPhrasesEnabled(bool v) { associatedPhrasesEnabled_ = v; }

    bool GetHalfWidthPunctuationEnabled() const { return halfWidthPunctuationEnabled_; }
    void SetHalfWidthPunctuationEnabled(bool v) { halfWidthPunctuationEnabled_ = v; }

    bool GetBopomofoFontAnnotationSupportEnabled() const { return bopomofoFontAnnotationSupportEnabled_; }
    void SetBopomofoFontAnnotationSupportEnabled(bool v) { bopomofoFontAnnotationSupportEnabled_ = v; }

    bool GetRepeatedPunctuationToSelectCandidateEnabled() const { return repeatedPunctuationToSelectCandidateEnabled_; }
    void SetRepeatedPunctuationToSelectCandidateEnabled(bool v) { repeatedPunctuationToSelectCandidateEnabled_ = v; }

    bool GetChooseCandidateUsingSpace() const { return chooseCandidateUsingSpace_; }
    void SetChooseCandidateUsingSpace(bool v) { chooseCandidateUsingSpace_ = v; }

    std::string GetCandidateKeys() const { return candidateKeys_; }
    void SetCandidateKeys(const std::string& v) { candidateKeys_ = v; }

    int GetCandidateKeysCount() const { return candidateKeysCount_; }
    void SetCandidateKeysCount(int v) { candidateKeysCount_ = v; }

    bool GetCandidateWindowVertical() const { return candidateWindowVertical_; }
    void SetCandidateWindowVertical(bool v) { candidateWindowVertical_ = v; }

private:
    std::wstring GetIniFilePath() const;
    std::wstring ReadString(const wchar_t* section, const wchar_t* key, const wchar_t* defaultVal);
    void WriteString(const wchar_t* section, const wchar_t* key, const std::wstring& val);
    
    int ReadInt(const wchar_t* section, const wchar_t* key, int defaultVal);
    void WriteInt(const wchar_t* section, const wchar_t* key, int val);

    bool ReadBool(const wchar_t* section, const wchar_t* key, bool defaultVal);
    void WriteBool(const wchar_t* section, const wchar_t* key, bool val);

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
    bool bopomofoFontAnnotationSupportEnabled_ = false;
    bool repeatedPunctuationToSelectCandidateEnabled_ = false;
    bool chooseCandidateUsingSpace_ = true;
    std::string candidateKeys_ = "123456789";
    int candidateKeysCount_ = 9;
    bool candidateWindowVertical_ = false;
};

} // namespace McBopomofo
