#include "Settings.h"
#include "PathCompat.h"
#include "UTFHelper.h"
#include "Log.h"
#include <windows.h>
#include <filesystem>

namespace McBopomofo {

Settings::Settings() {
    Load();
}

std::wstring Settings::GetIniFilePath() const {
    std::string dir = fcitx5_compat::userDirectory();
    std::filesystem::path p(dir);
    p /= "mcbopomofo.ini";
    return p.wstring();
}

std::wstring Settings::ReadString(const wchar_t* section, const wchar_t* key, const wchar_t* defaultVal) {
    std::wstring path = GetIniFilePath();
    wchar_t buffer[256];
    GetPrivateProfileStringW(section, key, defaultVal, buffer, 256, path.c_str());
    return std::wstring(buffer);
}

void Settings::WriteString(const wchar_t* section, const wchar_t* key, const std::wstring& val) {
    std::wstring path = GetIniFilePath();
    WritePrivateProfileStringW(section, key, val.c_str(), path.c_str());
}

int Settings::ReadInt(const wchar_t* section, const wchar_t* key, int defaultVal) {
    std::wstring path = GetIniFilePath();
    return GetPrivateProfileIntW(section, key, defaultVal, path.c_str());
}

void Settings::WriteInt(const wchar_t* section, const wchar_t* key, int val) {
    WriteString(section, key, std::to_wstring(val));
}

bool Settings::ReadBool(const wchar_t* section, const wchar_t* key, bool defaultVal) {
    return ReadInt(section, key, defaultVal ? 1 : 0) != 0;
}

void Settings::WriteBool(const wchar_t* section, const wchar_t* key, bool val) {
    WriteInt(section, key, val ? 1 : 0);
}

void Settings::Load() {
    inputMode_ = (InputMode)ReadInt(L"General", L"InputMode", (int)InputMode::McBopomofo);
    
    std::wstring layoutW = ReadString(L"General", L"KeyboardLayout", L"Standard");
    keyboardLayout_ = Utf16ToUtf8(layoutW);

    selectPhraseAfterCursorAsCandidate_ = ReadBool(L"General", L"SelectPhraseAfterCursorAsCandidate", false);
    moveCursorAfterSelection_ = ReadBool(L"General", L"MoveCursorAfterSelection", false);
    putLowercaseLettersToComposingBuffer_ = ReadBool(L"General", L"PutLowercaseLettersToComposingBuffer", false);
    escKeyClearsEntireComposingBuffer_ = ReadBool(L"General", L"EscKeyClearsEntireComposingBuffer", false);
    shiftEnterEnabled_ = ReadBool(L"General", L"ShiftEnterEnabled", true);
    ctrlEnterKeyBehavior_ = (KeyHandlerCtrlEnter)ReadInt(L"General", L"CtrlEnterKeyBehavior", (int)KeyHandlerCtrlEnter::Disabled);
    associatedPhrasesEnabled_ = ReadBool(L"General", L"AssociatedPhrasesEnabled", false);
    halfWidthPunctuationEnabled_ = ReadBool(L"General", L"HalfWidthPunctuationEnabled", false);
    chineseConversionEnabled_ = ReadBool(L"General", L"ChineseConversionEnabled", false);
    bopomofoFontAnnotationSupportEnabled_ = ReadBool(L"General", L"BopomofoFontAnnotationSupportEnabled", false);
    repeatedPunctuationToSelectCandidateEnabled_ = ReadBool(L"General", L"RepeatedPunctuationToSelectCandidateEnabled", false);
    chooseCandidateUsingSpace_ = ReadBool(L"General", L"ChooseCandidateUsingSpace", true);
    candidateKeys_ = Utf16ToUtf8(ReadString(L"General", L"CandidateKeys", L"123456789"));
    if (candidateKeys_ != "123456789" && candidateKeys_ != "asdfghjkl" && candidateKeys_ != "asdfzxcvb") {
        candidateKeys_ = "123456789";
    }
    candidateKeysCount_ = ReadInt(L"General", L"CandidateKeysCount", 9);
    if (candidateKeysCount_ < 4 || candidateKeysCount_ > 9) {
        candidateKeysCount_ = 9;
    }
    candidateWindowVertical_ = ReadBool(L"UI", L"CandidateWindowVertical", false);
}

void Settings::Save() {
    WriteInt(L"General", L"InputMode", (int)inputMode_);
    WriteString(L"General", L"KeyboardLayout", Utf8ToUtf16(keyboardLayout_));
    WriteBool(L"General", L"SelectPhraseAfterCursorAsCandidate", selectPhraseAfterCursorAsCandidate_);
    WriteBool(L"General", L"MoveCursorAfterSelection", moveCursorAfterSelection_);
    WriteBool(L"General", L"PutLowercaseLettersToComposingBuffer", putLowercaseLettersToComposingBuffer_);
    WriteBool(L"General", L"EscKeyClearsEntireComposingBuffer", escKeyClearsEntireComposingBuffer_);
    WriteBool(L"General", L"ShiftEnterEnabled", shiftEnterEnabled_);
    WriteInt(L"General", L"CtrlEnterKeyBehavior", (int)ctrlEnterKeyBehavior_);
    WriteBool(L"General", L"AssociatedPhrasesEnabled", associatedPhrasesEnabled_);
    WriteBool(L"General", L"HalfWidthPunctuationEnabled", halfWidthPunctuationEnabled_);
    WriteBool(L"General", L"ChineseConversionEnabled", chineseConversionEnabled_);
    WriteBool(L"General", L"BopomofoFontAnnotationSupportEnabled", bopomofoFontAnnotationSupportEnabled_);
    WriteBool(L"General", L"RepeatedPunctuationToSelectCandidateEnabled", repeatedPunctuationToSelectCandidateEnabled_);
    WriteBool(L"General", L"ChooseCandidateUsingSpace", chooseCandidateUsingSpace_);
    WriteString(L"General", L"CandidateKeys", Utf8ToUtf16(candidateKeys_));
    WriteInt(L"General", L"CandidateKeysCount", candidateKeysCount_);
    WriteBool(L"UI", L"CandidateWindowVertical", candidateWindowVertical_);
}

void Settings::ApplyTo(InputController& controller) {
    controller.SetInputMode(inputMode_);
    
    const Formosa::Mandarin::BopomofoKeyboardLayout* layout = Formosa::Mandarin::BopomofoKeyboardLayout::StandardLayout();
    if (keyboardLayout_ == "ETen") layout = Formosa::Mandarin::BopomofoKeyboardLayout::ETenLayout();
    else if (keyboardLayout_ == "Hsu") layout = Formosa::Mandarin::BopomofoKeyboardLayout::HsuLayout();
    else if (keyboardLayout_ == "ETen26") layout = Formosa::Mandarin::BopomofoKeyboardLayout::ETen26Layout();
    else if (keyboardLayout_ == "HanyuPinyin") layout = Formosa::Mandarin::BopomofoKeyboardLayout::HanyuPinyinLayout();
    else if (keyboardLayout_ == "IBM") layout = Formosa::Mandarin::BopomofoKeyboardLayout::IBMLayout();
    
    controller.SetKeyboardLayout(layout);
    
    controller.SetSelectPhraseAfterCursorAsCandidate(selectPhraseAfterCursorAsCandidate_);
    controller.SetMoveCursorAfterSelection(moveCursorAfterSelection_);
    controller.SetPutLowercaseLettersToComposingBuffer(putLowercaseLettersToComposingBuffer_);
    controller.SetEscKeyClearsEntireComposingBuffer(escKeyClearsEntireComposingBuffer_);
    controller.SetShiftEnterEnabled(shiftEnterEnabled_);
    controller.SetCtrlEnterKeyBehavior(ctrlEnterKeyBehavior_);
    controller.SetAssociatedPhrasesEnabled(associatedPhrasesEnabled_);
    controller.SetHalfWidthPunctuationEnabled(halfWidthPunctuationEnabled_);
    controller.SetChineseConversionEnabled(chineseConversionEnabled_);
    controller.SetBopomofoFontAnnotationSupportEnabled(bopomofoFontAnnotationSupportEnabled_);
    controller.SetRepeatedPunctuationToSelectCandidateEnabled(repeatedPunctuationToSelectCandidateEnabled_);
    controller.SetChooseCandidateUsingSpace(chooseCandidateUsingSpace_);
    controller.SetCandidateKeys(candidateKeys_);
    controller.SetCandidateKeysCount(candidateKeysCount_);
    controller.SetCandidateWindowVertical(candidateWindowVertical_);
    FCITX_MCBOPOMOFO_INFO() << "Settings applied: ChineseConversionEnabled="
                            << chineseConversionEnabled_;
}

} // namespace McBopomofo
