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

#include "Settings.h"

#include <windows.h>
#include <winrt/Windows.UI.ViewManagement.h>
#include <winrt/base.h>

#include <algorithm>
#include <filesystem>

#include "Log.h"
#include "PathCompat.h"
#include "UTFHelper.h"

namespace McBopomofo {

Settings::Settings() { load(); }

namespace {

uint32_t ToRgb(const winrt::Windows::UI::Color& color) {
  return (static_cast<uint32_t>(color.R) << 16) |
         (static_cast<uint32_t>(color.G) << 8) |
         static_cast<uint32_t>(color.B);
}

bool EnsureWinrtApartmentInitialized() {
  static thread_local bool initialized = false;
  if (initialized) {
    return true;
  }

  try {
    winrt::init_apartment(winrt::apartment_type::multi_threaded);
    initialized = true;
    return true;
  } catch (const winrt::hresult_error& e) {
    if (e.code() == RPC_E_CHANGED_MODE) {
      initialized = true;
      return true;
    }
    return false;
  }
}

}  // namespace

IPC::CandidateWindowColors Settings::readCandidateWindowColors_() const {
  DWORD useLightTheme = 1;
  DWORD size = sizeof(useLightTheme);
  HKEY hKey = nullptr;
  if (RegOpenKeyExW(
          HKEY_CURRENT_USER,
          L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
          0, KEY_READ, &hKey) == ERROR_SUCCESS) {
    RegQueryValueExW(hKey, L"AppsUseLightTheme", nullptr, nullptr,
                     reinterpret_cast<LPBYTE>(&useLightTheme), &size);
    RegCloseKey(hKey);
  }

  IPC::CandidateWindowColors colors;
  if (useLightTheme == 0) {
    colors.text = 0xF0F0F0;
    colors.background = 0x202020;
    colors.border = 0x404040;
  }

  if (EnsureWinrtApartmentInitialized()) {
    try {
      using namespace winrt::Windows::UI::ViewManagement;
      UISettings uiSettings;
      colors.highlightBackground =
          ToRgb(uiSettings.GetColorValue(UIColorType::Accent));
    } catch (const winrt::hresult_error& e) {
      FCITX_MCBOPOMOFO_WARN()
          << "Failed to read system accent color from UISettings: "
          << Utf16ToUtf8(std::wstring(e.message().c_str()));
    }
  }

  return colors;
}

std::wstring Settings::iniFilePath_() const {
  std::string dir = fcitx5_compat::userDirectory();
  std::filesystem::path p(dir);
  p /= "mcbopomofo.ini";
  return p.wstring();
}

std::wstring Settings::readString_(const wchar_t* section, const wchar_t* key,
                                   const wchar_t* defaultVal) {
  std::wstring path = iniFilePath_();
  wchar_t buffer[256];
  GetPrivateProfileStringW(section, key, defaultVal, buffer, 256, path.c_str());
  return std::wstring(buffer);
}

void Settings::writeString_(const wchar_t* section, const wchar_t* key,
                            const std::wstring& val) {
  std::wstring path = iniFilePath_();
  WritePrivateProfileStringW(section, key, val.c_str(), path.c_str());
}

int Settings::readInt_(const wchar_t* section, const wchar_t* key,
                       int defaultVal) {
  std::wstring path = iniFilePath_();
  return GetPrivateProfileIntW(section, key, defaultVal, path.c_str());
}

void Settings::writeInt_(const wchar_t* section, const wchar_t* key, int val) {
  writeString_(section, key, std::to_wstring(val));
}

bool Settings::readBool_(const wchar_t* section, const wchar_t* key,
                         bool defaultVal) {
  return readInt_(section, key, defaultVal ? 1 : 0) != 0;
}

void Settings::writeBool_(const wchar_t* section, const wchar_t* key,
                          bool val) {
  writeInt_(section, key, val ? 1 : 0);
}

void Settings::load() {
  inputMode_ =
      (InputMode)readInt_(L"General", L"InputMode", (int)InputMode::McBopomofo);

  std::wstring layoutW =
      readString_(L"General", L"KeyboardLayout", L"Standard");
  keyboardLayout_ = Utf16ToUtf8(layoutW);

  selectPhraseAfterCursorAsCandidate_ =
      readBool_(L"General", L"SelectPhraseAfterCursorAsCandidate", false);
  moveCursorAfterSelection_ =
      readBool_(L"General", L"MoveCursorAfterSelection", false);
  putLowercaseLettersToComposingBuffer_ =
      readBool_(L"General", L"PutLowercaseLettersToComposingBuffer", false);
  escKeyClearsEntireComposingBuffer_ =
      readBool_(L"General", L"EscKeyClearsEntireComposingBuffer", false);
  shiftEnterEnabled_ = readBool_(L"General", L"ShiftEnterEnabled", true);
  ctrlEnterKeyBehavior_ = (KeyHandlerCtrlEnter)readInt_(
      L"General", L"CtrlEnterKeyBehavior", (int)KeyHandlerCtrlEnter::Disabled);
  associatedPhrasesEnabled_ =
      readBool_(L"General", L"AssociatedPhrasesEnabled", false);
  halfWidthPunctuationEnabled_ =
      readBool_(L"General", L"HalfWidthPunctuationEnabled", false);
  chineseConversionEnabled_ =
      readBool_(L"General", L"ChineseConversionEnabled", false);
  bopomofoFontAnnotationSupportEnabled_ =
      readBool_(L"General", L"BopomofoFontAnnotationSupportEnabled", false);
  repeatedPunctuationToSelectCandidateEnabled_ = readBool_(
      L"General", L"RepeatedPunctuationToSelectCandidateEnabled", false);
  chooseCandidateUsingSpace_ =
      readBool_(L"General", L"ChooseCandidateUsingSpace", true);
  candidateKeys_ =
      Utf16ToUtf8(readString_(L"General", L"CandidateKeys", L"123456789"));
  if (candidateKeys_ != "123456789" && candidateKeys_ != "asdfghjkl" &&
      candidateKeys_ != "asdfzxcvb") {
    candidateKeys_ = "123456789";
  }
  candidateKeysCount_ = readInt_(L"General", L"CandidateKeysCount", 9);
  if (candidateKeysCount_ < 4 || candidateKeysCount_ > 9) {
    candidateKeysCount_ = 9;
  }
  candidateWindowVertical_ =
      readBool_(L"UI", L"CandidateWindowVertical", false);
  selectionAction_ =
      Utf16ToUtf8(readString_(L"UI", L"SelectionAction", L"None"));
  if (selectionAction_ != "None" && selectionAction_ != "JK" &&
      selectionAction_ != "HL") {
    selectionAction_ = "None";
  }
  candidateFontSize_ = readInt_(L"UI", L"CandidateFontSize", 16);
  const int kFontSizes[] = {10, 12, 14, 16, 18, 20, 24, 28};
  if (std::find(std::begin(kFontSizes), std::end(kFontSizes),
                candidateFontSize_) == std::end(kFontSizes)) {
    candidateFontSize_ = 16;
  }
  candidateWindowColors_ = readCandidateWindowColors_();
  shiftToggleOpenClose_ =
      readBool_(L"General", L"ShiftToggleOpenClose", true);
  beepOnError_ = readBool_(L"General", L"BeepOnError", true);
}

void Settings::save() {
  writeInt_(L"General", L"InputMode", (int)inputMode_);
  writeString_(L"General", L"KeyboardLayout", Utf8ToUtf16(keyboardLayout_));
  writeBool_(L"General", L"SelectPhraseAfterCursorAsCandidate",
             selectPhraseAfterCursorAsCandidate_);
  writeBool_(L"General", L"MoveCursorAfterSelection",
             moveCursorAfterSelection_);
  writeBool_(L"General", L"PutLowercaseLettersToComposingBuffer",
             putLowercaseLettersToComposingBuffer_);
  writeBool_(L"General", L"EscKeyClearsEntireComposingBuffer",
             escKeyClearsEntireComposingBuffer_);
  writeBool_(L"General", L"ShiftEnterEnabled", shiftEnterEnabled_);
  writeInt_(L"General", L"CtrlEnterKeyBehavior", (int)ctrlEnterKeyBehavior_);
  writeBool_(L"General", L"AssociatedPhrasesEnabled",
             associatedPhrasesEnabled_);
  writeBool_(L"General", L"HalfWidthPunctuationEnabled",
             halfWidthPunctuationEnabled_);
  writeBool_(L"General", L"ChineseConversionEnabled",
             chineseConversionEnabled_);
  writeBool_(L"General", L"BopomofoFontAnnotationSupportEnabled",
             bopomofoFontAnnotationSupportEnabled_);
  writeBool_(L"General", L"RepeatedPunctuationToSelectCandidateEnabled",
             repeatedPunctuationToSelectCandidateEnabled_);
  writeBool_(L"General", L"ChooseCandidateUsingSpace",
             chooseCandidateUsingSpace_);
  writeString_(L"General", L"CandidateKeys", Utf8ToUtf16(candidateKeys_));
  writeInt_(L"General", L"CandidateKeysCount", candidateKeysCount_);
  writeBool_(L"UI", L"CandidateWindowVertical", candidateWindowVertical_);
  writeString_(L"UI", L"SelectionAction", Utf8ToUtf16(selectionAction_));
  writeInt_(L"UI", L"CandidateFontSize", candidateFontSize_);
  writeBool_(L"General", L"ShiftToggleOpenClose", shiftToggleOpenClose_);
  writeBool_(L"General", L"BeepOnError", beepOnError_);
}

void Settings::applyTo(InputController& controller) {
  controller.setInputMode(inputMode_);

  const Formosa::Mandarin::BopomofoKeyboardLayout* layout =
      Formosa::Mandarin::BopomofoKeyboardLayout::StandardLayout();
  if (keyboardLayout_ == "ETen")
    layout = Formosa::Mandarin::BopomofoKeyboardLayout::ETenLayout();
  else if (keyboardLayout_ == "Hsu")
    layout = Formosa::Mandarin::BopomofoKeyboardLayout::HsuLayout();
  else if (keyboardLayout_ == "ETen26")
    layout = Formosa::Mandarin::BopomofoKeyboardLayout::ETen26Layout();
  else if (keyboardLayout_ == "HanyuPinyin")
    layout = Formosa::Mandarin::BopomofoKeyboardLayout::HanyuPinyinLayout();
  else if (keyboardLayout_ == "IBM")
    layout = Formosa::Mandarin::BopomofoKeyboardLayout::IBMLayout();

  controller.setKeyboardLayout(layout);

  controller.setSelectPhraseAfterCursorAsCandidate(
      selectPhraseAfterCursorAsCandidate_);
  controller.setMoveCursorAfterSelection(moveCursorAfterSelection_);
  controller.setPutLowercaseLettersToComposingBuffer(
      putLowercaseLettersToComposingBuffer_);
  controller.setEscKeyClearsEntireComposingBuffer(
      escKeyClearsEntireComposingBuffer_);
  controller.setShiftEnterEnabled(shiftEnterEnabled_);
  controller.setCtrlEnterKeyBehavior(ctrlEnterKeyBehavior_);
  controller.setAssociatedPhrasesEnabled(associatedPhrasesEnabled_);
  controller.setHalfWidthPunctuationEnabled(halfWidthPunctuationEnabled_);
  controller.setChineseConversionEnabled(chineseConversionEnabled_);
  controller.setBopomofoFontAnnotationSupportEnabled(
      bopomofoFontAnnotationSupportEnabled_);
  controller.setRepeatedPunctuationToSelectCandidateEnabled(
      repeatedPunctuationToSelectCandidateEnabled_);
  controller.setChooseCandidateUsingSpace(chooseCandidateUsingSpace_);
  controller.setCandidateKeys(candidateKeys_);
  controller.setCandidateKeysCount(candidateKeysCount_);
  controller.setCandidateWindowVertical(candidateWindowVertical_);
  controller.setCandidateFontSize(candidateFontSize_);
  controller.setCandidateWindowColors(candidateWindowColors_);
  controller.setBeepOnError(beepOnError_);
  FCITX_MCBOPOMOFO_INFO() << "Settings applied: ChineseConversionEnabled="
                          << chineseConversionEnabled_;
}

}  // namespace McBopomofo
